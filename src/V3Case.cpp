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
    ثابت AstNode* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    ساكن فراغ detectMultipleDefaults(CaseItem* itemsp) {
        منطقي hitDefault = خطأ;
        لكل (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            𐡄𐡍 (!itemp->isDefault()) استمر;
            𐡄𐡍 (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = 𐡒𐡔𐡈;
        }
    }

    template <typename CaseItem>
    فراغ checkXZinNonCaseX(AstNode* casep, AstNodeExpr* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        لكل (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    فراغ visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    فراغ visit(AstCase* nodep) override {
        𐡄𐡍 (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ visit(AstConst* nodep) override {
        𐡄𐡍 (!nodep->num().isFourState()) أرجع;

        // Error if generate case
        𐡄𐡍 (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            أرجع;
        }

        // Otherwise must be a case statement
        ثابت AstCase* ثابت casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        𐡄𐡍 (casep->casex()) أرجع;

        𐡄𐡍 (casep->casez() || casep->caseInside()) {
            𐡄𐡍 (nodep->num().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            أرجع;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    فراغ visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = افتراضي;

عام:
    ساكن فراغ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ساكن فراغ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

صنف CaseVisitor final : عام VNVisitor {
    // Maximum width for detailed analysis
    constexpr ساكن صحيح CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ساكن صحيح CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ساكن size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ساكن size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ساكن size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    بنية CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        AstNode* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    بنية LhsRecord final {
        AstNodeExpr* lhsp = nullptr;  // LHS of the assignment
        AstNodeAssign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        ساكن size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    بنية Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت AstNode* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقي m_caseOpaque = خطأ;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقي m_caseHasDefault = خطأ;  // Indicates the case statement has a default case
    size_t m_caseNCaseItems = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<AstNodeExpr>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    منطقي m_caseDetailsValid = خطأ;  // Indicates m_caseDetails is valid
    بنية final {
        منطقي exhaustive = خطأ;  // Proven exhaustive
        منطقي exhaustiveOverEnumOnly = خطأ;  // Exhaustive over enum values only
        منطقي noOverlaps = خطأ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ساكن منطقي neverItem(ثابت AstCase* casep, ثابت AstNodeExpr* itemExprp) {
        ثابت AstConst* ثابت constp = VN_CAST(itemExprp, Const);
        𐡄𐡍 (!constp) أرجع خطأ;
        𐡄𐡍 (casep->casex() || casep->caseInside()) أرجع خطأ;
        𐡄𐡍 (casep->casez()) أرجع constp->num().isAnyX();
        أرجع constp->num().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ساكن std::pair<V3Number, V3Number> matchPattern(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3Number, V3Number> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->num());
        pairMaskBits.second.opBitsOne(condp->num());
        أرجع pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ساكن AstNodeAssign* checkDecoderAssign(AstNode* stmtp) {
        // Only Assign and AssignDly are supported
        𐡄𐡍 (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) أرجع nullptr;
        AstNodeAssign* ثابت assp = VN_AS(stmtp, NodeAssign);
        // Only if no timing control
        𐡄𐡍 (assp->timingControlp()) أرجع nullptr;
        // Only if assigning a constant
        𐡄𐡍 (!VN_IS(assp->rhsp(), Const)) أرجع nullptr;
        // Only if it's a packed value
        AstNodeDType* ثابت dtypep = assp->rhsp()->dtypep();
        𐡄𐡍 (dtypep->isString() || dtypep->isDouble()) أرجع nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        𐡄𐡍 (assp->lhsp()->exists([](AstVarRef* refp) { أرجع refp->access().isReadOrRW(); })) {
            أرجع nullptr;
        }
        // This is an assignment that fits the decoder pattern
        أرجع assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقي analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1InUse user1InUse;
        لكل (AstNode* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            AstNodeAssign* ثابت assp = checkDecoderAssign(stmtp);
            𐡄𐡍 (!assp) أرجع خطأ;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقي multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                أرجع refp->varScopep()->user1SetOnce();
            });
            𐡄𐡍 (multipleAssignments) أرجع خطأ;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            𐡄𐡍 (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        أرجع 𐡒𐡔𐡈;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ساكن ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        𐡄𐡍 (!nodep->uniquePragma() && !nodep->unique0Pragma()) أرجع nullptr;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        𐡄𐡍 (!enumDtp) أرجع nullptr;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        𐡄𐡍 (!basicp) أرجع nullptr;  // Not simple type (perhaps IEEE illegal)
        أرجع enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقي checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t numCases = 1UL << nodep->exprp()->width();
        منطقي fullyCovered = 𐡒𐡔𐡈;
        string missingItems;
        لكل (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ثابت auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t mask = match.first.toUInt();
            ثابت uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            لكل (uint32_t i = 0; i < numCases; ++i) {
                𐡄𐡍 ((i & mask) != bits) استمر;  // This case is not for this enum value
                𐡄𐡍 (m_caseDetails.records[i].itemp) استمر;  // Covered case
                𐡄𐡍 (!missingItems.empty()) missingItems += ", ";
                missingItems += eip->prettyNameQ();
                fullyCovered = خطأ;
                اكسر;
            }
        }
        𐡄𐡍 (!missingItems.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missingItems);
        }
        أرجع fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقي checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t numCases = 1UL << nodep->exprp()->width();
        لكل (uint32_t i = 0; i < numCases; ++i) {
            𐡄𐡍 (m_caseDetails.records[i].itemp) استمر;  // Covered case
            𐡄𐡍 (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            أرجع خطأ;
        }

        // It's an exhaustive case statement
        أرجع 𐡒𐡔𐡈;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    فراغ analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t numValues = 1UL << nodep->exprp()->width();
        // Clear case records
        لكل (uint32_t i = 0; i < numValues; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقي reportedOverlap = خطأ;
        منطقي hasDefault = خطأ;
        m_caseDetails.noOverlaps = 𐡒𐡔𐡈;
        لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            𐡄𐡍 (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                لكل (uint32_t i = 0; i < numValues; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    𐡄𐡍 (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = 𐡒𐡔𐡈;
                استمر;
            }

            لكل (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                𐡄𐡍 (neverItem(nodep, iconstp)) استمر;

                ثابت auto& match = matchPattern(nodep, iconstp);
                ثابت uint32_t mask = match.first.toUInt();
                ثابت uint32_t bits = match.second.toUInt();

                منطقي foundNewCase = خطأ;
                ثابت AstConst* firstOverlapConstp = nullptr;
                uint32_t firstOverlapValue = 0;
                لكل (uint32_t i = 0; i < numValues; ++i) {
                    𐡄𐡍 ((i & mask) != bits) استمر;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    𐡄𐡍 (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = 𐡒𐡔𐡈;
                        استمر;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    𐡄𐡍 (caseRecord.itemp == itemp) استمر;

                    // Otherwise record the first overlapping case
                    𐡄𐡍 (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        firstOverlapValue = i;
                        m_caseDetails.noOverlaps = خطأ;
                    }
                }

                // Only report first overlap
                𐡄𐡍 (reportedOverlap || !firstOverlapConstp) استمر;

                // Report first overlap
                𐡄𐡍 (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    𐡄𐡍 (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = 𐡒𐡔𐡈;
                    }
                } وإلا {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    𐡄𐡍 (iconstp->num().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << firstOverlapValue
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = 𐡒𐡔𐡈;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = خطأ;
        m_caseDetails.exhaustive = hasDefault;
        𐡄𐡍 (!hasDefault) {
            𐡄𐡍 (ثابت AstEnumDType* ثابت enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ثابت منطقي exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } وإلا {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = 𐡒𐡔𐡈;
    }

    فراغ analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        لكل (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ثابت LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            𐡄𐡍 (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                استمر;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            𐡄𐡍 (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) أرجع;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            𐡄𐡍 (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                𐡄𐡍 (lhsRecord.nCaseAssigns == m_caseNCaseItems) استمر;
                𐡄𐡍 (lhsRecord.nCaseAssignDlys == m_caseNCaseItems) استمر;
            }

            // Otherwise it needs to have a pre-default assignment
            AstNode* ثابت preDefaultp = lhsRecord.preDefaultp;
            𐡄𐡍 (!preDefaultp) أرجع;
            // And the pre-default needs to be the same type
            𐡄𐡍 (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) أرجع;
            𐡄𐡍 (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) أرجع;
        }
        // All cases check out, can optimize if there are some entries left
        𐡄𐡍 (m_caseLhsRecords.empty()) أرجع;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        لكل (ثابت auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ثابت LhsRecord& a, ثابت LhsRecord& b) {
                      // Sort by width, then id
                      ثابت صحيح aWidth = a.lhsp->width();
                      ثابت صحيح bWidth = b.lhsp->width();
                      𐡄𐡍 (aWidth != bWidth) أرجع aWidth < bWidth;
                      أرجع a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحيح caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> منطقي {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            لكل (صحيح i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                𐡄𐡍 (totalWidth > maxWidth) أرجع خطأ;
            }
            أرجع 𐡒𐡔𐡈;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        لكل (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        𐡄𐡍 (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            أرجع;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        لكل (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت size_t width = lhsRecord.lhsp->width();
            ثابت size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ثابت size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            𐡄𐡍 (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ثابت size_t alignedEntryWidth = VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        𐡄𐡍 (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            أرجع;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    فراغ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = خطأ;
        m_caseHasDefault = خطأ;
        m_caseNCaseItems = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = خطأ;

        AstNode* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        𐡄𐡍 (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = 𐡒𐡔𐡈;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1InUse user1InUse;
            لكل (AstNode* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                AstNodeAssign* ثابت assp = checkDecoderAssign(prevp);
                𐡄𐡍 (!assp) اكسر;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقي multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    أرجع refp->varScopep()->user1SetOnce();
                });
                𐡄𐡍 (multipleAssignments) اكسر;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقي canBeDecoder = 𐡒𐡔𐡈;
        لكل (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            لكل (AstNode* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                𐡄𐡍 (!neverItem(nodep, VN_AS(condp, NodeExpr))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                𐡄𐡍 (!VN_IS(condp, Const)) {
                    m_caseOpaque = 𐡒𐡔𐡈;
                    canBeDecoder = خطأ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            𐡄𐡍 (cip->isDefault()) m_caseHasDefault = 𐡒𐡔𐡈;
            // Count case items
            ++m_caseNCaseItems;
            // Check if it fits the decoder pattern, if still possible
            𐡄𐡍 (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        𐡄𐡍 (m_caseOpaque) أرجع;

        // If small enough, analyse details
        𐡄𐡍 (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        𐡄𐡍 (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    AstNodeStmt* connectDecoderOutputs(AstCase* nodep, AstNodeExpr* exprp, V3SharedTmps& tmps) {
        FileLine* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        𐡄𐡍 (m_caseDecoderRecords.size() == 1) {
            ثابت LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ثابت صحيح width = lhsRecord.lhsp->width();
            AstNodeExpr* ثابت rhsp
                = exprp->width() == width ? exprp : جديد AstSel{flp, exprp, 0, width};
            AstNodeExpr* ثابت lhsp = lhsRecord.lhsp->cloneTreePure(خطأ);
            𐡄𐡍 (lhsRecord.nCaseAssigns) {
                أرجع جديد AstAssign{flp, lhsp, rhsp};
            } وإلا 𐡄𐡍 (lhsRecord.nCaseAssignDlys) {
                أرجع جديد AstAssignDly{flp, lhsp, rhsp};
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        AstNodeExpr* ثابت tempWritep = جديد AstVarRef{flp, tempVscp, VAccess::WRITE};
        AstNodeStmt* ثابت resultp = جديد AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        لكل (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت صحيح width = lhsRecord.lhsp->width();
            ثابت صحيح lsb = lhsRecord.offset;
            AstNodeExpr* ثابت tempReadp = جديد AstVarRef{flp, tempVscp, VAccess::READ};
            AstNodeExpr* ثابت rhsp = جديد AstSel{flp, tempReadp, lsb, width};
            AstNodeExpr* ثابت lhsp = lhsRecord.lhsp->cloneTreePure(خطأ);
            𐡄𐡍 (lhsRecord.nCaseAssigns) {
                resultp->addNext(جديد AstAssign{flp, lhsp, rhsp});
            } وإلا 𐡄𐡍 (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(جديد AstAssignDly{flp, lhsp, rhsp});
            } وإلا {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        أرجع resultp;
    }

    AstNodeStmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        FileLine* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جديد AstConst{flp, AstConst::WidthedValue{}, static_cast<صحيح>(m_caseTableWidth), 0};
        ثابت uint32_t tableEntries = 1U << nodep->exprp()->width();

        ثابت منطقي isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        𐡄𐡍 (isTinyTable) {
            ++m_stats.caseTableTiny;
        } وإلا {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        لكل (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت صحيح lhsWidth = lhsRecord.lhsp->width();
            ثابت صحيح lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            𐡄𐡍 (lhsRecord.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                لكل (uint32_t index = 0; index < tableEntries; ++index) {
                    ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            لكل (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    لكل (AstNode* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        AstNodeAssign* ثابت ap = VN_AS(stmtp, NodeAssign);
                        𐡄𐡍 (lhsRecord.lhsp->sameTree(ap->lhsp())) أرجع VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    أرجع VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                𐡄𐡍 (cip->isDefault()) {
                    لكل (uint32_t index = 0; index < tableEntries; ++index) {
                        ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                    }
                    استمر;
                }

                // Iterate case conditions in reverse order
                لكل (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    𐡄𐡍 (neverItem(nodep, condp)) استمر;  // If item never matches, ignore it
                    ثابت auto& match = matchPattern(nodep, condp);
                    ثابت uint32_t matchMask = match.first.toUInt();
                    ثابت uint32_t matchBits = match.second.toUInt();
                    ثابت uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    لكل (uint32_t i = inverseMask; 𐡒𐡔𐡈; i = (i - 1) & inverseMask) {
                        ثابت uint32_t index = i | matchBits;
                        ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                        𐡄𐡍 (!i) اكسر;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        AstNodeExpr* ثابت tableRefp = [&]() -> AstNodeExpr* {
            𐡄𐡍 (isTinyTable) أرجع tablep;
            AstVarRef* ثابت refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->deleteTree(), tablep);  // V3ConstPool::find clones
            أرجع refp;
        }();

        // Create the lookup index
        AstNodeExpr* ثابت caseExprp
            = جديد AstExtend{flp, nodep->exprp()->cloneTreePure(خطأ), 32};
        AstNodeExpr* ثابت scalep = جديد AstConst{flp, entryWidth};
        AstNodeExpr* ثابت tableLsbp = جديد AstMul{flp, scalep, caseExprp};
        AstNodeExpr* ثابت tableSelp
            = جديد AstSel{flp, tableRefp, tableLsbp, static_cast<صحيح>(m_caseDecoderEntryWidth)};

        // Connect outputs
        أرجع connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    AstNodeStmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        FileLine* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, AstNode*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        لكل (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            𐡄𐡍 (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                استمر;
            }
            لكل (AstNode* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                𐡄𐡍 (neverItem(nodep, iconstp)) استمر;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقي provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        𐡄𐡍 (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ثابت صحيح decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحيح condWidth = nodep->exprp()->width();
        ثابت صحيح matchWidth = 2 * VL_WORDS_I(condWidth) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جديد AstConst{flp, AstConst::WidthedValue{}, decoderEnries * matchWidth, 0};
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            ثابت صحيح entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            𐡄𐡍 (AstConst* ثابت condp = clauses[i].first) {
                ثابت auto& match = matchPattern(nodep, condp);
                matchp->num().opSelInto(match.first, entryLsb, condWidth);
                matchp->num().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                استمر;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3Number numZero{flp, condWidth, 0};
            matchp->num().opSelInto(numZero, entryLsb, condWidth);
            matchp->num().opSelInto(numZero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جديد AstRange{flp, decoderEnries - 1, 0};
        AstNodeDType* ثابت entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        AstNodeDType* ثابت tableDtypep = جديد AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ثابت tablep = جديد AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        لكل (صحيح i = 0; i < decoderEnries; ++i) {
            AstNode* ثابت stmtsp = clauses[i].second;
            AstConst* ثابت entryp = جديد AstConst{flp, AstConst::WidthedValue{},
                                                  static_cast<صحيح>(m_caseDecoderEntryWidth), 0};
            لكل (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
                AstNodeExpr* ثابت lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                لكل (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    AstNodeAssign* ثابت assignp = VN_AS(stmtp, NodeAssign);
                    𐡄𐡍 (!lhsp->sameTree(assignp->lhsp())) استمر;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    اكسر;
                }
                // Not assigned in this clause, so use the pre-assigned default
                𐡄𐡍 (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->num().opSelInto(valConstp->num(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ثابت tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->deleteTree(), matchp);
        VL_DO_DANGLING(tablep->deleteTree(), tablep);

        // AstMatchMasked produces the index of the matching entry
        AstNodeExpr* ثابت caseExprp = nodep->exprp()->cloneTreePure(خطأ);
        AstMatchMasked* ثابت indexp = جديد AstMatchMasked{flp, caseExprp, matchRefp};
        AstNodeExpr* ثابت entryp = جديد AstArraySel{flp, tableRefp, indexp};

        أرجع connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCaseFastRecurse(AstNodeExpr* cexprp, صحيح msb, uint32_t upperValue) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        𐡄𐡍 (msb < 0) أرجع m_caseDetails.records[upperValue].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t upperValue0 = upperValue;
        ثابت uint32_t upperValue1 = upperValue | (1UL << msb);
        AstNode* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        AstNode* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        𐡄𐡍 (tree0p == tree1p) أرجع tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقي same = 𐡒𐡔𐡈;
            لكل (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                𐡄𐡍 (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = خطأ;
                    اكسر;
                }
            }
            𐡄𐡍 (same) {
                VL_DO_DANGLING(tree1p->deleteTree(), tree1p);
                أرجع tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        𐡄𐡍 (tree0p && tree0p->backp()) tree0p = tree0p->cloneTree(𐡒𐡔𐡈);
        𐡄𐡍 (tree1p && tree1p->backp()) tree1p = tree1p->cloneTree(𐡒𐡔𐡈);
        // Create the If statement
        FileLine* ثابت flp = cexprp->fileline();
        AstNodeExpr* ثابت condp = جديد AstSel{flp, cexprp->cloneTreePure(خطأ), msb, 1};
        AstIf* ثابت ifp = جديد AstIf{flp, condp, tree1p, tree0p};
        أرجع ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحيح caseWidth = nodep->exprp()->width();
        AstNode* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        أرجع ifrootp && ifrootp->backp() ? ifrootp->cloneTree(𐡒𐡔𐡈) : ifrootp;
    }

    AstNodeExpr* newCaseCompare(ثابت AstCase* casep, AstNodeExpr* itemExprp) {
        FileLine* ثابت flp = itemExprp->fileline();
        𐡄𐡍 (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(pushDeletep(itemExprp), itemExprp);
            أرجع جديد AstConst{flp, AstConst::BitFalse{}};
        }

        𐡄𐡍 (AstCond* ثابت condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            AstNodeExpr* ثابت testp = condp->condp()->unlinkFrBack();
            AstNodeExpr* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            AstNodeExpr* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, condp);
            أرجع جديد AstCond{flp, testp, thenp, elsep};
        }

        AstNodeExpr* ثابت caseExprp = casep->exprp()->cloneTreePure(خطأ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        𐡄𐡍 (AstInsideRange* ثابت itemRangep = VN_CAST(itemExprp, InsideRange)) {
            AstNodeExpr* ثابت resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, itemRangep);
            أرجع resultp;
        }

        𐡄𐡍 (AstConst* ثابت itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            𐡄𐡍 (itemConstp->num().isFourState()) {
                ثابت auto& match = matchPattern(casep, itemConstp);
                ثابت V3Number& matchMask = match.first;
                ثابت V3Number& matchBits = match.second;
                VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, itemConstp);
                أرجع AstEq::newTyped(flp, جديد AstConst{flp, matchBits},
                                       جديد AstAnd{flp, caseExprp, جديد AstConst{flp, matchMask}});
            }
        }

        أرجع AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        منطقي hasDefault = خطأ;
        لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            FileLine* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            𐡄𐡍 (itemp->isDefault()) {
                itemp->addCondsp(جديد AstConst{flp, AstConst::BitTrue{}});
                hasDefault = 𐡒𐡔𐡈;
                استمر;
            }

            // Regular clause. Construct the condition expression.
            AstNodeExpr* newCondp = nullptr;
            لكل (AstNodeExpr *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), NodeExpr);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                𐡄𐡍 (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(pushDeletep(itemExprp), itemExprp);
                    استمر;
                }

                // Compute the term to add to the condition expression
                AstNodeExpr* ثابت termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جديد AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            𐡄𐡍 (!newCondp) newCondp = جديد AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        𐡄𐡍 (!hasDefault) {
            𐡄𐡍 (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                pushDeletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(جديد AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } وإلا {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(جديد AstCaseItem{
                    nodep->fileline(), جديد AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحيح depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        لكل (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            AstNode* ثابت istmtsp = itemp->stmtsp();
            𐡄𐡍 (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            AstNodeExpr* ثابت ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                𐡄𐡍 (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                𐡄𐡍 (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ثابت newp = جديد AstIf{itemp->fileline(), ifexprp->cloneTreePure(𐡒𐡔𐡈)};
                    𐡄𐡍 (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } وإلا {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وإلا {  // Continue group, modify if condition to OR in this new condition
                    AstNodeExpr* ثابت condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        جديد AstOr{ifexprp->fileline(), condp, ifexprp->cloneTreePure(𐡒𐡔𐡈)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                AstNodeExpr* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                𐡄𐡍 (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->deleteTree(), itemexprp);
                    itemexprp = جديد AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ثابت newp = جديد AstIf{itemp->fileline(), itemexprp, istmtsp};
                𐡄𐡍 (itemnextp) {
                    itemnextp->addElsesp(newp);
                } وإلا {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        أرجع grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقي useTable = [&]() {
            // Not if disabled
            𐡄𐡍 (!v3Global.opt.fCaseTable()) أرجع خطأ;
            // Not if analysis tells us we can't
            𐡄𐡍 (!m_caseTableWidth) أرجع خطأ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            𐡄𐡍 (m_caseTableWidth <= CASE_TABLE_TINY_BITS) أرجع 𐡒𐡔𐡈;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            𐡄𐡍 (branches < CASE_TABLE_MIN_BRANCHES) أرجع خطأ;
            أرجع 𐡒𐡔𐡈;
        }();
        𐡄𐡍 (useTable) أرجع convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقي useDecoder = [&]() {
            // Not if disabled
            𐡄𐡍 (!v3Global.opt.fCaseDecoder()) أرجع خطأ;
            // Not if not a decoder pattern
            𐡄𐡍 (m_caseDecoderRecords.empty()) أرجع خطأ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            𐡄𐡍 (branches < CASE_TABLE_MIN_BRANCHES) أرجع خطأ;
            أرجع 𐡒𐡔𐡈;
        }();
        𐡄𐡍 (useDecoder) أرجع convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقي useFastBitTree = [&]() {
            // Not if disabled
            𐡄𐡍 (!v3Global.opt.fCaseTree()) أرجع خطأ;
            // Can't do it without the detailed analysis
            𐡄𐡍 (!m_caseDetailsValid) أرجع خطأ;
            // Can't do it if not exhaustive
            𐡄𐡍 (!m_caseDetails.exhaustive) أرجع خطأ;
            // Not worth doing if there are few conditions
            𐡄𐡍 (m_caseNConditions <= 3) أرجع خطأ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت size_t caseWidth = nodep->exprp()->width();
            𐡄𐡍 (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) أرجع خطأ;
            // Otherwise use the bit tree
            أرجع 𐡒𐡔𐡈;
        }();
        𐡄𐡍 (useFastBitTree) أرجع convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        𐡄𐡍 (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, 𐡒𐡔𐡈);
        }
        أرجع convertCaseGeneric(nodep);
    }

    // VISITORS
    فراغ visit(AstCase* nodep) override {
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
        𐡄𐡍 (AstNode* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            𐡄𐡍 (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->deleteTree(), stmtp);
            } وإلا {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        𐡄𐡍 (AstNode* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وإلا {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }

    فراغ visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    فراغ visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    فراغ visit(AstNode* nodep) override { iterateChildren(nodep); }

عام:
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

فراغ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("case", 0, dumpTreeEitherLevel() >= 3);
}
فراغ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
