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

#অন্তর্ভাব "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাব "V3Case.h"

#অন্তর্ভাব "V3ConstPool.h"
#অন্তর্ভাব "V3SharedTmps.h"
#অন্তর্ভাব "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

শ্রেণী CaseLintVisitor final : সর্বজনীন VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ধ্রুবক Ast_গ্রন্থি* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    স্থির শূন্য detectMultipleDefaults(CaseItem* itemsp) {
        যৌক্তিক hitDefault = মিথ্যা;
        জন্য (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<CaseItem>(itemp->nextp())) {
            যদি (!itemp->isDefault()) চালু;
            যদি (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = সত্য;
        }
    }

    template <typename CaseItem>
    শূন্য checkXZinNonCaseX(Ast_গ্রন্থি* casep, Ast_গ্রন্থি_রাশিমালা* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        জন্য (CaseItem* itemp = itemsp; itemp; itemp = Ast_গ্রন্থি::as<CaseItem>(itemp->nextp())) {
            iterate_And_পরবর্তী_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    শূন্য visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    শূন্য visit(AstCase* nodep) override {
        যদি (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    শূন্য visit(AstConst* nodep) override {
        যদি (!nodep->অঙ্ক().isFourState()) ফেরত;

        // Error if generate case
        যদি (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            ফেরত;
        }

        // Otherwise must be a case statement
        ধ্রুবক AstCase* ধ্রুবক casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        যদি (casep->casex()) ফেরত;

        যদি (casep->casez() || casep->caseInside()) {
            যদি (nodep->অঙ্ক().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            ফেরত;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = পূর্বনির্ধারিত;

সর্বজনীন:
    স্থির শূন্য apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    স্থির শূন্য apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

শ্রেণী CaseVisitor final : সর্বজনীন VNVisitor {
    // Maximum width for detailed analysis
    constexpr স্থির পূর্ণসংখ্যা CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr স্থির পূর্ণসংখ্যা CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr স্থির আকার_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr স্থির আকার_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr স্থির আকার_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    কাঠামো CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_গ্রন্থি* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    কাঠামো LhsRecord final {
        Ast_গ্রন্থি_রাশিমালা* lhsp = nullptr;  // LHS of the assignment
        Ast_গ্রন্থি_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        আকার_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        আকার_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        আকার_t offset = 0;  // Offset in the table for this LHS

        স্থির আকার_t s_পরবর্তী_Id;  // Static unique Id counter
        আকার_t id = ++s_পরবর্তী_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    কাঠামো Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ধ্রুবক Ast_গ্রন্থি* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    যৌক্তিক m_caseOpaque = মিথ্যা;  // Case statement is opaque (non-packed, or non-const conditions)
    যৌক্তিক m_caseHasDefault = মিথ্যা;  // Indicates the case statement has a default case
    আকার_t m_case_NCase_বস্তুসমূহ = 0;  // Number of AstCaseItems in the case statement
    আকার_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_গ্রন্থি_রাশিমালা>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    আকার_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    আকার_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    যৌক্তিক m_caseDetailsValid = মিথ্যা;  // Indicates m_caseDetails is valid
    কাঠামো final {
        যৌক্তিক exhaustive = মিথ্যা;  // Proven exhaustive
        যৌক্তিক exhaustiveOverEnumOnly = মিথ্যা;  // Exhaustive over enum values only
        যৌক্তিক noOverlaps = মিথ্যা;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    স্থির যৌক্তিক neverItem(ধ্রুবক AstCase* casep, ধ্রুবক Ast_গ্রন্থি_রাশিমালা* itemExprp) {
        ধ্রুবক AstConst* ধ্রুবক constp = VN_CAST(itemExprp, Const);
        যদি (!constp) ফেরত মিথ্যা;
        যদি (casep->casex() || casep->caseInside()) ফেরত মিথ্যা;
        যদি (casep->casez()) ফেরত constp->অঙ্ক().isAnyX();
        ফেরত constp->অঙ্ক().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    স্থির std::pair<V3_সংখ্যা, V3_সংখ্যা> matchPattern(ধ্রুবক AstCase* /*casep*/,
                                                      ধ্রুবক AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_সংখ্যা, V3_সংখ্যা> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->অঙ্ক());
        pairMaskBits.second.opBitsOne(condp->অঙ্ক());
        ফেরত pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    স্থির Ast_গ্রন্থি_Assign* checkDecoderAssign(Ast_গ্রন্থি* stmtp) {
        // Only Assign and AssignDly are supported
        যদি (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) ফেরত nullptr;
        Ast_গ্রন্থি_Assign* ধ্রুবক assp = VN_AS(stmtp, গ্রন্থি_Assign);
        // Only if no timing control
        যদি (assp->timingControlp()) ফেরত nullptr;
        // Only if assigning a constant
        যদি (!VN_IS(assp->rhsp(), Const)) ফেরত nullptr;
        // Only if it's a packed value
        Ast_গ্রন্থি_DType* ধ্রুবক dtypep = assp->rhsp()->dtypep();
        যদি (dtypep->isString() || dtypep->isDouble()) ফেরত nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        যদি (assp->lhsp()->exists([](AstVarRef* refp) { ফেরত refp->access().isReadOrRW(); })) {
            ফেরত nullptr;
        }
        // This is an assignment that fits the decoder pattern
        ফেরত assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    যৌক্তিক analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ধ্রুবক VNUser1InUse user1InUse;
        জন্য (Ast_গ্রন্থি* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_গ্রন্থি_Assign* ধ্রুবক assp = checkDecoderAssign(stmtp);
            যদি (!assp) ফেরত মিথ্যা;
            // Must assign each LHS exactly once - RHS is Const
            ধ্রুবক যৌক্তিক multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                ফেরত refp->varScopep()->user1SetOnce();
            });
            যদি (multipleAssignments) ফেরত মিথ্যা;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            যদি (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        ফেরত সত্য;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    স্থির ধ্রুবক AstEnumDType* getEnumCompletionCheckDType(ধ্রুবক AstCase* ধ্রুবক nodep) {
        যদি (!nodep->uniquePragma() && !nodep->unique0Pragma()) ফেরত nullptr;
        ধ্রুবক AstEnumDType* ধ্রুবক enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        যদি (!enumDtp) ফেরত nullptr;  // Case isn't enum
        ধ্রুবক AstBasicDType* ধ্রুবক basicp = enumDtp->subDTypep()->basicp();
        যদি (!basicp) ফেরত nullptr;  // Not simple type (perhaps IEEE illegal)
        ফেরত enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    যৌক্তিক checkExhaustiveEnum(ধ্রুবক AstCase* ধ্রুবক nodep, ধ্রুবক AstEnumDType* ধ্রুবক enump) {
        ধ্রুবক uint32_t অঙ্ক_Cases = 1UL << nodep->exprp()->width();
        যৌক্তিক fullyCovered = সত্য;
        string missing_বস্তুসমূহ;
        জন্য (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ধ্রুবক auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ধ্রুবক uint32_t mask = match.first.toUInt();
            ধ্রুবক uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            জন্য (uint32_t i = 0; i < অঙ্ক_Cases; ++i) {
                যদি ((i & mask) != bits) চালু;  // This case is not for this enum value
                যদি (m_caseDetails.records[i].itemp) চালু;  // Covered case
                যদি (!missing_বস্তুসমূহ.empty()) missing_বস্তুসমূহ += ", ";
                missing_বস্তুসমূহ += eip->prettyNameQ();
                fullyCovered = মিথ্যা;
                বিরতি;
            }
        }
        যদি (!missing_বস্তুসমূহ.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_বস্তুসমূহ);
        }
        ফেরত fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    যৌক্তিক checkExhaustivePacked(AstCase* nodep) {
        ধ্রুবক uint32_t অঙ্ক_Cases = 1UL << nodep->exprp()->width();
        জন্য (uint32_t i = 0; i < অঙ্ক_Cases; ++i) {
            যদি (m_caseDetails.records[i].itemp) চালু;  // Covered case
            যদি (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            ফেরত মিথ্যা;
        }

        // It's an exhaustive case statement
        ফেরত সত্য;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    শূন্য analyzeCaseDetails(AstCase* nodep) {
        ধ্রুবক uint32_t অঙ্ক_মানসমূহ = 1UL << nodep->exprp()->width();
        // Clear case records
        জন্য (uint32_t i = 0; i < অঙ্ক_মানসমূহ; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        যৌক্তিক reportedOverlap = মিথ্যা;
        যৌক্তিক hasDefault = মিথ্যা;
        m_caseDetails.noOverlaps = সত্য;
        জন্য (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            যদি (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                জন্য (uint32_t i = 0; i < অঙ্ক_মানসমূহ; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    যদি (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = সত্য;
                চালু;
            }

            জন্য (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                যদি (neverItem(nodep, iconstp)) চালু;

                ধ্রুবক auto& match = matchPattern(nodep, iconstp);
                ধ্রুবক uint32_t mask = match.first.toUInt();
                ধ্রুবক uint32_t bits = match.second.toUInt();

                যৌক্তিক foundNewCase = মিথ্যা;
                ধ্রুবক AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_মান = 0;
                জন্য (uint32_t i = 0; i < অঙ্ক_মানসমূহ; ++i) {
                    যদি ((i & mask) != bits) চালু;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    যদি (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = সত্য;
                        চালু;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    যদি (caseRecord.itemp == itemp) চালু;

                    // Otherwise record the first overlapping case
                    যদি (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_মান = i;
                        m_caseDetails.noOverlaps = মিথ্যা;
                    }
                }

                // Only report first overlap
                যদি (reportedOverlap || !firstOverlapConstp) চালু;

                // Report first overlap
                যদি (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    যদি (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = সত্য;
                    }
                } অন্যথা {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    যদি (iconstp->অঙ্ক().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_মান
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = সত্য;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = মিথ্যা;
        m_caseDetails.exhaustive = hasDefault;
        যদি (!hasDefault) {
            যদি (ধ্রুবক AstEnumDType* ধ্রুবক enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ধ্রুবক যৌক্তিক exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } অন্যথা {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = সত্য;
    }

    শূন্য analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        জন্য (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ধ্রুবক LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            যদি (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                চালু;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            যদি (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) ফেরত;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            যদি (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                যদি (lhsRecord.nCaseAssigns == m_case_NCase_বস্তুসমূহ) চালু;
                যদি (lhsRecord.nCaseAssignDlys == m_case_NCase_বস্তুসমূহ) চালু;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_গ্রন্থি* ধ্রুবক preDefaultp = lhsRecord.preDefaultp;
            যদি (!preDefaultp) ফেরত;
            // And the pre-default needs to be the same type
            যদি (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) ফেরত;
            যদি (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) ফেরত;
        }
        // All cases check out, can optimize if there are some entries left
        যদি (m_caseLhsRecords.empty()) ফেরত;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.আকার_2());
        জন্য (ধ্রুবক auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ধ্রুবক LhsRecord& a, ধ্রুবক LhsRecord& b) {
                      // Sort by width, then id
                      ধ্রুবক পূর্ণসংখ্যা aWidth = a.lhsp->width();
                      ধ্রুবক পূর্ণসংখ্যা bWidth = b.lhsp->width();
                      যদি (aWidth != bWidth) ফেরত aWidth < bWidth;
                      ফেরত a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ধ্রুবক পূর্ণসংখ্যা caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ধ্রুবক auto fitsLimit = [&](আকার_t entryWidth, আকার_t maxWidth) -> যৌক্তিক {
            আকার_t যোগ_Width = entryWidth;
            // Multiply cases - iterative to avoid overflow
            জন্য (পূর্ণসংখ্যা i = 0; i < caseWidth; ++i) {
                যোগ_Width <<= 1;
                যদি (যোগ_Width > maxWidth) ফেরত মিথ্যা;
            }
            ফেরত সত্য;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        জন্য (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        যদি (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            ফেরত;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        জন্য (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ধ্রুবক আকার_t width = lhsRecord.lhsp->width();
            ধ্রুবক আকার_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ধ্রুবক আকার_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            যদি (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_শব্দগুলি_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ধ্রুবক আকার_t alignedEntryWidth = VL_শব্দগুলি_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        যদি (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            ফেরত;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    শূন্য analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = মিথ্যা;
        m_caseHasDefault = মিথ্যা;
        m_case_NCase_বস্তুসমূহ = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = মিথ্যা;

        Ast_গ্রন্থি* ধ্রুবক caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        যদি (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = সত্য;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ধ্রুবক VNUser1InUse user1InUse;
            জন্য (Ast_গ্রন্থি* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_গ্রন্থি_Assign* ধ্রুবক assp = checkDecoderAssign(prevp);
                যদি (!assp) বিরতি;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ধ্রুবক যৌক্তিক multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    ফেরত refp->varScopep()->user1SetOnce();
                });
                যদি (multipleAssignments) বিরতি;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        যৌক্তিক canBeDecoder = সত্য;
        জন্য (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            জন্য (Ast_গ্রন্থি* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                যদি (!neverItem(nodep, VN_AS(condp, গ্রন্থি_রাশিমালা))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                যদি (!VN_IS(condp, Const)) {
                    m_caseOpaque = সত্য;
                    canBeDecoder = মিথ্যা;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            যদি (cip->isDefault()) m_caseHasDefault = সত্য;
            // Count case items
            ++m_case_NCase_বস্তুসমূহ;
            // Check if it fits the decoder pattern, if still possible
            যদি (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        যদি (m_caseOpaque) ফেরত;

        // If small enough, analyse details
        যদি (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        যদি (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_গ্রন্থি_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_গ্রন্থি_রাশিমালা* exprp, V3SharedTmps& tmps) {
        File_সারি* ধ্রুবক flp = nodep->fileline();

        // If there is only one LHS, just use the result
        যদি (m_caseDecoderRecords.আকার_2() == 1) {
            ধ্রুবক LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ধ্রুবক পূর্ণসংখ্যা width = lhsRecord.lhsp->width();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp
                = exprp->width() == width ? exprp : নতুন AstSel{flp, exprp, 0, width};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = lhsRecord.lhsp->clone_বৃক্ষ_Pure(মিথ্যা);
            যদি (lhsRecord.nCaseAssigns) {
                ফেরত নতুন AstAssign{flp, lhsp, rhsp};
            } অন্যথা যদি (lhsRecord.nCaseAssignDlys) {
                ফেরত নতুন AstAssignDly{flp, lhsp, rhsp};
            } অন্যথা {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ধ্রুবক অস্থায়ী_Vscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক অস্থায়ী_Writep = নতুন AstVarRef{flp, অস্থায়ী_Vscp, VAccess::WRITE};
        Ast_গ্রন্থি_Stmt* ধ্রুবক resultp = নতুন AstAssign{flp, অস্থায়ী_Writep, exprp};

        // For each LHS, select out the result
        জন্য (ধ্রুবক LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ধ্রুবক পূর্ণসংখ্যা width = lhsRecord.lhsp->width();
            ধ্রুবক পূর্ণসংখ্যা lsb = lhsRecord.offset;
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক অস্থায়ী_Readp = নতুন AstVarRef{flp, অস্থায়ী_Vscp, VAccess::READ};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক rhsp = নতুন AstSel{flp, অস্থায়ী_Readp, lsb, width};
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = lhsRecord.lhsp->clone_বৃক্ষ_Pure(মিথ্যা);
            যদি (lhsRecord.nCaseAssigns) {
                resultp->add_পরবর্তী(নতুন AstAssign{flp, lhsp, rhsp});
            } অন্যথা যদি (lhsRecord.nCaseAssignDlys) {
                resultp->add_পরবর্তী(নতুন AstAssignDly{flp, lhsp, rhsp});
            } অন্যথা {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        ফেরত resultp;
    }

    Ast_গ্রন্থি_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_সারি* ধ্রুবক flp = nodep->fileline();
        AstConst* ধ্রুবক tablep
            = নতুন AstConst{flp, AstConst::Widthed_মান{}, static_cast<পূর্ণসংখ্যা>(m_caseTableWidth), 0};
        ধ্রুবক uint32_t tableEntries = 1U << nodep->exprp()->width();

        ধ্রুবক যৌক্তিক isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        যদি (isTinyTable) {
            ++m_stats.caseTableTiny;
        } অন্যথা {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ধ্রুবক uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_শব্দগুলি_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        জন্য (ধ্রুবক LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ধ্রুবক পূর্ণসংখ্যা lhsWidth = lhsRecord.lhsp->width();
            ধ্রুবক পূর্ণসংখ্যা lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            যদি (lhsRecord.preDefaultp) {
                AstConst* ধ্রুবক rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                জন্য (uint32_t সূচক = 0; সূচক < tableEntries; ++সূচক) {
                    ধ্রুবক uint32_t tableOffset = সূচক * entryWidth + lhsOffset;
                    tablep->অঙ্ক().opSelInto(rhsp->অঙ্ক(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            জন্য (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ধ্রুবক rhsp = [&]() -> AstConst* {
                    জন্য (Ast_গ্রন্থি* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_গ্রন্থি_Assign* ধ্রুবক ap = VN_AS(stmtp, গ্রন্থি_Assign);
                        যদি (lhsRecord.lhsp->same_বৃক্ষ(ap->lhsp())) ফেরত VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    ফেরত VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                যদি (cip->isDefault()) {
                    জন্য (uint32_t সূচক = 0; সূচক < tableEntries; ++সূচক) {
                        ধ্রুবক uint32_t tableOffset = সূচক * entryWidth + lhsOffset;
                        tablep->অঙ্ক().opSelInto(rhsp->অঙ্ক(), tableOffset, lhsWidth);
                    }
                    চালু;
                }

                // Iterate case conditions in reverse order
                জন্য (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    যদি (neverItem(nodep, condp)) চালু;  // If item never matches, ignore it
                    ধ্রুবক auto& match = matchPattern(nodep, condp);
                    ধ্রুবক uint32_t matchMask = match.first.toUInt();
                    ধ্রুবক uint32_t matchBits = match.second.toUInt();
                    ধ্রুবক uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    জন্য (uint32_t i = inverseMask; সত্য; i = (i - 1) & inverseMask) {
                        ধ্রুবক uint32_t সূচক = i | matchBits;
                        ধ্রুবক uint32_t tableOffset = সূচক * entryWidth + lhsOffset;
                        tablep->অঙ্ক().opSelInto(rhsp->অঙ্ক(), tableOffset, lhsWidth);
                        যদি (!i) বিরতি;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক tableRefp = [&]() -> Ast_গ্রন্থি_রাশিমালা* {
            যদি (isTinyTable) ফেরত tablep;
            AstVarRef* ধ্রুবক refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_বৃক্ষ(), tablep);  // V3ConstPool::find clones
            ফেরত refp;
        }();

        // Create the lookup index
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক caseExprp
            = নতুন AstExtend{flp, nodep->exprp()->clone_বৃক্ষ_Pure(মিথ্যা), 32};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক scalep = নতুন AstConst{flp, entryWidth};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক tableLsbp = নতুন AstMul{flp, scalep, caseExprp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক tableSelp
            = নতুন AstSel{flp, tableRefp, tableLsbp, static_cast<পূর্ণসংখ্যা>(m_caseDecoderEntryWidth)};

        // Connect outputs
        ফেরত connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_গ্রন্থি_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_সারি* ধ্রুবক flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_গ্রন্থি*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        জন্য (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            যদি (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                চালু;
            }
            জন্য (Ast_গ্রন্থি* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ধ্রুবক iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                যদি (neverItem(nodep, iconstp)) চালু;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ধ্রুবক যৌক্তিক provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        যদি (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ধ্রুবক পূর্ণসংখ্যা decoderEnries = clauses.আকার_2();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ধ্রুবক পূর্ণসংখ্যা condWidth = nodep->exprp()->width();
        ধ্রুবক পূর্ণসংখ্যা matchWidth = 2 * VL_শব্দগুলি_I(condWidth) * VL_EDATASIZE;
        AstConst* ধ্রুবক matchp
            = নতুন AstConst{flp, AstConst::Widthed_মান{}, decoderEnries * matchWidth, 0};
        জন্য (পূর্ণসংখ্যা i = 0; i < decoderEnries; ++i) {
            ধ্রুবক পূর্ণসংখ্যা entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            যদি (AstConst* ধ্রুবক condp = clauses[i].first) {
                ধ্রুবক auto& match = matchPattern(nodep, condp);
                matchp->অঙ্ক().opSelInto(match.first, entryLsb, condWidth);
                matchp->অঙ্ক().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                চালু;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_সংখ্যা অঙ্ক_Zero{flp, condWidth, 0};
            matchp->অঙ্ক().opSelInto(অঙ্ক_Zero, entryLsb, condWidth);
            matchp->অঙ্ক().opSelInto(অঙ্ক_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ধ্রুবক rangep = নতুন AstRange{flp, decoderEnries - 1, 0};
        Ast_গ্রন্থি_DType* ধ্রুবক entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_গ্রন্থি_DType* ধ্রুবক tableDtypep = নতুন AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ধ্রুবক tablep = নতুন AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        জন্য (পূর্ণসংখ্যা i = 0; i < decoderEnries; ++i) {
            Ast_গ্রন্থি* ধ্রুবক stmtsp = clauses[i].second;
            AstConst* ধ্রুবক entryp = নতুন AstConst{flp, AstConst::Widthed_মান{},
                                                  static_cast<পূর্ণসংখ্যা>(m_caseDecoderEntryWidth), 0};
            জন্য (ধ্রুবক LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                জন্য (Ast_গ্রন্থি* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_গ্রন্থি_Assign* ধ্রুবক assignp = VN_AS(stmtp, গ্রন্থি_Assign);
                    যদি (!lhsp->same_বৃক্ষ(assignp->lhsp())) চালু;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    বিরতি;
                }
                // Not assigned in this clause, so use the pre-assigned default
                যদি (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->অঙ্ক().opSelInto(valConstp->অঙ্ক(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ধ্রুবক matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ধ্রুবক tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_বৃক্ষ(), matchp);
        VL_DO_DANGLING(tablep->delete_বৃক্ষ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক caseExprp = nodep->exprp()->clone_বৃক্ষ_Pure(মিথ্যা);
        AstMatchMasked* ধ্রুবক indexp = নতুন AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক entryp = নতুন AstArraySel{flp, tableRefp, indexp};

        ফেরত connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_গ্রন্থি* convertCaseFastRecurse(Ast_গ্রন্থি_রাশিমালা* cexprp, পূর্ণসংখ্যা msb, uint32_t upper_মান) ধ্রুবক {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        যদি (msb < 0) ফেরত m_caseDetails.records[upper_মান].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ধ্রুবক uint32_t upperValue0 = upper_মান;
        ধ্রুবক uint32_t upperValue1 = upper_মান | (1UL << msb);
        Ast_গ্রন্থি* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_গ্রন্থি* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        যদি (tree0p == tree1p) ফেরত tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            যৌক্তিক same = সত্য;
            জন্য (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                যদি (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = মিথ্যা;
                    বিরতি;
                }
            }
            যদি (same) {
                VL_DO_DANGLING(tree1p->delete_বৃক্ষ(), tree1p);
                ফেরত tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        যদি (tree0p && tree0p->backp()) tree0p = tree0p->clone_বৃক্ষ(সত্য);
        যদি (tree1p && tree1p->backp()) tree1p = tree1p->clone_বৃক্ষ(সত্য);
        // Create the If statement
        File_সারি* ধ্রুবক flp = cexprp->fileline();
        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = নতুন AstSel{flp, cexprp->clone_বৃক্ষ_Pure(মিথ্যা), msb, 1};
        AstIf* ধ্রুবক ifp = নতুন AstIf{flp, condp, tree1p, tree0p};
        ফেরত ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_গ্রন্থি* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ধ্রুবক পূর্ণসংখ্যা caseWidth = nodep->exprp()->width();
        Ast_গ্রন্থি* ধ্রুবক ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        ফেরত ifrootp && ifrootp->backp() ? ifrootp->clone_বৃক্ষ(সত্য) : ifrootp;
    }

    Ast_গ্রন্থি_রাশিমালা* newCaseCompare(ধ্রুবক AstCase* casep, Ast_গ্রন্থি_রাশিমালা* itemExprp) {
        File_সারি* ধ্রুবক flp = itemExprp->fileline();
        যদি (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(ঠেলো_Deletep(itemExprp), itemExprp);
            ফেরত নতুন AstConst{flp, AstConst::BitFalse{}};
        }

        যদি (AstCond* ধ্রুবক condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক testp = condp->condp()->unlinkFrBack();
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(ঠেলো_Deletep(itemExprp), itemExprp, condp);
            ফেরত নতুন AstCond{flp, testp, thenp, elsep};
        }

        Ast_গ্রন্থি_রাশিমালা* ধ্রুবক caseExprp = casep->exprp()->clone_বৃক্ষ_Pure(মিথ্যা);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        যদি (AstInsideRange* ধ্রুবক itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(ঠেলো_Deletep(itemExprp), itemExprp, itemRangep);
            ফেরত resultp;
        }

        যদি (AstConst* ধ্রুবক itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            যদি (itemConstp->অঙ্ক().isFourState()) {
                ধ্রুবক auto& match = matchPattern(casep, itemConstp);
                ধ্রুবক V3_সংখ্যা& matchMask = match.first;
                ধ্রুবক V3_সংখ্যা& matchBits = match.second;
                VL_DO_DANGLING2(ঠেলো_Deletep(itemExprp), itemExprp, itemConstp);
                ফেরত AstEq::newTyped(flp, নতুন AstConst{flp, matchBits},
                                       নতুন AstAnd{flp, caseExprp, নতুন AstConst{flp, matchMask}});
            }
        }

        ফেরত AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_গ্রন্থি* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        যৌক্তিক hasDefault = মিথ্যা;
        জন্য (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_সারি* ধ্রুবক flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            যদি (itemp->isDefault()) {
                itemp->addCondsp(নতুন AstConst{flp, AstConst::BitTrue{}});
                hasDefault = সত্য;
                চালু;
            }

            // Regular clause. Construct the condition expression.
            Ast_গ্রন্থি_রাশিমালা* newCondp = nullptr;
            জন্য (Ast_গ্রন্থি_রাশিমালা *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), গ্রন্থি_রাশিমালা);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                যদি (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(ঠেলো_Deletep(itemExprp), itemExprp);
                    চালু;
                }

                // Compute the term to add to the condition expression
                Ast_গ্রন্থি_রাশিমালা* ধ্রুবক termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? নতুন AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            যদি (!newCondp) newCondp = নতুন AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        যদি (!hasDefault) {
            যদি (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ধ্রুবক lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                ঠেলো_Deletep(lastp->condsp()->unlink_Fr_Back_With_পরবর্তী());
                lastp->addCondsp(নতুন AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } অন্যথা {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(নতুন AstCaseItem{
                    nodep->fileline(), নতুন AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        পূর্ণসংখ্যা depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        জন্য (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_গ্রন্থি* ধ্রুবক istmtsp = itemp->stmtsp();
            যদি (istmtsp) istmtsp->unlink_Fr_Back_With_পরবর্তী();

            // Expressioned clause
            Ast_গ্রন্থি_রাশিমালা* ধ্রুবক ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                যদি (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                যদি (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ধ্রুবক newp = নতুন AstIf{itemp->fileline(), ifexprp->clone_বৃক্ষ_Pure(সত্য)};
                    যদি (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } অন্যথা {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } অন্যথা {  // Continue group, modify if condition to OR in this new condition
                    Ast_গ্রন্থি_রাশিমালা* ধ্রুবক condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        নতুন AstOr{ifexprp->fileline(), condp, ifexprp->clone_বৃক্ষ_Pure(সত্য)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_গ্রন্থি_রাশিমালা* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                যদি (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_বৃক্ষ(), itemexprp);
                    itemexprp = নতুন AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ধ্রুবক newp = নতুন AstIf{itemp->fileline(), itemexprp, istmtsp};
                যদি (itemnextp) {
                    itemnextp->addElsesp(newp);
                } অন্যথা {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        ফেরত grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_গ্রন্থি* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ধ্রুবক যৌক্তিক useTable = [&]() {
            // Not if disabled
            যদি (!v3Global.opt.fCaseTable()) ফেরত মিথ্যা;
            // Not if analysis tells us we can't
            যদি (!m_caseTableWidth) ফেরত মিথ্যা;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            যদি (m_caseTableWidth <= CASE_TABLE_TINY_BITS) ফেরত সত্য;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ধ্রুবক আকার_t branches = std::min<আকার_t>(nodep->exprp()->width(), m_caseNConditions);
            যদি (branches < CASE_TABLE_MIN_BRANCHES) ফেরত মিথ্যা;
            ফেরত সত্য;
        }();
        যদি (useTable) ফেরত convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ধ্রুবক যৌক্তিক useDecoder = [&]() {
            // Not if disabled
            যদি (!v3Global.opt.fCaseDecoder()) ফেরত মিথ্যা;
            // Not if not a decoder pattern
            যদি (m_caseDecoderRecords.empty()) ফেরত মিথ্যা;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ধ্রুবক আকার_t branches = std::min<আকার_t>(nodep->exprp()->width(), m_caseNConditions);
            যদি (branches < CASE_TABLE_MIN_BRANCHES) ফেরত মিথ্যা;
            ফেরত সত্য;
        }();
        যদি (useDecoder) ফেরত convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ধ্রুবক যৌক্তিক use_Fast_Bit_বৃক্ষ = [&]() {
            // Not if disabled
            যদি (!v3Global.opt.f_Case_বৃক্ষ()) ফেরত মিথ্যা;
            // Can't do it without the detailed analysis
            যদি (!m_caseDetailsValid) ফেরত মিথ্যা;
            // Can't do it if not exhaustive
            যদি (!m_caseDetails.exhaustive) ফেরত মিথ্যা;
            // Not worth doing if there are few conditions
            যদি (m_caseNConditions <= 3) ফেরত মিথ্যা;
            // Avoid e.g. priority expanders from going crazy in expansion
            ধ্রুবক আকার_t caseWidth = nodep->exprp()->width();
            যদি (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) ফেরত মিথ্যা;
            // Otherwise use the bit tree
            ফেরত সত্য;
        }();
        যদি (use_Fast_Bit_বৃক্ষ) ফেরত convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        যদি (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, সত্য);
        }
        ফেরত convertCaseGeneric(nodep);
    }

    // VISITORS
    শূন্য visit(AstCase* nodep) override {
        UASSERT_OBJ(nodep->exprp()->isPure(), nodep,
                    "Impure case expression should have been removed by V3LiftExpr");

        CaseLintVisitor::apply(nodep);

        // Convert any children first
        iterateChildren(nodep);

        // Analyze this case statement
        analyzeCase(nodep);

        // Take the 'notParallelp' statements under the case statement created by V3Assert.
        // If the statement was proven to have no overlaps and all cases covered,
        // it can be removed. Otherwise insert the assertion after the case statement.
        যদি (Ast_গ্রন্থি* ধ্রুবক stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_Back_With_পরবর্তী();
            যদি (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_বৃক্ষ(), stmtp);
            } অন্যথা {
                nodep->add_পরবর্তী_Here(stmtp);
            }
        }

        // Convert the case statement and replace the original
        যদি (Ast_গ্রন্থি* ধ্রুবক replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } অন্যথা {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_বৃক্ষ(), nodep);
    }

    শূন্য visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    শূন্য visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    শূন্য visit(Ast_গ্রন্থি* nodep) override { iterateChildren(nodep); }

সর্বজনীন:
    // CONSTRUCTORS
    explicit CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::addStat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::addStat("Optimizations, Cases table normal", m_stats.caseTableNormal);
        V3Stats::addStat("Optimizations, Cases table tiny", m_stats.caseTableTiny);
        V3Stats::addStat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::addStat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::addStat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

আকার_t CaseVisitor::LhsRecord::s_পরবর্তী_Id = 0;

//######################################################################
// Case class functions

শূন্য V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("case", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
শূন্য V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
