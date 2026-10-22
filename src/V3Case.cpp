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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Case.h"

#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3SharedTmps.h"
#ಅನ್ತರ್ಭಾವ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ CaseLintVisitor final : ಸಾರ್ವಜನಿಕ VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ಧ್ರುವ Ast_ಗಂಟು* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    ಸ್ಥಿರ ಶೂನ್ಯ detectMultipleDefaults(CaseItem* itemsp) {
        ತಾರ್ಕಿಕ hitDefault = ಅಸತ್ಯ;
        ಪ್ರತಿ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<CaseItem>(itemp->nextp())) {
            ಯದಿ (!itemp->isDefault()) ಅನುವರ್ತ;
            ಯದಿ (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = ಸತ್ಯ;
        }
    }

    template <typename CaseItem>
    ಶೂನ್ಯ checkXZinNonCaseX(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        ಪ್ರತಿ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    ಶೂನ್ಯ visit(AstCase* nodep) override {
        ಯದಿ (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        ಯದಿ (!nodep->ಅಂಕೆ().isFourState()) ಪ್ರತ್ಯಾಗಮ;

        // Error if generate case
        ಯದಿ (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Otherwise must be a case statement
        ಧ್ರುವ AstCase* ಧ್ರುವ casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        ಯದಿ (casep->casex()) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (casep->casez() || casep->caseInside()) {
            ಯದಿ (nodep->ಅಂಕೆ().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            ಪ್ರತ್ಯಾಗಮ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

ವರ್ಗ CaseVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Maximum width for detailed analysis
    constexpr ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ಸ್ಥಿರ size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ಸ್ಥಿರ size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ಸ್ಥಿರ size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ಸಂರಚನಾ CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ಗಂಟು* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ಸಂರಚನಾ LhsRecord final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nullptr;  // LHS of the assignment
        Ast_ಗಂಟು_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        ಸ್ಥಿರ size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ಸಂರಚನಾ Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ಧ್ರುವ Ast_ಗಂಟು* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    ತಾರ್ಕಿಕ m_caseOpaque = ಅಸತ್ಯ;  // Case statement is opaque (non-packed, or non-const conditions)
    ತಾರ್ಕಿಕ m_caseHasDefault = ಅಸತ್ಯ;  // Indicates the case statement has a default case
    size_t m_case_NCase_ವಸ್ತುಗಳು = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    ತಾರ್ಕಿಕ m_caseDetailsValid = ಅಸತ್ಯ;  // Indicates m_caseDetails is valid
    ಸಂರಚನಾ final {
        ತಾರ್ಕಿಕ exhaustive = ಅಸತ್ಯ;  // Proven exhaustive
        ತಾರ್ಕಿಕ exhaustiveOverEnumOnly = ಅಸತ್ಯ;  // Exhaustive over enum values only
        ತಾರ್ಕಿಕ noOverlaps = ಅಸತ್ಯ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ಸ್ಥಿರ ತಾರ್ಕಿಕ neverItem(ಧ್ರುವ AstCase* casep, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemExprp) {
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(itemExprp, Const);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (casep->casex() || casep->caseInside()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (casep->casez()) ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().isAnyX();
        ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ಸ್ಥಿರ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ> matchPattern(ಧ್ರುವ AstCase* /*casep*/,
                                                      ಧ್ರುವ AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->ಅಂಕೆ());
        pairMaskBits.second.opBitsOne(condp->ಅಂಕೆ());
        ಪ್ರತ್ಯಾಗಮ pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ಸ್ಥಿರ Ast_ಗಂಟು_Assign* checkDecoderAssign(Ast_ಗಂಟು* stmtp) {
        // Only Assign and AssignDly are supported
        ಯದಿ (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) ಪ್ರತ್ಯಾಗಮ nullptr;
        Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = VN_AS(stmtp, ಗಂಟು_Assign);
        // Only if no timing control
        ಯದಿ (assp->timingControlp()) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Only if assigning a constant
        ಯದಿ (!VN_IS(assp->rhsp(), Const)) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Only if it's a packed value
        Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = assp->rhsp()->dtypep();
        ಯದಿ (dtypep->isString() || dtypep->isDouble()) ಪ್ರತ್ಯಾಗಮ nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        ಯದಿ (assp->lhsp()->exists([](AstVarRef* refp) { ಪ್ರತ್ಯಾಗಮ refp->access().isReadOrRW(); })) {
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // This is an assignment that fits the decoder pattern
        ಪ್ರತ್ಯಾಗಮ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    ತಾರ್ಕಿಕ analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ಧ್ರುವ VNUser1InUse user1InUse;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = checkDecoderAssign(stmtp);
            ಯದಿ (!assp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Must assign each LHS exactly once - RHS is Const
            ಧ್ರುವ ತಾರ್ಕಿಕ multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                ಪ್ರತ್ಯಾಗಮ refp->varScopep()->user1SetOnce();
            });
            ಯದಿ (multipleAssignments) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            ಯದಿ (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ಸ್ಥಿರ ಧ್ರುವ AstEnumDType* getEnumCompletionCheckDType(ಧ್ರುವ AstCase* ಧ್ರುವ nodep) {
        ಯದಿ (!nodep->uniquePragma() && !nodep->unique0Pragma()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ AstEnumDType* ಧ್ರುವ enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        ಯದಿ (!enumDtp) ಪ್ರತ್ಯಾಗಮ nullptr;  // Case isn't enum
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = enumDtp->subDTypep()->basicp();
        ಯದಿ (!basicp) ಪ್ರತ್ಯಾಗಮ nullptr;  // Not simple type (perhaps IEEE illegal)
        ಪ್ರತ್ಯಾಗಮ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    ತಾರ್ಕಿಕ checkExhaustiveEnum(ಧ್ರುವ AstCase* ಧ್ರುವ nodep, ಧ್ರುವ AstEnumDType* ಧ್ರುವ enump) {
        ಧ್ರುವ uint32_t ಅಂಕೆ_Cases = 1UL << nodep->exprp()->width();
        ತಾರ್ಕಿಕ fullyCovered = ಸತ್ಯ;
        string missing_ವಸ್ತುಗಳು;
        ಪ್ರತಿ (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ಧ್ರುವ auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ಧ್ರುವ uint32_t mask = match.first.toUInt();
            ಧ್ರುವ uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            ಪ್ರತಿ (uint32_t i = 0; i < ಅಂಕೆ_Cases; ++i) {
                ಯದಿ ((i & mask) != bits) ಅನುವರ್ತ;  // This case is not for this enum value
                ಯದಿ (m_caseDetails.records[i].itemp) ಅನುವರ್ತ;  // Covered case
                ಯದಿ (!missing_ವಸ್ತುಗಳು.empty()) missing_ವಸ್ತುಗಳು += ", ";
                missing_ವಸ್ತುಗಳು += eip->prettyNameQ();
                fullyCovered = ಅಸತ್ಯ;
                ಭಙ್ಗ;
            }
        }
        ಯದಿ (!missing_ವಸ್ತುಗಳು.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_ವಸ್ತುಗಳು);
        }
        ಪ್ರತ್ಯಾಗಮ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    ತಾರ್ಕಿಕ checkExhaustivePacked(AstCase* nodep) {
        ಧ್ರುವ uint32_t ಅಂಕೆ_Cases = 1UL << nodep->exprp()->width();
        ಪ್ರತಿ (uint32_t i = 0; i < ಅಂಕೆ_Cases; ++i) {
            ಯದಿ (m_caseDetails.records[i].itemp) ಅನುವರ್ತ;  // Covered case
            ಯದಿ (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // It's an exhaustive case statement
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    ಶೂನ್ಯ analyzeCaseDetails(AstCase* nodep) {
        ಧ್ರುವ uint32_t ಅಂಕೆ_ಮೌಲ್ಯಗಳು = 1UL << nodep->exprp()->width();
        // Clear case records
        ಪ್ರತಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        ತಾರ್ಕಿಕ reportedOverlap = ಅಸತ್ಯ;
        ತಾರ್ಕಿಕ hasDefault = ಅಸತ್ಯ;
        m_caseDetails.noOverlaps = ಸತ್ಯ;
        ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            ಯದಿ (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                ಪ್ರತಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    ಯದಿ (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = ಸತ್ಯ;
                ಅನುವರ್ತ;
            }

            ಪ್ರತಿ (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                ಯದಿ (neverItem(nodep, iconstp)) ಅನುವರ್ತ;

                ಧ್ರುವ auto& match = matchPattern(nodep, iconstp);
                ಧ್ರುವ uint32_t mask = match.first.toUInt();
                ಧ್ರುವ uint32_t bits = match.second.toUInt();

                ತಾರ್ಕಿಕ foundNewCase = ಅಸತ್ಯ;
                ಧ್ರುವ AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_ಮೌಲ್ಯ = 0;
                ಪ್ರತಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
                    ಯದಿ ((i & mask) != bits) ಅನುವರ್ತ;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    ಯದಿ (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = ಸತ್ಯ;
                        ಅನುವರ್ತ;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    ಯದಿ (caseRecord.itemp == itemp) ಅನುವರ್ತ;

                    // Otherwise record the first overlapping case
                    ಯದಿ (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_ಮೌಲ್ಯ = i;
                        m_caseDetails.noOverlaps = ಅಸತ್ಯ;
                    }
                }

                // Only report first overlap
                ಯದಿ (reportedOverlap || !firstOverlapConstp) ಅನುವರ್ತ;

                // Report first overlap
                ಯದಿ (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    ಯದಿ (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = ಸತ್ಯ;
                    }
                } ಅನ್ಯಥಾ {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    ಯದಿ (iconstp->ಅಂಕೆ().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_ಮೌಲ್ಯ
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = ಸತ್ಯ;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = ಅಸತ್ಯ;
        m_caseDetails.exhaustive = hasDefault;
        ಯದಿ (!hasDefault) {
            ಯದಿ (ಧ್ರುವ AstEnumDType* ಧ್ರುವ enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ಧ್ರುವ ತಾರ್ಕಿಕ exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } ಅನ್ಯಥಾ {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = ಸತ್ಯ;
    }

    ಶೂನ್ಯ analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        ಪ್ರತಿ (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ಧ್ರುವ LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            ಯದಿ (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                ಅನುವರ್ತ;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            ಯದಿ (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) ಪ್ರತ್ಯಾಗಮ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            ಯದಿ (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                ಯದಿ (lhsRecord.nCaseAssigns == m_case_NCase_ವಸ್ತುಗಳು) ಅನುವರ್ತ;
                ಯದಿ (lhsRecord.nCaseAssignDlys == m_case_NCase_ವಸ್ತುಗಳು) ಅನುವರ್ತ;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ಗಂಟು* ಧ್ರುವ preDefaultp = lhsRecord.preDefaultp;
            ಯದಿ (!preDefaultp) ಪ್ರತ್ಯಾಗಮ;
            // And the pre-default needs to be the same type
            ಯದಿ (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) ಪ್ರತ್ಯಾಗಮ;
            ಯದಿ (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) ಪ್ರತ್ಯಾಗಮ;
        }
        // All cases check out, can optimize if there are some entries left
        ಯದಿ (m_caseLhsRecords.empty()) ಪ್ರತ್ಯಾಗಮ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        ಪ್ರತಿ (ಧ್ರುವ auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ಧ್ರುವ LhsRecord& a, ಧ್ರುವ LhsRecord& b) {
                      // Sort by width, then id
                      ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ aWidth = a.lhsp->width();
                      ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bWidth = b.lhsp->width();
                      ಯದಿ (aWidth != bWidth) ಪ್ರತ್ಯಾಗಮ aWidth < bWidth;
                      ಪ್ರತ್ಯಾಗಮ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ಧ್ರುವ auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> ತಾರ್ಕಿಕ {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                ಯದಿ (totalWidth > maxWidth) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        ಪ್ರತಿ (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        ಯದಿ (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        ಪ್ರತಿ (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ಧ್ರುವ size_t width = lhsRecord.lhsp->width();
            ಧ್ರುವ size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ಧ್ರುವ size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            ಯದಿ (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_ಪದಗಳು_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ಧ್ರುವ size_t alignedEntryWidth = VL_ಪದಗಳು_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        ಯದಿ (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    ಶೂನ್ಯ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = ಅಸತ್ಯ;
        m_caseHasDefault = ಅಸತ್ಯ;
        m_case_NCase_ವಸ್ತುಗಳು = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = ಅಸತ್ಯ;

        Ast_ಗಂಟು* ಧ್ರುವ caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        ಯದಿ (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = ಸತ್ಯ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ಧ್ರುವ VNUser1InUse user1InUse;
            ಪ್ರತಿ (Ast_ಗಂಟು* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ಗಂಟು_Assign* ಧ್ರುವ assp = checkDecoderAssign(prevp);
                ಯದಿ (!assp) ಭಙ್ಗ;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ಧ್ರುವ ತಾರ್ಕಿಕ multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    ಪ್ರತ್ಯಾಗಮ refp->varScopep()->user1SetOnce();
                });
                ಯದಿ (multipleAssignments) ಭಙ್ಗ;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        ತಾರ್ಕಿಕ canBeDecoder = ಸತ್ಯ;
        ಪ್ರತಿ (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            ಪ್ರತಿ (Ast_ಗಂಟು* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                ಯದಿ (!neverItem(nodep, VN_AS(condp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                ಯದಿ (!VN_IS(condp, Const)) {
                    m_caseOpaque = ಸತ್ಯ;
                    canBeDecoder = ಅಸತ್ಯ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            ಯದಿ (cip->isDefault()) m_caseHasDefault = ಸತ್ಯ;
            // Count case items
            ++m_case_NCase_ವಸ್ತುಗಳು;
            // Check if it fits the decoder pattern, if still possible
            ಯದಿ (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        ಯದಿ (m_caseOpaque) ಪ್ರತ್ಯಾಗಮ;

        // If small enough, analyse details
        ಯದಿ (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        ಯದಿ (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_ಗಂಟು_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, V3SharedTmps& tmps) {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        // If there is only one LHS, just use the result
        ಯದಿ (m_caseDecoderRecords.size() == 1) {
            ಧ್ರುವ LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = lhsRecord.lhsp->width();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp
                = exprp->width() == width ? exprp : ನೂತನ AstSel{flp, exprp, 0, width};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = lhsRecord.lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (lhsRecord.nCaseAssigns) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssign{flp, lhsp, rhsp};
            } ಅನ್ಯಥಾ ಯದಿ (lhsRecord.nCaseAssignDlys) {
                ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssignDly{flp, lhsp, rhsp};
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ಧ್ರುವ tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tempWritep = ನೂತನ AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_ಗಂಟು_Stmt* ಧ್ರುವ resultp = ನೂತನ AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        ಪ್ರತಿ (ಧ್ರುವ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = lhsRecord.lhsp->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsb = lhsRecord.offset;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tempReadp = ನೂತನ AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = ನೂತನ AstSel{flp, tempReadp, lsb, width};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = lhsRecord.lhsp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
            ಯದಿ (lhsRecord.nCaseAssigns) {
                resultp->addNext(ನೂತನ AstAssign{flp, lhsp, rhsp});
            } ಅನ್ಯಥಾ ಯದಿ (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(ನೂತನ AstAssignDly{flp, lhsp, rhsp});
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        ಪ್ರತ್ಯಾಗಮ resultp;
    }

    Ast_ಗಂಟು_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstConst* ಧ್ರುವ tablep
            = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_caseTableWidth), 0};
        ಧ್ರುವ uint32_t tableEntries = 1U << nodep->exprp()->width();

        ಧ್ರುವ ತಾರ್ಕಿಕ isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        ಯದಿ (isTinyTable) {
            ++m_stats.caseTableTiny;
        } ಅನ್ಯಥಾ {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ಧ್ರುವ uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_ಪದಗಳು_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        ಪ್ರತಿ (ಧ್ರುವ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsWidth = lhsRecord.lhsp->width();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            ಯದಿ (lhsRecord.preDefaultp) {
                AstConst* ಧ್ರುವ rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                ಪ್ರತಿ (uint32_t index = 0; index < tableEntries; ++index) {
                    ಧ್ರುವ uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            ಪ್ರತಿ (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ಧ್ರುವ rhsp = [&]() -> AstConst* {
                    ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ಗಂಟು_Assign* ಧ್ರುವ ap = VN_AS(stmtp, ಗಂಟು_Assign);
                        ಯದಿ (lhsRecord.lhsp->same_ವೃಕ್ಷ(ap->lhsp())) ಪ್ರತ್ಯಾಗಮ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    ಪ್ರತ್ಯಾಗಮ VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                ಯದಿ (cip->isDefault()) {
                    ಪ್ರತಿ (uint32_t index = 0; index < tableEntries; ++index) {
                        ಧ್ರುವ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), tableOffset, lhsWidth);
                    }
                    ಅನುವರ್ತ;
                }

                // Iterate case conditions in reverse order
                ಪ್ರತಿ (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    ಯದಿ (neverItem(nodep, condp)) ಅನುವರ್ತ;  // If item never matches, ignore it
                    ಧ್ರುವ auto& match = matchPattern(nodep, condp);
                    ಧ್ರುವ uint32_t matchMask = match.first.toUInt();
                    ಧ್ರುವ uint32_t matchBits = match.second.toUInt();
                    ಧ್ರುವ uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    ಪ್ರತಿ (uint32_t i = inverseMask; ಸತ್ಯ; i = (i - 1) & inverseMask) {
                        ಧ್ರುವ uint32_t index = i | matchBits;
                        ಧ್ರುವ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), tableOffset, lhsWidth);
                        ಯದಿ (!i) ಭಙ್ಗ;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tableRefp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಯದಿ (isTinyTable) ಪ್ರತ್ಯಾಗಮ tablep;
            AstVarRef* ಧ್ರುವ refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_ವೃಕ್ಷ(), tablep);  // V3ConstPool::find clones
            ಪ್ರತ್ಯಾಗಮ refp;
        }();

        // Create the lookup index
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ caseExprp
            = ನೂತನ AstExtend{flp, nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 32};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ scalep = ನೂತನ AstConst{flp, entryWidth};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tableLsbp = ನೂತನ AstMul{flp, scalep, caseExprp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ tableSelp
            = ನೂತನ AstSel{flp, tableRefp, tableLsbp, static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_caseDecoderEntryWidth)};

        // Connect outputs
        ಪ್ರತ್ಯಾಗಮ connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_ಗಂಟು_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_ಗಂಟು*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        ಪ್ರತಿ (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            ಯದಿ (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                ಅನುವರ್ತ;
            }
            ಪ್ರತಿ (Ast_ಗಂಟು* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ಧ್ರುವ iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                ಯದಿ (neverItem(nodep, iconstp)) ಅನುವರ್ತ;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ಧ್ರುವ ತಾರ್ಕಿಕ provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        ಯದಿ (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ condWidth = nodep->exprp()->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ matchWidth = 2 * VL_ಪದಗಳು_I(condWidth) * VL_EDATASIZE;
        AstConst* ಧ್ರುವ matchp
            = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, decoderEnries * matchWidth, 0};
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < decoderEnries; ++i) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            ಯದಿ (AstConst* ಧ್ರುವ condp = clauses[i].first) {
                ಧ್ರುವ auto& match = matchPattern(nodep, condp);
                matchp->ಅಂಕೆ().opSelInto(match.first, entryLsb, condWidth);
                matchp->ಅಂಕೆ().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                ಅನುವರ್ತ;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{flp, condWidth, 0};
            matchp->ಅಂಕೆ().opSelInto(ಅಂಕೆ_Zero, entryLsb, condWidth);
            matchp->ಅಂಕೆ().opSelInto(ಅಂಕೆ_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ಧ್ರುವ rangep = ನೂತನ AstRange{flp, decoderEnries - 1, 0};
        Ast_ಗಂಟು_DType* ಧ್ರುವ entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_ಗಂಟು_DType* ಧ್ರುವ tableDtypep = ನೂತನ AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ಧ್ರುವ tablep = ನೂತನ AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < decoderEnries; ++i) {
            Ast_ಗಂಟು* ಧ್ರುವ stmtsp = clauses[i].second;
            AstConst* ಧ್ರುವ entryp = ನೂತನ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                  static_cast<ಪೂರ್ಣಾಙ್ಕ>(m_caseDecoderEntryWidth), 0};
            ಪ್ರತಿ (ಧ್ರುವ LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ಗಂಟು_Assign* ಧ್ರುವ assignp = VN_AS(stmtp, ಗಂಟು_Assign);
                    ಯದಿ (!lhsp->same_ವೃಕ್ಷ(assignp->lhsp())) ಅನುವರ್ತ;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    ಭಙ್ಗ;
                }
                // Not assigned in this clause, so use the pre-assigned default
                ಯದಿ (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->ಅಂಕೆ().opSelInto(valConstp->ಅಂಕೆ(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ಧ್ರುವ matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ಧ್ರುವ tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_ವೃಕ್ಷ(), matchp);
        VL_DO_DANGLING(tablep->delete_ವೃಕ್ಷ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ caseExprp = nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        AstMatchMasked* ಧ್ರುವ indexp = ನೂತನ AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ entryp = ನೂತನ AstArraySel{flp, tableRefp, indexp};

        ಪ್ರತ್ಯಾಗಮ connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCaseFastRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cexprp, ಪೂರ್ಣಾಙ್ಕ msb, uint32_t upper_ಮೌಲ್ಯ) ಧ್ರುವ {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        ಯದಿ (msb < 0) ಪ್ರತ್ಯಾಗಮ m_caseDetails.records[upper_ಮೌಲ್ಯ].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ಧ್ರುವ uint32_t upperValue0 = upper_ಮೌಲ್ಯ;
        ಧ್ರುವ uint32_t upperValue1 = upper_ಮೌಲ್ಯ | (1UL << msb);
        Ast_ಗಂಟು* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_ಗಂಟು* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        ಯದಿ (tree0p == tree1p) ಪ್ರತ್ಯಾಗಮ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            ತಾರ್ಕಿಕ same = ಸತ್ಯ;
            ಪ್ರತಿ (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                ಯದಿ (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = ಅಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (same) {
                VL_DO_DANGLING(tree1p->delete_ವೃಕ್ಷ(), tree1p);
                ಪ್ರತ್ಯಾಗಮ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        ಯದಿ (tree0p && tree0p->backp()) tree0p = tree0p->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        ಯದಿ (tree1p && tree1p->backp()) tree1p = tree1p->clone_ವೃಕ್ಷ(ಸತ್ಯ);
        // Create the If statement
        File_ಸಾಲು* ಧ್ರುವ flp = cexprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = ನೂತನ AstSel{flp, cexprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), msb, 1};
        AstIf* ಧ್ರುವ ifp = ನೂತನ AstIf{flp, condp, tree1p, tree0p};
        ಪ್ರತ್ಯಾಗಮ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ caseWidth = nodep->exprp()->width();
        Ast_ಗಂಟು* ಧ್ರುವ ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        ಪ್ರತ್ಯಾಗಮ ifrootp && ifrootp->backp() ? ifrootp->clone_ವೃಕ್ಷ(ಸತ್ಯ) : ifrootp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newCaseCompare(ಧ್ರುವ AstCase* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemExprp) {
        File_ಸಾಲು* ಧ್ರುವ flp = itemExprp->fileline();
        ಯದಿ (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemExprp), itemExprp);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConst{flp, AstConst::BitFalse{}};
        }

        ಯದಿ (AstCond* ಧ್ರುವ condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ testp = condp->condp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(itemExprp), itemExprp, condp);
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstCond{flp, testp, thenp, elsep};
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ caseExprp = casep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        ಯದಿ (AstInsideRange* ಧ್ರುವ itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(itemExprp), itemExprp, itemRangep);
            ಪ್ರತ್ಯಾಗಮ resultp;
        }

        ಯದಿ (AstConst* ಧ್ರುವ itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            ಯದಿ (itemConstp->ಅಂಕೆ().isFourState()) {
                ಧ್ರುವ auto& match = matchPattern(casep, itemConstp);
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& matchMask = match.first;
                ಧ್ರುವ V3_ಸಂಖ್ಯೆ& matchBits = match.second;
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(itemExprp), itemExprp, itemConstp);
                ಪ್ರತ್ಯಾಗಮ AstEq::newTyped(flp, ನೂತನ AstConst{flp, matchBits},
                                       ನೂತನ AstAnd{flp, caseExprp, ನೂತನ AstConst{flp, matchMask}});
            }
        }

        ಪ್ರತ್ಯಾಗಮ AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        ತಾರ್ಕಿಕ hasDefault = ಅಸತ್ಯ;
        ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_ಸಾಲು* ಧ್ರುವ flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            ಯದಿ (itemp->isDefault()) {
                itemp->addCondsp(ನೂತನ AstConst{flp, AstConst::BitTrue{}});
                hasDefault = ಸತ್ಯ;
                ಅನುವರ್ತ;
            }

            // Regular clause. Construct the condition expression.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newCondp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                ಯದಿ (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemExprp), itemExprp);
                    ಅನುವರ್ತ;
                }

                // Compute the term to add to the condition expression
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? ನೂತನ AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            ಯದಿ (!newCondp) newCondp = ನೂತನ AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        ಯದಿ (!hasDefault) {
            ಯದಿ (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ಧ್ರುವ lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                ತಳ್ಳು_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(ನೂತನ AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } ಅನ್ಯಥಾ {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(ನೂತನ AstCaseItem{
                    nodep->fileline(), ನೂತನ AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        ಪೂರ್ಣಾಙ್ಕ depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        ಪ್ರತಿ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_ಗಂಟು* ಧ್ರುವ istmtsp = itemp->stmtsp();
            ಯದಿ (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                ಯದಿ (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                ಯದಿ (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ಧ್ರುವ newp = ನೂತನ AstIf{itemp->fileline(), ifexprp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)};
                    ಯದಿ (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } ಅನ್ಯಥಾ {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } ಅನ್ಯಥಾ {  // Continue group, modify if condition to OR in this new condition
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        ನೂತನ AstOr{ifexprp->fileline(), condp, ifexprp->clone_ವೃಕ್ಷ_Pure(ಸತ್ಯ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                ಯದಿ (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_ವೃಕ್ಷ(), itemexprp);
                    itemexprp = ನೂತನ AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ಧ್ರುವ newp = ನೂತನ AstIf{itemp->fileline(), itemexprp, istmtsp};
                ಯದಿ (itemnextp) {
                    itemnextp->addElsesp(newp);
                } ಅನ್ಯಥಾ {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ಧ್ರುವ ತಾರ್ಕಿಕ useTable = [&]() {
            // Not if disabled
            ಯದಿ (!v3Global.opt.fCaseTable()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Not if analysis tells us we can't
            ಯದಿ (!m_caseTableWidth) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            ಯದಿ (m_caseTableWidth <= CASE_TABLE_TINY_BITS) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ಧ್ರುವ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            ಯದಿ (branches < CASE_TABLE_MIN_BRANCHES) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }();
        ಯದಿ (useTable) ಪ್ರತ್ಯಾಗಮ convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ಧ್ರುವ ತಾರ್ಕಿಕ useDecoder = [&]() {
            // Not if disabled
            ಯದಿ (!v3Global.opt.fCaseDecoder()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Not if not a decoder pattern
            ಯದಿ (m_caseDecoderRecords.empty()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ಧ್ರುವ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            ಯದಿ (branches < CASE_TABLE_MIN_BRANCHES) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }();
        ಯದಿ (useDecoder) ಪ್ರತ್ಯಾಗಮ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ಧ್ರುವ ತಾರ್ಕಿಕ use_Fast_Bit_ವೃಕ್ಷ = [&]() {
            // Not if disabled
            ಯದಿ (!v3Global.opt.f_Case_ವೃಕ್ಷ()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Can't do it without the detailed analysis
            ಯದಿ (!m_caseDetailsValid) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Can't do it if not exhaustive
            ಯದಿ (!m_caseDetails.exhaustive) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Not worth doing if there are few conditions
            ಯದಿ (m_caseNConditions <= 3) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ಧ್ರುವ size_t caseWidth = nodep->exprp()->width();
            ಯದಿ (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            // Otherwise use the bit tree
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }();
        ಯದಿ (use_Fast_Bit_ವೃಕ್ಷ) ಪ್ರತ್ಯಾಗಮ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        ಯದಿ (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, ಸತ್ಯ);
        }
        ಪ್ರತ್ಯಾಗಮ convertCaseGeneric(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ visit(AstCase* nodep) override {
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
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            ಯದಿ (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_ವೃಕ್ಷ(), stmtp);
            } ಅನ್ಯಥಾ {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
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

ಶೂನ್ಯ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("case", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
ಶೂನ್ಯ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
