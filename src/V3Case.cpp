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

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Case.h"

#شامل "V3ConstPool.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

رده CaseLintVisitor final : عمومی VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ثابت Ast_گره* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    ایستا تهی detectMultipleDefaults(CaseItem* itemsp) {
        منطقی hitDefault = نادرست;
        برای (CaseItem* itemp = itemsp; itemp; itemp = Ast_گره::as<CaseItem>(itemp->nextp())) {
            اگر (!itemp->isDefault()) ادامه;
            اگر (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = درست;
        }
    }

    template <typename CaseItem>
    تهی checkXZinNonCaseX(Ast_گره* casep, Ast_گره_عبارت* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        برای (CaseItem* itemp = itemsp; itemp; itemp = Ast_گره::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    تهی visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    تهی visit(AstCase* nodep) override {
        اگر (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    تهی visit(AstConst* nodep) override {
        اگر (!nodep->رقم().isFourState()) بازگشت;

        // Error if generate case
        اگر (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            بازگشت;
        }

        // Otherwise must be a case statement
        ثابت AstCase* ثابت casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        اگر (casep->casex()) بازگشت;

        اگر (casep->casez() || casep->caseInside()) {
            اگر (nodep->رقم().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            بازگشت;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = پیشفرض;

عمومی:
    ایستا تهی apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ایستا تهی apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

رده CaseVisitor final : عمومی VNVisitor {
    // Maximum width for detailed analysis
    constexpr ایستا صحیح CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ایستا صحیح CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ایستا size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ایستا size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ایستا size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ساختار CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_گره* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ساختار LhsRecord final {
        Ast_گره_عبارت* lhsp = nullptr;  // LHS of the assignment
        Ast_گره_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        ایستا size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ساختار Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ثابت Ast_گره* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    منطقی m_caseOpaque = نادرست;  // Case statement is opaque (non-packed, or non-const conditions)
    منطقی m_caseHasDefault = نادرست;  // Indicates the case statement has a default case
    size_t m_case_NCase_اقلام = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_گره_عبارت>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    منطقی m_caseDetailsValid = نادرست;  // Indicates m_caseDetails is valid
    ساختار final {
        منطقی exhaustive = نادرست;  // Proven exhaustive
        منطقی exhaustiveOverEnumOnly = نادرست;  // Exhaustive over enum values only
        منطقی noOverlaps = نادرست;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ایستا منطقی neverItem(ثابت AstCase* casep, ثابت Ast_گره_عبارت* itemExprp) {
        ثابت AstConst* ثابت constp = VN_CAST(itemExprp, Const);
        اگر (!constp) بازگشت نادرست;
        اگر (casep->casex() || casep->caseInside()) بازگشت نادرست;
        اگر (casep->casez()) بازگشت constp->رقم().isAnyX();
        بازگشت constp->رقم().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ایستا std::pair<V3_عدد, V3_عدد> matchPattern(ثابت AstCase* /*casep*/,
                                                      ثابت AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_عدد, V3_عدد> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->رقم());
        pairMaskBits.second.opBitsOne(condp->رقم());
        بازگشت pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ایستا Ast_گره_Assign* checkDecoderAssign(Ast_گره* stmtp) {
        // Only Assign and AssignDly are supported
        اگر (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) بازگشت nullptr;
        Ast_گره_Assign* ثابت assp = VN_AS(stmtp, گره_Assign);
        // Only if no timing control
        اگر (assp->timingControlp()) بازگشت nullptr;
        // Only if assigning a constant
        اگر (!VN_IS(assp->rhsp(), Const)) بازگشت nullptr;
        // Only if it's a packed value
        Ast_گره_DType* ثابت dtypep = assp->rhsp()->dtypep();
        اگر (dtypep->isString() || dtypep->isDouble()) بازگشت nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        اگر (assp->lhsp()->exists([](AstVarRef* refp) { بازگشت refp->access().isReadOrRW(); })) {
            بازگشت nullptr;
        }
        // This is an assignment that fits the decoder pattern
        بازگشت assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    منطقی analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ثابت VNUser1InUse user1InUse;
        برای (Ast_گره* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_گره_Assign* ثابت assp = checkDecoderAssign(stmtp);
            اگر (!assp) بازگشت نادرست;
            // Must assign each LHS exactly once - RHS is Const
            ثابت منطقی multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                بازگشت refp->varScopep()->user1SetOnce();
            });
            اگر (multipleAssignments) بازگشت نادرست;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            اگر (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        بازگشت درست;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ایستا ثابت AstEnumDType* getEnumCompletionCheckDType(ثابت AstCase* ثابت nodep) {
        اگر (!nodep->uniquePragma() && !nodep->unique0Pragma()) بازگشت nullptr;
        ثابت AstEnumDType* ثابت enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        اگر (!enumDtp) بازگشت nullptr;  // Case isn't enum
        ثابت AstBasicDType* ثابت basicp = enumDtp->subDTypep()->basicp();
        اگر (!basicp) بازگشت nullptr;  // Not simple type (perhaps IEEE illegal)
        بازگشت enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    منطقی checkExhaustiveEnum(ثابت AstCase* ثابت nodep, ثابت AstEnumDType* ثابت enump) {
        ثابت uint32_t رقم_Cases = 1UL << nodep->exprp()->width();
        منطقی fullyCovered = درست;
        string missing_اقلام;
        برای (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            ثابت auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            ثابت uint32_t mask = match.first.toUInt();
            ثابت uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            برای (uint32_t i = 0; i < رقم_Cases; ++i) {
                اگر ((i & mask) != bits) ادامه;  // This case is not for this enum value
                اگر (m_caseDetails.records[i].itemp) ادامه;  // Covered case
                اگر (!missing_اقلام.empty()) missing_اقلام += ", ";
                missing_اقلام += eip->prettyNameQ();
                fullyCovered = نادرست;
                بشکن;
            }
        }
        اگر (!missing_اقلام.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_اقلام);
        }
        بازگشت fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    منطقی checkExhaustivePacked(AstCase* nodep) {
        ثابت uint32_t رقم_Cases = 1UL << nodep->exprp()->width();
        برای (uint32_t i = 0; i < رقم_Cases; ++i) {
            اگر (m_caseDetails.records[i].itemp) ادامه;  // Covered case
            اگر (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            بازگشت نادرست;
        }

        // It's an exhaustive case statement
        بازگشت درست;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    تهی analyzeCaseDetails(AstCase* nodep) {
        ثابت uint32_t رقم_مقادیر = 1UL << nodep->exprp()->width();
        // Clear case records
        برای (uint32_t i = 0; i < رقم_مقادیر; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        منطقی reportedOverlap = نادرست;
        منطقی hasDefault = نادرست;
        m_caseDetails.noOverlaps = درست;
        برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            اگر (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                برای (uint32_t i = 0; i < رقم_مقادیر; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    اگر (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = درست;
                ادامه;
            }

            برای (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                اگر (neverItem(nodep, iconstp)) ادامه;

                ثابت auto& match = matchPattern(nodep, iconstp);
                ثابت uint32_t mask = match.first.toUInt();
                ثابت uint32_t bits = match.second.toUInt();

                منطقی foundNewCase = نادرست;
                ثابت AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_مقدار = 0;
                برای (uint32_t i = 0; i < رقم_مقادیر; ++i) {
                    اگر ((i & mask) != bits) ادامه;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    اگر (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = درست;
                        ادامه;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    اگر (caseRecord.itemp == itemp) ادامه;

                    // Otherwise record the first overlapping case
                    اگر (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_مقدار = i;
                        m_caseDetails.noOverlaps = نادرست;
                    }
                }

                // Only report first overlap
                اگر (reportedOverlap || !firstOverlapConstp) ادامه;

                // Report first overlap
                اگر (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    اگر (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = درست;
                    }
                } وگرنه {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    اگر (iconstp->رقم().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_مقدار
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = درست;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = نادرست;
        m_caseDetails.exhaustive = hasDefault;
        اگر (!hasDefault) {
            اگر (ثابت AstEnumDType* ثابت enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ثابت منطقی exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } وگرنه {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = درست;
    }

    تهی analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        برای (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            ثابت LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            اگر (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                ادامه;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            اگر (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) بازگشت;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            اگر (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                اگر (lhsRecord.nCaseAssigns == m_case_NCase_اقلام) ادامه;
                اگر (lhsRecord.nCaseAssignDlys == m_case_NCase_اقلام) ادامه;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_گره* ثابت preDefaultp = lhsRecord.preDefaultp;
            اگر (!preDefaultp) بازگشت;
            // And the pre-default needs to be the same type
            اگر (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) بازگشت;
            اگر (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) بازگشت;
        }
        // All cases check out, can optimize if there are some entries left
        اگر (m_caseLhsRecords.empty()) بازگشت;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        برای (ثابت auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](ثابت LhsRecord& a, ثابت LhsRecord& b) {
                      // Sort by width, then id
                      ثابت صحیح aWidth = a.lhsp->width();
                      ثابت صحیح bWidth = b.lhsp->width();
                      اگر (aWidth != bWidth) بازگشت aWidth < bWidth;
                      بازگشت a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ثابت صحیح caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ثابت auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> منطقی {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            برای (صحیح i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                اگر (totalWidth > maxWidth) بازگشت نادرست;
            }
            بازگشت درست;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        برای (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        اگر (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            بازگشت;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        برای (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت size_t width = lhsRecord.lhsp->width();
            ثابت size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            ثابت size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            اگر (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_واژگان_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        ثابت size_t alignedEntryWidth = VL_واژگان_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        اگر (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            بازگشت;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    تهی analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = نادرست;
        m_caseHasDefault = نادرست;
        m_case_NCase_اقلام = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = نادرست;

        Ast_گره* ثابت caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        اگر (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = درست;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ثابت VNUser1InUse user1InUse;
            برای (Ast_گره* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_گره_Assign* ثابت assp = checkDecoderAssign(prevp);
                اگر (!assp) بشکن;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ثابت منطقی multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    بازگشت refp->varScopep()->user1SetOnce();
                });
                اگر (multipleAssignments) بشکن;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        منطقی canBeDecoder = درست;
        برای (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            برای (Ast_گره* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                اگر (!neverItem(nodep, VN_AS(condp, گره_عبارت))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                اگر (!VN_IS(condp, Const)) {
                    m_caseOpaque = درست;
                    canBeDecoder = نادرست;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            اگر (cip->isDefault()) m_caseHasDefault = درست;
            // Count case items
            ++m_case_NCase_اقلام;
            // Check if it fits the decoder pattern, if still possible
            اگر (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        اگر (m_caseOpaque) بازگشت;

        // If small enough, analyse details
        اگر (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        اگر (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_گره_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_گره_عبارت* exprp, V3SharedTmps& tmps) {
        File_سطر* ثابت flp = nodep->fileline();

        // If there is only one LHS, just use the result
        اگر (m_caseDecoderRecords.size() == 1) {
            ثابت LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            ثابت صحیح width = lhsRecord.lhsp->width();
            Ast_گره_عبارت* ثابت rhsp
                = exprp->width() == width ? exprp : جدید AstSel{flp, exprp, 0, width};
            Ast_گره_عبارت* ثابت lhsp = lhsRecord.lhsp->clone_درخت_Pure(نادرست);
            اگر (lhsRecord.nCaseAssigns) {
                بازگشت جدید AstAssign{flp, lhsp, rhsp};
            } وگرنه اگر (lhsRecord.nCaseAssignDlys) {
                بازگشت جدید AstAssignDly{flp, lhsp, rhsp};
            } وگرنه {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ثابت tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_گره_عبارت* ثابت tempWritep = جدید AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_گره_Stmt* ثابت resultp = جدید AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        برای (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت صحیح width = lhsRecord.lhsp->width();
            ثابت صحیح lsb = lhsRecord.offset;
            Ast_گره_عبارت* ثابت tempReadp = جدید AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_گره_عبارت* ثابت rhsp = جدید AstSel{flp, tempReadp, lsb, width};
            Ast_گره_عبارت* ثابت lhsp = lhsRecord.lhsp->clone_درخت_Pure(نادرست);
            اگر (lhsRecord.nCaseAssigns) {
                resultp->addNext(جدید AstAssign{flp, lhsp, rhsp});
            } وگرنه اگر (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(جدید AstAssignDly{flp, lhsp, rhsp});
            } وگرنه {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        بازگشت resultp;
    }

    Ast_گره_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_سطر* ثابت flp = nodep->fileline();
        AstConst* ثابت tablep
            = جدید AstConst{flp, AstConst::Widthed_مقدار{}, static_cast<صحیح>(m_caseTableWidth), 0};
        ثابت uint32_t tableEntries = 1U << nodep->exprp()->width();

        ثابت منطقی isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        اگر (isTinyTable) {
            ++m_stats.caseTableTiny;
        } وگرنه {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ثابت uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_واژگان_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        برای (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
            ثابت صحیح lhsWidth = lhsRecord.lhsp->width();
            ثابت صحیح lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            اگر (lhsRecord.preDefaultp) {
                AstConst* ثابت rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                برای (uint32_t index = 0; index < tableEntries; ++index) {
                    ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->رقم().opSelInto(rhsp->رقم(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            برای (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* ثابت rhsp = [&]() -> AstConst* {
                    برای (Ast_گره* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_گره_Assign* ثابت ap = VN_AS(stmtp, گره_Assign);
                        اگر (lhsRecord.lhsp->same_درخت(ap->lhsp())) بازگشت VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    بازگشت VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                اگر (cip->isDefault()) {
                    برای (uint32_t index = 0; index < tableEntries; ++index) {
                        ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->رقم().opSelInto(rhsp->رقم(), tableOffset, lhsWidth);
                    }
                    ادامه;
                }

                // Iterate case conditions in reverse order
                برای (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    اگر (neverItem(nodep, condp)) ادامه;  // If item never matches, ignore it
                    ثابت auto& match = matchPattern(nodep, condp);
                    ثابت uint32_t matchMask = match.first.toUInt();
                    ثابت uint32_t matchBits = match.second.toUInt();
                    ثابت uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    برای (uint32_t i = inverseMask; درست; i = (i - 1) & inverseMask) {
                        ثابت uint32_t index = i | matchBits;
                        ثابت uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->رقم().opSelInto(rhsp->رقم(), tableOffset, lhsWidth);
                        اگر (!i) بشکن;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_گره_عبارت* ثابت tableRefp = [&]() -> Ast_گره_عبارت* {
            اگر (isTinyTable) بازگشت tablep;
            AstVarRef* ثابت refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_درخت(), tablep);  // V3ConstPool::find clones
            بازگشت refp;
        }();

        // Create the lookup index
        Ast_گره_عبارت* ثابت caseExprp
            = جدید AstExtend{flp, nodep->exprp()->clone_درخت_Pure(نادرست), 32};
        Ast_گره_عبارت* ثابت scalep = جدید AstConst{flp, entryWidth};
        Ast_گره_عبارت* ثابت tableLsbp = جدید AstMul{flp, scalep, caseExprp};
        Ast_گره_عبارت* ثابت tableSelp
            = جدید AstSel{flp, tableRefp, tableLsbp, static_cast<صحیح>(m_caseDecoderEntryWidth)};

        // Connect outputs
        بازگشت connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_گره_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_سطر* ثابت flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_گره*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        برای (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            اگر (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                ادامه;
            }
            برای (Ast_گره* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ثابت iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                اگر (neverItem(nodep, iconstp)) ادامه;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ثابت منطقی provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        اگر (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        ثابت صحیح decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ثابت صحیح condWidth = nodep->exprp()->width();
        ثابت صحیح matchWidth = 2 * VL_واژگان_I(condWidth) * VL_EDATASIZE;
        AstConst* ثابت matchp
            = جدید AstConst{flp, AstConst::Widthed_مقدار{}, decoderEnries * matchWidth, 0};
        برای (صحیح i = 0; i < decoderEnries; ++i) {
            ثابت صحیح entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            اگر (AstConst* ثابت condp = clauses[i].first) {
                ثابت auto& match = matchPattern(nodep, condp);
                matchp->رقم().opSelInto(match.first, entryLsb, condWidth);
                matchp->رقم().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                ادامه;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_عدد رقم_Zero{flp, condWidth, 0};
            matchp->رقم().opSelInto(رقم_Zero, entryLsb, condWidth);
            matchp->رقم().opSelInto(رقم_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* ثابت rangep = جدید AstRange{flp, decoderEnries - 1, 0};
        Ast_گره_DType* ثابت entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_گره_DType* ثابت tableDtypep = جدید AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* ثابت tablep = جدید AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        برای (صحیح i = 0; i < decoderEnries; ++i) {
            Ast_گره* ثابت stmtsp = clauses[i].second;
            AstConst* ثابت entryp = جدید AstConst{flp, AstConst::Widthed_مقدار{},
                                                  static_cast<صحیح>(m_caseDecoderEntryWidth), 0};
            برای (ثابت LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_گره_عبارت* ثابت lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                برای (Ast_گره* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_گره_Assign* ثابت assignp = VN_AS(stmtp, گره_Assign);
                    اگر (!lhsp->same_درخت(assignp->lhsp())) ادامه;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    بشکن;
                }
                // Not assigned in this clause, so use the pre-assigned default
                اگر (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->رقم().opSelInto(valConstp->رقم(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ثابت matchRefp = V3ConstPool::find(matchp);
        AstVarRef* ثابت tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_درخت(), matchp);
        VL_DO_DANGLING(tablep->delete_درخت(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_گره_عبارت* ثابت caseExprp = nodep->exprp()->clone_درخت_Pure(نادرست);
        AstMatchMasked* ثابت indexp = جدید AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_گره_عبارت* ثابت entryp = جدید AstArraySel{flp, tableRefp, indexp};

        بازگشت connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCaseFastRecurse(Ast_گره_عبارت* cexprp, صحیح msb, uint32_t upper_مقدار) ثابت {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        اگر (msb < 0) بازگشت m_caseDetails.records[upper_مقدار].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ثابت uint32_t upperValue0 = upper_مقدار;
        ثابت uint32_t upperValue1 = upper_مقدار | (1UL << msb);
        Ast_گره* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_گره* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        اگر (tree0p == tree1p) بازگشت tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            منطقی same = درست;
            برای (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                اگر (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = نادرست;
                    بشکن;
                }
            }
            اگر (same) {
                VL_DO_DANGLING(tree1p->delete_درخت(), tree1p);
                بازگشت tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        اگر (tree0p && tree0p->backp()) tree0p = tree0p->clone_درخت(درست);
        اگر (tree1p && tree1p->backp()) tree1p = tree1p->clone_درخت(درست);
        // Create the If statement
        File_سطر* ثابت flp = cexprp->fileline();
        Ast_گره_عبارت* ثابت condp = جدید AstSel{flp, cexprp->clone_درخت_Pure(نادرست), msb, 1};
        AstIf* ثابت ifp = جدید AstIf{flp, condp, tree1p, tree0p};
        بازگشت ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ثابت صحیح caseWidth = nodep->exprp()->width();
        Ast_گره* ثابت ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        بازگشت ifrootp && ifrootp->backp() ? ifrootp->clone_درخت(درست) : ifrootp;
    }

    Ast_گره_عبارت* newCaseCompare(ثابت AstCase* casep, Ast_گره_عبارت* itemExprp) {
        File_سطر* ثابت flp = itemExprp->fileline();
        اگر (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(افزودن_Deletep(itemExprp), itemExprp);
            بازگشت جدید AstConst{flp, AstConst::BitFalse{}};
        }

        اگر (AstCond* ثابت condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_گره_عبارت* ثابت testp = condp->condp()->unlinkFrBack();
            Ast_گره_عبارت* ثابت thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_گره_عبارت* ثابت elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(افزودن_Deletep(itemExprp), itemExprp, condp);
            بازگشت جدید AstCond{flp, testp, thenp, elsep};
        }

        Ast_گره_عبارت* ثابت caseExprp = casep->exprp()->clone_درخت_Pure(نادرست);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        اگر (AstInsideRange* ثابت itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_گره_عبارت* ثابت resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(افزودن_Deletep(itemExprp), itemExprp, itemRangep);
            بازگشت resultp;
        }

        اگر (AstConst* ثابت itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            اگر (itemConstp->رقم().isFourState()) {
                ثابت auto& match = matchPattern(casep, itemConstp);
                ثابت V3_عدد& matchMask = match.first;
                ثابت V3_عدد& matchBits = match.second;
                VL_DO_DANGLING2(افزودن_Deletep(itemExprp), itemExprp, itemConstp);
                بازگشت AstEq::newTyped(flp, جدید AstConst{flp, matchBits},
                                       جدید AstAnd{flp, caseExprp, جدید AstConst{flp, matchMask}});
            }
        }

        بازگشت AstEq::newTyped(flp, caseExprp, itemExprp);
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
        منطقی hasDefault = نادرست;
        برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_سطر* ثابت flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            اگر (itemp->isDefault()) {
                itemp->addCondsp(جدید AstConst{flp, AstConst::BitTrue{}});
                hasDefault = درست;
                ادامه;
            }

            // Regular clause. Construct the condition expression.
            Ast_گره_عبارت* newCondp = nullptr;
            برای (Ast_گره_عبارت *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), گره_عبارت);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                اگر (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(افزودن_Deletep(itemExprp), itemExprp);
                    ادامه;
                }

                // Compute the term to add to the condition expression
                Ast_گره_عبارت* ثابت termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? جدید AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            اگر (!newCondp) newCondp = جدید AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        اگر (!hasDefault) {
            اگر (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* ثابت lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                افزودن_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(جدید AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } وگرنه {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(جدید AstCaseItem{
                    nodep->fileline(), جدید AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        صحیح depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        برای (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_گره* ثابت istmtsp = itemp->stmtsp();
            اگر (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_گره_عبارت* ثابت ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                اگر (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                اگر (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* ثابت newp = جدید AstIf{itemp->fileline(), ifexprp->clone_درخت_Pure(درست)};
                    اگر (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } وگرنه {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } وگرنه {  // Continue group, modify if condition to OR in this new condition
                    Ast_گره_عبارت* ثابت condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        جدید AstOr{ifexprp->fileline(), condp, ifexprp->clone_درخت_Pure(درست)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_گره_عبارت* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                اگر (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_درخت(), itemexprp);
                    itemexprp = جدید AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* ثابت newp = جدید AstIf{itemp->fileline(), itemexprp, istmtsp};
                اگر (itemnextp) {
                    itemnextp->addElsesp(newp);
                } وگرنه {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        بازگشت grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_گره* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ثابت منطقی useTable = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.fCaseTable()) بازگشت نادرست;
            // Not if analysis tells us we can't
            اگر (!m_caseTableWidth) بازگشت نادرست;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            اگر (m_caseTableWidth <= CASE_TABLE_TINY_BITS) بازگشت درست;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ثابت size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            اگر (branches < CASE_TABLE_MIN_BRANCHES) بازگشت نادرست;
            بازگشت درست;
        }();
        اگر (useTable) بازگشت convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        ثابت منطقی useDecoder = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.fCaseDecoder()) بازگشت نادرست;
            // Not if not a decoder pattern
            اگر (m_caseDecoderRecords.empty()) بازگشت نادرست;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ثابت size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            اگر (branches < CASE_TABLE_MIN_BRANCHES) بازگشت نادرست;
            بازگشت درست;
        }();
        اگر (useDecoder) بازگشت convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ثابت منطقی use_Fast_Bit_درخت = [&]() {
            // Not if disabled
            اگر (!v3Global.opt.f_Case_درخت()) بازگشت نادرست;
            // Can't do it without the detailed analysis
            اگر (!m_caseDetailsValid) بازگشت نادرست;
            // Can't do it if not exhaustive
            اگر (!m_caseDetails.exhaustive) بازگشت نادرست;
            // Not worth doing if there are few conditions
            اگر (m_caseNConditions <= 3) بازگشت نادرست;
            // Avoid e.g. priority expanders from going crazy in expansion
            ثابت size_t caseWidth = nodep->exprp()->width();
            اگر (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) بازگشت نادرست;
            // Otherwise use the bit tree
            بازگشت درست;
        }();
        اگر (use_Fast_Bit_درخت) بازگشت convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        اگر (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, درست);
        }
        بازگشت convertCaseGeneric(nodep);
    }

    // VISITORS
    تهی visit(AstCase* nodep) override {
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
        اگر (Ast_گره* ثابت stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            اگر (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_درخت(), stmtp);
            } وگرنه {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        اگر (Ast_گره* ثابت replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } وگرنه {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }

    تهی visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    تهی visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    تهی visit(Ast_گره* nodep) override { iterateChildren(nodep); }

عمومی:
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

تهی V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("case", 0, dump_درخت_Either_Level() >= 3);
}
تهی V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
