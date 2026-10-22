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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3Case.h"

#அந்தர்பாவ "V3ConstPool.h"
#அந்தர்பாவ "V3SharedTmps.h"
#அந்தர்பாவ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

வகுப்பு CaseLintVisitor final : பொது VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    மாறிலி Ast_முனை* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    நிலையான வெறுமை detectMultipleDefaults(CaseItem* itemsp) {
        தருக்க hitDefault = பொய்;
        க்கு (CaseItem* itemp = itemsp; itemp; itemp = Ast_முனை::as<CaseItem>(itemp->nextp())) {
            எனில் (!itemp->isDefault()) தொடர்;
            எனில் (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = உண்மை;
        }
    }

    template <typename CaseItem>
    வெறுமை checkXZinNonCaseX(Ast_முனை* casep, Ast_முனை_கோவை* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        க்கு (CaseItem* itemp = itemsp; itemp; itemp = Ast_முனை::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    வெறுமை visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    வெறுமை visit(AstCase* nodep) override {
        எனில் (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    வெறுமை visit(AstConst* nodep) override {
        எனில் (!nodep->இலக்கம்().isFourState()) திரும்பு;

        // Error if generate case
        எனில் (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            திரும்பு;
        }

        // Otherwise must be a case statement
        மாறிலி AstCase* மாறிலி casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        எனில் (casep->casex()) திரும்பு;

        எனில் (casep->casez() || casep->caseInside()) {
            எனில் (nodep->இலக்கம்().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            திரும்பு;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = இயல்பு;

பொது:
    நிலையான வெறுமை apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    நிலையான வெறுமை apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

வகுப்பு CaseVisitor final : பொது VNVisitor {
    // Maximum width for detailed analysis
    constexpr நிலையான முழு CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr நிலையான முழு CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr நிலையான size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr நிலையான size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr நிலையான size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    கட்டமைப்பு CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_முனை* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    கட்டமைப்பு LhsRecord final {
        Ast_முனை_கோவை* lhsp = nullptr;  // LHS of the assignment
        Ast_முனை_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        நிலையான size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    கட்டமைப்பு Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    மாறிலி Ast_முனை* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    தருக்க m_caseOpaque = பொய்;  // Case statement is opaque (non-packed, or non-const conditions)
    தருக்க m_caseHasDefault = பொய்;  // Indicates the case statement has a default case
    size_t m_case_NCase_பொருட்கள் = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_முனை_கோவை>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    தருக்க m_caseDetailsValid = பொய்;  // Indicates m_caseDetails is valid
    கட்டமைப்பு final {
        தருக்க exhaustive = பொய்;  // Proven exhaustive
        தருக்க exhaustiveOverEnumOnly = பொய்;  // Exhaustive over enum values only
        தருக்க noOverlaps = பொய்;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    நிலையான தருக்க neverItem(மாறிலி AstCase* casep, மாறிலி Ast_முனை_கோவை* itemExprp) {
        மாறிலி AstConst* மாறிலி constp = VN_CAST(itemExprp, Const);
        எனில் (!constp) திரும்பு பொய்;
        எனில் (casep->casex() || casep->caseInside()) திரும்பு பொய்;
        எனில் (casep->casez()) திரும்பு constp->இலக்கம்().isAnyX();
        திரும்பு constp->இலக்கம்().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    நிலையான std::pair<V3_எண், V3_எண்> matchPattern(மாறிலி AstCase* /*casep*/,
                                                      மாறிலி AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_எண், V3_எண்> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->இலக்கம்());
        pairMaskBits.second.opBitsOne(condp->இலக்கம்());
        திரும்பு pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    நிலையான Ast_முனை_Assign* checkDecoderAssign(Ast_முனை* stmtp) {
        // Only Assign and AssignDly are supported
        எனில் (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) திரும்பு nullptr;
        Ast_முனை_Assign* மாறிலி assp = VN_AS(stmtp, முனை_Assign);
        // Only if no timing control
        எனில் (assp->timingControlp()) திரும்பு nullptr;
        // Only if assigning a constant
        எனில் (!VN_IS(assp->rhsp(), Const)) திரும்பு nullptr;
        // Only if it's a packed value
        Ast_முனை_DType* மாறிலி dtypep = assp->rhsp()->dtypep();
        எனில் (dtypep->isString() || dtypep->isDouble()) திரும்பு nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        எனில் (assp->lhsp()->exists([](AstVarRef* refp) { திரும்பு refp->access().isReadOrRW(); })) {
            திரும்பு nullptr;
        }
        // This is an assignment that fits the decoder pattern
        திரும்பு assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    தருக்க analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        மாறிலி VNUser1InUse user1InUse;
        க்கு (Ast_முனை* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_முனை_Assign* மாறிலி assp = checkDecoderAssign(stmtp);
            எனில் (!assp) திரும்பு பொய்;
            // Must assign each LHS exactly once - RHS is Const
            மாறிலி தருக்க multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                திரும்பு refp->varScopep()->user1SetOnce();
            });
            எனில் (multipleAssignments) திரும்பு பொய்;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            எனில் (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        திரும்பு உண்மை;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    நிலையான மாறிலி AstEnumDType* getEnumCompletionCheckDType(மாறிலி AstCase* மாறிலி nodep) {
        எனில் (!nodep->uniquePragma() && !nodep->unique0Pragma()) திரும்பு nullptr;
        மாறிலி AstEnumDType* மாறிலி enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        எனில் (!enumDtp) திரும்பு nullptr;  // Case isn't enum
        மாறிலி AstBasicDType* மாறிலி basicp = enumDtp->subDTypep()->basicp();
        எனில் (!basicp) திரும்பு nullptr;  // Not simple type (perhaps IEEE illegal)
        திரும்பு enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    தருக்க checkExhaustiveEnum(மாறிலி AstCase* மாறிலி nodep, மாறிலி AstEnumDType* மாறிலி enump) {
        மாறிலி uint32_t இலக்கம்_Cases = 1UL << nodep->exprp()->width();
        தருக்க fullyCovered = உண்மை;
        string missing_பொருட்கள்;
        க்கு (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            மாறிலி auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            மாறிலி uint32_t mask = match.first.toUInt();
            மாறிலி uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            க்கு (uint32_t i = 0; i < இலக்கம்_Cases; ++i) {
                எனில் ((i & mask) != bits) தொடர்;  // This case is not for this enum value
                எனில் (m_caseDetails.records[i].itemp) தொடர்;  // Covered case
                எனில் (!missing_பொருட்கள்.empty()) missing_பொருட்கள் += ", ";
                missing_பொருட்கள் += eip->prettyNameQ();
                fullyCovered = பொய்;
                நிறுத்து;
            }
        }
        எனில் (!missing_பொருட்கள்.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_பொருட்கள்);
        }
        திரும்பு fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    தருக்க checkExhaustivePacked(AstCase* nodep) {
        மாறிலி uint32_t இலக்கம்_Cases = 1UL << nodep->exprp()->width();
        க்கு (uint32_t i = 0; i < இலக்கம்_Cases; ++i) {
            எனில் (m_caseDetails.records[i].itemp) தொடர்;  // Covered case
            எனில் (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            திரும்பு பொய்;
        }

        // It's an exhaustive case statement
        திரும்பு உண்மை;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    வெறுமை analyzeCaseDetails(AstCase* nodep) {
        மாறிலி uint32_t இலக்கம்_மதிப்புகள் = 1UL << nodep->exprp()->width();
        // Clear case records
        க்கு (uint32_t i = 0; i < இலக்கம்_மதிப்புகள்; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        தருக்க reportedOverlap = பொய்;
        தருக்க hasDefault = பொய்;
        m_caseDetails.noOverlaps = உண்மை;
        க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            எனில் (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                க்கு (uint32_t i = 0; i < இலக்கம்_மதிப்புகள்; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    எனில் (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = உண்மை;
                தொடர்;
            }

            க்கு (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                எனில் (neverItem(nodep, iconstp)) தொடர்;

                மாறிலி auto& match = matchPattern(nodep, iconstp);
                மாறிலி uint32_t mask = match.first.toUInt();
                மாறிலி uint32_t bits = match.second.toUInt();

                தருக்க foundNewCase = பொய்;
                மாறிலி AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_மதிப்பு = 0;
                க்கு (uint32_t i = 0; i < இலக்கம்_மதிப்புகள்; ++i) {
                    எனில் ((i & mask) != bits) தொடர்;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    எனில் (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = உண்மை;
                        தொடர்;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    எனில் (caseRecord.itemp == itemp) தொடர்;

                    // Otherwise record the first overlapping case
                    எனில் (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_மதிப்பு = i;
                        m_caseDetails.noOverlaps = பொய்;
                    }
                }

                // Only report first overlap
                எனில் (reportedOverlap || !firstOverlapConstp) தொடர்;

                // Report first overlap
                எனில் (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    எனில் (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = உண்மை;
                    }
                } இல்லையெனில் {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    எனில் (iconstp->இலக்கம்().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_மதிப்பு
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = உண்மை;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = பொய்;
        m_caseDetails.exhaustive = hasDefault;
        எனில் (!hasDefault) {
            எனில் (மாறிலி AstEnumDType* மாறிலி enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                மாறிலி தருக்க exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } இல்லையெனில் {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = உண்மை;
    }

    வெறுமை analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        க்கு (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            மாறிலி LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            எனில் (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                தொடர்;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            எனில் (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) திரும்பு;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            எனில் (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                எனில் (lhsRecord.nCaseAssigns == m_case_NCase_பொருட்கள்) தொடர்;
                எனில் (lhsRecord.nCaseAssignDlys == m_case_NCase_பொருட்கள்) தொடர்;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_முனை* மாறிலி preDefaultp = lhsRecord.preDefaultp;
            எனில் (!preDefaultp) திரும்பு;
            // And the pre-default needs to be the same type
            எனில் (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) திரும்பு;
            எனில் (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) திரும்பு;
        }
        // All cases check out, can optimize if there are some entries left
        எனில் (m_caseLhsRecords.empty()) திரும்பு;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        க்கு (மாறிலி auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](மாறிலி LhsRecord& a, மாறிலி LhsRecord& b) {
                      // Sort by width, then id
                      மாறிலி முழு aWidth = a.lhsp->width();
                      மாறிலி முழு bWidth = b.lhsp->width();
                      எனில் (aWidth != bWidth) திரும்பு aWidth < bWidth;
                      திரும்பு a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        மாறிலி முழு caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        மாறிலி auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> தருக்க {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            க்கு (முழு i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                எனில் (totalWidth > maxWidth) திரும்பு பொய்;
            }
            திரும்பு உண்மை;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        க்கு (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        எனில் (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            திரும்பு;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        க்கு (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            மாறிலி size_t width = lhsRecord.lhsp->width();
            மாறிலி size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            மாறிலி size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            எனில் (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_சொற்கள்_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        மாறிலி size_t alignedEntryWidth = VL_சொற்கள்_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        எனில் (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            திரும்பு;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    வெறுமை analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = பொய்;
        m_caseHasDefault = பொய்;
        m_case_NCase_பொருட்கள் = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = பொய்;

        Ast_முனை* மாறிலி caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        எனில் (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = உண்மை;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            மாறிலி VNUser1InUse user1InUse;
            க்கு (Ast_முனை* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_முனை_Assign* மாறிலி assp = checkDecoderAssign(prevp);
                எனில் (!assp) நிறுத்து;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                மாறிலி தருக்க multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    திரும்பு refp->varScopep()->user1SetOnce();
                });
                எனில் (multipleAssignments) நிறுத்து;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        தருக்க canBeDecoder = உண்மை;
        க்கு (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            க்கு (Ast_முனை* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                எனில் (!neverItem(nodep, VN_AS(condp, முனை_கோவை))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                எனில் (!VN_IS(condp, Const)) {
                    m_caseOpaque = உண்மை;
                    canBeDecoder = பொய்;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            எனில் (cip->isDefault()) m_caseHasDefault = உண்மை;
            // Count case items
            ++m_case_NCase_பொருட்கள்;
            // Check if it fits the decoder pattern, if still possible
            எனில் (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        எனில் (m_caseOpaque) திரும்பு;

        // If small enough, analyse details
        எனில் (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        எனில் (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_முனை_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_முனை_கோவை* exprp, V3SharedTmps& tmps) {
        File_வரி* மாறிலி flp = nodep->fileline();

        // If there is only one LHS, just use the result
        எனில் (m_caseDecoderRecords.size() == 1) {
            மாறிலி LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            மாறிலி முழு width = lhsRecord.lhsp->width();
            Ast_முனை_கோவை* மாறிலி rhsp
                = exprp->width() == width ? exprp : புதிய AstSel{flp, exprp, 0, width};
            Ast_முனை_கோவை* மாறிலி lhsp = lhsRecord.lhsp->clone_மரம்_Pure(பொய்);
            எனில் (lhsRecord.nCaseAssigns) {
                திரும்பு புதிய AstAssign{flp, lhsp, rhsp};
            } இல்லையெனில் எனில் (lhsRecord.nCaseAssignDlys) {
                திரும்பு புதிய AstAssignDly{flp, lhsp, rhsp};
            } இல்லையெனில் {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* மாறிலி tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_முனை_கோவை* மாறிலி tempWritep = புதிய AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_முனை_Stmt* மாறிலி resultp = புதிய AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        க்கு (மாறிலி LhsRecord& lhsRecord : m_caseDecoderRecords) {
            மாறிலி முழு width = lhsRecord.lhsp->width();
            மாறிலி முழு lsb = lhsRecord.offset;
            Ast_முனை_கோவை* மாறிலி tempReadp = புதிய AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_முனை_கோவை* மாறிலி rhsp = புதிய AstSel{flp, tempReadp, lsb, width};
            Ast_முனை_கோவை* மாறிலி lhsp = lhsRecord.lhsp->clone_மரம்_Pure(பொய்);
            எனில் (lhsRecord.nCaseAssigns) {
                resultp->addNext(புதிய AstAssign{flp, lhsp, rhsp});
            } இல்லையெனில் எனில் (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(புதிய AstAssignDly{flp, lhsp, rhsp});
            } இல்லையெனில் {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        திரும்பு resultp;
    }

    Ast_முனை_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_வரி* மாறிலி flp = nodep->fileline();
        AstConst* மாறிலி tablep
            = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, static_cast<முழு>(m_caseTableWidth), 0};
        மாறிலி uint32_t tableEntries = 1U << nodep->exprp()->width();

        மாறிலி தருக்க isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        எனில் (isTinyTable) {
            ++m_stats.caseTableTiny;
        } இல்லையெனில் {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        மாறிலி uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_சொற்கள்_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        க்கு (மாறிலி LhsRecord& lhsRecord : m_caseDecoderRecords) {
            மாறிலி முழு lhsWidth = lhsRecord.lhsp->width();
            மாறிலி முழு lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            எனில் (lhsRecord.preDefaultp) {
                AstConst* மாறிலி rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                க்கு (uint32_t index = 0; index < tableEntries; ++index) {
                    மாறிலி uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->இலக்கம்().opSelInto(rhsp->இலக்கம்(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            க்கு (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* மாறிலி rhsp = [&]() -> AstConst* {
                    க்கு (Ast_முனை* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_முனை_Assign* மாறிலி ap = VN_AS(stmtp, முனை_Assign);
                        எனில் (lhsRecord.lhsp->same_மரம்(ap->lhsp())) திரும்பு VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    திரும்பு VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                எனில் (cip->isDefault()) {
                    க்கு (uint32_t index = 0; index < tableEntries; ++index) {
                        மாறிலி uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->இலக்கம்().opSelInto(rhsp->இலக்கம்(), tableOffset, lhsWidth);
                    }
                    தொடர்;
                }

                // Iterate case conditions in reverse order
                க்கு (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    எனில் (neverItem(nodep, condp)) தொடர்;  // If item never matches, ignore it
                    மாறிலி auto& match = matchPattern(nodep, condp);
                    மாறிலி uint32_t matchMask = match.first.toUInt();
                    மாறிலி uint32_t matchBits = match.second.toUInt();
                    மாறிலி uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    க்கு (uint32_t i = inverseMask; உண்மை; i = (i - 1) & inverseMask) {
                        மாறிலி uint32_t index = i | matchBits;
                        மாறிலி uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->இலக்கம்().opSelInto(rhsp->இலக்கம்(), tableOffset, lhsWidth);
                        எனில் (!i) நிறுத்து;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_முனை_கோவை* மாறிலி tableRefp = [&]() -> Ast_முனை_கோவை* {
            எனில் (isTinyTable) திரும்பு tablep;
            AstVarRef* மாறிலி refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_மரம்(), tablep);  // V3ConstPool::find clones
            திரும்பு refp;
        }();

        // Create the lookup index
        Ast_முனை_கோவை* மாறிலி caseExprp
            = புதிய AstExtend{flp, nodep->exprp()->clone_மரம்_Pure(பொய்), 32};
        Ast_முனை_கோவை* மாறிலி scalep = புதிய AstConst{flp, entryWidth};
        Ast_முனை_கோவை* மாறிலி tableLsbp = புதிய AstMul{flp, scalep, caseExprp};
        Ast_முனை_கோவை* மாறிலி tableSelp
            = புதிய AstSel{flp, tableRefp, tableLsbp, static_cast<முழு>(m_caseDecoderEntryWidth)};

        // Connect outputs
        திரும்பு connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_முனை_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_வரி* மாறிலி flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_முனை*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        க்கு (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            எனில் (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                தொடர்;
            }
            க்கு (Ast_முனை* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* மாறிலி iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                எனில் (neverItem(nodep, iconstp)) தொடர்;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        மாறிலி தருக்க provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        எனில் (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        மாறிலி முழு decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        மாறிலி முழு condWidth = nodep->exprp()->width();
        மாறிலி முழு matchWidth = 2 * VL_சொற்கள்_I(condWidth) * VL_EDATASIZE;
        AstConst* மாறிலி matchp
            = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{}, decoderEnries * matchWidth, 0};
        க்கு (முழு i = 0; i < decoderEnries; ++i) {
            மாறிலி முழு entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            எனில் (AstConst* மாறிலி condp = clauses[i].first) {
                மாறிலி auto& match = matchPattern(nodep, condp);
                matchp->இலக்கம்().opSelInto(match.first, entryLsb, condWidth);
                matchp->இலக்கம்().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                தொடர்;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_எண் இலக்கம்_Zero{flp, condWidth, 0};
            matchp->இலக்கம்().opSelInto(இலக்கம்_Zero, entryLsb, condWidth);
            matchp->இலக்கம்().opSelInto(இலக்கம்_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* மாறிலி rangep = புதிய AstRange{flp, decoderEnries - 1, 0};
        Ast_முனை_DType* மாறிலி entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_முனை_DType* மாறிலி tableDtypep = புதிய AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* மாறிலி tablep = புதிய AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        க்கு (முழு i = 0; i < decoderEnries; ++i) {
            Ast_முனை* மாறிலி stmtsp = clauses[i].second;
            AstConst* மாறிலி entryp = புதிய AstConst{flp, AstConst::Widthed_மதிப்பு{},
                                                  static_cast<முழு>(m_caseDecoderEntryWidth), 0};
            க்கு (மாறிலி LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_முனை_கோவை* மாறிலி lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                க்கு (Ast_முனை* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_முனை_Assign* மாறிலி assignp = VN_AS(stmtp, முனை_Assign);
                    எனில் (!lhsp->same_மரம்(assignp->lhsp())) தொடர்;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    நிறுத்து;
                }
                // Not assigned in this clause, so use the pre-assigned default
                எனில் (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->இலக்கம்().opSelInto(valConstp->இலக்கம்(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* மாறிலி matchRefp = V3ConstPool::find(matchp);
        AstVarRef* மாறிலி tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_மரம்(), matchp);
        VL_DO_DANGLING(tablep->delete_மரம்(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_முனை_கோவை* மாறிலி caseExprp = nodep->exprp()->clone_மரம்_Pure(பொய்);
        AstMatchMasked* மாறிலி indexp = புதிய AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_முனை_கோவை* மாறிலி entryp = புதிய AstArraySel{flp, tableRefp, indexp};

        திரும்பு connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_முனை* convertCaseFastRecurse(Ast_முனை_கோவை* cexprp, முழு msb, uint32_t upper_மதிப்பு) மாறிலி {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        எனில் (msb < 0) திரும்பு m_caseDetails.records[upper_மதிப்பு].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        மாறிலி uint32_t upperValue0 = upper_மதிப்பு;
        மாறிலி uint32_t upperValue1 = upper_மதிப்பு | (1UL << msb);
        Ast_முனை* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_முனை* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        எனில் (tree0p == tree1p) திரும்பு tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            தருக்க same = உண்மை;
            க்கு (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                எனில் (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = பொய்;
                    நிறுத்து;
                }
            }
            எனில் (same) {
                VL_DO_DANGLING(tree1p->delete_மரம்(), tree1p);
                திரும்பு tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        எனில் (tree0p && tree0p->backp()) tree0p = tree0p->clone_மரம்(உண்மை);
        எனில் (tree1p && tree1p->backp()) tree1p = tree1p->clone_மரம்(உண்மை);
        // Create the If statement
        File_வரி* மாறிலி flp = cexprp->fileline();
        Ast_முனை_கோவை* மாறிலி condp = புதிய AstSel{flp, cexprp->clone_மரம்_Pure(பொய்), msb, 1};
        AstIf* மாறிலி ifp = புதிய AstIf{flp, condp, tree1p, tree0p};
        திரும்பு ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_முனை* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        மாறிலி முழு caseWidth = nodep->exprp()->width();
        Ast_முனை* மாறிலி ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        திரும்பு ifrootp && ifrootp->backp() ? ifrootp->clone_மரம்(உண்மை) : ifrootp;
    }

    Ast_முனை_கோவை* newCaseCompare(மாறிலி AstCase* casep, Ast_முனை_கோவை* itemExprp) {
        File_வரி* மாறிலி flp = itemExprp->fileline();
        எனில் (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(தள்ளு_Deletep(itemExprp), itemExprp);
            திரும்பு புதிய AstConst{flp, AstConst::BitFalse{}};
        }

        எனில் (AstCond* மாறிலி condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_முனை_கோவை* மாறிலி testp = condp->condp()->unlinkFrBack();
            Ast_முனை_கோவை* மாறிலி thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_முனை_கோவை* மாறிலி elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(தள்ளு_Deletep(itemExprp), itemExprp, condp);
            திரும்பு புதிய AstCond{flp, testp, thenp, elsep};
        }

        Ast_முனை_கோவை* மாறிலி caseExprp = casep->exprp()->clone_மரம்_Pure(பொய்);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        எனில் (AstInsideRange* மாறிலி itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_முனை_கோவை* மாறிலி resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(தள்ளு_Deletep(itemExprp), itemExprp, itemRangep);
            திரும்பு resultp;
        }

        எனில் (AstConst* மாறிலி itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            எனில் (itemConstp->இலக்கம்().isFourState()) {
                மாறிலி auto& match = matchPattern(casep, itemConstp);
                மாறிலி V3_எண்& matchMask = match.first;
                மாறிலி V3_எண்& matchBits = match.second;
                VL_DO_DANGLING2(தள்ளு_Deletep(itemExprp), itemExprp, itemConstp);
                திரும்பு AstEq::newTyped(flp, புதிய AstConst{flp, matchBits},
                                       புதிய AstAnd{flp, caseExprp, புதிய AstConst{flp, matchMask}});
            }
        }

        திரும்பு AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_முனை* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        தருக்க hasDefault = பொய்;
        க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_வரி* மாறிலி flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            எனில் (itemp->isDefault()) {
                itemp->addCondsp(புதிய AstConst{flp, AstConst::BitTrue{}});
                hasDefault = உண்மை;
                தொடர்;
            }

            // Regular clause. Construct the condition expression.
            Ast_முனை_கோவை* newCondp = nullptr;
            க்கு (Ast_முனை_கோவை *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), முனை_கோவை);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                எனில் (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(தள்ளு_Deletep(itemExprp), itemExprp);
                    தொடர்;
                }

                // Compute the term to add to the condition expression
                Ast_முனை_கோவை* மாறிலி termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? புதிய AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            எனில் (!newCondp) newCondp = புதிய AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        எனில் (!hasDefault) {
            எனில் (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* மாறிலி lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                தள்ளு_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(புதிய AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } இல்லையெனில் {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(புதிய AstCaseItem{
                    nodep->fileline(), புதிய AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        முழு depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        க்கு (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_முனை* மாறிலி istmtsp = itemp->stmtsp();
            எனில் (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_முனை_கோவை* மாறிலி ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                எனில் (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                எனில் (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* மாறிலி newp = புதிய AstIf{itemp->fileline(), ifexprp->clone_மரம்_Pure(உண்மை)};
                    எனில் (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } இல்லையெனில் {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } இல்லையெனில் {  // Continue group, modify if condition to OR in this new condition
                    Ast_முனை_கோவை* மாறிலி condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        புதிய AstOr{ifexprp->fileline(), condp, ifexprp->clone_மரம்_Pure(உண்மை)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_முனை_கோவை* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                எனில் (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_மரம்(), itemexprp);
                    itemexprp = புதிய AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* மாறிலி newp = புதிய AstIf{itemp->fileline(), itemexprp, istmtsp};
                எனில் (itemnextp) {
                    itemnextp->addElsesp(newp);
                } இல்லையெனில் {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        திரும்பு grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_முனை* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        மாறிலி தருக்க useTable = [&]() {
            // Not if disabled
            எனில் (!v3Global.opt.fCaseTable()) திரும்பு பொய்;
            // Not if analysis tells us we can't
            எனில் (!m_caseTableWidth) திரும்பு பொய்;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            எனில் (m_caseTableWidth <= CASE_TABLE_TINY_BITS) திரும்பு உண்மை;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            மாறிலி size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            எனில் (branches < CASE_TABLE_MIN_BRANCHES) திரும்பு பொய்;
            திரும்பு உண்மை;
        }();
        எனில் (useTable) திரும்பு convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        மாறிலி தருக்க useDecoder = [&]() {
            // Not if disabled
            எனில் (!v3Global.opt.fCaseDecoder()) திரும்பு பொய்;
            // Not if not a decoder pattern
            எனில் (m_caseDecoderRecords.empty()) திரும்பு பொய்;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            மாறிலி size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            எனில் (branches < CASE_TABLE_MIN_BRANCHES) திரும்பு பொய்;
            திரும்பு உண்மை;
        }();
        எனில் (useDecoder) திரும்பு convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        மாறிலி தருக்க use_Fast_Bit_மரம் = [&]() {
            // Not if disabled
            எனில் (!v3Global.opt.f_Case_மரம்()) திரும்பு பொய்;
            // Can't do it without the detailed analysis
            எனில் (!m_caseDetailsValid) திரும்பு பொய்;
            // Can't do it if not exhaustive
            எனில் (!m_caseDetails.exhaustive) திரும்பு பொய்;
            // Not worth doing if there are few conditions
            எனில் (m_caseNConditions <= 3) திரும்பு பொய்;
            // Avoid e.g. priority expanders from going crazy in expansion
            மாறிலி size_t caseWidth = nodep->exprp()->width();
            எனில் (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) திரும்பு பொய்;
            // Otherwise use the bit tree
            திரும்பு உண்மை;
        }();
        எனில் (use_Fast_Bit_மரம்) திரும்பு convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        எனில் (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, உண்மை);
        }
        திரும்பு convertCaseGeneric(nodep);
    }

    // VISITORS
    வெறுமை visit(AstCase* nodep) override {
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
        எனில் (Ast_முனை* மாறிலி stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            எனில் (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_மரம்(), stmtp);
            } இல்லையெனில் {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        எனில் (Ast_முனை* மாறிலி replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } இல்லையெனில் {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_மரம்(), nodep);
    }

    வெறுமை visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    வெறுமை visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    வெறுமை visit(Ast_முனை* nodep) override { iterateChildren(nodep); }

பொது:
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

size_t CaseVisitor::LhsRecord::s_nextId = 0;

//######################################################################
// Case class functions

வெறுமை V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_மரம்("case", 0, dump_மரம்_Either_Level() >= 3);
}
வெறுமை V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
