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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Case.h"

#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3SharedTmps.h"
#अन्तर्भाव "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वग्ग CaseLintVisitor final : सार्वजनिक VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ध्रुव Ast_ग्रन्थिः* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    स्थिर शून्य detectMultipleDefaults(CaseItem* itemsp) {
        तार्किक hitDefault = मिच्छा;
        अत्थाय (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            सचे (!itemp->isDefault()) अनुवर्त;
            सचे (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = सच्च;
        }
    }

    template <typename CaseItem>
    शून्य checkXZinNonCaseX(Ast_ग्रन्थिः* casep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        अत्थाय (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    शून्य visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    शून्य visit(AstCase* nodep) override {
        सचे (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य visit(AstConst* nodep) override {
        सचे (!nodep->अङ्कः().isFourState()) पटिनिवत्त;

        // Error if generate case
        सचे (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            पटिनिवत्त;
        }

        // Otherwise must be a case statement
        ध्रुव AstCase* ध्रुव casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        सचे (casep->casex()) पटिनिवत्त;

        सचे (casep->casez() || casep->caseInside()) {
            सचे (nodep->अङ्कः().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            पटिनिवत्त;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = स्वभाव;

सार्वजनिक:
    स्थिर शून्य apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    स्थिर शून्य apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

वग्ग CaseVisitor final : सार्वजनिक VNVisitor {
    // Maximum width for detailed analysis
    constexpr स्थिर पूर्णाङ्क CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr स्थिर पूर्णाङ्क CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr स्थिर size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr स्थिर size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr स्थिर size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    संरचना CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ग्रन्थिः* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    संरचना LhsRecord final {
        Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nullptr;  // LHS of the assignment
        Ast_ग्रन्थिः_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        स्थिर size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    संरचना Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ध्रुव Ast_ग्रन्थिः* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    तार्किक m_caseOpaque = मिच्छा;  // Case statement is opaque (non-packed, or non-const conditions)
    तार्किक m_caseHasDefault = मिच्छा;  // Indicates the case statement has a default case
    size_t m_case_NCase_वस्तूनि = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ग्रन्थिः_व्यञ्जकम्>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    तार्किक m_caseDetailsValid = मिच्छा;  // Indicates m_caseDetails is valid
    संरचना final {
        तार्किक exhaustive = मिच्छा;  // Proven exhaustive
        तार्किक exhaustiveOverEnumOnly = मिच्छा;  // Exhaustive over enum values only
        तार्किक noOverlaps = मिच्छा;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    स्थिर तार्किक neverItem(ध्रुव AstCase* casep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* itemExprp) {
        ध्रुव AstConst* ध्रुव constp = VN_CAST(itemExprp, Const);
        सचे (!constp) पटिनिवत्त मिच्छा;
        सचे (casep->casex() || casep->caseInside()) पटिनिवत्त मिच्छा;
        सचे (casep->casez()) पटिनिवत्त constp->अङ्कः().isAnyX();
        पटिनिवत्त constp->अङ्कः().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    स्थिर std::pair<V3_सङ्ख्या, V3_सङ्ख्या> matchPattern(ध्रुव AstCase* /*casep*/,
                                                      ध्रुव AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_सङ्ख्या, V3_सङ्ख्या> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->अङ्कः());
        pairMaskBits.second.opBitsOne(condp->अङ्कः());
        पटिनिवत्त pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    स्थिर Ast_ग्रन्थिः_Assign* checkDecoderAssign(Ast_ग्रन्थिः* stmtp) {
        // Only Assign and AssignDly are supported
        सचे (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) पटिनिवत्त nullptr;
        Ast_ग्रन्थिः_Assign* ध्रुव assp = VN_AS(stmtp, ग्रन्थिः_Assign);
        // Only if no timing control
        सचे (assp->timingControlp()) पटिनिवत्त nullptr;
        // Only if assigning a constant
        सचे (!VN_IS(assp->rhsp(), Const)) पटिनिवत्त nullptr;
        // Only if it's a packed value
        Ast_ग्रन्थिः_DType* ध्रुव dtypep = assp->rhsp()->dtypep();
        सचे (dtypep->isString() || dtypep->isDouble()) पटिनिवत्त nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        सचे (assp->lhsp()->exists([](AstVarRef* refp) { पटिनिवत्त refp->access().isReadOrRW(); })) {
            पटिनिवत्त nullptr;
        }
        // This is an assignment that fits the decoder pattern
        पटिनिवत्त assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    तार्किक analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ध्रुव VNUser1InUse user1InUse;
        अत्थाय (Ast_ग्रन्थिः* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ग्रन्थिः_Assign* ध्रुव assp = checkDecoderAssign(stmtp);
            सचे (!assp) पटिनिवत्त मिच्छा;
            // Must assign each LHS exactly once - RHS is Const
            ध्रुव तार्किक multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                पटिनिवत्त refp->varScopep()->user1SetOnce();
            });
            सचे (multipleAssignments) पटिनिवत्त मिच्छा;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            सचे (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        पटिनिवत्त सच्च;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    स्थिर ध्रुव AstEnumDType* getEnumCompletionCheckDType(ध्रुव AstCase* ध्रुव nodep) {
        सचे (!nodep->uniquePragma() && !nodep->unique0Pragma()) पटिनिवत्त nullptr;
        ध्रुव AstEnumDType* ध्रुव enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        सचे (!enumDtp) पटिनिवत्त nullptr;  // Case isn't enum
        ध्रुव AstBasicDType* ध्रुव basicp = enumDtp->subDTypep()->basicp();
        सचे (!basicp) पटिनिवत्त nullptr;  // Not simple type (perhaps IEEE illegal)
        पटिनिवत्त enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    तार्किक checkExhaustiveEnum(ध्रुव AstCase* ध्रुव nodep, ध्रुव AstEnumDType* ध्रुव enump) {
        ध्रुव uint32_t अङ्कः_Cases = 1UL << nodep->exprp()->width();
        तार्किक fullyCovered = सच्च;
        string missing_वस्तूनि;
        अत्थाय (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ध्रुव auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ध्रुव uint32_t mask = match.first.toUInt();
            ध्रुव uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            अत्थाय (uint32_t i = 0; i < अङ्कः_Cases; ++i) {
                सचे ((i & mask) != bits) अनुवर्त;  // This case is not for this enum value
                सचे (m_caseDetails.records[i].itemp) अनुवर्त;  // Covered case
                सचे (!missing_वस्तूनि.empty()) missing_वस्तूनि += ", ";
                missing_वस्तूनि += eip->prettyNameQ();
                fullyCovered = मिच्छा;
                भङ्ग;
            }
        }
        सचे (!missing_वस्तूनि.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_वस्तूनि);
        }
        पटिनिवत्त fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    तार्किक checkExhaustivePacked(AstCase* nodep) {
        ध्रुव uint32_t अङ्कः_Cases = 1UL << nodep->exprp()->width();
        अत्थाय (uint32_t i = 0; i < अङ्कः_Cases; ++i) {
            सचे (m_caseDetails.records[i].itemp) अनुवर्त;  // Covered case
            सचे (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            पटिनिवत्त मिच्छा;
        }

        // It's an exhaustive case statement
        पटिनिवत्त सच्च;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    शून्य analyzeCaseDetails(AstCase* nodep) {
        ध्रुव uint32_t अङ्कः_मानानि = 1UL << nodep->exprp()->width();
        // Clear case records
        अत्थाय (uint32_t i = 0; i < अङ्कः_मानानि; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        तार्किक reportedOverlap = मिच्छा;
        तार्किक hasDefault = मिच्छा;
        m_caseDetails.noOverlaps = सच्च;
        अत्थाय (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            सचे (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                अत्थाय (uint32_t i = 0; i < अङ्कः_मानानि; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    सचे (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = सच्च;
                अनुवर्त;
            }

            अत्थाय (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                सचे (neverItem(nodep, iconstp)) अनुवर्त;

                ध्रुव auto& match = matchPattern(nodep, iconstp);
                ध्रुव uint32_t mask = match.first.toUInt();
                ध्रुव uint32_t bits = match.second.toUInt();

                तार्किक foundNewCase = मिच्छा;
                ध्रुव AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_मानम् = 0;
                अत्थाय (uint32_t i = 0; i < अङ्कः_मानानि; ++i) {
                    सचे ((i & mask) != bits) अनुवर्त;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    सचे (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = सच्च;
                        अनुवर्त;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    सचे (caseRecord.itemp == itemp) अनुवर्त;

                    // Otherwise record the first overlapping case
                    सचे (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_मानम् = i;
                        m_caseDetails.noOverlaps = मिच्छा;
                    }
                }

                // Only report first overlap
                सचे (reportedOverlap || !firstOverlapConstp) अनुवर्त;

                // Report first overlap
                सचे (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    सचे (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = सच्च;
                    }
                } अञ्ञथा {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    सचे (iconstp->अङ्कः().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_मानम्
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = सच्च;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = मिच्छा;
        m_caseDetails.exhaustive = hasDefault;
        सचे (!hasDefault) {
            सचे (ध्रुव AstEnumDType* ध्रुव enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ध्रुव तार्किक exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } अञ्ञथा {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = सच्च;
    }

    शून्य analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        अत्थाय (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ध्रुव LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            सचे (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                अनुवर्त;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            सचे (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) पटिनिवत्त;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            सचे (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                सचे (lhsRecord.nCaseAssigns == m_case_NCase_वस्तूनि) अनुवर्त;
                सचे (lhsRecord.nCaseAssignDlys == m_case_NCase_वस्तूनि) अनुवर्त;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ग्रन्थिः* ध्रुव preDefaultp = lhsRecord.preDefaultp;
            सचे (!preDefaultp) पटिनिवत्त;
            // And the pre-default needs to be the same type
            सचे (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) पटिनिवत्त;
            सचे (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) पटिनिवत्त;
        }
        // All cases check out, can optimize if there are some entries left
        सचे (m_caseLhsRecords.empty()) पटिनिवत्त;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        अत्थाय (ध्रुव auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ध्रुव LhsRecord& a, ध्रुव LhsRecord& b) {
                      // Sort by width, then id
                      ध्रुव पूर्णाङ्क aWidth = a.lhsp->width();
                      ध्रुव पूर्णाङ्क bWidth = b.lhsp->width();
                      सचे (aWidth != bWidth) पटिनिवत्त aWidth < bWidth;
                      पटिनिवत्त a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ध्रुव पूर्णाङ्क caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ध्रुव auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> तार्किक {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            अत्थाय (पूर्णाङ्क i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                सचे (totalWidth > maxWidth) पटिनिवत्त मिच्छा;
            }
            पटिनिवत्त सच्च;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        अत्थाय (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        सचे (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            पटिनिवत्त;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        अत्थाय (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ध्रुव size_t width = lhsRecord.lhsp->width();
            ध्रुव size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ध्रुव size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            सचे (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_शब्दाः_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ध्रुव size_t alignedEntryWidth = VL_शब्दाः_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        सचे (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            पटिनिवत्त;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    शून्य analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = मिच्छा;
        m_caseHasDefault = मिच्छा;
        m_case_NCase_वस्तूनि = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = मिच्छा;

        Ast_ग्रन्थिः* ध्रुव caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        सचे (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = सच्च;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ध्रुव VNUser1InUse user1InUse;
            अत्थाय (Ast_ग्रन्थिः* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ग्रन्थिः_Assign* ध्रुव assp = checkDecoderAssign(prevp);
                सचे (!assp) भङ्ग;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ध्रुव तार्किक multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    पटिनिवत्त refp->varScopep()->user1SetOnce();
                });
                सचे (multipleAssignments) भङ्ग;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        तार्किक canBeDecoder = सच्च;
        अत्थाय (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            अत्थाय (Ast_ग्रन्थिः* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                सचे (!neverItem(nodep, VN_AS(condp, ग्रन्थिः_व्यञ्जकम्))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                सचे (!VN_IS(condp, Const)) {
                    m_caseOpaque = सच्च;
                    canBeDecoder = मिच्छा;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            सचे (cip->isDefault()) m_caseHasDefault = सच्च;
            // Count case items
            ++m_case_NCase_वस्तूनि;
            // Check if it fits the decoder pattern, if still possible
            सचे (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        सचे (m_caseOpaque) पटिनिवत्त;

        // If small enough, analyse details
        सचे (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        सचे (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_ग्रन्थिः_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, V3SharedTmps& tmps) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // If there is only one LHS, just use the result
        सचे (m_caseDecoderRecords.size() == 1) {
            ध्रुव LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ध्रुव पूर्णाङ्क width = lhsRecord.lhsp->width();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp
                = exprp->width() == width ? exprp : नूतन AstSel{flp, exprp, 0, width};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = lhsRecord.lhsp->clone_वृक्षः_Pure(मिच्छा);
            सचे (lhsRecord.nCaseAssigns) {
                पटिनिवत्त नूतन AstAssign{flp, lhsp, rhsp};
            } अञ्ञथा सचे (lhsRecord.nCaseAssignDlys) {
                पटिनिवत्त नूतन AstAssignDly{flp, lhsp, rhsp};
            } अञ्ञथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ध्रुव tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tempWritep = नूतन AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_ग्रन्थिः_Stmt* ध्रुव resultp = नूतन AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        अत्थाय (ध्रुव LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ध्रुव पूर्णाङ्क width = lhsRecord.lhsp->width();
            ध्रुव पूर्णाङ्क lsb = lhsRecord.offset;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tempReadp = नूतन AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = नूतन AstSel{flp, tempReadp, lsb, width};
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = lhsRecord.lhsp->clone_वृक्षः_Pure(मिच्छा);
            सचे (lhsRecord.nCaseAssigns) {
                resultp->addNext(नूतन AstAssign{flp, lhsp, rhsp});
            } अञ्ञथा सचे (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(नूतन AstAssignDly{flp, lhsp, rhsp});
            } अञ्ञथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        पटिनिवत्त resultp;
    }

    Ast_ग्रन्थिः_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        AstConst* ध्रुव tablep
            = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, static_cast<पूर्णाङ्क>(m_caseTableWidth), 0};
        ध्रुव uint32_t tableEntries = 1U << nodep->exprp()->width();

        ध्रुव तार्किक isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        सचे (isTinyTable) {
            ++m_stats.caseTableTiny;
        } अञ्ञथा {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ध्रुव uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_शब्दाः_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        अत्थाय (ध्रुव LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ध्रुव पूर्णाङ्क lhsWidth = lhsRecord.lhsp->width();
            ध्रुव पूर्णाङ्क lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            सचे (lhsRecord.preDefaultp) {
                AstConst* ध्रुव rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                अत्थाय (uint32_t index = 0; index < tableEntries; ++index) {
                    ध्रुव uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->अङ्कः().opSelInto(rhsp->अङ्कः(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            अत्थाय (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ध्रुव rhsp = [&]() -> AstConst* {
                    अत्थाय (Ast_ग्रन्थिः* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ग्रन्थिः_Assign* ध्रुव ap = VN_AS(stmtp, ग्रन्थिः_Assign);
                        सचे (lhsRecord.lhsp->same_वृक्षः(ap->lhsp())) पटिनिवत्त VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    पटिनिवत्त VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                सचे (cip->isDefault()) {
                    अत्थाय (uint32_t index = 0; index < tableEntries; ++index) {
                        ध्रुव uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अङ्कः().opSelInto(rhsp->अङ्कः(), tableOffset, lhsWidth);
                    }
                    अनुवर्त;
                }

                // Iterate case conditions in reverse order
                अत्थाय (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    सचे (neverItem(nodep, condp)) अनुवर्त;  // If item never matches, ignore it
                    ध्रुव auto& match = matchPattern(nodep, condp);
                    ध्रुव uint32_t matchMask = match.first.toUInt();
                    ध्रुव uint32_t matchBits = match.second.toUInt();
                    ध्रुव uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    अत्थाय (uint32_t i = inverseMask; सच्च; i = (i - 1) & inverseMask) {
                        ध्रुव uint32_t index = i | matchBits;
                        ध्रुव uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अङ्कः().opSelInto(rhsp->अङ्कः(), tableOffset, lhsWidth);
                        सचे (!i) भङ्ग;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tableRefp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
            सचे (isTinyTable) पटिनिवत्त tablep;
            AstVarRef* ध्रुव refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_वृक्षः(), tablep);  // V3ConstPool::find clones
            पटिनिवत्त refp;
        }();

        // Create the lookup index
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव caseExprp
            = नूतन AstExtend{flp, nodep->exprp()->clone_वृक्षः_Pure(मिच्छा), 32};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव scalep = नूतन AstConst{flp, entryWidth};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tableLsbp = नूतन AstMul{flp, scalep, caseExprp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव tableSelp
            = नूतन AstSel{flp, tableRefp, tableLsbp, static_cast<पूर्णाङ्क>(m_caseDecoderEntryWidth)};

        // Connect outputs
        पटिनिवत्त connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_ग्रन्थिः_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_ग्रन्थिः*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        अत्थाय (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            सचे (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                अनुवर्त;
            }
            अत्थाय (Ast_ग्रन्थिः* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ध्रुव iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                सचे (neverItem(nodep, iconstp)) अनुवर्त;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ध्रुव तार्किक provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        सचे (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ध्रुव पूर्णाङ्क decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ध्रुव पूर्णाङ्क condWidth = nodep->exprp()->width();
        ध्रुव पूर्णाङ्क matchWidth = 2 * VL_शब्दाः_I(condWidth) * VL_EDATASIZE;
        AstConst* ध्रुव matchp
            = नूतन AstConst{flp, AstConst::Widthed_मानम्{}, decoderEnries * matchWidth, 0};
        अत्थाय (पूर्णाङ्क i = 0; i < decoderEnries; ++i) {
            ध्रुव पूर्णाङ्क entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            सचे (AstConst* ध्रुव condp = clauses[i].first) {
                ध्रुव auto& match = matchPattern(nodep, condp);
                matchp->अङ्कः().opSelInto(match.first, entryLsb, condWidth);
                matchp->अङ्कः().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                अनुवर्त;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_सङ्ख्या अङ्कः_Zero{flp, condWidth, 0};
            matchp->अङ्कः().opSelInto(अङ्कः_Zero, entryLsb, condWidth);
            matchp->अङ्कः().opSelInto(अङ्कः_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ध्रुव rangep = नूतन AstRange{flp, decoderEnries - 1, 0};
        Ast_ग्रन्थिः_DType* ध्रुव entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_ग्रन्थिः_DType* ध्रुव tableDtypep = नूतन AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ध्रुव tablep = नूतन AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        अत्थाय (पूर्णाङ्क i = 0; i < decoderEnries; ++i) {
            Ast_ग्रन्थिः* ध्रुव stmtsp = clauses[i].second;
            AstConst* ध्रुव entryp = नूतन AstConst{flp, AstConst::Widthed_मानम्{},
                                                  static_cast<पूर्णाङ्क>(m_caseDecoderEntryWidth), 0};
            अत्थाय (ध्रुव LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                अत्थाय (Ast_ग्रन्थिः* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ग्रन्थिः_Assign* ध्रुव assignp = VN_AS(stmtp, ग्रन्थिः_Assign);
                    सचे (!lhsp->same_वृक्षः(assignp->lhsp())) अनुवर्त;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    भङ्ग;
                }
                // Not assigned in this clause, so use the pre-assigned default
                सचे (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->अङ्कः().opSelInto(valConstp->अङ्कः(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ध्रुव matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ध्रुव tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_वृक्षः(), matchp);
        VL_DO_DANGLING(tablep->delete_वृक्षः(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव caseExprp = nodep->exprp()->clone_वृक्षः_Pure(मिच्छा);
        AstMatchMasked* ध्रुव indexp = नूतन AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव entryp = नूतन AstArraySel{flp, tableRefp, indexp};

        पटिनिवत्त connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ग्रन्थिः* convertCaseFastRecurse(Ast_ग्रन्थिः_व्यञ्जकम्* cexprp, पूर्णाङ्क msb, uint32_t upper_मानम्) ध्रुव {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        सचे (msb < 0) पटिनिवत्त m_caseDetails.records[upper_मानम्].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ध्रुव uint32_t upperValue0 = upper_मानम्;
        ध्रुव uint32_t upperValue1 = upper_मानम् | (1UL << msb);
        Ast_ग्रन्थिः* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_ग्रन्थिः* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        सचे (tree0p == tree1p) पटिनिवत्त tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            तार्किक same = सच्च;
            अत्थाय (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                सचे (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = मिच्छा;
                    भङ्ग;
                }
            }
            सचे (same) {
                VL_DO_DANGLING(tree1p->delete_वृक्षः(), tree1p);
                पटिनिवत्त tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        सचे (tree0p && tree0p->backp()) tree0p = tree0p->clone_वृक्षः(सच्च);
        सचे (tree1p && tree1p->backp()) tree1p = tree1p->clone_वृक्षः(सच्च);
        // Create the If statement
        File_पङ्क्तिः* ध्रुव flp = cexprp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = नूतन AstSel{flp, cexprp->clone_वृक्षः_Pure(मिच्छा), msb, 1};
        AstIf* ध्रुव ifp = नूतन AstIf{flp, condp, tree1p, tree0p};
        पटिनिवत्त ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ग्रन्थिः* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ध्रुव पूर्णाङ्क caseWidth = nodep->exprp()->width();
        Ast_ग्रन्थिः* ध्रुव ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        पटिनिवत्त ifrootp && ifrootp->backp() ? ifrootp->clone_वृक्षः(सच्च) : ifrootp;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* newCaseCompare(ध्रुव AstCase* casep, Ast_ग्रन्थिः_व्यञ्जकम्* itemExprp) {
        File_पङ्क्तिः* ध्रुव flp = itemExprp->fileline();
        सचे (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(itemExprp), itemExprp);
            पटिनिवत्त नूतन AstConst{flp, AstConst::BitFalse{}};
        }

        सचे (AstCond* ध्रुव condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव testp = condp->condp()->unlinkFrBack();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(प्रक्षिप_Deletep(itemExprp), itemExprp, condp);
            पटिनिवत्त नूतन AstCond{flp, testp, thenp, elsep};
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव caseExprp = casep->exprp()->clone_वृक्षः_Pure(मिच्छा);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        सचे (AstInsideRange* ध्रुव itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(प्रक्षिप_Deletep(itemExprp), itemExprp, itemRangep);
            पटिनिवत्त resultp;
        }

        सचे (AstConst* ध्रुव itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            सचे (itemConstp->अङ्कः().isFourState()) {
                ध्रुव auto& match = matchPattern(casep, itemConstp);
                ध्रुव V3_सङ्ख्या& matchMask = match.first;
                ध्रुव V3_सङ्ख्या& matchBits = match.second;
                VL_DO_DANGLING2(प्रक्षिप_Deletep(itemExprp), itemExprp, itemConstp);
                पटिनिवत्त AstEq::newTyped(flp, नूतन AstConst{flp, matchBits},
                                       नूतन AstAnd{flp, caseExprp, नूतन AstConst{flp, matchMask}});
            }
        }

        पटिनिवत्त AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ग्रन्थिः* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        तार्किक hasDefault = मिच्छा;
        अत्थाय (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_पङ्क्तिः* ध्रुव flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            सचे (itemp->isDefault()) {
                itemp->addCondsp(नूतन AstConst{flp, AstConst::BitTrue{}});
                hasDefault = सच्च;
                अनुवर्त;
            }

            // Regular clause. Construct the condition expression.
            Ast_ग्रन्थिः_व्यञ्जकम्* newCondp = nullptr;
            अत्थाय (Ast_ग्रन्थिः_व्यञ्जकम् *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), ग्रन्थिः_व्यञ्जकम्);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                सचे (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(itemExprp), itemExprp);
                    अनुवर्त;
                }

                // Compute the term to add to the condition expression
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? नूतन AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            सचे (!newCondp) newCondp = नूतन AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        सचे (!hasDefault) {
            सचे (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ध्रुव lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                प्रक्षिप_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(नूतन AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } अञ्ञथा {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(नूतन AstCaseItem{
                    nodep->fileline(), नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        पूर्णाङ्क depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        अत्थाय (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_ग्रन्थिः* ध्रुव istmtsp = itemp->stmtsp();
            सचे (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                सचे (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                सचे (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ध्रुव newp = नूतन AstIf{itemp->fileline(), ifexprp->clone_वृक्षः_Pure(सच्च)};
                    सचे (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } अञ्ञथा {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } अञ्ञथा {  // Continue group, modify if condition to OR in this new condition
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        नूतन AstOr{ifexprp->fileline(), condp, ifexprp->clone_वृक्षः_Pure(सच्च)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ग्रन्थिः_व्यञ्जकम्* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                सचे (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_वृक्षः(), itemexprp);
                    itemexprp = नूतन AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ध्रुव newp = नूतन AstIf{itemp->fileline(), itemexprp, istmtsp};
                सचे (itemnextp) {
                    itemnextp->addElsesp(newp);
                } अञ्ञथा {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        पटिनिवत्त grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ग्रन्थिः* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ध्रुव तार्किक useTable = [&]() {
            // Not if disabled
            सचे (!v3Global.opt.fCaseTable()) पटिनिवत्त मिच्छा;
            // Not if analysis tells us we can't
            सचे (!m_caseTableWidth) पटिनिवत्त मिच्छा;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            सचे (m_caseTableWidth <= CASE_TABLE_TINY_BITS) पटिनिवत्त सच्च;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ध्रुव size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            सचे (branches < CASE_TABLE_MIN_BRANCHES) पटिनिवत्त मिच्छा;
            पटिनिवत्त सच्च;
        }();
        सचे (useTable) पटिनिवत्त convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ध्रुव तार्किक useDecoder = [&]() {
            // Not if disabled
            सचे (!v3Global.opt.fCaseDecoder()) पटिनिवत्त मिच्छा;
            // Not if not a decoder pattern
            सचे (m_caseDecoderRecords.empty()) पटिनिवत्त मिच्छा;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ध्रुव size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            सचे (branches < CASE_TABLE_MIN_BRANCHES) पटिनिवत्त मिच्छा;
            पटिनिवत्त सच्च;
        }();
        सचे (useDecoder) पटिनिवत्त convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ध्रुव तार्किक use_Fast_Bit_वृक्षः = [&]() {
            // Not if disabled
            सचे (!v3Global.opt.f_Case_वृक्षः()) पटिनिवत्त मिच्छा;
            // Can't do it without the detailed analysis
            सचे (!m_caseDetailsValid) पटिनिवत्त मिच्छा;
            // Can't do it if not exhaustive
            सचे (!m_caseDetails.exhaustive) पटिनिवत्त मिच्छा;
            // Not worth doing if there are few conditions
            सचे (m_caseNConditions <= 3) पटिनिवत्त मिच्छा;
            // Avoid e.g. priority expanders from going crazy in expansion
            ध्रुव size_t caseWidth = nodep->exprp()->width();
            सचे (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) पटिनिवत्त मिच्छा;
            // Otherwise use the bit tree
            पटिनिवत्त सच्च;
        }();
        सचे (use_Fast_Bit_वृक्षः) पटिनिवत्त convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        सचे (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, सच्च);
        }
        पटिनिवत्त convertCaseGeneric(nodep);
    }

    // VISITORS
    शून्य visit(AstCase* nodep) override {
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
        सचे (Ast_ग्रन्थिः* ध्रुव stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            सचे (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_वृक्षः(), stmtp);
            } अञ्ञथा {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        सचे (Ast_ग्रन्थिः* ध्रुव replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } अञ्ञथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }

    शून्य visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    शून्य visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    शून्य visit(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
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

शून्य V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("case", 0, dump_वृक्षः_Either_Level() >= 3);
}
शून्य V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
