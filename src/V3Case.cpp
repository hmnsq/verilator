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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3Case.h"

#અન્તર્ભાવ "V3ConstPool.h"
#અન્તર્ભાવ "V3SharedTmps.h"
#અન્તર્ભાવ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

વર્ગ CaseLintVisitor final : જાહેર VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    અચલ Ast_ગાંઠ* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    સ્થિર શૂન્ય detectMultipleDefaults(CaseItem* itemsp) {
        તાર્કિક hitDefault = ખોટું;
        માટે (CaseItem* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<CaseItem>(itemp->nextp())) {
            જો (!itemp->isDefault()) ચાલુ;
            જો (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = સાચું;
        }
    }

    template <typename CaseItem>
    શૂન્ય checkXZinNonCaseX(Ast_ગાંઠ* casep, Ast_ગાંઠ_પદાવલિ* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        માટે (CaseItem* itemp = itemsp; itemp; itemp = Ast_ગાંઠ::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    શૂન્ય visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    શૂન્ય visit(AstCase* nodep) override {
        જો (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    શૂન્ય visit(AstConst* nodep) override {
        જો (!nodep->અંક().isFourState()) પાછા;

        // Error if generate case
        જો (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            પાછા;
        }

        // Otherwise must be a case statement
        અચલ AstCase* અચલ casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        જો (casep->casex()) પાછા;

        જો (casep->casez() || casep->caseInside()) {
            જો (nodep->અંક().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            પાછા;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = મૂળભૂત;

જાહેર:
    સ્થિર શૂન્ય apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    સ્થિર શૂન્ય apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

વર્ગ CaseVisitor final : જાહેર VNVisitor {
    // Maximum width for detailed analysis
    constexpr સ્થિર પૂર્ણાંક CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr સ્થિર પૂર્ણાંક CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr સ્થિર size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr સ્થિર size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr સ્થિર size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    સંરચના CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ગાંઠ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    સંરચના LhsRecord final {
        Ast_ગાંઠ_પદાવલિ* lhsp = nullptr;  // LHS of the assignment
        Ast_ગાંઠ_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        સ્થિર size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    સંરચના Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    અચલ Ast_ગાંઠ* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    તાર્કિક m_caseOpaque = ખોટું;  // Case statement is opaque (non-packed, or non-const conditions)
    તાર્કિક m_caseHasDefault = ખોટું;  // Indicates the case statement has a default case
    size_t m_case_NCase_વસ્તુઓ = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ગાંઠ_પદાવલિ>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    તાર્કિક m_caseDetailsValid = ખોટું;  // Indicates m_caseDetails is valid
    સંરચના final {
        તાર્કિક exhaustive = ખોટું;  // Proven exhaustive
        તાર્કિક exhaustiveOverEnumOnly = ખોટું;  // Exhaustive over enum values only
        તાર્કિક noOverlaps = ખોટું;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    સ્થિર તાર્કિક neverItem(અચલ AstCase* casep, અચલ Ast_ગાંઠ_પદાવલિ* itemExprp) {
        અચલ AstConst* અચલ constp = VN_CAST(itemExprp, Const);
        જો (!constp) પાછા ખોટું;
        જો (casep->casex() || casep->caseInside()) પાછા ખોટું;
        જો (casep->casez()) પાછા constp->અંક().isAnyX();
        પાછા constp->અંક().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    સ્થિર std::pair<V3_સંખ્યા, V3_સંખ્યા> matchPattern(અચલ AstCase* /*casep*/,
                                                      અચલ AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_સંખ્યા, V3_સંખ્યા> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->અંક());
        pairMaskBits.second.opBitsOne(condp->અંક());
        પાછા pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    સ્થિર Ast_ગાંઠ_Assign* checkDecoderAssign(Ast_ગાંઠ* stmtp) {
        // Only Assign and AssignDly are supported
        જો (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) પાછા nullptr;
        Ast_ગાંઠ_Assign* અચલ assp = VN_AS(stmtp, ગાંઠ_Assign);
        // Only if no timing control
        જો (assp->timingControlp()) પાછા nullptr;
        // Only if assigning a constant
        જો (!VN_IS(assp->rhsp(), Const)) પાછા nullptr;
        // Only if it's a packed value
        Ast_ગાંઠ_DType* અચલ dtypep = assp->rhsp()->dtypep();
        જો (dtypep->isString() || dtypep->isDouble()) પાછા nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        જો (assp->lhsp()->exists([](AstVarRef* refp) { પાછા refp->access().isReadOrRW(); })) {
            પાછા nullptr;
        }
        // This is an assignment that fits the decoder pattern
        પાછા assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    તાર્કિક analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        અચલ VNUser1InUse user1InUse;
        માટે (Ast_ગાંઠ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ગાંઠ_Assign* અચલ assp = checkDecoderAssign(stmtp);
            જો (!assp) પાછા ખોટું;
            // Must assign each LHS exactly once - RHS is Const
            અચલ તાર્કિક multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                પાછા refp->varScopep()->user1SetOnce();
            });
            જો (multipleAssignments) પાછા ખોટું;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            જો (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        પાછા સાચું;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    સ્થિર અચલ AstEnumDType* getEnumCompletionCheckDType(અચલ AstCase* અચલ nodep) {
        જો (!nodep->uniquePragma() && !nodep->unique0Pragma()) પાછા nullptr;
        અચલ AstEnumDType* અચલ enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        જો (!enumDtp) પાછા nullptr;  // Case isn't enum
        અચલ AstBasicDType* અચલ basicp = enumDtp->subDTypep()->basicp();
        જો (!basicp) પાછા nullptr;  // Not simple type (perhaps IEEE illegal)
        પાછા enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    તાર્કિક checkExhaustiveEnum(અચલ AstCase* અચલ nodep, અચલ AstEnumDType* અચલ enump) {
        અચલ uint32_t અંક_Cases = 1UL << nodep->exprp()->width();
        તાર્કિક fullyCovered = સાચું;
        string missing_વસ્તુઓ;
        માટે (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            અચલ auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            અચલ uint32_t mask = match.first.toUInt();
            અચલ uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            માટે (uint32_t i = 0; i < અંક_Cases; ++i) {
                જો ((i & mask) != bits) ચાલુ;  // This case is not for this enum value
                જો (m_caseDetails.records[i].itemp) ચાલુ;  // Covered case
                જો (!missing_વસ્તુઓ.empty()) missing_વસ્તુઓ += ", ";
                missing_વસ્તુઓ += eip->prettyNameQ();
                fullyCovered = ખોટું;
                તોડો;
            }
        }
        જો (!missing_વસ્તુઓ.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_વસ્તુઓ);
        }
        પાછા fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    તાર્કિક checkExhaustivePacked(AstCase* nodep) {
        અચલ uint32_t અંક_Cases = 1UL << nodep->exprp()->width();
        માટે (uint32_t i = 0; i < અંક_Cases; ++i) {
            જો (m_caseDetails.records[i].itemp) ચાલુ;  // Covered case
            જો (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            પાછા ખોટું;
        }

        // It's an exhaustive case statement
        પાછા સાચું;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    શૂન્ય analyzeCaseDetails(AstCase* nodep) {
        અચલ uint32_t અંક_કિંમતો = 1UL << nodep->exprp()->width();
        // Clear case records
        માટે (uint32_t i = 0; i < અંક_કિંમતો; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        તાર્કિક reportedOverlap = ખોટું;
        તાર્કિક hasDefault = ખોટું;
        m_caseDetails.noOverlaps = સાચું;
        માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            જો (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                માટે (uint32_t i = 0; i < અંક_કિંમતો; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    જો (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = સાચું;
                ચાલુ;
            }

            માટે (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                જો (neverItem(nodep, iconstp)) ચાલુ;

                અચલ auto& match = matchPattern(nodep, iconstp);
                અચલ uint32_t mask = match.first.toUInt();
                અચલ uint32_t bits = match.second.toUInt();

                તાર્કિક foundNewCase = ખોટું;
                અચલ AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_કિંમત = 0;
                માટે (uint32_t i = 0; i < અંક_કિંમતો; ++i) {
                    જો ((i & mask) != bits) ચાલુ;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    જો (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = સાચું;
                        ચાલુ;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    જો (caseRecord.itemp == itemp) ચાલુ;

                    // Otherwise record the first overlapping case
                    જો (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_કિંમત = i;
                        m_caseDetails.noOverlaps = ખોટું;
                    }
                }

                // Only report first overlap
                જો (reportedOverlap || !firstOverlapConstp) ચાલુ;

                // Report first overlap
                જો (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    જો (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = સાચું;
                    }
                } નહીંતર {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    જો (iconstp->અંક().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_કિંમત
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = સાચું;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = ખોટું;
        m_caseDetails.exhaustive = hasDefault;
        જો (!hasDefault) {
            જો (અચલ AstEnumDType* અચલ enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                અચલ તાર્કિક exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } નહીંતર {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = સાચું;
    }

    શૂન્ય analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        માટે (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            અચલ LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            જો (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                ચાલુ;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            જો (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) પાછા;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            જો (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                જો (lhsRecord.nCaseAssigns == m_case_NCase_વસ્તુઓ) ચાલુ;
                જો (lhsRecord.nCaseAssignDlys == m_case_NCase_વસ્તુઓ) ચાલુ;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ગાંઠ* અચલ preDefaultp = lhsRecord.preDefaultp;
            જો (!preDefaultp) પાછા;
            // And the pre-default needs to be the same type
            જો (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) પાછા;
            જો (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) પાછા;
        }
        // All cases check out, can optimize if there are some entries left
        જો (m_caseLhsRecords.empty()) પાછા;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        માટે (અચલ auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](અચલ LhsRecord& a, અચલ LhsRecord& b) {
                      // Sort by width, then id
                      અચલ પૂર્ણાંક aWidth = a.lhsp->width();
                      અચલ પૂર્ણાંક bWidth = b.lhsp->width();
                      જો (aWidth != bWidth) પાછા aWidth < bWidth;
                      પાછા a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        અચલ પૂર્ણાંક caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        અચલ auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> તાર્કિક {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            માટે (પૂર્ણાંક i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                જો (totalWidth > maxWidth) પાછા ખોટું;
            }
            પાછા સાચું;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        માટે (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        જો (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            પાછા;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        માટે (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            અચલ size_t width = lhsRecord.lhsp->width();
            અચલ size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            અચલ size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            જો (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_શબ્દો_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        અચલ size_t alignedEntryWidth = VL_શબ્દો_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        જો (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            પાછા;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    શૂન્ય analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = ખોટું;
        m_caseHasDefault = ખોટું;
        m_case_NCase_વસ્તુઓ = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = ખોટું;

        Ast_ગાંઠ* અચલ caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        જો (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = સાચું;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            અચલ VNUser1InUse user1InUse;
            માટે (Ast_ગાંઠ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ગાંઠ_Assign* અચલ assp = checkDecoderAssign(prevp);
                જો (!assp) તોડો;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                અચલ તાર્કિક multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    પાછા refp->varScopep()->user1SetOnce();
                });
                જો (multipleAssignments) તોડો;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        તાર્કિક canBeDecoder = સાચું;
        માટે (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            માટે (Ast_ગાંઠ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                જો (!neverItem(nodep, VN_AS(condp, ગાંઠ_પદાવલિ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                જો (!VN_IS(condp, Const)) {
                    m_caseOpaque = સાચું;
                    canBeDecoder = ખોટું;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            જો (cip->isDefault()) m_caseHasDefault = સાચું;
            // Count case items
            ++m_case_NCase_વસ્તુઓ;
            // Check if it fits the decoder pattern, if still possible
            જો (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        જો (m_caseOpaque) પાછા;

        // If small enough, analyse details
        જો (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        જો (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_ગાંઠ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ગાંઠ_પદાવલિ* exprp, V3SharedTmps& tmps) {
        File_લીટી* અચલ flp = nodep->fileline();

        // If there is only one LHS, just use the result
        જો (m_caseDecoderRecords.size() == 1) {
            અચલ LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            અચલ પૂર્ણાંક width = lhsRecord.lhsp->width();
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp
                = exprp->width() == width ? exprp : નવું AstSel{flp, exprp, 0, width};
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = lhsRecord.lhsp->clone_વૃક્ષ_Pure(ખોટું);
            જો (lhsRecord.nCaseAssigns) {
                પાછા નવું AstAssign{flp, lhsp, rhsp};
            } નહીંતર જો (lhsRecord.nCaseAssignDlys) {
                પાછા નવું AstAssignDly{flp, lhsp, rhsp};
            } નહીંતર {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* અચલ tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_ગાંઠ_પદાવલિ* અચલ tempWritep = નવું AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_ગાંઠ_Stmt* અચલ resultp = નવું AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        માટે (અચલ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            અચલ પૂર્ણાંક width = lhsRecord.lhsp->width();
            અચલ પૂર્ણાંક lsb = lhsRecord.offset;
            Ast_ગાંઠ_પદાવલિ* અચલ tempReadp = નવું AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_ગાંઠ_પદાવલિ* અચલ rhsp = નવું AstSel{flp, tempReadp, lsb, width};
            Ast_ગાંઠ_પદાવલિ* અચલ lhsp = lhsRecord.lhsp->clone_વૃક્ષ_Pure(ખોટું);
            જો (lhsRecord.nCaseAssigns) {
                resultp->addNext(નવું AstAssign{flp, lhsp, rhsp});
            } નહીંતર જો (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(નવું AstAssignDly{flp, lhsp, rhsp});
            } નહીંતર {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        પાછા resultp;
    }

    Ast_ગાંઠ_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_લીટી* અચલ flp = nodep->fileline();
        AstConst* અચલ tablep
            = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, static_cast<પૂર્ણાંક>(m_caseTableWidth), 0};
        અચલ uint32_t tableEntries = 1U << nodep->exprp()->width();

        અચલ તાર્કિક isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        જો (isTinyTable) {
            ++m_stats.caseTableTiny;
        } નહીંતર {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        અચલ uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_શબ્દો_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        માટે (અચલ LhsRecord& lhsRecord : m_caseDecoderRecords) {
            અચલ પૂર્ણાંક lhsWidth = lhsRecord.lhsp->width();
            અચલ પૂર્ણાંક lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            જો (lhsRecord.preDefaultp) {
                AstConst* અચલ rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                માટે (uint32_t index = 0; index < tableEntries; ++index) {
                    અચલ uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->અંક().opSelInto(rhsp->અંક(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            માટે (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* અચલ rhsp = [&]() -> AstConst* {
                    માટે (Ast_ગાંઠ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ગાંઠ_Assign* અચલ ap = VN_AS(stmtp, ગાંઠ_Assign);
                        જો (lhsRecord.lhsp->same_વૃક્ષ(ap->lhsp())) પાછા VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    પાછા VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                જો (cip->isDefault()) {
                    માટે (uint32_t index = 0; index < tableEntries; ++index) {
                        અચલ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->અંક().opSelInto(rhsp->અંક(), tableOffset, lhsWidth);
                    }
                    ચાલુ;
                }

                // Iterate case conditions in reverse order
                માટે (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    જો (neverItem(nodep, condp)) ચાલુ;  // If item never matches, ignore it
                    અચલ auto& match = matchPattern(nodep, condp);
                    અચલ uint32_t matchMask = match.first.toUInt();
                    અચલ uint32_t matchBits = match.second.toUInt();
                    અચલ uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    માટે (uint32_t i = inverseMask; સાચું; i = (i - 1) & inverseMask) {
                        અચલ uint32_t index = i | matchBits;
                        અચલ uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->અંક().opSelInto(rhsp->અંક(), tableOffset, lhsWidth);
                        જો (!i) તોડો;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ગાંઠ_પદાવલિ* અચલ tableRefp = [&]() -> Ast_ગાંઠ_પદાવલિ* {
            જો (isTinyTable) પાછા tablep;
            AstVarRef* અચલ refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_વૃક્ષ(), tablep);  // V3ConstPool::find clones
            પાછા refp;
        }();

        // Create the lookup index
        Ast_ગાંઠ_પદાવલિ* અચલ caseExprp
            = નવું AstExtend{flp, nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું), 32};
        Ast_ગાંઠ_પદાવલિ* અચલ scalep = નવું AstConst{flp, entryWidth};
        Ast_ગાંઠ_પદાવલિ* અચલ tableLsbp = નવું AstMul{flp, scalep, caseExprp};
        Ast_ગાંઠ_પદાવલિ* અચલ tableSelp
            = નવું AstSel{flp, tableRefp, tableLsbp, static_cast<પૂર્ણાંક>(m_caseDecoderEntryWidth)};

        // Connect outputs
        પાછા connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_ગાંઠ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_લીટી* અચલ flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_ગાંઠ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        માટે (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            જો (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                ચાલુ;
            }
            માટે (Ast_ગાંઠ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* અચલ iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                જો (neverItem(nodep, iconstp)) ચાલુ;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        અચલ તાર્કિક provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        જો (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        અચલ પૂર્ણાંક decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        અચલ પૂર્ણાંક condWidth = nodep->exprp()->width();
        અચલ પૂર્ણાંક matchWidth = 2 * VL_શબ્દો_I(condWidth) * VL_EDATASIZE;
        AstConst* અચલ matchp
            = નવું AstConst{flp, AstConst::Widthed_કિંમત{}, decoderEnries * matchWidth, 0};
        માટે (પૂર્ણાંક i = 0; i < decoderEnries; ++i) {
            અચલ પૂર્ણાંક entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            જો (AstConst* અચલ condp = clauses[i].first) {
                અચલ auto& match = matchPattern(nodep, condp);
                matchp->અંક().opSelInto(match.first, entryLsb, condWidth);
                matchp->અંક().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                ચાલુ;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_સંખ્યા અંક_Zero{flp, condWidth, 0};
            matchp->અંક().opSelInto(અંક_Zero, entryLsb, condWidth);
            matchp->અંક().opSelInto(અંક_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* અચલ rangep = નવું AstRange{flp, decoderEnries - 1, 0};
        Ast_ગાંઠ_DType* અચલ entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_ગાંઠ_DType* અચલ tableDtypep = નવું AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* અચલ tablep = નવું AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        માટે (પૂર્ણાંક i = 0; i < decoderEnries; ++i) {
            Ast_ગાંઠ* અચલ stmtsp = clauses[i].second;
            AstConst* અચલ entryp = નવું AstConst{flp, AstConst::Widthed_કિંમત{},
                                                  static_cast<પૂર્ણાંક>(m_caseDecoderEntryWidth), 0};
            માટે (અચલ LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_ગાંઠ_પદાવલિ* અચલ lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                માટે (Ast_ગાંઠ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ગાંઠ_Assign* અચલ assignp = VN_AS(stmtp, ગાંઠ_Assign);
                    જો (!lhsp->same_વૃક્ષ(assignp->lhsp())) ચાલુ;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    તોડો;
                }
                // Not assigned in this clause, so use the pre-assigned default
                જો (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->અંક().opSelInto(valConstp->અંક(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* અચલ matchRefp = V3ConstPool::find(matchp);
        AstVarRef* અચલ tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_વૃક્ષ(), matchp);
        VL_DO_DANGLING(tablep->delete_વૃક્ષ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ગાંઠ_પદાવલિ* અચલ caseExprp = nodep->exprp()->clone_વૃક્ષ_Pure(ખોટું);
        AstMatchMasked* અચલ indexp = નવું AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_ગાંઠ_પદાવલિ* અચલ entryp = નવું AstArraySel{flp, tableRefp, indexp};

        પાછા connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ગાંઠ* convertCaseFastRecurse(Ast_ગાંઠ_પદાવલિ* cexprp, પૂર્ણાંક msb, uint32_t upper_કિંમત) અચલ {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        જો (msb < 0) પાછા m_caseDetails.records[upper_કિંમત].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        અચલ uint32_t upperValue0 = upper_કિંમત;
        અચલ uint32_t upperValue1 = upper_કિંમત | (1UL << msb);
        Ast_ગાંઠ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_ગાંઠ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        જો (tree0p == tree1p) પાછા tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            તાર્કિક same = સાચું;
            માટે (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                જો (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = ખોટું;
                    તોડો;
                }
            }
            જો (same) {
                VL_DO_DANGLING(tree1p->delete_વૃક્ષ(), tree1p);
                પાછા tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        જો (tree0p && tree0p->backp()) tree0p = tree0p->clone_વૃક્ષ(સાચું);
        જો (tree1p && tree1p->backp()) tree1p = tree1p->clone_વૃક્ષ(સાચું);
        // Create the If statement
        File_લીટી* અચલ flp = cexprp->fileline();
        Ast_ગાંઠ_પદાવલિ* અચલ condp = નવું AstSel{flp, cexprp->clone_વૃક્ષ_Pure(ખોટું), msb, 1};
        AstIf* અચલ ifp = નવું AstIf{flp, condp, tree1p, tree0p};
        પાછા ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ગાંઠ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        અચલ પૂર્ણાંક caseWidth = nodep->exprp()->width();
        Ast_ગાંઠ* અચલ ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        પાછા ifrootp && ifrootp->backp() ? ifrootp->clone_વૃક્ષ(સાચું) : ifrootp;
    }

    Ast_ગાંઠ_પદાવલિ* newCaseCompare(અચલ AstCase* casep, Ast_ગાંઠ_પદાવલિ* itemExprp) {
        File_લીટી* અચલ flp = itemExprp->fileline();
        જો (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(ધકેલો_Deletep(itemExprp), itemExprp);
            પાછા નવું AstConst{flp, AstConst::BitFalse{}};
        }

        જો (AstCond* અચલ condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ગાંઠ_પદાવલિ* અચલ testp = condp->condp()->unlinkFrBack();
            Ast_ગાંઠ_પદાવલિ* અચલ thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_ગાંઠ_પદાવલિ* અચલ elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(ધકેલો_Deletep(itemExprp), itemExprp, condp);
            પાછા નવું AstCond{flp, testp, thenp, elsep};
        }

        Ast_ગાંઠ_પદાવલિ* અચલ caseExprp = casep->exprp()->clone_વૃક્ષ_Pure(ખોટું);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        જો (AstInsideRange* અચલ itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_ગાંઠ_પદાવલિ* અચલ resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(ધકેલો_Deletep(itemExprp), itemExprp, itemRangep);
            પાછા resultp;
        }

        જો (AstConst* અચલ itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            જો (itemConstp->અંક().isFourState()) {
                અચલ auto& match = matchPattern(casep, itemConstp);
                અચલ V3_સંખ્યા& matchMask = match.first;
                અચલ V3_સંખ્યા& matchBits = match.second;
                VL_DO_DANGLING2(ધકેલો_Deletep(itemExprp), itemExprp, itemConstp);
                પાછા AstEq::newTyped(flp, નવું AstConst{flp, matchBits},
                                       નવું AstAnd{flp, caseExprp, નવું AstConst{flp, matchMask}});
            }
        }

        પાછા AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ગાંઠ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        તાર્કિક hasDefault = ખોટું;
        માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_લીટી* અચલ flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            જો (itemp->isDefault()) {
                itemp->addCondsp(નવું AstConst{flp, AstConst::BitTrue{}});
                hasDefault = સાચું;
                ચાલુ;
            }

            // Regular clause. Construct the condition expression.
            Ast_ગાંઠ_પદાવલિ* newCondp = nullptr;
            માટે (Ast_ગાંઠ_પદાવલિ *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), ગાંઠ_પદાવલિ);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                જો (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(ધકેલો_Deletep(itemExprp), itemExprp);
                    ચાલુ;
                }

                // Compute the term to add to the condition expression
                Ast_ગાંઠ_પદાવલિ* અચલ termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? નવું AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            જો (!newCondp) newCondp = નવું AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        જો (!hasDefault) {
            જો (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* અચલ lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                ધકેલો_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(નવું AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } નહીંતર {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(નવું AstCaseItem{
                    nodep->fileline(), નવું AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        પૂર્ણાંક depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        માટે (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_ગાંઠ* અચલ istmtsp = itemp->stmtsp();
            જો (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_ગાંઠ_પદાવલિ* અચલ ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                જો (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                જો (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* અચલ newp = નવું AstIf{itemp->fileline(), ifexprp->clone_વૃક્ષ_Pure(સાચું)};
                    જો (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } નહીંતર {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } નહીંતર {  // Continue group, modify if condition to OR in this new condition
                    Ast_ગાંઠ_પદાવલિ* અચલ condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        નવું AstOr{ifexprp->fileline(), condp, ifexprp->clone_વૃક્ષ_Pure(સાચું)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ગાંઠ_પદાવલિ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                જો (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_વૃક્ષ(), itemexprp);
                    itemexprp = નવું AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* અચલ newp = નવું AstIf{itemp->fileline(), itemexprp, istmtsp};
                જો (itemnextp) {
                    itemnextp->addElsesp(newp);
                } નહીંતર {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        પાછા grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ગાંઠ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        અચલ તાર્કિક useTable = [&]() {
            // Not if disabled
            જો (!v3Global.opt.fCaseTable()) પાછા ખોટું;
            // Not if analysis tells us we can't
            જો (!m_caseTableWidth) પાછા ખોટું;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            જો (m_caseTableWidth <= CASE_TABLE_TINY_BITS) પાછા સાચું;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            અચલ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            જો (branches < CASE_TABLE_MIN_BRANCHES) પાછા ખોટું;
            પાછા સાચું;
        }();
        જો (useTable) પાછા convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        અચલ તાર્કિક useDecoder = [&]() {
            // Not if disabled
            જો (!v3Global.opt.fCaseDecoder()) પાછા ખોટું;
            // Not if not a decoder pattern
            જો (m_caseDecoderRecords.empty()) પાછા ખોટું;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            અચલ size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            જો (branches < CASE_TABLE_MIN_BRANCHES) પાછા ખોટું;
            પાછા સાચું;
        }();
        જો (useDecoder) પાછા convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        અચલ તાર્કિક use_Fast_Bit_વૃક્ષ = [&]() {
            // Not if disabled
            જો (!v3Global.opt.f_Case_વૃક્ષ()) પાછા ખોટું;
            // Can't do it without the detailed analysis
            જો (!m_caseDetailsValid) પાછા ખોટું;
            // Can't do it if not exhaustive
            જો (!m_caseDetails.exhaustive) પાછા ખોટું;
            // Not worth doing if there are few conditions
            જો (m_caseNConditions <= 3) પાછા ખોટું;
            // Avoid e.g. priority expanders from going crazy in expansion
            અચલ size_t caseWidth = nodep->exprp()->width();
            જો (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) પાછા ખોટું;
            // Otherwise use the bit tree
            પાછા સાચું;
        }();
        જો (use_Fast_Bit_વૃક્ષ) પાછા convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        જો (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, સાચું);
        }
        પાછા convertCaseGeneric(nodep);
    }

    // VISITORS
    શૂન્ય visit(AstCase* nodep) override {
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
        જો (Ast_ગાંઠ* અચલ stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            જો (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_વૃક્ષ(), stmtp);
            } નહીંતર {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        જો (Ast_ગાંઠ* અચલ replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } નહીંતર {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_વૃક્ષ(), nodep);
    }

    શૂન્ય visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    શૂન્ય visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
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

શૂન્ય V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("case", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
શૂન્ય V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
