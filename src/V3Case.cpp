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

वर्ग CaseLintVisitor final : सार्वजनिक VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    स्थिरांक Ast_गाठ* m_casep = nullptr;

    // METHODS
    ढाँचा <typename CaseItem>
    स्थिर शून्य detectMultipleDefaults(CaseItem* itemsp) {
        तार्किक hitDefault = असत्य;
        साठी (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाठ::as<CaseItem>(itemp->nextp())) {
            जर (!itemp->isDefault()) सुरूठेवा;
            जर (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = सत्य;
        }
    }

    ढाँचा <typename CaseItem>
    शून्य checkXZinNonCaseX(Ast_गाठ* casep, Ast_गाठ_अभिव्यक्ती* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        साठी (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाठ::as<CaseItem>(itemp->nextp())) {
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
        जर (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य visit(AstConst* nodep) override {
        जर (!nodep->अंक().isFourState()) परत;

        // Error if generate case
        जर (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            परत;
        }

        // Otherwise must be a case statement
        स्थिरांक AstCase* स्थिरांक casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        जर (casep->casex()) परत;

        जर (casep->casez() || casep->caseInside()) {
            जर (nodep->अंक().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            परत;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    शून्य visit(Ast_गाठ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = पूर्वनिर्धारित;

सार्वजनिक:
    स्थिर शून्य apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    स्थिर शून्य apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

वर्ग CaseVisitor final : सार्वजनिक VNVisitor {
    // Maximum width for detailed analysis
    constexpr स्थिर पूर्णांक CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr स्थिर पूर्णांक CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr स्थिर माप_प्रकार CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr स्थिर माप_प्रकार CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr स्थिर माप_प्रकार CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    संरचना CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_गाठ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    संरचना LhsRecord final {
        Ast_गाठ_अभिव्यक्ती* lhsp = nullptr;  // LHS of the assignment
        Ast_गाठ_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        माप_प्रकार nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        माप_प्रकार nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        माप_प्रकार offset = 0;  // Offset in the table for this LHS

        स्थिर माप_प्रकार s_nextId;  // Static unique Id counter
        माप_प्रकार id = ++s_nextId;  // Unique Id for sorting
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
    स्थिरांक Ast_गाठ* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    तार्किक m_caseOpaque = असत्य;  // Case statement is opaque (non-packed, or non-const conditions)
    तार्किक m_caseHasDefault = असत्य;  // Indicates the case statement has a default case
    माप_प्रकार m_case_NCase_वस्तुसंच = 0;  // Number of AstCaseItems in the case statement
    माप_प्रकार m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_गाठ_अभिव्यक्ती>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    माप_प्रकार m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    माप_प्रकार m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    तार्किक m_caseDetailsValid = असत्य;  // Indicates m_caseDetails is valid
    संरचना final {
        तार्किक exhaustive = असत्य;  // Proven exhaustive
        तार्किक exhaustiveOverEnumOnly = असत्य;  // Exhaustive over enum values only
        तार्किक noOverlaps = असत्य;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    स्थिर तार्किक neverItem(स्थिरांक AstCase* casep, स्थिरांक Ast_गाठ_अभिव्यक्ती* itemExprp) {
        स्थिरांक AstConst* स्थिरांक constp = VN_CAST(itemExprp, Const);
        जर (!constp) परत असत्य;
        जर (casep->casex() || casep->caseInside()) परत असत्य;
        जर (casep->casez()) परत constp->अंक().isAnyX();
        परत constp->अंक().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    स्थिर std::pair<V3_संख्या, V3_संख्या> matchPattern(स्थिरांक AstCase* /*casep*/,
                                                      स्थिरांक AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_संख्या, V3_संख्या> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->अंक());
        pairMaskBits.second.opBitsOne(condp->अंक());
        परत pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    स्थिर Ast_गाठ_Assign* checkDecoderAssign(Ast_गाठ* stmtp) {
        // Only Assign and AssignDly are supported
        जर (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) परत nullptr;
        Ast_गाठ_Assign* स्थिरांक assp = VN_AS(stmtp, गाठ_Assign);
        // Only if no timing control
        जर (assp->timingControlp()) परत nullptr;
        // Only if assigning a constant
        जर (!VN_IS(assp->rhsp(), Const)) परत nullptr;
        // Only if it's a packed value
        Ast_गाठ_DType* स्थिरांक dtypep = assp->rhsp()->dtypep();
        जर (dtypep->isString() || dtypep->isDouble()) परत nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        जर (assp->lhsp()->exists([](AstVarRef* refp) { परत refp->access().isReadOrRW(); })) {
            परत nullptr;
        }
        // This is an assignment that fits the decoder pattern
        परत assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    तार्किक analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        स्थिरांक VNUser1InUse user1InUse;
        साठी (Ast_गाठ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_गाठ_Assign* स्थिरांक assp = checkDecoderAssign(stmtp);
            जर (!assp) परत असत्य;
            // Must assign each LHS exactly once - RHS is Const
            स्थिरांक तार्किक multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                परत refp->varScopep()->user1SetOnce();
            });
            जर (multipleAssignments) परत असत्य;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            जर (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        परत सत्य;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    स्थिर स्थिरांक AstEnumDType* getEnumCompletionCheckDType(स्थिरांक AstCase* स्थिरांक nodep) {
        जर (!nodep->uniquePragma() && !nodep->unique0Pragma()) परत nullptr;
        स्थिरांक AstEnumDType* स्थिरांक enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        जर (!enumDtp) परत nullptr;  // Case isn't enum
        स्थिरांक AstBasicDType* स्थिरांक basicp = enumDtp->subDTypep()->basicp();
        जर (!basicp) परत nullptr;  // Not simple type (perhaps IEEE illegal)
        परत enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    तार्किक checkExhaustiveEnum(स्थिरांक AstCase* स्थिरांक nodep, स्थिरांक AstEnumDType* स्थिरांक enump) {
        स्थिरांक uint32_t अंक_Cases = 1UL << nodep->exprp()->width();
        तार्किक fullyCovered = सत्य;
        string missing_वस्तुसंच;
        साठी (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            स्थिरांक स्वतः& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            स्थिरांक uint32_t mask = match.first.toUInt();
            स्थिरांक uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            साठी (uint32_t i = 0; i < अंक_Cases; ++i) {
                जर ((i & mask) != bits) सुरूठेवा;  // This case is not for this enum value
                जर (m_caseDetails.records[i].itemp) सुरूठेवा;  // Covered case
                जर (!missing_वस्तुसंच.empty()) missing_वस्तुसंच += ", ";
                missing_वस्तुसंच += eip->prettyNameQ();
                fullyCovered = असत्य;
                थांबा;
            }
        }
        जर (!missing_वस्तुसंच.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_वस्तुसंच);
        }
        परत fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    तार्किक checkExhaustivePacked(AstCase* nodep) {
        स्थिरांक uint32_t अंक_Cases = 1UL << nodep->exprp()->width();
        साठी (uint32_t i = 0; i < अंक_Cases; ++i) {
            जर (m_caseDetails.records[i].itemp) सुरूठेवा;  // Covered case
            जर (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            परत असत्य;
        }

        // It's an exhaustive case statement
        परत सत्य;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    शून्य analyzeCaseDetails(AstCase* nodep) {
        स्थिरांक uint32_t अंक_मूल्ये = 1UL << nodep->exprp()->width();
        // Clear case records
        साठी (uint32_t i = 0; i < अंक_मूल्ये; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        तार्किक reportedOverlap = असत्य;
        तार्किक hasDefault = असत्य;
        m_caseDetails.noOverlaps = सत्य;
        साठी (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            जर (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                साठी (uint32_t i = 0; i < अंक_मूल्ये; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    जर (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = सत्य;
                सुरूठेवा;
            }

            साठी (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                जर (neverItem(nodep, iconstp)) सुरूठेवा;

                स्थिरांक स्वतः& match = matchPattern(nodep, iconstp);
                स्थिरांक uint32_t mask = match.first.toUInt();
                स्थिरांक uint32_t bits = match.second.toUInt();

                तार्किक foundNewCase = असत्य;
                स्थिरांक AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_मूल्य = 0;
                साठी (uint32_t i = 0; i < अंक_मूल्ये; ++i) {
                    जर ((i & mask) != bits) सुरूठेवा;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    जर (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = सत्य;
                        सुरूठेवा;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    जर (caseRecord.itemp == itemp) सुरूठेवा;

                    // Otherwise record the first overlapping case
                    जर (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_मूल्य = i;
                        m_caseDetails.noOverlaps = असत्य;
                    }
                }

                // Only report first overlap
                जर (reportedOverlap || !firstOverlapConstp) सुरूठेवा;

                // Report first overlap
                जर (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    जर (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = सत्य;
                    }
                } अन्यथा {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    जर (iconstp->अंक().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_मूल्य
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = सत्य;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = असत्य;
        m_caseDetails.exhaustive = hasDefault;
        जर (!hasDefault) {
            जर (स्थिरांक AstEnumDType* स्थिरांक enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                स्थिरांक तार्किक exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } अन्यथा {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = सत्य;
    }

    शून्य analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        साठी (स्वतः it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            स्थिरांक LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            जर (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                सुरूठेवा;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            जर (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) परत;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            जर (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                जर (lhsRecord.nCaseAssigns == m_case_NCase_वस्तुसंच) सुरूठेवा;
                जर (lhsRecord.nCaseAssignDlys == m_case_NCase_वस्तुसंच) सुरूठेवा;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_गाठ* स्थिरांक preDefaultp = lhsRecord.preDefaultp;
            जर (!preDefaultp) परत;
            // And the pre-default needs to be the same type
            जर (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) परत;
            जर (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) परत;
        }
        // All cases check out, can optimize if there are some entries left
        जर (m_caseLhsRecords.empty()) परत;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        साठी (स्थिरांक स्वतः& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](स्थिरांक LhsRecord& a, स्थिरांक LhsRecord& b) {
                      // Sort by width, then id
                      स्थिरांक पूर्णांक aWidth = a.lhsp->width();
                      स्थिरांक पूर्णांक bWidth = b.lhsp->width();
                      जर (aWidth != bWidth) परत aWidth < bWidth;
                      परत a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        स्थिरांक पूर्णांक caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        स्थिरांक स्वतः fitsLimit = [&](माप_प्रकार entryWidth, माप_प्रकार maxWidth) -> तार्किक {
            माप_प्रकार totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            साठी (पूर्णांक i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                जर (totalWidth > maxWidth) परत असत्य;
            }
            परत सत्य;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        साठी (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        जर (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            परत;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        साठी (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिरांक माप_प्रकार width = lhsRecord.lhsp->width();
            स्थिरांक माप_प्रकार lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            स्थिरांक माप_प्रकार msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            जर (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_शब्द_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        स्थिरांक माप_प्रकार alignedEntryWidth = VL_शब्द_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        जर (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            परत;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    शून्य analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = असत्य;
        m_caseHasDefault = असत्य;
        m_case_NCase_वस्तुसंच = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = असत्य;

        Ast_गाठ* स्थिरांक caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        जर (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = सत्य;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            स्थिरांक VNUser1InUse user1InUse;
            साठी (Ast_गाठ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_गाठ_Assign* स्थिरांक assp = checkDecoderAssign(prevp);
                जर (!assp) थांबा;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                स्थिरांक तार्किक multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    परत refp->varScopep()->user1SetOnce();
                });
                जर (multipleAssignments) थांबा;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        तार्किक canBeDecoder = सत्य;
        साठी (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            साठी (Ast_गाठ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                जर (!neverItem(nodep, VN_AS(condp, गाठ_अभिव्यक्ती))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                जर (!VN_IS(condp, Const)) {
                    m_caseOpaque = सत्य;
                    canBeDecoder = असत्य;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            जर (cip->isDefault()) m_caseHasDefault = सत्य;
            // Count case items
            ++m_case_NCase_वस्तुसंच;
            // Check if it fits the decoder pattern, if still possible
            जर (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        जर (m_caseOpaque) परत;

        // If small enough, analyse details
        जर (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        जर (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_गाठ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_गाठ_अभिव्यक्ती* exprp, V3SharedTmps& tmps) {
        File_ओळ* स्थिरांक flp = nodep->fileline();

        // If there is only one LHS, just use the result
        जर (m_caseDecoderRecords.size() == 1) {
            स्थिरांक LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            स्थिरांक पूर्णांक width = lhsRecord.lhsp->width();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp
                = exprp->width() == width ? exprp : नवीन AstSel{flp, exprp, 0, width};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = lhsRecord.lhsp->clone_वृक्ष_Pure(असत्य);
            जर (lhsRecord.nCaseAssigns) {
                परत नवीन AstAssign{flp, lhsp, rhsp};
            } अन्यथा जर (lhsRecord.nCaseAssignDlys) {
                परत नवीन AstAssignDly{flp, lhsp, rhsp};
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* स्थिरांक tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_गाठ_अभिव्यक्ती* स्थिरांक tempWritep = नवीन AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_गाठ_Stmt* स्थिरांक resultp = नवीन AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        साठी (स्थिरांक LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिरांक पूर्णांक width = lhsRecord.lhsp->width();
            स्थिरांक पूर्णांक lsb = lhsRecord.offset;
            Ast_गाठ_अभिव्यक्ती* स्थिरांक tempReadp = नवीन AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक rhsp = नवीन AstSel{flp, tempReadp, lsb, width};
            Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = lhsRecord.lhsp->clone_वृक्ष_Pure(असत्य);
            जर (lhsRecord.nCaseAssigns) {
                resultp->addNext(नवीन AstAssign{flp, lhsp, rhsp});
            } अन्यथा जर (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(नवीन AstAssignDly{flp, lhsp, rhsp});
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        परत resultp;
    }

    Ast_गाठ_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_ओळ* स्थिरांक flp = nodep->fileline();
        AstConst* स्थिरांक tablep
            = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, static_cast<पूर्णांक>(m_caseTableWidth), 0};
        स्थिरांक uint32_t tableEntries = 1U << nodep->exprp()->width();

        स्थिरांक तार्किक isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        जर (isTinyTable) {
            ++m_stats.caseTableTiny;
        } अन्यथा {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        स्थिरांक uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_शब्द_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        साठी (स्थिरांक LhsRecord& lhsRecord : m_caseDecoderRecords) {
            स्थिरांक पूर्णांक lhsWidth = lhsRecord.lhsp->width();
            स्थिरांक पूर्णांक lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            जर (lhsRecord.preDefaultp) {
                AstConst* स्थिरांक rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                साठी (uint32_t index = 0; index < tableEntries; ++index) {
                    स्थिरांक uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            साठी (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* स्थिरांक rhsp = [&]() -> AstConst* {
                    साठी (Ast_गाठ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_गाठ_Assign* स्थिरांक ap = VN_AS(stmtp, गाठ_Assign);
                        जर (lhsRecord.lhsp->same_वृक्ष(ap->lhsp())) परत VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    परत VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                जर (cip->isDefault()) {
                    साठी (uint32_t index = 0; index < tableEntries; ++index) {
                        स्थिरांक uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                    }
                    सुरूठेवा;
                }

                // Iterate case conditions in reverse order
                साठी (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    जर (neverItem(nodep, condp)) सुरूठेवा;  // If item never matches, ignore it
                    स्थिरांक स्वतः& match = matchPattern(nodep, condp);
                    स्थिरांक uint32_t matchMask = match.first.toUInt();
                    स्थिरांक uint32_t matchBits = match.second.toUInt();
                    स्थिरांक uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    साठी (uint32_t i = inverseMask; सत्य; i = (i - 1) & inverseMask) {
                        स्थिरांक uint32_t index = i | matchBits;
                        स्थिरांक uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अंक().opSelInto(rhsp->अंक(), tableOffset, lhsWidth);
                        जर (!i) थांबा;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_गाठ_अभिव्यक्ती* स्थिरांक tableRefp = [&]() -> Ast_गाठ_अभिव्यक्ती* {
            जर (isTinyTable) परत tablep;
            AstVarRef* स्थिरांक refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_वृक्ष(), tablep);  // V3ConstPool::find clones
            परत refp;
        }();

        // Create the lookup index
        Ast_गाठ_अभिव्यक्ती* स्थिरांक caseExprp
            = नवीन AstExtend{flp, nodep->exprp()->clone_वृक्ष_Pure(असत्य), 32};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक scalep = नवीन AstConst{flp, entryWidth};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक tableLsbp = नवीन AstMul{flp, scalep, caseExprp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक tableSelp
            = नवीन AstSel{flp, tableRefp, tableLsbp, static_cast<पूर्णांक>(m_caseDecoderEntryWidth)};

        // Connect outputs
        परत connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_गाठ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_ओळ* स्थिरांक flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_गाठ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        साठी (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            जर (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                सुरूठेवा;
            }
            साठी (Ast_गाठ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* स्थिरांक iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                जर (neverItem(nodep, iconstp)) सुरूठेवा;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        स्थिरांक तार्किक provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        जर (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        स्थिरांक पूर्णांक decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        स्थिरांक पूर्णांक condWidth = nodep->exprp()->width();
        स्थिरांक पूर्णांक matchWidth = 2 * VL_शब्द_I(condWidth) * VL_EDATASIZE;
        AstConst* स्थिरांक matchp
            = नवीन AstConst{flp, AstConst::Widthed_मूल्य{}, decoderEnries * matchWidth, 0};
        साठी (पूर्णांक i = 0; i < decoderEnries; ++i) {
            स्थिरांक पूर्णांक entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            जर (AstConst* स्थिरांक condp = clauses[i].first) {
                स्थिरांक स्वतः& match = matchPattern(nodep, condp);
                matchp->अंक().opSelInto(match.first, entryLsb, condWidth);
                matchp->अंक().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                सुरूठेवा;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_संख्या अंक_Zero{flp, condWidth, 0};
            matchp->अंक().opSelInto(अंक_Zero, entryLsb, condWidth);
            matchp->अंक().opSelInto(अंक_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* स्थिरांक rangep = नवीन AstRange{flp, decoderEnries - 1, 0};
        Ast_गाठ_DType* स्थिरांक entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_गाठ_DType* स्थिरांक tableDtypep = नवीन AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* स्थिरांक tablep = नवीन AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        साठी (पूर्णांक i = 0; i < decoderEnries; ++i) {
            Ast_गाठ* स्थिरांक stmtsp = clauses[i].second;
            AstConst* स्थिरांक entryp = नवीन AstConst{flp, AstConst::Widthed_मूल्य{},
                                                  static_cast<पूर्णांक>(m_caseDecoderEntryWidth), 0};
            साठी (स्थिरांक LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_गाठ_अभिव्यक्ती* स्थिरांक lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                साठी (Ast_गाठ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_गाठ_Assign* स्थिरांक assignp = VN_AS(stmtp, गाठ_Assign);
                    जर (!lhsp->same_वृक्ष(assignp->lhsp())) सुरूठेवा;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    थांबा;
                }
                // Not assigned in this clause, so use the pre-assigned default
                जर (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->अंक().opSelInto(valConstp->अंक(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* स्थिरांक matchRefp = V3ConstPool::find(matchp);
        AstVarRef* स्थिरांक tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_वृक्ष(), matchp);
        VL_DO_DANGLING(tablep->delete_वृक्ष(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_गाठ_अभिव्यक्ती* स्थिरांक caseExprp = nodep->exprp()->clone_वृक्ष_Pure(असत्य);
        AstMatchMasked* स्थिरांक indexp = नवीन AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_गाठ_अभिव्यक्ती* स्थिरांक entryp = नवीन AstArraySel{flp, tableRefp, indexp};

        परत connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_गाठ* convertCaseFastRecurse(Ast_गाठ_अभिव्यक्ती* cexprp, पूर्णांक msb, uint32_t upper_मूल्य) स्थिरांक {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        जर (msb < 0) परत m_caseDetails.records[upper_मूल्य].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        स्थिरांक uint32_t upperValue0 = upper_मूल्य;
        स्थिरांक uint32_t upperValue1 = upper_मूल्य | (1UL << msb);
        Ast_गाठ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_गाठ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        जर (tree0p == tree1p) परत tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            तार्किक same = सत्य;
            साठी (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                जर (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = असत्य;
                    थांबा;
                }
            }
            जर (same) {
                VL_DO_DANGLING(tree1p->delete_वृक्ष(), tree1p);
                परत tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        जर (tree0p && tree0p->backp()) tree0p = tree0p->clone_वृक्ष(सत्य);
        जर (tree1p && tree1p->backp()) tree1p = tree1p->clone_वृक्ष(सत्य);
        // Create the If statement
        File_ओळ* स्थिरांक flp = cexprp->fileline();
        Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = नवीन AstSel{flp, cexprp->clone_वृक्ष_Pure(असत्य), msb, 1};
        AstIf* स्थिरांक ifp = नवीन AstIf{flp, condp, tree1p, tree0p};
        परत ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_गाठ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        स्थिरांक पूर्णांक caseWidth = nodep->exprp()->width();
        Ast_गाठ* स्थिरांक ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        परत ifrootp && ifrootp->backp() ? ifrootp->clone_वृक्ष(सत्य) : ifrootp;
    }

    Ast_गाठ_अभिव्यक्ती* newCaseCompare(स्थिरांक AstCase* casep, Ast_गाठ_अभिव्यक्ती* itemExprp) {
        File_ओळ* स्थिरांक flp = itemExprp->fileline();
        जर (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(ढकला_Deletep(itemExprp), itemExprp);
            परत नवीन AstConst{flp, AstConst::BitFalse{}};
        }

        जर (AstCond* स्थिरांक condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_गाठ_अभिव्यक्ती* स्थिरांक testp = condp->condp()->unlinkFrBack();
            Ast_गाठ_अभिव्यक्ती* स्थिरांक thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_गाठ_अभिव्यक्ती* स्थिरांक elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(ढकला_Deletep(itemExprp), itemExprp, condp);
            परत नवीन AstCond{flp, testp, thenp, elsep};
        }

        Ast_गाठ_अभिव्यक्ती* स्थिरांक caseExprp = casep->exprp()->clone_वृक्ष_Pure(असत्य);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        जर (AstInsideRange* स्थिरांक itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_गाठ_अभिव्यक्ती* स्थिरांक resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(ढकला_Deletep(itemExprp), itemExprp, itemRangep);
            परत resultp;
        }

        जर (AstConst* स्थिरांक itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            जर (itemConstp->अंक().isFourState()) {
                स्थिरांक स्वतः& match = matchPattern(casep, itemConstp);
                स्थिरांक V3_संख्या& matchMask = match.first;
                स्थिरांक V3_संख्या& matchBits = match.second;
                VL_DO_DANGLING2(ढकला_Deletep(itemExprp), itemExprp, itemConstp);
                परत AstEq::newTyped(flp, नवीन AstConst{flp, matchBits},
                                       नवीन AstAnd{flp, caseExprp, नवीन AstConst{flp, matchMask}});
            }
        }

        परत AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_गाठ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        तार्किक hasDefault = असत्य;
        साठी (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_ओळ* स्थिरांक flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            जर (itemp->isDefault()) {
                itemp->addCondsp(नवीन AstConst{flp, AstConst::BitTrue{}});
                hasDefault = सत्य;
                सुरूठेवा;
            }

            // Regular clause. Construct the condition expression.
            Ast_गाठ_अभिव्यक्ती* newCondp = nullptr;
            साठी (Ast_गाठ_अभिव्यक्ती *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), गाठ_अभिव्यक्ती);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                जर (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(ढकला_Deletep(itemExprp), itemExprp);
                    सुरूठेवा;
                }

                // Compute the term to add to the condition expression
                Ast_गाठ_अभिव्यक्ती* स्थिरांक termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? नवीन AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            जर (!newCondp) newCondp = नवीन AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        जर (!hasDefault) {
            जर (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* स्थिरांक lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                ढकला_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(नवीन AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } अन्यथा {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(नवीन AstCaseItem{
                    nodep->fileline(), नवीन AstConst{nodep->fileline(), AstConst::BitTrue{}},
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
        साठी (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_गाठ* स्थिरांक istmtsp = itemp->stmtsp();
            जर (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_गाठ_अभिव्यक्ती* स्थिरांक ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                जर (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                जर (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* स्थिरांक newp = नवीन AstIf{itemp->fileline(), ifexprp->clone_वृक्ष_Pure(सत्य)};
                    जर (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } अन्यथा {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } अन्यथा {  // Continue group, modify if condition to OR in this new condition
                    Ast_गाठ_अभिव्यक्ती* स्थिरांक condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        नवीन AstOr{ifexprp->fileline(), condp, ifexprp->clone_वृक्ष_Pure(सत्य)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_गाठ_अभिव्यक्ती* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                जर (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_वृक्ष(), itemexprp);
                    itemexprp = नवीन AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* स्थिरांक newp = नवीन AstIf{itemp->fileline(), itemexprp, istmtsp};
                जर (itemnextp) {
                    itemnextp->addElsesp(newp);
                } अन्यथा {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        परत grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_गाठ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        स्थिरांक तार्किक useTable = [&]() {
            // Not if disabled
            जर (!v3Global.opt.fCaseTable()) परत असत्य;
            // Not if analysis tells us we can't
            जर (!m_caseTableWidth) परत असत्य;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            जर (m_caseTableWidth <= CASE_TABLE_TINY_BITS) परत सत्य;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            स्थिरांक माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            जर (branches < CASE_TABLE_MIN_BRANCHES) परत असत्य;
            परत सत्य;
        }();
        जर (useTable) परत convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        स्थिरांक तार्किक useDecoder = [&]() {
            // Not if disabled
            जर (!v3Global.opt.fCaseDecoder()) परत असत्य;
            // Not if not a decoder pattern
            जर (m_caseDecoderRecords.empty()) परत असत्य;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            स्थिरांक माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            जर (branches < CASE_TABLE_MIN_BRANCHES) परत असत्य;
            परत सत्य;
        }();
        जर (useDecoder) परत convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        स्थिरांक तार्किक use_Fast_Bit_वृक्ष = [&]() {
            // Not if disabled
            जर (!v3Global.opt.f_Case_वृक्ष()) परत असत्य;
            // Can't do it without the detailed analysis
            जर (!m_caseDetailsValid) परत असत्य;
            // Can't do it if not exhaustive
            जर (!m_caseDetails.exhaustive) परत असत्य;
            // Not worth doing if there are few conditions
            जर (m_caseNConditions <= 3) परत असत्य;
            // Avoid e.g. priority expanders from going crazy in expansion
            स्थिरांक माप_प्रकार caseWidth = nodep->exprp()->width();
            जर (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) परत असत्य;
            // Otherwise use the bit tree
            परत सत्य;
        }();
        जर (use_Fast_Bit_वृक्ष) परत convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        जर (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, सत्य);
        }
        परत convertCaseGeneric(nodep);
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
        जर (Ast_गाठ* स्थिरांक stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            जर (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_वृक्ष(), stmtp);
            } अन्यथा {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        जर (Ast_गाठ* स्थिरांक replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_वृक्ष(), nodep);
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
    शून्य visit(Ast_गाठ* nodep) override { iterateChildren(nodep); }

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

माप_प्रकार CaseVisitor::LhsRecord::s_nextId = 0;

//######################################################################
// Case class functions

शून्य V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्ष("case", 0, dump_वृक्ष_Either_Level() >= 3);
}
शून्य V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
