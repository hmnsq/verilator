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

#समावेश "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#समावेश "V3Case.h"

#समावेश "V3ConstPool.h"
#समावेश "V3SharedTmps.h"
#समावेश "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

श्रेणी CaseLintVisitor final : खुला VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    स्थिर Ast_गाँठ* m_casep = nullptr;

    // METHODS
    ढाँचा <typename CaseItem>
    अटल व्योम detectMultipleDefaults(CaseItem* itemsp) {
        bool hitDefault = false;
        क्रम (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<CaseItem>(itemp->nextp())) {
            अगर (!itemp->isDefault()) जारी;
            अगर (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = true;
        }
    }

    ढाँचा <typename CaseItem>
    व्योम checkXZinNonCaseX(Ast_गाँठ* casep, Ast_गाँठ_व्यंजक* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        क्रम (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठ::as<CaseItem>(itemp->nextp())) {
            iterate_And_अग्र_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    व्योम visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    व्योम visit(AstCase* nodep) override {
        अगर (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    व्योम visit(AstConst* nodep) override {
        अगर (!nodep->अंक().isFourState()) वापस;

        // Error if generate case
        अगर (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            वापस;
        }

        // Otherwise must be a case statement
        स्थिर AstCase* स्थिर casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        अगर (casep->casex()) वापस;

        अगर (casep->casez() || casep->caseInside()) {
            अगर (nodep->अंक().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            वापस;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = शेष;

खुला:
    अटल व्योम apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    अटल व्योम apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

श्रेणी CaseVisitor final : खुला VNVisitor {
    // Maximum width for detailed analysis
    constexpr अटल पूर्णांक CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr अटल पूर्णांक CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr अटल माप_प्रकार CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr अटल माप_प्रकार CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr अटल माप_प्रकार CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    काष्ठा CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_गाँठ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    काष्ठा LhsRecord final {
        Ast_गाँठ_व्यंजक* lhsp = nullptr;  // LHS of the assignment
        Ast_गाँठ_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        माप_प्रकार nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        माप_प्रकार nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        माप_प्रकार offset = 0;  // Offset in the table for this LHS

        अटल माप_प्रकार s_अग्र_Id;  // Static unique Id counter
        माप_प्रकार id = ++s_अग्र_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    काष्ठा Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    स्थिर Ast_गाँठ* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    bool m_caseOpaque = false;  // Case statement is opaque (non-packed, or non-const conditions)
    bool m_caseHasDefault = false;  // Indicates the case statement has a default case
    माप_प्रकार m_case_NCase_वस्तुएँ = 0;  // Number of AstCaseItems in the case statement
    माप_प्रकार m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_गाँठ_व्यंजक>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    माप_प्रकार m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    माप_प्रकार m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    bool m_caseDetailsValid = false;  // Indicates m_caseDetails is valid
    काष्ठा final {
        bool exhaustive = false;  // Proven exhaustive
        bool exhaustiveOverEnumOnly = false;  // Exhaustive over enum values only
        bool noOverlaps = false;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    अटल bool neverItem(स्थिर AstCase* casep, स्थिर Ast_गाँठ_व्यंजक* itemExprp) {
        स्थिर AstConst* स्थिर constp = VN_CAST(itemExprp, Const);
        अगर (!constp) वापस false;
        अगर (casep->casex() || casep->caseInside()) वापस false;
        अगर (casep->casez()) वापस constp->अंक().isAnyX();
        वापस constp->अंक().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    अटल std::pair<V3_संख्या, V3_संख्या> matchPattern(स्थिर AstCase* /*casep*/,
                                                      स्थिर AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_संख्या, V3_संख्या> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->अंक());
        pairMaskBits.second.opBitsOne(condp->अंक());
        वापस pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    अटल Ast_गाँठ_Assign* checkDecoderAssign(Ast_गाँठ* stmtp) {
        // Only Assign and AssignDly are supported
        अगर (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) वापस nullptr;
        Ast_गाँठ_Assign* स्थिर assp = VN_AS(stmtp, गाँठ_Assign);
        // Only if no timing control
        अगर (assp->timingControlp()) वापस nullptr;
        // Only if assigning a constant
        अगर (!VN_IS(assp->rhsp(), Const)) वापस nullptr;
        // Only if it's a packed value
        Ast_गाँठ_DType* स्थिर dtypep = assp->rhsp()->dtypep();
        अगर (dtypep->isString() || dtypep->isDouble()) वापस nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        अगर (assp->lhsp()->exists([](AstVarRef* refp) { वापस refp->access().isReadOrRW(); })) {
            वापस nullptr;
        }
        // This is an assignment that fits the decoder pattern
        वापस assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    bool analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        स्थिर VNUser1InUse user1InUse;
        क्रम (Ast_गाँठ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_गाँठ_Assign* स्थिर assp = checkDecoderAssign(stmtp);
            अगर (!assp) वापस false;
            // Must assign each LHS exactly once - RHS is Const
            स्थिर bool multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                वापस refp->varScopep()->user1SetOnce();
            });
            अगर (multipleAssignments) वापस false;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            अगर (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        वापस true;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    अटल स्थिर AstEnumDType* getEnumCompletionCheckDType(स्थिर AstCase* स्थिर nodep) {
        अगर (!nodep->uniquePragma() && !nodep->unique0Pragma()) वापस nullptr;
        स्थिर AstEnumDType* स्थिर enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        अगर (!enumDtp) वापस nullptr;  // Case isn't enum
        स्थिर AstBasicDType* स्थिर basicp = enumDtp->subDTypep()->basicp();
        अगर (!basicp) वापस nullptr;  // Not simple type (perhaps IEEE illegal)
        वापस enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    bool checkExhaustiveEnum(स्थिर AstCase* स्थिर nodep, स्थिर AstEnumDType* स्थिर enump) {
        स्थिर uint32_t अंक_Cases = 1UL << nodep->exprp()->width();
        bool fullyCovered = true;
        string missing_वस्तुएँ;
        क्रम (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            स्थिर स्वतः& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            स्थिर uint32_t mask = match.first.toUInt();
            स्थिर uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            क्रम (uint32_t i = 0; i < अंक_Cases; ++i) {
                अगर ((i & mask) != bits) जारी;  // This case is not for this enum value
                अगर (m_caseDetails.records[i].itemp) जारी;  // Covered case
                अगर (!missing_वस्तुएँ.empty()) missing_वस्तुएँ += ", ";
                missing_वस्तुएँ += eip->prettyNameQ();
                fullyCovered = false;
                अवरोध;
            }
        }
        अगर (!missing_वस्तुएँ.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_वस्तुएँ);
        }
        वापस fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    bool checkExhaustivePacked(AstCase* nodep) {
        स्थिर uint32_t अंक_Cases = 1UL << nodep->exprp()->width();
        क्रम (uint32_t i = 0; i < अंक_Cases; ++i) {
            अगर (m_caseDetails.records[i].itemp) जारी;  // Covered case
            अगर (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::षष्ठ
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            वापस false;
        }

        // It's an exhaustive case statement
        वापस true;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    व्योम analyzeCaseDetails(AstCase* nodep) {
        स्थिर uint32_t अंक_मूल्यावली = 1UL << nodep->exprp()->width();
        // Clear case records
        क्रम (uint32_t i = 0; i < अंक_मूल्यावली; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        bool reportedOverlap = false;
        bool hasDefault = false;
        m_caseDetails.noOverlaps = true;
        क्रम (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            अगर (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                क्रम (uint32_t i = 0; i < अंक_मूल्यावली; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    अगर (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = true;
                जारी;
            }

            क्रम (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                अगर (neverItem(nodep, iconstp)) जारी;

                स्थिर स्वतः& match = matchPattern(nodep, iconstp);
                स्थिर uint32_t mask = match.first.toUInt();
                स्थिर uint32_t bits = match.second.toUInt();

                bool foundNewCase = false;
                स्थिर AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_मूल्य = 0;
                क्रम (uint32_t i = 0; i < अंक_मूल्यावली; ++i) {
                    अगर ((i & mask) != bits) जारी;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    अगर (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = true;
                        जारी;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    अगर (caseRecord.itemp == itemp) जारी;

                    // Otherwise record the first overlapping case
                    अगर (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_मूल्य = i;
                        m_caseDetails.noOverlaps = false;
                    }
                }

                // Only report first overlap
                अगर (reportedOverlap || !firstOverlapConstp) जारी;

                // Report first overlap
                अगर (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    अगर (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = true;
                    }
                } अन्यथा {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    अगर (iconstp->अंक().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::षष्ठ << first_Overlap_मूल्य
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = true;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = false;
        m_caseDetails.exhaustive = hasDefault;
        अगर (!hasDefault) {
            अगर (स्थिर AstEnumDType* स्थिर enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                स्थिर bool exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } अन्यथा {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = true;
    }

    व्योम analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        क्रम (स्वतः it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            स्थिर LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            अगर (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                जारी;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            अगर (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) वापस;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            अगर (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                अगर (lhsRecord.nCaseAssigns == m_case_NCase_वस्तुएँ) जारी;
                अगर (lhsRecord.nCaseAssignDlys == m_case_NCase_वस्तुएँ) जारी;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_गाँठ* स्थिर preDefaultp = lhsRecord.preDefaultp;
            अगर (!preDefaultp) वापस;
            // And the pre-default needs to be the same type
            अगर (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) वापस;
            अगर (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) वापस;
        }
        // All cases check out, can optimize if there are some entries left
        अगर (m_caseLhsRecords.empty()) वापस;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.आकार());
        क्रम (स्थिर स्वतः& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](स्थिर LhsRecord& a, स्थिर LhsRecord& b) {
                      // Sort by width, then id
                      स्थिर पूर्णांक aWidth = a.lhsp->width();
                      स्थिर पूर्णांक bWidth = b.lhsp->width();
                      अगर (aWidth != bWidth) वापस aWidth < bWidth;
                      वापस a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        स्थिर पूर्णांक caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        स्थिर स्वतः fitsLimit = [&](माप_प्रकार entryWidth, माप_प्रकार maxWidth) -> bool {
            माप_प्रकार योग_Width = entryWidth;
            // Multiply cases - iterative to avoid overflow
            क्रम (पूर्णांक i = 0; i < caseWidth; ++i) {
                योग_Width <<= 1;
                अगर (योग_Width > maxWidth) वापस false;
            }
            वापस true;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        क्रम (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        अगर (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            वापस;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        क्रम (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिर माप_प्रकार width = lhsRecord.lhsp->width();
            स्थिर माप_प्रकार lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            स्थिर माप_प्रकार msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            अगर (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_शब्दगण_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        स्थिर माप_प्रकार alignedEntryWidth = VL_शब्दगण_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        अगर (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            वापस;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    व्योम analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = false;
        m_caseHasDefault = false;
        m_case_NCase_वस्तुएँ = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.सफा();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.सफा();
        m_caseDetailsValid = false;

        Ast_गाँठ* स्थिर caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        अगर (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = true;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            स्थिर VNUser1InUse user1InUse;
            क्रम (Ast_गाँठ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_गाँठ_Assign* स्थिर assp = checkDecoderAssign(prevp);
                अगर (!assp) अवरोध;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                स्थिर bool multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    वापस refp->varScopep()->user1SetOnce();
                });
                अगर (multipleAssignments) अवरोध;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        bool canBeDecoder = true;
        क्रम (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            क्रम (Ast_गाँठ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                अगर (!neverItem(nodep, VN_AS(condp, गाँठ_व्यंजक))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                अगर (!VN_IS(condp, Const)) {
                    m_caseOpaque = true;
                    canBeDecoder = false;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            अगर (cip->isDefault()) m_caseHasDefault = true;
            // Count case items
            ++m_case_NCase_वस्तुएँ;
            // Check if it fits the decoder pattern, if still possible
            अगर (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        अगर (m_caseOpaque) वापस;

        // If small enough, analyse details
        अगर (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        अगर (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_गाँठ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_गाँठ_व्यंजक* exprp, V3SharedTmps& tmps) {
        File_पंक्ति* स्थिर flp = nodep->fileline();

        // If there is only one LHS, just use the result
        अगर (m_caseDecoderRecords.आकार() == 1) {
            स्थिर LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            स्थिर पूर्णांक width = lhsRecord.lhsp->width();
            Ast_गाँठ_व्यंजक* स्थिर rhsp
                = exprp->width() == width ? exprp : नया AstSel{flp, exprp, 0, width};
            Ast_गाँठ_व्यंजक* स्थिर lhsp = lhsRecord.lhsp->clone_वृक्ष_Pure(false);
            अगर (lhsRecord.nCaseAssigns) {
                वापस नया AstAssign{flp, lhsp, rhsp};
            } अन्यथा अगर (lhsRecord.nCaseAssignDlys) {
                वापस नया AstAssignDly{flp, lhsp, rhsp};
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* स्थिर अस्थायी_Vscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_गाँठ_व्यंजक* स्थिर अस्थायी_Writep = नया AstVarRef{flp, अस्थायी_Vscp, VAccess::WRITE};
        Ast_गाँठ_Stmt* स्थिर resultp = नया AstAssign{flp, अस्थायी_Writep, exprp};

        // For each LHS, select out the result
        क्रम (स्थिर LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिर पूर्णांक width = lhsRecord.lhsp->width();
            स्थिर पूर्णांक lsb = lhsRecord.offset;
            Ast_गाँठ_व्यंजक* स्थिर अस्थायी_Readp = नया AstVarRef{flp, अस्थायी_Vscp, VAccess::READ};
            Ast_गाँठ_व्यंजक* स्थिर rhsp = नया AstSel{flp, अस्थायी_Readp, lsb, width};
            Ast_गाँठ_व्यंजक* स्थिर lhsp = lhsRecord.lhsp->clone_वृक्ष_Pure(false);
            अगर (lhsRecord.nCaseAssigns) {
                resultp->add_अग्र(नया AstAssign{flp, lhsp, rhsp});
            } अन्यथा अगर (lhsRecord.nCaseAssignDlys) {
                resultp->add_अग्र(नया AstAssignDly{flp, lhsp, rhsp});
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        वापस resultp;
    }

    Ast_गाँठ_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_पंक्ति* स्थिर flp = nodep->fileline();
        AstConst* स्थिर tablep
            = नया AstConst{flp, AstConst::Widthed_मूल्य{}, static_cast<पूर्णांक>(m_caseTableWidth), 0};
        स्थिर uint32_t tableEntries = 1U << nodep->exprp()->width();

        स्थिर bool isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        अगर (isTinyTable) {
            ++m_stats.caseTableTiny;
        } अन्यथा {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        स्थिर uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_शब्दगण_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        क्रम (स्थिर LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिर पूर्णांक lhsWidth = lhsRecord.lhsp->width();
            स्थिर पूर्णांक lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            अगर (lhsRecord.preDefaultp) {
                AstConst* स्थिर rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                क्रम (uint32_t सूचकांक_2 = 0; सूचकांक_2 < tableEntries; ++सूचकांक_2) {
                    स्थिर uint32_t tableOffset = सूचकांक_2 * entryWidth + lhsOffset;
                    tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            क्रम (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* स्थिर rhsp = [&]() -> AstConst* {
                    क्रम (Ast_गाँठ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_गाँठ_Assign* स्थिर ap = VN_AS(stmtp, गाँठ_Assign);
                        अगर (lhsRecord.lhsp->same_वृक्ष(ap->lhsp())) वापस VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    वापस VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                अगर (cip->isDefault()) {
                    क्रम (uint32_t सूचकांक_2 = 0; सूचकांक_2 < tableEntries; ++सूचकांक_2) {
                        स्थिर uint32_t tableOffset = सूचकांक_2 * entryWidth + lhsOffset;
                        tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                    }
                    जारी;
                }

                // Iterate case conditions in reverse order
                क्रम (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    अगर (neverItem(nodep, condp)) जारी;  // If item never matches, ignore it
                    स्थिर स्वतः& match = matchPattern(nodep, condp);
                    स्थिर uint32_t matchMask = match.first.toUInt();
                    स्थिर uint32_t matchBits = match.second.toUInt();
                    स्थिर uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    क्रम (uint32_t i = inverseMask; true; i = (i - 1) & inverseMask) {
                        स्थिर uint32_t सूचकांक_2 = i | matchBits;
                        स्थिर uint32_t tableOffset = सूचकांक_2 * entryWidth + lhsOffset;
                        tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                        अगर (!i) अवरोध;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_गाँठ_व्यंजक* स्थिर tableRefp = [&]() -> Ast_गाँठ_व्यंजक* {
            अगर (isTinyTable) वापस tablep;
            AstVarRef* स्थिर refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_वृक्ष(), tablep);  // V3ConstPool::find clones
            वापस refp;
        }();

        // Create the lookup index
        Ast_गाँठ_व्यंजक* स्थिर caseExprp
            = नया AstExtend{flp, nodep->exprp()->clone_वृक्ष_Pure(false), 32};
        Ast_गाँठ_व्यंजक* स्थिर scalep = नया AstConst{flp, entryWidth};
        Ast_गाँठ_व्यंजक* स्थिर tableLsbp = नया AstMul{flp, scalep, caseExprp};
        Ast_गाँठ_व्यंजक* स्थिर tableSelp
            = नया AstSel{flp, tableRefp, tableLsbp, static_cast<पूर्णांक>(m_caseDecoderEntryWidth)};

        // Connect outputs
        वापस connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_गाँठ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_पंक्ति* स्थिर flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_गाँठ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        क्रम (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            अगर (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                जारी;
            }
            क्रम (Ast_गाँठ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* स्थिर iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                अगर (neverItem(nodep, iconstp)) जारी;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        स्थिर bool provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        अगर (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        स्थिर पूर्णांक decoderEnries = clauses.आकार();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        स्थिर पूर्णांक condWidth = nodep->exprp()->width();
        स्थिर पूर्णांक matchWidth = 2 * VL_शब्दगण_I(condWidth) * VL_EDATASIZE;
        AstConst* स्थिर matchp
            = नया AstConst{flp, AstConst::Widthed_मूल्य{}, decoderEnries * matchWidth, 0};
        क्रम (पूर्णांक i = 0; i < decoderEnries; ++i) {
            स्थिर पूर्णांक entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            अगर (AstConst* स्थिर condp = clauses[i].first) {
                स्थिर स्वतः& match = matchPattern(nodep, condp);
                matchp->अंक().opSelInto(match.first, entryLsb, condWidth);
                matchp->अंक().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                जारी;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_संख्या अंक_Zero{flp, condWidth, 0};
            matchp->अंक().opSelInto(अंक_Zero, entryLsb, condWidth);
            matchp->अंक().opSelInto(अंक_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* स्थिर rangep = नया AstRange{flp, decoderEnries - 1, 0};
        Ast_गाँठ_DType* स्थिर entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_गाँठ_DType* स्थिर tableDtypep = नया AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* स्थिर tablep = नया AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        क्रम (पूर्णांक i = 0; i < decoderEnries; ++i) {
            Ast_गाँठ* स्थिर stmtsp = clauses[i].second;
            AstConst* स्थिर entryp = नया AstConst{flp, AstConst::Widthed_मूल्य{},
                                                  static_cast<पूर्णांक>(m_caseDecoderEntryWidth), 0};
            क्रम (स्थिर LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_गाँठ_व्यंजक* स्थिर lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                क्रम (Ast_गाँठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_गाँठ_Assign* स्थिर assignp = VN_AS(stmtp, गाँठ_Assign);
                    अगर (!lhsp->same_वृक्ष(assignp->lhsp())) जारी;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    अवरोध;
                }
                // Not assigned in this clause, so use the pre-assigned default
                अगर (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->अंक().opSelInto(valConstp->अंक(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* स्थिर matchRefp = V3ConstPool::find(matchp);
        AstVarRef* स्थिर tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_वृक्ष(), matchp);
        VL_DO_DANGLING(tablep->delete_वृक्ष(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_गाँठ_व्यंजक* स्थिर caseExprp = nodep->exprp()->clone_वृक्ष_Pure(false);
        AstMatchMasked* स्थिर indexp = नया AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_गाँठ_व्यंजक* स्थिर entryp = नया AstArraySel{flp, tableRefp, indexp};

        वापस connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठ* convertCaseFastRecurse(Ast_गाँठ_व्यंजक* cexprp, पूर्णांक msb, uint32_t upper_मूल्य) स्थिर {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        अगर (msb < 0) वापस m_caseDetails.records[upper_मूल्य].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        स्थिर uint32_t upperValue0 = upper_मूल्य;
        स्थिर uint32_t upperValue1 = upper_मूल्य | (1UL << msb);
        Ast_गाँठ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_गाँठ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        अगर (tree0p == tree1p) वापस tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            bool same = true;
            क्रम (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                अगर (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = false;
                    अवरोध;
                }
            }
            अगर (same) {
                VL_DO_DANGLING(tree1p->delete_वृक्ष(), tree1p);
                वापस tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        अगर (tree0p && tree0p->backp()) tree0p = tree0p->clone_वृक्ष(true);
        अगर (tree1p && tree1p->backp()) tree1p = tree1p->clone_वृक्ष(true);
        // Create the If statement
        File_पंक्ति* स्थिर flp = cexprp->fileline();
        Ast_गाँठ_व्यंजक* स्थिर condp = नया AstSel{flp, cexprp->clone_वृक्ष_Pure(false), msb, 1};
        AstIf* स्थिर ifp = नया AstIf{flp, condp, tree1p, tree0p};
        वापस ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        स्थिर पूर्णांक caseWidth = nodep->exprp()->width();
        Ast_गाँठ* स्थिर ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        वापस ifrootp && ifrootp->backp() ? ifrootp->clone_वृक्ष(true) : ifrootp;
    }

    Ast_गाँठ_व्यंजक* newCaseCompare(स्थिर AstCase* casep, Ast_गाँठ_व्यंजक* itemExprp) {
        File_पंक्ति* स्थिर flp = itemExprp->fileline();
        अगर (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(धकेलो_Deletep(itemExprp), itemExprp);
            वापस नया AstConst{flp, AstConst::BitFalse{}};
        }

        अगर (AstCond* स्थिर condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_गाँठ_व्यंजक* स्थिर testp = condp->condp()->unlinkFrBack();
            Ast_गाँठ_व्यंजक* स्थिर thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_गाँठ_व्यंजक* स्थिर elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(धकेलो_Deletep(itemExprp), itemExprp, condp);
            वापस नया AstCond{flp, testp, thenp, elsep};
        }

        Ast_गाँठ_व्यंजक* स्थिर caseExprp = casep->exprp()->clone_वृक्ष_Pure(false);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        अगर (AstInsideRange* स्थिर itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_गाँठ_व्यंजक* स्थिर resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(धकेलो_Deletep(itemExprp), itemExprp, itemRangep);
            वापस resultp;
        }

        अगर (AstConst* स्थिर itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            अगर (itemConstp->अंक().isFourState()) {
                स्थिर स्वतः& match = matchPattern(casep, itemConstp);
                स्थिर V3_संख्या& matchMask = match.first;
                स्थिर V3_संख्या& matchBits = match.second;
                VL_DO_DANGLING2(धकेलो_Deletep(itemExprp), itemExprp, itemConstp);
                वापस AstEq::newTyped(flp, नया AstConst{flp, matchBits},
                                       नया AstAnd{flp, caseExprp, नया AstConst{flp, matchMask}});
            }
        }

        वापस AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        bool hasDefault = false;
        क्रम (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_पंक्ति* स्थिर flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            अगर (itemp->isDefault()) {
                itemp->addCondsp(नया AstConst{flp, AstConst::BitTrue{}});
                hasDefault = true;
                जारी;
            }

            // Regular clause. Construct the condition expression.
            Ast_गाँठ_व्यंजक* newCondp = nullptr;
            क्रम (Ast_गाँठ_व्यंजक *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), गाँठ_व्यंजक);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                अगर (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(धकेलो_Deletep(itemExprp), itemExprp);
                    जारी;
                }

                // Compute the term to add to the condition expression
                Ast_गाँठ_व्यंजक* स्थिर termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? नया AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            अगर (!newCondp) newCondp = नया AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        अगर (!hasDefault) {
            अगर (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* स्थिर lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                धकेलो_Deletep(lastp->condsp()->unlink_Fr_Back_With_अग्र());
                lastp->addCondsp(नया AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } अन्यथा {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(नया AstCaseItem{
                    nodep->fileline(), नया AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        पूर्णांक depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        क्रम (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_गाँठ* स्थिर istmtsp = itemp->stmtsp();
            अगर (istmtsp) istmtsp->unlink_Fr_Back_With_अग्र();

            // Expressioned clause
            Ast_गाँठ_व्यंजक* स्थिर ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                अगर (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                अगर (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* स्थिर newp = नया AstIf{itemp->fileline(), ifexprp->clone_वृक्ष_Pure(true)};
                    अगर (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } अन्यथा {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } अन्यथा {  // Continue group, modify if condition to OR in this new condition
                    Ast_गाँठ_व्यंजक* स्थिर condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        नया AstOr{ifexprp->fileline(), condp, ifexprp->clone_वृक्ष_Pure(true)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_गाँठ_व्यंजक* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                अगर (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_वृक्ष(), itemexprp);
                    itemexprp = नया AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* स्थिर newp = नया AstIf{itemp->fileline(), itemexprp, istmtsp};
                अगर (itemnextp) {
                    itemnextp->addElsesp(newp);
                } अन्यथा {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        वापस grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        स्थिर bool useTable = [&]() {
            // Not if disabled
            अगर (!v3Global.opt.fCaseTable()) वापस false;
            // Not if analysis tells us we can't
            अगर (!m_caseTableWidth) वापस false;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            अगर (m_caseTableWidth <= CASE_TABLE_TINY_BITS) वापस true;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            स्थिर माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            अगर (branches < CASE_TABLE_MIN_BRANCHES) वापस false;
            वापस true;
        }();
        अगर (useTable) वापस convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        स्थिर bool useDecoder = [&]() {
            // Not if disabled
            अगर (!v3Global.opt.fCaseDecoder()) वापस false;
            // Not if not a decoder pattern
            अगर (m_caseDecoderRecords.empty()) वापस false;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            स्थिर माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            अगर (branches < CASE_TABLE_MIN_BRANCHES) वापस false;
            वापस true;
        }();
        अगर (useDecoder) वापस convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        स्थिर bool use_Fast_Bit_वृक्ष = [&]() {
            // Not if disabled
            अगर (!v3Global.opt.f_Case_वृक्ष()) वापस false;
            // Can't do it without the detailed analysis
            अगर (!m_caseDetailsValid) वापस false;
            // Can't do it if not exhaustive
            अगर (!m_caseDetails.exhaustive) वापस false;
            // Not worth doing if there are few conditions
            अगर (m_caseNConditions <= 3) वापस false;
            // Avoid e.g. priority expanders from going crazy in expansion
            स्थिर माप_प्रकार caseWidth = nodep->exprp()->width();
            अगर (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) वापस false;
            // Otherwise use the bit tree
            वापस true;
        }();
        अगर (use_Fast_Bit_वृक्ष) वापस convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        अगर (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, true);
        }
        वापस convertCaseGeneric(nodep);
    }

    // VISITORS
    व्योम visit(AstCase* nodep) override {
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
        अगर (Ast_गाँठ* स्थिर stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_Back_With_अग्र();
            अगर (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_वृक्ष(), stmtp);
            } अन्यथा {
                nodep->add_अग्र_Here(stmtp);
            }
        }

        // Convert the case statement and replace the original
        अगर (Ast_गाँठ* स्थिर replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
    }

    व्योम visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    व्योम visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    व्योम visit(Ast_गाँठ* nodep) override { iterateChildren(nodep); }

खुला:
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

माप_प्रकार CaseVisitor::LhsRecord::s_अग्र_Id = 0;

//######################################################################
// Case class functions

व्योम V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("case", 0, dump_वृक्ष_Either_Level() >= 3);
}
व्योम V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
