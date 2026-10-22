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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3Case.h"

#ଅନ୍ତର୍ଭାଵ "V3ConstPool.h"
#ଅନ୍ତର୍ଭାଵ "V3SharedTmps.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ଶ୍ରେଣୀ CaseLintVisitor final : ସର୍ବସାଧାରଣ VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ଅଚଳ Ast_ଗଣ୍ଠି* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    ସ୍ଥିର ଶୂନ୍ୟ detectMultipleDefaults(CaseItem* itemsp) {
        ତାର୍କିକ hitDefault = ମିଥ୍ୟା;
        ପାଇଁ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<CaseItem>(itemp->nextp())) {
            ଯଦି (!itemp->isDefault()) ଜାରି;
            ଯଦି (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = ସତ୍ୟ;
        }
    }

    template <typename CaseItem>
    ଶୂନ୍ୟ checkXZinNonCaseX(Ast_ଗଣ୍ଠି* casep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        ପାଇଁ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ଗଣ୍ଠି::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    ଶୂନ୍ୟ visit(AstCase* nodep) override {
        ଯଦି (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    ଶୂନ୍ୟ visit(AstConst* nodep) override {
        ଯଦି (!nodep->ଅଙ୍କ().isFourState()) ଫେରନ୍ତୁ;

        // Error if generate case
        ଯଦି (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            ଫେରନ୍ତୁ;
        }

        // Otherwise must be a case statement
        ଅଚଳ AstCase* ଅଚଳ casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        ଯଦି (casep->casex()) ଫେରନ୍ତୁ;

        ଯଦି (casep->casez() || casep->caseInside()) {
            ଯଦି (nodep->ଅଙ୍କ().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            ଫେରନ୍ତୁ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ସ୍ଥିର ଶୂନ୍ୟ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

ଶ୍ରେଣୀ CaseVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // Maximum width for detailed analysis
    constexpr ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ସ୍ଥିର size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ସ୍ଥିର size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ସ୍ଥିର size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ସଂରଚନା CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ଗଣ୍ଠି* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ସଂରଚନା LhsRecord final {
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* lhsp = nullptr;  // LHS of the assignment
        Ast_ଗଣ୍ଠି_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        ସ୍ଥିର size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ସଂରଚନା Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ଅଚଳ Ast_ଗଣ୍ଠି* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    ତାର୍କିକ m_caseOpaque = ମିଥ୍ୟା;  // Case statement is opaque (non-packed, or non-const conditions)
    ତାର୍କିକ m_caseHasDefault = ମିଥ୍ୟା;  // Indicates the case statement has a default case
    size_t m_case_NCase_ବସ୍ତୁଗୁଡ଼ିକ = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    ତାର୍କିକ m_caseDetailsValid = ମିଥ୍ୟା;  // Indicates m_caseDetails is valid
    ସଂରଚନା final {
        ତାର୍କିକ exhaustive = ମିଥ୍ୟା;  // Proven exhaustive
        ତାର୍କିକ exhaustiveOverEnumOnly = ମିଥ୍ୟା;  // Exhaustive over enum values only
        ତାର୍କିକ noOverlaps = ମିଥ୍ୟା;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ସ୍ଥିର ତାର୍କିକ neverItem(ଅଚଳ AstCase* casep, ଅଚଳ Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemExprp) {
        ଅଚଳ AstConst* ଅଚଳ constp = VN_CAST(itemExprp, Const);
        ଯଦି (!constp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (casep->casex() || casep->caseInside()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (casep->casez()) ଫେରନ୍ତୁ constp->ଅଙ୍କ().isAnyX();
        ଫେରନ୍ତୁ constp->ଅଙ୍କ().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ସ୍ଥିର std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା> matchPattern(ଅଚଳ AstCase* /*casep*/,
                                                      ଅଚଳ AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_ସଂଖ୍ୟା, V3_ସଂଖ୍ୟା> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->ଅଙ୍କ());
        pairMaskBits.second.opBitsOne(condp->ଅଙ୍କ());
        ଫେରନ୍ତୁ pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ସ୍ଥିର Ast_ଗଣ୍ଠି_Assign* checkDecoderAssign(Ast_ଗଣ୍ଠି* stmtp) {
        // Only Assign and AssignDly are supported
        ଯଦି (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) ଫେରନ୍ତୁ nullptr;
        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = VN_AS(stmtp, ଗଣ୍ଠି_Assign);
        // Only if no timing control
        ଯଦି (assp->timingControlp()) ଫେରନ୍ତୁ nullptr;
        // Only if assigning a constant
        ଯଦି (!VN_IS(assp->rhsp(), Const)) ଫେରନ୍ତୁ nullptr;
        // Only if it's a packed value
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ dtypep = assp->rhsp()->dtypep();
        ଯଦି (dtypep->isString() || dtypep->isDouble()) ଫେରନ୍ତୁ nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        ଯଦି (assp->lhsp()->exists([](AstVarRef* refp) { ଫେରନ୍ତୁ refp->access().isReadOrRW(); })) {
            ଫେରନ୍ତୁ nullptr;
        }
        // This is an assignment that fits the decoder pattern
        ଫେରନ୍ତୁ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    ତାର୍କିକ analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ଅଚଳ VNUser1InUse user1InUse;
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = checkDecoderAssign(stmtp);
            ଯଦି (!assp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Must assign each LHS exactly once - RHS is Const
            ଅଚଳ ତାର୍କିକ multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                ଫେରନ୍ତୁ refp->varScopep()->user1SetOnce();
            });
            ଯଦି (multipleAssignments) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            ଯଦି (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ସ୍ଥିର ଅଚଳ AstEnumDType* getEnumCompletionCheckDType(ଅଚଳ AstCase* ଅଚଳ nodep) {
        ଯଦି (!nodep->uniquePragma() && !nodep->unique0Pragma()) ଫେରନ୍ତୁ nullptr;
        ଅଚଳ AstEnumDType* ଅଚଳ enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        ଯଦି (!enumDtp) ଫେରନ୍ତୁ nullptr;  // Case isn't enum
        ଅଚଳ AstBasicDType* ଅଚଳ basicp = enumDtp->subDTypep()->basicp();
        ଯଦି (!basicp) ଫେରନ୍ତୁ nullptr;  // Not simple type (perhaps IEEE illegal)
        ଫେରନ୍ତୁ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    ତାର୍କିକ checkExhaustiveEnum(ଅଚଳ AstCase* ଅଚଳ nodep, ଅଚଳ AstEnumDType* ଅଚଳ enump) {
        ଅଚଳ uint32_t ଅଙ୍କ_Cases = 1UL << nodep->exprp()->width();
        ତାର୍କିକ fullyCovered = ସତ୍ୟ;
        string missing_ବସ୍ତୁଗୁଡ଼ିକ;
        ପାଇଁ (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ଅଚଳ auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ଅଚଳ uint32_t mask = match.first.toUInt();
            ଅଚଳ uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            ପାଇଁ (uint32_t i = 0; i < ଅଙ୍କ_Cases; ++i) {
                ଯଦି ((i & mask) != bits) ଜାରି;  // This case is not for this enum value
                ଯଦି (m_caseDetails.records[i].itemp) ଜାରି;  // Covered case
                ଯଦି (!missing_ବସ୍ତୁଗୁଡ଼ିକ.empty()) missing_ବସ୍ତୁଗୁଡ଼ିକ += ", ";
                missing_ବସ୍ତୁଗୁଡ଼ିକ += eip->prettyNameQ();
                fullyCovered = ମିଥ୍ୟା;
                ଭାଙ୍ଗ;
            }
        }
        ଯଦି (!missing_ବସ୍ତୁଗୁଡ଼ିକ.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_ବସ୍ତୁଗୁଡ଼ିକ);
        }
        ଫେରନ୍ତୁ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    ତାର୍କିକ checkExhaustivePacked(AstCase* nodep) {
        ଅଚଳ uint32_t ଅଙ୍କ_Cases = 1UL << nodep->exprp()->width();
        ପାଇଁ (uint32_t i = 0; i < ଅଙ୍କ_Cases; ++i) {
            ଯଦି (m_caseDetails.records[i].itemp) ଜାରି;  // Covered case
            ଯଦି (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            ଫେରନ୍ତୁ ମିଥ୍ୟା;
        }

        // It's an exhaustive case statement
        ଫେରନ୍ତୁ ସତ୍ୟ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    ଶୂନ୍ୟ analyzeCaseDetails(AstCase* nodep) {
        ଅଚଳ uint32_t ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ = 1UL << nodep->exprp()->width();
        // Clear case records
        ପାଇଁ (uint32_t i = 0; i < ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        ତାର୍କିକ reportedOverlap = ମିଥ୍ୟା;
        ତାର୍କିକ hasDefault = ମିଥ୍ୟା;
        m_caseDetails.noOverlaps = ସତ୍ୟ;
        ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            ଯଦି (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                ପାଇଁ (uint32_t i = 0; i < ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    ଯଦି (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = ସତ୍ୟ;
                ଜାରି;
            }

            ପାଇଁ (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                ଯଦି (neverItem(nodep, iconstp)) ଜାରି;

                ଅଚଳ auto& match = matchPattern(nodep, iconstp);
                ଅଚଳ uint32_t mask = match.first.toUInt();
                ଅଚଳ uint32_t bits = match.second.toUInt();

                ତାର୍କିକ foundNewCase = ମିଥ୍ୟା;
                ଅଚଳ AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_ମୂଲ୍ୟ = 0;
                ପାଇଁ (uint32_t i = 0; i < ଅଙ୍କ_ମୂଲ୍ୟଗୁଡ଼ିକ; ++i) {
                    ଯଦି ((i & mask) != bits) ଜାରି;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    ଯଦି (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = ସତ୍ୟ;
                        ଜାରି;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    ଯଦି (caseRecord.itemp == itemp) ଜାରି;

                    // Otherwise record the first overlapping case
                    ଯଦି (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_ମୂଲ୍ୟ = i;
                        m_caseDetails.noOverlaps = ମିଥ୍ୟା;
                    }
                }

                // Only report first overlap
                ଯଦି (reportedOverlap || !firstOverlapConstp) ଜାରି;

                // Report first overlap
                ଯଦି (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    ଯଦି (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = ସତ୍ୟ;
                    }
                } ଅନ୍ୟଥା {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    ଯଦି (iconstp->ଅଙ୍କ().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_ମୂଲ୍ୟ
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = ସତ୍ୟ;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = ମିଥ୍ୟା;
        m_caseDetails.exhaustive = hasDefault;
        ଯଦି (!hasDefault) {
            ଯଦି (ଅଚଳ AstEnumDType* ଅଚଳ enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ଅଚଳ ତାର୍କିକ exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } ଅନ୍ୟଥା {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = ସତ୍ୟ;
    }

    ଶୂନ୍ୟ analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        ପାଇଁ (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ଅଚଳ LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            ଯଦି (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                ଜାରି;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            ଯଦି (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) ଫେରନ୍ତୁ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            ଯଦି (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                ଯଦି (lhsRecord.nCaseAssigns == m_case_NCase_ବସ୍ତୁଗୁଡ଼ିକ) ଜାରି;
                ଯଦି (lhsRecord.nCaseAssignDlys == m_case_NCase_ବସ୍ତୁଗୁଡ଼ିକ) ଜାରି;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ଗଣ୍ଠି* ଅଚଳ preDefaultp = lhsRecord.preDefaultp;
            ଯଦି (!preDefaultp) ଫେରନ୍ତୁ;
            // And the pre-default needs to be the same type
            ଯଦି (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) ଫେରନ୍ତୁ;
            ଯଦି (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) ଫେରନ୍ତୁ;
        }
        // All cases check out, can optimize if there are some entries left
        ଯଦି (m_caseLhsRecords.empty()) ଫେରନ୍ତୁ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        ପାଇଁ (ଅଚଳ auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ଅଚଳ LhsRecord& a, ଅଚଳ LhsRecord& b) {
                      // Sort by width, then id
                      ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା aWidth = a.lhsp->width();
                      ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା bWidth = b.lhsp->width();
                      ଯଦି (aWidth != bWidth) ଫେରନ୍ତୁ aWidth < bWidth;
                      ଫେରନ୍ତୁ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ଅଚଳ auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> ତାର୍କିକ {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                ଯଦି (totalWidth > maxWidth) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            }
            ଫେରନ୍ତୁ ସତ୍ୟ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        ପାଇଁ (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        ଯଦି (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            ଫେରନ୍ତୁ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        ପାଇଁ (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ଅଚଳ size_t width = lhsRecord.lhsp->width();
            ଅଚଳ size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ଅଚଳ size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            ଯଦି (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_ଶବ୍ଦଗୁଡ଼ିକ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ଅଚଳ size_t alignedEntryWidth = VL_ଶବ୍ଦଗୁଡ଼ିକ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        ଯଦି (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            ଫେରନ୍ତୁ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    ଶୂନ୍ୟ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = ମିଥ୍ୟା;
        m_caseHasDefault = ମିଥ୍ୟା;
        m_case_NCase_ବସ୍ତୁଗୁଡ଼ିକ = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = ମିଥ୍ୟା;

        Ast_ଗଣ୍ଠି* ଅଚଳ caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        ଯଦି (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = ସତ୍ୟ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ଅଚଳ VNUser1InUse user1InUse;
            ପାଇଁ (Ast_ଗଣ୍ଠି* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assp = checkDecoderAssign(prevp);
                ଯଦି (!assp) ଭାଙ୍ଗ;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ଅଚଳ ତାର୍କିକ multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    ଫେରନ୍ତୁ refp->varScopep()->user1SetOnce();
                });
                ଯଦି (multipleAssignments) ଭାଙ୍ଗ;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        ତାର୍କିକ canBeDecoder = ସତ୍ୟ;
        ପାଇଁ (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            ପାଇଁ (Ast_ଗଣ୍ଠି* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                ଯଦି (!neverItem(nodep, VN_AS(condp, ଗଣ୍ଠି_ପଦସମଷ୍ଟି))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                ଯଦି (!VN_IS(condp, Const)) {
                    m_caseOpaque = ସତ୍ୟ;
                    canBeDecoder = ମିଥ୍ୟା;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            ଯଦି (cip->isDefault()) m_caseHasDefault = ସତ୍ୟ;
            // Count case items
            ++m_case_NCase_ବସ୍ତୁଗୁଡ଼ିକ;
            // Check if it fits the decoder pattern, if still possible
            ଯଦି (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        ଯଦି (m_caseOpaque) ଫେରନ୍ତୁ;

        // If small enough, analyse details
        ଯଦି (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        ଯଦି (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_ଗଣ୍ଠି_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* exprp, V3SharedTmps& tmps) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        // If there is only one LHS, just use the result
        ଯଦି (m_caseDecoderRecords.size() == 1) {
            ଅଚଳ LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = lhsRecord.lhsp->width();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp
                = exprp->width() == width ? exprp : ନୂତନ AstSel{flp, exprp, 0, width};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = lhsRecord.lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (lhsRecord.nCaseAssigns) {
                ଫେରନ୍ତୁ ନୂତନ AstAssign{flp, lhsp, rhsp};
            } ଅନ୍ୟଥା ଯଦି (lhsRecord.nCaseAssignDlys) {
                ଫେରନ୍ତୁ ନୂତନ AstAssignDly{flp, lhsp, rhsp};
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ଅଚଳ tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tempWritep = ନୂତନ AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ resultp = ନୂତନ AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        ପାଇଁ (ଅଚଳ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = lhsRecord.lhsp->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = lhsRecord.offset;
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tempReadp = ନୂତନ AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ rhsp = ନୂତନ AstSel{flp, tempReadp, lsb, width};
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = lhsRecord.lhsp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
            ଯଦି (lhsRecord.nCaseAssigns) {
                resultp->addNext(ନୂତନ AstAssign{flp, lhsp, rhsp});
            } ଅନ୍ୟଥା ଯଦି (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(ନୂତନ AstAssignDly{flp, lhsp, rhsp});
            } ଅନ୍ୟଥା {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        ଫେରନ୍ତୁ resultp;
    }

    Ast_ଗଣ୍ଠି_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        AstConst* ଅଚଳ tablep
            = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_caseTableWidth), 0};
        ଅଚଳ uint32_t tableEntries = 1U << nodep->exprp()->width();

        ଅଚଳ ତାର୍କିକ isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        ଯଦି (isTinyTable) {
            ++m_stats.caseTableTiny;
        } ଅନ୍ୟଥା {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ଅଚଳ uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_ଶବ୍ଦଗୁଡ଼ିକ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        ପାଇଁ (ଅଚଳ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsWidth = lhsRecord.lhsp->width();
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            ଯଦି (lhsRecord.preDefaultp) {
                AstConst* ଅଚଳ rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                ପାଇଁ (uint32_t index = 0; index < tableEntries; ++index) {
                    ଅଚଳ uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->ଅଙ୍କ().opSelInto(rhsp->ଅଙ୍କ(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            ପାଇଁ (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ଅଚଳ rhsp = [&]() -> AstConst* {
                    ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ଗଣ୍ଠି_Assign* ଅଚଳ ap = VN_AS(stmtp, ଗଣ୍ଠି_Assign);
                        ଯଦି (lhsRecord.lhsp->same_ବୃକ୍ଷ(ap->lhsp())) ଫେରନ୍ତୁ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    ଫେରନ୍ତୁ VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                ଯଦି (cip->isDefault()) {
                    ପାଇଁ (uint32_t index = 0; index < tableEntries; ++index) {
                        ଅଚଳ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->ଅଙ୍କ().opSelInto(rhsp->ଅଙ୍କ(), tableOffset, lhsWidth);
                    }
                    ଜାରି;
                }

                // Iterate case conditions in reverse order
                ପାଇଁ (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    ଯଦି (neverItem(nodep, condp)) ଜାରି;  // If item never matches, ignore it
                    ଅଚଳ auto& match = matchPattern(nodep, condp);
                    ଅଚଳ uint32_t matchMask = match.first.toUInt();
                    ଅଚଳ uint32_t matchBits = match.second.toUInt();
                    ଅଚଳ uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    ପାଇଁ (uint32_t i = inverseMask; ସତ୍ୟ; i = (i - 1) & inverseMask) {
                        ଅଚଳ uint32_t index = i | matchBits;
                        ଅଚଳ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->ଅଙ୍କ().opSelInto(rhsp->ଅଙ୍କ(), tableOffset, lhsWidth);
                        ଯଦି (!i) ଭାଙ୍ଗ;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tableRefp = [&]() -> Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* {
            ଯଦି (isTinyTable) ଫେରନ୍ତୁ tablep;
            AstVarRef* ଅଚଳ refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_ବୃକ୍ଷ(), tablep);  // V3ConstPool::find clones
            ଫେରନ୍ତୁ refp;
        }();

        // Create the lookup index
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ caseExprp
            = ନୂତନ AstExtend{flp, nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), 32};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ scalep = ନୂତନ AstConst{flp, entryWidth};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tableLsbp = ନୂତନ AstMul{flp, scalep, caseExprp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ tableSelp
            = ନୂତନ AstSel{flp, tableRefp, tableLsbp, static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_caseDecoderEntryWidth)};

        // Connect outputs
        ଫେରନ୍ତୁ connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_ଗଣ୍ଠି_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_ଗଣ୍ଠି*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        ପାଇଁ (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            ଯଦି (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                ଜାରି;
            }
            ପାଇଁ (Ast_ଗଣ୍ଠି* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ଅଚଳ iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                ଯଦି (neverItem(nodep, iconstp)) ଜାରି;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ଅଚଳ ତାର୍କିକ provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        ଯଦି (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା condWidth = nodep->exprp()->width();
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା matchWidth = 2 * VL_ଶବ୍ଦଗୁଡ଼ିକ_I(condWidth) * VL_EDATASIZE;
        AstConst* ଅଚଳ matchp
            = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{}, decoderEnries * matchWidth, 0};
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < decoderEnries; ++i) {
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            ଯଦି (AstConst* ଅଚଳ condp = clauses[i].first) {
                ଅଚଳ auto& match = matchPattern(nodep, condp);
                matchp->ଅଙ୍କ().opSelInto(match.first, entryLsb, condWidth);
                matchp->ଅଙ୍କ().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                ଜାରି;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_ସଂଖ୍ୟା ଅଙ୍କ_Zero{flp, condWidth, 0};
            matchp->ଅଙ୍କ().opSelInto(ଅଙ୍କ_Zero, entryLsb, condWidth);
            matchp->ଅଙ୍କ().opSelInto(ଅଙ୍କ_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ଅଚଳ rangep = ନୂତନ AstRange{flp, decoderEnries - 1, 0};
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ tableDtypep = ନୂତନ AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ଅଚଳ tablep = ନୂତନ AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < decoderEnries; ++i) {
            Ast_ଗଣ୍ଠି* ଅଚଳ stmtsp = clauses[i].second;
            AstConst* ଅଚଳ entryp = ନୂତନ AstConst{flp, AstConst::Widthed_ମୂଲ୍ୟ{},
                                                  static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(m_caseDecoderEntryWidth), 0};
            ପାଇଁ (ଅଚଳ LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ଗଣ୍ଠି_Assign* ଅଚଳ assignp = VN_AS(stmtp, ଗଣ୍ଠି_Assign);
                    ଯଦି (!lhsp->same_ବୃକ୍ଷ(assignp->lhsp())) ଜାରି;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    ଭାଙ୍ଗ;
                }
                // Not assigned in this clause, so use the pre-assigned default
                ଯଦି (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->ଅଙ୍କ().opSelInto(valConstp->ଅଙ୍କ(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ଅଚଳ matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ଅଚଳ tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_ବୃକ୍ଷ(), matchp);
        VL_DO_DANGLING(tablep->delete_ବୃକ୍ଷ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ caseExprp = nodep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        AstMatchMasked* ଅଚଳ indexp = ନୂତନ AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ entryp = ନୂତନ AstArraySel{flp, tableRefp, indexp};

        ଫେରନ୍ତୁ connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ଗଣ୍ଠି* convertCaseFastRecurse(Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* cexprp, ପୂର୍ଣ୍ଣସଂଖ୍ୟା msb, uint32_t upper_ମୂଲ୍ୟ) ଅଚଳ {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        ଯଦି (msb < 0) ଫେରନ୍ତୁ m_caseDetails.records[upper_ମୂଲ୍ୟ].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ଅଚଳ uint32_t upperValue0 = upper_ମୂଲ୍ୟ;
        ଅଚଳ uint32_t upperValue1 = upper_ମୂଲ୍ୟ | (1UL << msb);
        Ast_ଗଣ୍ଠି* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_ଗଣ୍ଠି* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        ଯଦି (tree0p == tree1p) ଫେରନ୍ତୁ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            ତାର୍କିକ same = ସତ୍ୟ;
            ପାଇଁ (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                ଯଦି (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = ମିଥ୍ୟା;
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (same) {
                VL_DO_DANGLING(tree1p->delete_ବୃକ୍ଷ(), tree1p);
                ଫେରନ୍ତୁ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        ଯଦି (tree0p && tree0p->backp()) tree0p = tree0p->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        ଯଦି (tree1p && tree1p->backp()) tree1p = tree1p->clone_ବୃକ୍ଷ(ସତ୍ୟ);
        // Create the If statement
        File_ଧାଡ଼ି* ଅଚଳ flp = cexprp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = ନୂତନ AstSel{flp, cexprp->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା), msb, 1};
        AstIf* ଅଚଳ ifp = ନୂତନ AstIf{flp, condp, tree1p, tree0p};
        ଫେରନ୍ତୁ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ଗଣ୍ଠି* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା caseWidth = nodep->exprp()->width();
        Ast_ଗଣ୍ଠି* ଅଚଳ ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        ଫେରନ୍ତୁ ifrootp && ifrootp->backp() ? ifrootp->clone_ବୃକ୍ଷ(ସତ୍ୟ) : ifrootp;
    }

    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newCaseCompare(ଅଚଳ AstCase* casep, Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemExprp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = itemExprp->fileline();
        ଯଦି (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(ଠେଲ_Deletep(itemExprp), itemExprp);
            ଫେରନ୍ତୁ ନୂତନ AstConst{flp, AstConst::BitFalse{}};
        }

        ଯଦି (AstCond* ଅଚଳ condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ testp = condp->condp()->unlinkFrBack();
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(ଠେଲ_Deletep(itemExprp), itemExprp, condp);
            ଫେରନ୍ତୁ ନୂତନ AstCond{flp, testp, thenp, elsep};
        }

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ caseExprp = casep->exprp()->clone_ବୃକ୍ଷ_Pure(ମିଥ୍ୟା);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        ଯଦି (AstInsideRange* ଅଚଳ itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(ଠେଲ_Deletep(itemExprp), itemExprp, itemRangep);
            ଫେରନ୍ତୁ resultp;
        }

        ଯଦି (AstConst* ଅଚଳ itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            ଯଦି (itemConstp->ଅଙ୍କ().isFourState()) {
                ଅଚଳ auto& match = matchPattern(casep, itemConstp);
                ଅଚଳ V3_ସଂଖ୍ୟା& matchMask = match.first;
                ଅଚଳ V3_ସଂଖ୍ୟା& matchBits = match.second;
                VL_DO_DANGLING2(ଠେଲ_Deletep(itemExprp), itemExprp, itemConstp);
                ଫେରନ୍ତୁ AstEq::newTyped(flp, ନୂତନ AstConst{flp, matchBits},
                                       ନୂତନ AstAnd{flp, caseExprp, ନୂତନ AstConst{flp, matchMask}});
            }
        }

        ଫେରନ୍ତୁ AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ଗଣ୍ଠି* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        ତାର୍କିକ hasDefault = ମିଥ୍ୟା;
        ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_ଧାଡ଼ି* ଅଚଳ flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            ଯଦି (itemp->isDefault()) {
                itemp->addCondsp(ନୂତନ AstConst{flp, AstConst::BitTrue{}});
                hasDefault = ସତ୍ୟ;
                ଜାରି;
            }

            // Regular clause. Construct the condition expression.
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newCondp = nullptr;
            ପାଇଁ (Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), ଗଣ୍ଠି_ପଦସମଷ୍ଟି);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                ଯଦି (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(ଠେଲ_Deletep(itemExprp), itemExprp);
                    ଜାରି;
                }

                // Compute the term to add to the condition expression
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? ନୂତନ AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            ଯଦି (!newCondp) newCondp = ନୂତନ AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        ଯଦି (!hasDefault) {
            ଯଦି (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ଅଚଳ lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                ଠେଲ_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(ନୂତନ AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } ଅନ୍ୟଥା {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(ନୂତନ AstCaseItem{
                    nodep->fileline(), ନୂତନ AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        ପାଇଁ (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_ଗଣ୍ଠି* ଅଚଳ istmtsp = itemp->stmtsp();
            ଯଦି (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                ଯଦି (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                ଯଦି (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ଅଚଳ newp = ନୂତନ AstIf{itemp->fileline(), ifexprp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)};
                    ଯଦି (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } ଅନ୍ୟଥା {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } ଅନ୍ୟଥା {  // Continue group, modify if condition to OR in this new condition
                    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        ନୂତନ AstOr{ifexprp->fileline(), condp, ifexprp->clone_ବୃକ୍ଷ_Pure(ସତ୍ୟ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                ଯଦି (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_ବୃକ୍ଷ(), itemexprp);
                    itemexprp = ନୂତନ AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ଅଚଳ newp = ନୂତନ AstIf{itemp->fileline(), itemexprp, istmtsp};
                ଯଦି (itemnextp) {
                    itemnextp->addElsesp(newp);
                } ଅନ୍ୟଥା {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        ଫେରନ୍ତୁ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ଗଣ୍ଠି* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ଅଚଳ ତାର୍କିକ useTable = [&]() {
            // Not if disabled
            ଯଦି (!v3Global.opt.fCaseTable()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Not if analysis tells us we can't
            ଯଦି (!m_caseTableWidth) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            ଯଦି (m_caseTableWidth <= CASE_TABLE_TINY_BITS) ଫେରନ୍ତୁ ସତ୍ୟ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ଅଚଳ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            ଯଦି (branches < CASE_TABLE_MIN_BRANCHES) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }();
        ଯଦି (useTable) ଫେରନ୍ତୁ convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ଅଚଳ ତାର୍କିକ useDecoder = [&]() {
            // Not if disabled
            ଯଦି (!v3Global.opt.fCaseDecoder()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Not if not a decoder pattern
            ଯଦି (m_caseDecoderRecords.empty()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ଅଚଳ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            ଯଦି (branches < CASE_TABLE_MIN_BRANCHES) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }();
        ଯଦି (useDecoder) ଫେରନ୍ତୁ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ଅଚଳ ତାର୍କିକ use_Fast_Bit_ବୃକ୍ଷ = [&]() {
            // Not if disabled
            ଯଦି (!v3Global.opt.f_Case_ବୃକ୍ଷ()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Can't do it without the detailed analysis
            ଯଦି (!m_caseDetailsValid) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Can't do it if not exhaustive
            ଯଦି (!m_caseDetails.exhaustive) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Not worth doing if there are few conditions
            ଯଦି (m_caseNConditions <= 3) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Avoid e.g. priority expanders from going crazy in expansion
            ଅଚଳ size_t caseWidth = nodep->exprp()->width();
            ଯଦି (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) ଫେରନ୍ତୁ ମିଥ୍ୟା;
            // Otherwise use the bit tree
            ଫେରନ୍ତୁ ସତ୍ୟ;
        }();
        ଯଦି (use_Fast_Bit_ବୃକ୍ଷ) ଫେରନ୍ତୁ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        ଯଦି (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, ସତ୍ୟ);
        }
        ଫେରନ୍ତୁ convertCaseGeneric(nodep);
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstCase* nodep) override {
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
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            ଯଦି (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_ବୃକ୍ଷ(), stmtp);
            } ଅନ୍ୟଥା {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        ଯଦି (Ast_ଗଣ୍ଠି* ଅଚଳ replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } ଅନ୍ୟଥା {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ବୃକ୍ଷ(), nodep);
    }

    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
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

ଶୂନ୍ୟ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("case", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
ଶୂନ୍ୟ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
