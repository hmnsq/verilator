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
    अचर Ast_गाँठो* m_casep = nullptr;

    // METHODS
    ढाँचा <typename CaseItem>
    स्थिर शून्य detectMultipleDefaults(CaseItem* itemsp) {
        तार्किक hitDefault = असत्य;
        लागि (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठो::as<CaseItem>(itemp->nextp())) {
            यदि (!itemp->isDefault()) जारी;
            यदि (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = सत्य;
        }
    }

    ढाँचा <typename CaseItem>
    शून्य checkXZinNonCaseX(Ast_गाँठो* casep, Ast_गाँठो_अभिव्यक्ति* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        लागि (CaseItem* itemp = itemsp; itemp; itemp = Ast_गाँठो::as<CaseItem>(itemp->nextp())) {
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
        यदि (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य visit(AstConst* nodep) override {
        यदि (!nodep->अङ्क().isFourState()) फर्काऊ;

        // Error if generate case
        यदि (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            फर्काऊ;
        }

        // Otherwise must be a case statement
        अचर AstCase* अचर casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        यदि (casep->casex()) फर्काऊ;

        यदि (casep->casez() || casep->caseInside()) {
            यदि (nodep->अङ्क().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            फर्काऊ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildrenConst(nodep); }

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

वर्ग CaseVisitor final : सार्वजनिक VNVisitor {
    // Maximum width for detailed analysis
    constexpr स्थिर पूर्णाङ्क CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr स्थिर पूर्णाङ्क CASE_ENCODER_GROUP_DEPTH = 8;
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
        Ast_गाँठो* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    संरचना LhsRecord final {
        Ast_गाँठो_अभिव्यक्ति* lhsp = nullptr;  // LHS of the assignment
        Ast_गाँठो_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
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
    अचर Ast_गाँठो* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    तार्किक m_caseOpaque = असत्य;  // Case statement is opaque (non-packed, or non-const conditions)
    तार्किक m_caseHasDefault = असत्य;  // Indicates the case statement has a default case
    माप_प्रकार m_case_NCase_वस्तुहरू = 0;  // Number of AstCaseItems in the case statement
    माप_प्रकार m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_गाँठो_अभिव्यक्ति>, LhsRecord> m_caseLhsRecords;
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
    स्थिर तार्किक neverItem(अचर AstCase* casep, अचर Ast_गाँठो_अभिव्यक्ति* itemExprp) {
        अचर AstConst* अचर constp = VN_CAST(itemExprp, Const);
        यदि (!constp) फर्काऊ असत्य;
        यदि (casep->casex() || casep->caseInside()) फर्काऊ असत्य;
        यदि (casep->casez()) फर्काऊ constp->अङ्क().isAnyX();
        फर्काऊ constp->अङ्क().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    स्थिर std::pair<V3_सङ्ख्या, V3_सङ्ख्या> matchPattern(अचर AstCase* /*casep*/,
                                                      अचर AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_सङ्ख्या, V3_सङ्ख्या> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->अङ्क());
        pairMaskBits.second.opBitsOne(condp->अङ्क());
        फर्काऊ pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    स्थिर Ast_गाँठो_Assign* checkDecoderAssign(Ast_गाँठो* stmtp) {
        // Only Assign and AssignDly are supported
        यदि (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) फर्काऊ nullptr;
        Ast_गाँठो_Assign* अचर assp = VN_AS(stmtp, गाँठो_Assign);
        // Only if no timing control
        यदि (assp->timingControlp()) फर्काऊ nullptr;
        // Only if assigning a constant
        यदि (!VN_IS(assp->rhsp(), Const)) फर्काऊ nullptr;
        // Only if it's a packed value
        Ast_गाँठो_DType* अचर dtypep = assp->rhsp()->dtypep();
        यदि (dtypep->isString() || dtypep->isDouble()) फर्काऊ nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        यदि (assp->lhsp()->exists([](AstVarRef* refp) { फर्काऊ refp->access().isReadOrRW(); })) {
            फर्काऊ nullptr;
        }
        // This is an assignment that fits the decoder pattern
        फर्काऊ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    तार्किक analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        अचर VNUser1InUse user1InUse;
        लागि (Ast_गाँठो* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_गाँठो_Assign* अचर assp = checkDecoderAssign(stmtp);
            यदि (!assp) फर्काऊ असत्य;
            // Must assign each LHS exactly once - RHS is Const
            अचर तार्किक multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                फर्काऊ refp->varScopep()->user1SetOnce();
            });
            यदि (multipleAssignments) फर्काऊ असत्य;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            यदि (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        फर्काऊ सत्य;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    स्थिर अचर AstEnumDType* getEnumCompletionCheckDType(अचर AstCase* अचर nodep) {
        यदि (!nodep->uniquePragma() && !nodep->unique0Pragma()) फर्काऊ nullptr;
        अचर AstEnumDType* अचर enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        यदि (!enumDtp) फर्काऊ nullptr;  // Case isn't enum
        अचर AstBasicDType* अचर basicp = enumDtp->subDTypep()->basicp();
        यदि (!basicp) फर्काऊ nullptr;  // Not simple type (perhaps IEEE illegal)
        फर्काऊ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    तार्किक checkExhaustiveEnum(अचर AstCase* अचर nodep, अचर AstEnumDType* अचर enump) {
        अचर uint32_t अङ्क_Cases = 1UL << nodep->exprp()->width();
        तार्किक fullyCovered = सत्य;
        string missing_वस्तुहरू;
        लागि (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            अचर स्वतः& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            अचर uint32_t mask = match.first.toUInt();
            अचर uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            लागि (uint32_t i = 0; i < अङ्क_Cases; ++i) {
                यदि ((i & mask) != bits) जारी;  // This case is not for this enum value
                यदि (m_caseDetails.records[i].itemp) जारी;  // Covered case
                यदि (!missing_वस्तुहरू.empty()) missing_वस्तुहरू += ", ";
                missing_वस्तुहरू += eip->prettyNameQ();
                fullyCovered = असत्य;
                रोक;
            }
        }
        यदि (!missing_वस्तुहरू.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_वस्तुहरू);
        }
        फर्काऊ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    तार्किक checkExhaustivePacked(AstCase* nodep) {
        अचर uint32_t अङ्क_Cases = 1UL << nodep->exprp()->width();
        लागि (uint32_t i = 0; i < अङ्क_Cases; ++i) {
            यदि (m_caseDetails.records[i].itemp) जारी;  // Covered case
            यदि (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            फर्काऊ असत्य;
        }

        // It's an exhaustive case statement
        फर्काऊ सत्य;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    शून्य analyzeCaseDetails(AstCase* nodep) {
        अचर uint32_t अङ्क_मानहरू = 1UL << nodep->exprp()->width();
        // Clear case records
        लागि (uint32_t i = 0; i < अङ्क_मानहरू; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        तार्किक reportedOverlap = असत्य;
        तार्किक hasDefault = असत्य;
        m_caseDetails.noOverlaps = सत्य;
        लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            यदि (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                लागि (uint32_t i = 0; i < अङ्क_मानहरू; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    यदि (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = सत्य;
                जारी;
            }

            लागि (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                यदि (neverItem(nodep, iconstp)) जारी;

                अचर स्वतः& match = matchPattern(nodep, iconstp);
                अचर uint32_t mask = match.first.toUInt();
                अचर uint32_t bits = match.second.toUInt();

                तार्किक foundNewCase = असत्य;
                अचर AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_मान = 0;
                लागि (uint32_t i = 0; i < अङ्क_मानहरू; ++i) {
                    यदि ((i & mask) != bits) जारी;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    यदि (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = सत्य;
                        जारी;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    यदि (caseRecord.itemp == itemp) जारी;

                    // Otherwise record the first overlapping case
                    यदि (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_मान = i;
                        m_caseDetails.noOverlaps = असत्य;
                    }
                }

                // Only report first overlap
                यदि (reportedOverlap || !firstOverlapConstp) जारी;

                // Report first overlap
                यदि (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    यदि (!foundNewCase) {
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
                    यदि (iconstp->अङ्क().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_मान
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
        यदि (!hasDefault) {
            यदि (अचर AstEnumDType* अचर enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                अचर तार्किक exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
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
        लागि (स्वतः it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            अचर LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            यदि (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                जारी;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            यदि (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) फर्काऊ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            यदि (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                यदि (lhsRecord.nCaseAssigns == m_case_NCase_वस्तुहरू) जारी;
                यदि (lhsRecord.nCaseAssignDlys == m_case_NCase_वस्तुहरू) जारी;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_गाँठो* अचर preDefaultp = lhsRecord.preDefaultp;
            यदि (!preDefaultp) फर्काऊ;
            // And the pre-default needs to be the same type
            यदि (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) फर्काऊ;
            यदि (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) फर्काऊ;
        }
        // All cases check out, can optimize if there are some entries left
        यदि (m_caseLhsRecords.empty()) फर्काऊ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        लागि (अचर स्वतः& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](अचर LhsRecord& a, अचर LhsRecord& b) {
                      // Sort by width, then id
                      अचर पूर्णाङ्क aWidth = a.lhsp->width();
                      अचर पूर्णाङ्क bWidth = b.lhsp->width();
                      यदि (aWidth != bWidth) फर्काऊ aWidth < bWidth;
                      फर्काऊ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        अचर पूर्णाङ्क caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        अचर स्वतः fitsLimit = [&](माप_प्रकार entryWidth, माप_प्रकार maxWidth) -> तार्किक {
            माप_प्रकार totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            लागि (पूर्णाङ्क i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                यदि (totalWidth > maxWidth) फर्काऊ असत्य;
            }
            फर्काऊ सत्य;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        लागि (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        यदि (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            फर्काऊ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        लागि (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            अचर माप_प्रकार width = lhsRecord.lhsp->width();
            अचर माप_प्रकार lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            अचर माप_प्रकार msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            यदि (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_शब्दहरू_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        अचर माप_प्रकार alignedEntryWidth = VL_शब्दहरू_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        यदि (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            फर्काऊ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    शून्य analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = असत्य;
        m_caseHasDefault = असत्य;
        m_case_NCase_वस्तुहरू = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = असत्य;

        Ast_गाँठो* अचर caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        यदि (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = सत्य;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            अचर VNUser1InUse user1InUse;
            लागि (Ast_गाँठो* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_गाँठो_Assign* अचर assp = checkDecoderAssign(prevp);
                यदि (!assp) रोक;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                अचर तार्किक multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    फर्काऊ refp->varScopep()->user1SetOnce();
                });
                यदि (multipleAssignments) रोक;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        तार्किक canBeDecoder = सत्य;
        लागि (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            लागि (Ast_गाँठो* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                यदि (!neverItem(nodep, VN_AS(condp, गाँठो_अभिव्यक्ति))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                यदि (!VN_IS(condp, Const)) {
                    m_caseOpaque = सत्य;
                    canBeDecoder = असत्य;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            यदि (cip->isDefault()) m_caseHasDefault = सत्य;
            // Count case items
            ++m_case_NCase_वस्तुहरू;
            // Check if it fits the decoder pattern, if still possible
            यदि (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        यदि (m_caseOpaque) फर्काऊ;

        // If small enough, analyse details
        यदि (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        यदि (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_गाँठो_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_गाँठो_अभिव्यक्ति* exprp, V3SharedTmps& tmps) {
        File_पङ्क्ति* अचर flp = nodep->fileline();

        // If there is only one LHS, just use the result
        यदि (m_caseDecoderRecords.size() == 1) {
            अचर LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            अचर पूर्णाङ्क width = lhsRecord.lhsp->width();
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp
                = exprp->width() == width ? exprp : नूतन AstSel{flp, exprp, 0, width};
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = lhsRecord.lhsp->clone_रूख_Pure(असत्य);
            यदि (lhsRecord.nCaseAssigns) {
                फर्काऊ नूतन AstAssign{flp, lhsp, rhsp};
            } अन्यथा यदि (lhsRecord.nCaseAssignDlys) {
                फर्काऊ नूतन AstAssignDly{flp, lhsp, rhsp};
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* अचर tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_गाँठो_अभिव्यक्ति* अचर tempWritep = नूतन AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_गाँठो_Stmt* अचर resultp = नूतन AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        लागि (अचर LhsRecord& lhsRecord : m_caseDecoderRecords) {
            अचर पूर्णाङ्क width = lhsRecord.lhsp->width();
            अचर पूर्णाङ्क lsb = lhsRecord.offset;
            Ast_गाँठो_अभिव्यक्ति* अचर tempReadp = नूतन AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_गाँठो_अभिव्यक्ति* अचर rhsp = नूतन AstSel{flp, tempReadp, lsb, width};
            Ast_गाँठो_अभिव्यक्ति* अचर lhsp = lhsRecord.lhsp->clone_रूख_Pure(असत्य);
            यदि (lhsRecord.nCaseAssigns) {
                resultp->addNext(नूतन AstAssign{flp, lhsp, rhsp});
            } अन्यथा यदि (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(नूतन AstAssignDly{flp, lhsp, rhsp});
            } अन्यथा {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        फर्काऊ resultp;
    }

    Ast_गाँठो_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_पङ्क्ति* अचर flp = nodep->fileline();
        AstConst* अचर tablep
            = नूतन AstConst{flp, AstConst::Widthed_मान{}, static_cast<पूर्णाङ्क>(m_caseTableWidth), 0};
        अचर uint32_t tableEntries = 1U << nodep->exprp()->width();

        अचर तार्किक isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        यदि (isTinyTable) {
            ++m_stats.caseTableTiny;
        } अन्यथा {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        अचर uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_शब्दहरू_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        लागि (अचर LhsRecord& lhsRecord : m_caseDecoderRecords) {
            अचर पूर्णाङ्क lhsWidth = lhsRecord.lhsp->width();
            अचर पूर्णाङ्क lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            यदि (lhsRecord.preDefaultp) {
                AstConst* अचर rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                लागि (uint32_t index = 0; index < tableEntries; ++index) {
                    अचर uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->अङ्क().opSelInto(rhsp->अङ्क(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            लागि (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* अचर rhsp = [&]() -> AstConst* {
                    लागि (Ast_गाँठो* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_गाँठो_Assign* अचर ap = VN_AS(stmtp, गाँठो_Assign);
                        यदि (lhsRecord.lhsp->same_रूख(ap->lhsp())) फर्काऊ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    फर्काऊ VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                यदि (cip->isDefault()) {
                    लागि (uint32_t index = 0; index < tableEntries; ++index) {
                        अचर uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अङ्क().opSelInto(rhsp->अङ्क(), tableOffset, lhsWidth);
                    }
                    जारी;
                }

                // Iterate case conditions in reverse order
                लागि (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    यदि (neverItem(nodep, condp)) जारी;  // If item never matches, ignore it
                    अचर स्वतः& match = matchPattern(nodep, condp);
                    अचर uint32_t matchMask = match.first.toUInt();
                    अचर uint32_t matchBits = match.second.toUInt();
                    अचर uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    लागि (uint32_t i = inverseMask; सत्य; i = (i - 1) & inverseMask) {
                        अचर uint32_t index = i | matchBits;
                        अचर uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->अङ्क().opSelInto(rhsp->अङ्क(), tableOffset, lhsWidth);
                        यदि (!i) रोक;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_गाँठो_अभिव्यक्ति* अचर tableRefp = [&]() -> Ast_गाँठो_अभिव्यक्ति* {
            यदि (isTinyTable) फर्काऊ tablep;
            AstVarRef* अचर refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_रूख(), tablep);  // V3ConstPool::find clones
            फर्काऊ refp;
        }();

        // Create the lookup index
        Ast_गाँठो_अभिव्यक्ति* अचर caseExprp
            = नूतन AstExtend{flp, nodep->exprp()->clone_रूख_Pure(असत्य), 32};
        Ast_गाँठो_अभिव्यक्ति* अचर scalep = नूतन AstConst{flp, entryWidth};
        Ast_गाँठो_अभिव्यक्ति* अचर tableLsbp = नूतन AstMul{flp, scalep, caseExprp};
        Ast_गाँठो_अभिव्यक्ति* अचर tableSelp
            = नूतन AstSel{flp, tableRefp, tableLsbp, static_cast<पूर्णाङ्क>(m_caseDecoderEntryWidth)};

        // Connect outputs
        फर्काऊ connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_गाँठो_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_पङ्क्ति* अचर flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_गाँठो*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        लागि (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            यदि (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                जारी;
            }
            लागि (Ast_गाँठो* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* अचर iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                यदि (neverItem(nodep, iconstp)) जारी;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        अचर तार्किक provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        यदि (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        अचर पूर्णाङ्क decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        अचर पूर्णाङ्क condWidth = nodep->exprp()->width();
        अचर पूर्णाङ्क matchWidth = 2 * VL_शब्दहरू_I(condWidth) * VL_EDATASIZE;
        AstConst* अचर matchp
            = नूतन AstConst{flp, AstConst::Widthed_मान{}, decoderEnries * matchWidth, 0};
        लागि (पूर्णाङ्क i = 0; i < decoderEnries; ++i) {
            अचर पूर्णाङ्क entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            यदि (AstConst* अचर condp = clauses[i].first) {
                अचर स्वतः& match = matchPattern(nodep, condp);
                matchp->अङ्क().opSelInto(match.first, entryLsb, condWidth);
                matchp->अङ्क().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                जारी;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_सङ्ख्या अङ्क_Zero{flp, condWidth, 0};
            matchp->अङ्क().opSelInto(अङ्क_Zero, entryLsb, condWidth);
            matchp->अङ्क().opSelInto(अङ्क_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* अचर rangep = नूतन AstRange{flp, decoderEnries - 1, 0};
        Ast_गाँठो_DType* अचर entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_गाँठो_DType* अचर tableDtypep = नूतन AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* अचर tablep = नूतन AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        लागि (पूर्णाङ्क i = 0; i < decoderEnries; ++i) {
            Ast_गाँठो* अचर stmtsp = clauses[i].second;
            AstConst* अचर entryp = नूतन AstConst{flp, AstConst::Widthed_मान{},
                                                  static_cast<पूर्णाङ्क>(m_caseDecoderEntryWidth), 0};
            लागि (अचर LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_गाँठो_अभिव्यक्ति* अचर lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                लागि (Ast_गाँठो* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_गाँठो_Assign* अचर assignp = VN_AS(stmtp, गाँठो_Assign);
                    यदि (!lhsp->same_रूख(assignp->lhsp())) जारी;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    रोक;
                }
                // Not assigned in this clause, so use the pre-assigned default
                यदि (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->अङ्क().opSelInto(valConstp->अङ्क(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* अचर matchRefp = V3ConstPool::find(matchp);
        AstVarRef* अचर tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_रूख(), matchp);
        VL_DO_DANGLING(tablep->delete_रूख(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_गाँठो_अभिव्यक्ति* अचर caseExprp = nodep->exprp()->clone_रूख_Pure(असत्य);
        AstMatchMasked* अचर indexp = नूतन AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_गाँठो_अभिव्यक्ति* अचर entryp = नूतन AstArraySel{flp, tableRefp, indexp};

        फर्काऊ connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठो* convertCaseFastRecurse(Ast_गाँठो_अभिव्यक्ति* cexprp, पूर्णाङ्क msb, uint32_t upper_मान) अचर {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        यदि (msb < 0) फर्काऊ m_caseDetails.records[upper_मान].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        अचर uint32_t upperValue0 = upper_मान;
        अचर uint32_t upperValue1 = upper_मान | (1UL << msb);
        Ast_गाँठो* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_गाँठो* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        यदि (tree0p == tree1p) फर्काऊ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            तार्किक same = सत्य;
            लागि (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                यदि (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = असत्य;
                    रोक;
                }
            }
            यदि (same) {
                VL_DO_DANGLING(tree1p->delete_रूख(), tree1p);
                फर्काऊ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        यदि (tree0p && tree0p->backp()) tree0p = tree0p->clone_रूख(सत्य);
        यदि (tree1p && tree1p->backp()) tree1p = tree1p->clone_रूख(सत्य);
        // Create the If statement
        File_पङ्क्ति* अचर flp = cexprp->fileline();
        Ast_गाँठो_अभिव्यक्ति* अचर condp = नूतन AstSel{flp, cexprp->clone_रूख_Pure(असत्य), msb, 1};
        AstIf* अचर ifp = नूतन AstIf{flp, condp, tree1p, tree0p};
        फर्काऊ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठो* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        अचर पूर्णाङ्क caseWidth = nodep->exprp()->width();
        Ast_गाँठो* अचर ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        फर्काऊ ifrootp && ifrootp->backp() ? ifrootp->clone_रूख(सत्य) : ifrootp;
    }

    Ast_गाँठो_अभिव्यक्ति* newCaseCompare(अचर AstCase* casep, Ast_गाँठो_अभिव्यक्ति* itemExprp) {
        File_पङ्क्ति* अचर flp = itemExprp->fileline();
        यदि (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(थप_Deletep(itemExprp), itemExprp);
            फर्काऊ नूतन AstConst{flp, AstConst::BitFalse{}};
        }

        यदि (AstCond* अचर condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_गाँठो_अभिव्यक्ति* अचर testp = condp->condp()->unlinkFrBack();
            Ast_गाँठो_अभिव्यक्ति* अचर thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_गाँठो_अभिव्यक्ति* अचर elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(थप_Deletep(itemExprp), itemExprp, condp);
            फर्काऊ नूतन AstCond{flp, testp, thenp, elsep};
        }

        Ast_गाँठो_अभिव्यक्ति* अचर caseExprp = casep->exprp()->clone_रूख_Pure(असत्य);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        यदि (AstInsideRange* अचर itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_गाँठो_अभिव्यक्ति* अचर resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(थप_Deletep(itemExprp), itemExprp, itemRangep);
            फर्काऊ resultp;
        }

        यदि (AstConst* अचर itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            यदि (itemConstp->अङ्क().isFourState()) {
                अचर स्वतः& match = matchPattern(casep, itemConstp);
                अचर V3_सङ्ख्या& matchMask = match.first;
                अचर V3_सङ्ख्या& matchBits = match.second;
                VL_DO_DANGLING2(थप_Deletep(itemExprp), itemExprp, itemConstp);
                फर्काऊ AstEq::newTyped(flp, नूतन AstConst{flp, matchBits},
                                       नूतन AstAnd{flp, caseExprp, नूतन AstConst{flp, matchMask}});
            }
        }

        फर्काऊ AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठो* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        तार्किक hasDefault = असत्य;
        लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_पङ्क्ति* अचर flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            यदि (itemp->isDefault()) {
                itemp->addCondsp(नूतन AstConst{flp, AstConst::BitTrue{}});
                hasDefault = सत्य;
                जारी;
            }

            // Regular clause. Construct the condition expression.
            Ast_गाँठो_अभिव्यक्ति* newCondp = nullptr;
            लागि (Ast_गाँठो_अभिव्यक्ति *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), गाँठो_अभिव्यक्ति);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                यदि (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(थप_Deletep(itemExprp), itemExprp);
                    जारी;
                }

                // Compute the term to add to the condition expression
                Ast_गाँठो_अभिव्यक्ति* अचर termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? नूतन AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            यदि (!newCondp) newCondp = नूतन AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        यदि (!hasDefault) {
            यदि (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* अचर lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                थप_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(नूतन AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } अन्यथा {
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
        लागि (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_गाँठो* अचर istmtsp = itemp->stmtsp();
            यदि (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_गाँठो_अभिव्यक्ति* अचर ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                यदि (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                यदि (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* अचर newp = नूतन AstIf{itemp->fileline(), ifexprp->clone_रूख_Pure(सत्य)};
                    यदि (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } अन्यथा {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } अन्यथा {  // Continue group, modify if condition to OR in this new condition
                    Ast_गाँठो_अभिव्यक्ति* अचर condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        नूतन AstOr{ifexprp->fileline(), condp, ifexprp->clone_रूख_Pure(सत्य)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_गाँठो_अभिव्यक्ति* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                यदि (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_रूख(), itemexprp);
                    itemexprp = नूतन AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* अचर newp = नूतन AstIf{itemp->fileline(), itemexprp, istmtsp};
                यदि (itemnextp) {
                    itemnextp->addElsesp(newp);
                } अन्यथा {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        फर्काऊ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_गाँठो* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        अचर तार्किक useTable = [&]() {
            // Not if disabled
            यदि (!v3Global.opt.fCaseTable()) फर्काऊ असत्य;
            // Not if analysis tells us we can't
            यदि (!m_caseTableWidth) फर्काऊ असत्य;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            यदि (m_caseTableWidth <= CASE_TABLE_TINY_BITS) फर्काऊ सत्य;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            अचर माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            यदि (branches < CASE_TABLE_MIN_BRANCHES) फर्काऊ असत्य;
            फर्काऊ सत्य;
        }();
        यदि (useTable) फर्काऊ convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        अचर तार्किक useDecoder = [&]() {
            // Not if disabled
            यदि (!v3Global.opt.fCaseDecoder()) फर्काऊ असत्य;
            // Not if not a decoder pattern
            यदि (m_caseDecoderRecords.empty()) फर्काऊ असत्य;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            अचर माप_प्रकार branches = std::min<माप_प्रकार>(nodep->exprp()->width(), m_caseNConditions);
            यदि (branches < CASE_TABLE_MIN_BRANCHES) फर्काऊ असत्य;
            फर्काऊ सत्य;
        }();
        यदि (useDecoder) फर्काऊ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        अचर तार्किक use_Fast_Bit_रूख = [&]() {
            // Not if disabled
            यदि (!v3Global.opt.f_Case_रूख()) फर्काऊ असत्य;
            // Can't do it without the detailed analysis
            यदि (!m_caseDetailsValid) फर्काऊ असत्य;
            // Can't do it if not exhaustive
            यदि (!m_caseDetails.exhaustive) फर्काऊ असत्य;
            // Not worth doing if there are few conditions
            यदि (m_caseNConditions <= 3) फर्काऊ असत्य;
            // Avoid e.g. priority expanders from going crazy in expansion
            अचर माप_प्रकार caseWidth = nodep->exprp()->width();
            यदि (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) फर्काऊ असत्य;
            // Otherwise use the bit tree
            फर्काऊ सत्य;
        }();
        यदि (use_Fast_Bit_रूख) फर्काऊ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        यदि (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, सत्य);
        }
        फर्काऊ convertCaseGeneric(nodep);
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
        यदि (Ast_गाँठो* अचर stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            यदि (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_रूख(), stmtp);
            } अन्यथा {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        यदि (Ast_गाँठो* अचर replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } अन्यथा {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_रूख(), nodep);
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
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

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
    V3Global::dump_Check_Global_रूख("case", 0, dump_रूख_Either_Level() >= 3);
}
शून्य V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
