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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Case.h"

#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം CaseLintVisitor final : പൊതു VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    സ്ഥിരം Ast_കണ്ണി* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    സ്ഥിതം ശൂന്യം detectMultipleDefaults(CaseItem* itemsp) {
        തർക്കം hitDefault = അസത്യം;
        വേണ്ടി (CaseItem* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<CaseItem>(itemp->nextp())) {
            എങ്കിൽ (!itemp->isDefault()) തുടരുക;
            എങ്കിൽ (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = സത്യം;
        }
    }

    template <typename CaseItem>
    ശൂന്യം checkXZinNonCaseX(Ast_കണ്ണി* casep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        വേണ്ടി (CaseItem* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    ശൂന്യം visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    ശൂന്യം visit(AstCase* nodep) override {
        എങ്കിൽ (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    ശൂന്യം visit(AstConst* nodep) override {
        എങ്കിൽ (!nodep->അക്കം().isFourState()) തിരികെ;

        // Error if generate case
        എങ്കിൽ (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            തിരികെ;
        }

        // Otherwise must be a case statement
        സ്ഥിരം AstCase* സ്ഥിരം casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        എങ്കിൽ (casep->casex()) തിരികെ;

        എങ്കിൽ (casep->casez() || casep->caseInside()) {
            എങ്കിൽ (nodep->അക്കം().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            തിരികെ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = സ്ഥിരസ്ഥിതി;

പൊതു:
    സ്ഥിതം ശൂന്യം apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    സ്ഥിതം ശൂന്യം apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

വർഗ്ഗം CaseVisitor final : പൊതു VNVisitor {
    // Maximum width for detailed analysis
    constexpr സ്ഥിതം പൂർണ്ണസംഖ്യ CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr സ്ഥിതം പൂർണ്ണസംഖ്യ CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr സ്ഥിതം size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr സ്ഥിതം size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr സ്ഥിതം size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ഘടന CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_കണ്ണി* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ഘടന LhsRecord final {
        Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nullptr;  // LHS of the assignment
        Ast_കണ്ണി_Assign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        സ്ഥിതം size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ഘടന Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    സ്ഥിരം Ast_കണ്ണി* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    തർക്കം m_caseOpaque = അസത്യം;  // Case statement is opaque (non-packed, or non-const conditions)
    തർക്കം m_caseHasDefault = അസത്യം;  // Indicates the case statement has a default case
    size_t m_case_NCase_ഇനങ്ങൾ = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_കണ്ണി_വ്യഞ്ജകം>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    തർക്കം m_caseDetailsValid = അസത്യം;  // Indicates m_caseDetails is valid
    ഘടന final {
        തർക്കം exhaustive = അസത്യം;  // Proven exhaustive
        തർക്കം exhaustiveOverEnumOnly = അസത്യം;  // Exhaustive over enum values only
        തർക്കം noOverlaps = അസത്യം;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    സ്ഥിതം തർക്കം neverItem(സ്ഥിരം AstCase* casep, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* itemExprp) {
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(itemExprp, Const);
        എങ്കിൽ (!constp) തിരികെ അസത്യം;
        എങ്കിൽ (casep->casex() || casep->caseInside()) തിരികെ അസത്യം;
        എങ്കിൽ (casep->casez()) തിരികെ constp->അക്കം().isAnyX();
        തിരികെ constp->അക്കം().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    സ്ഥിതം std::pair<V3_സംഖ്യ, V3_സംഖ്യ> matchPattern(സ്ഥിരം AstCase* /*casep*/,
                                                      സ്ഥിരം AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_സംഖ്യ, V3_സംഖ്യ> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->അക്കം());
        pairMaskBits.second.opBitsOne(condp->അക്കം());
        തിരികെ pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    സ്ഥിതം Ast_കണ്ണി_Assign* checkDecoderAssign(Ast_കണ്ണി* stmtp) {
        // Only Assign and AssignDly are supported
        എങ്കിൽ (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) തിരികെ nullptr;
        Ast_കണ്ണി_Assign* സ്ഥിരം assp = VN_AS(stmtp, കണ്ണി_Assign);
        // Only if no timing control
        എങ്കിൽ (assp->timingControlp()) തിരികെ nullptr;
        // Only if assigning a constant
        എങ്കിൽ (!VN_IS(assp->rhsp(), Const)) തിരികെ nullptr;
        // Only if it's a packed value
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = assp->rhsp()->dtypep();
        എങ്കിൽ (dtypep->isString() || dtypep->isDouble()) തിരികെ nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        എങ്കിൽ (assp->lhsp()->exists([](AstVarRef* refp) { തിരികെ refp->access().isReadOrRW(); })) {
            തിരികെ nullptr;
        }
        // This is an assignment that fits the decoder pattern
        തിരികെ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    തർക്കം analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        സ്ഥിരം VNUser1InUse user1InUse;
        വേണ്ടി (Ast_കണ്ണി* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_കണ്ണി_Assign* സ്ഥിരം assp = checkDecoderAssign(stmtp);
            എങ്കിൽ (!assp) തിരികെ അസത്യം;
            // Must assign each LHS exactly once - RHS is Const
            സ്ഥിരം തർക്കം multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                തിരികെ refp->varScopep()->user1SetOnce();
            });
            എങ്കിൽ (multipleAssignments) തിരികെ അസത്യം;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            എങ്കിൽ (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        തിരികെ സത്യം;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    സ്ഥിതം സ്ഥിരം AstEnumDType* getEnumCompletionCheckDType(സ്ഥിരം AstCase* സ്ഥിരം nodep) {
        എങ്കിൽ (!nodep->uniquePragma() && !nodep->unique0Pragma()) തിരികെ nullptr;
        സ്ഥിരം AstEnumDType* സ്ഥിരം enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        എങ്കിൽ (!enumDtp) തിരികെ nullptr;  // Case isn't enum
        സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = enumDtp->subDTypep()->basicp();
        എങ്കിൽ (!basicp) തിരികെ nullptr;  // Not simple type (perhaps IEEE illegal)
        തിരികെ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    തർക്കം checkExhaustiveEnum(സ്ഥിരം AstCase* സ്ഥിരം nodep, സ്ഥിരം AstEnumDType* സ്ഥിരം enump) {
        സ്ഥിരം uint32_t അക്കം_Cases = 1UL << nodep->exprp()->width();
        തർക്കം fullyCovered = സത്യം;
        string missing_ഇനങ്ങൾ;
        വേണ്ടി (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            സ്ഥിരം auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            സ്ഥിരം uint32_t mask = match.first.toUInt();
            സ്ഥിരം uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            വേണ്ടി (uint32_t i = 0; i < അക്കം_Cases; ++i) {
                എങ്കിൽ ((i & mask) != bits) തുടരുക;  // This case is not for this enum value
                എങ്കിൽ (m_caseDetails.records[i].itemp) തുടരുക;  // Covered case
                എങ്കിൽ (!missing_ഇനങ്ങൾ.empty()) missing_ഇനങ്ങൾ += ", ";
                missing_ഇനങ്ങൾ += eip->prettyNameQ();
                fullyCovered = അസത്യം;
                നിർത്തുക;
            }
        }
        എങ്കിൽ (!missing_ഇനങ്ങൾ.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_ഇനങ്ങൾ);
        }
        തിരികെ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    തർക്കം checkExhaustivePacked(AstCase* nodep) {
        സ്ഥിരം uint32_t അക്കം_Cases = 1UL << nodep->exprp()->width();
        വേണ്ടി (uint32_t i = 0; i < അക്കം_Cases; ++i) {
            എങ്കിൽ (m_caseDetails.records[i].itemp) തുടരുക;  // Covered case
            എങ്കിൽ (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            തിരികെ അസത്യം;
        }

        // It's an exhaustive case statement
        തിരികെ സത്യം;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    ശൂന്യം analyzeCaseDetails(AstCase* nodep) {
        സ്ഥിരം uint32_t അക്കം_മൂല്യങ്ങൾ = 1UL << nodep->exprp()->width();
        // Clear case records
        വേണ്ടി (uint32_t i = 0; i < അക്കം_മൂല്യങ്ങൾ; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        തർക്കം reportedOverlap = അസത്യം;
        തർക്കം hasDefault = അസത്യം;
        m_caseDetails.noOverlaps = സത്യം;
        വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            എങ്കിൽ (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                വേണ്ടി (uint32_t i = 0; i < അക്കം_മൂല്യങ്ങൾ; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    എങ്കിൽ (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = സത്യം;
                തുടരുക;
            }

            വേണ്ടി (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                എങ്കിൽ (neverItem(nodep, iconstp)) തുടരുക;

                സ്ഥിരം auto& match = matchPattern(nodep, iconstp);
                സ്ഥിരം uint32_t mask = match.first.toUInt();
                സ്ഥിരം uint32_t bits = match.second.toUInt();

                തർക്കം foundNewCase = അസത്യം;
                സ്ഥിരം AstConst* firstOverlapConstp = nullptr;
                uint32_t first_Overlap_മൂല്യം = 0;
                വേണ്ടി (uint32_t i = 0; i < അക്കം_മൂല്യങ്ങൾ; ++i) {
                    എങ്കിൽ ((i & mask) != bits) തുടരുക;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    എങ്കിൽ (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = സത്യം;
                        തുടരുക;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    എങ്കിൽ (caseRecord.itemp == itemp) തുടരുക;

                    // Otherwise record the first overlapping case
                    എങ്കിൽ (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        first_Overlap_മൂല്യം = i;
                        m_caseDetails.noOverlaps = അസത്യം;
                    }
                }

                // Only report first overlap
                എങ്കിൽ (reportedOverlap || !firstOverlapConstp) തുടരുക;

                // Report first overlap
                എങ്കിൽ (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    എങ്കിൽ (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = സത്യം;
                    }
                } അല്ലെങ്കിൽ {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    എങ്കിൽ (iconstp->അക്കം().isAnyXZ()) {
                        examplePattern << " (example pattern 0x" << std::hex << first_Overlap_മൂല്യം
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << examplePattern.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << firstOverlapConstp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << firstOverlapConstp->warnContextSecondary());
                    reportedOverlap = സത്യം;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = അസത്യം;
        m_caseDetails.exhaustive = hasDefault;
        എങ്കിൽ (!hasDefault) {
            എങ്കിൽ (സ്ഥിരം AstEnumDType* സ്ഥിരം enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                സ്ഥിരം തർക്കം exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } അല്ലെങ്കിൽ {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = സത്യം;
    }

    ശൂന്യം analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        വേണ്ടി (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            സ്ഥിരം LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            എങ്കിൽ (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                തുടരുക;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            എങ്കിൽ (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) തിരികെ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            എങ്കിൽ (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                എങ്കിൽ (lhsRecord.nCaseAssigns == m_case_NCase_ഇനങ്ങൾ) തുടരുക;
                എങ്കിൽ (lhsRecord.nCaseAssignDlys == m_case_NCase_ഇനങ്ങൾ) തുടരുക;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_കണ്ണി* സ്ഥിരം preDefaultp = lhsRecord.preDefaultp;
            എങ്കിൽ (!preDefaultp) തിരികെ;
            // And the pre-default needs to be the same type
            എങ്കിൽ (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) തിരികെ;
            എങ്കിൽ (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) തിരികെ;
        }
        // All cases check out, can optimize if there are some entries left
        എങ്കിൽ (m_caseLhsRecords.empty()) തിരികെ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        വേണ്ടി (സ്ഥിരം auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](സ്ഥിരം LhsRecord& a, സ്ഥിരം LhsRecord& b) {
                      // Sort by width, then id
                      സ്ഥിരം പൂർണ്ണസംഖ്യ aWidth = a.lhsp->width();
                      സ്ഥിരം പൂർണ്ണസംഖ്യ bWidth = b.lhsp->width();
                      എങ്കിൽ (aWidth != bWidth) തിരികെ aWidth < bWidth;
                      തിരികെ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        സ്ഥിരം പൂർണ്ണസംഖ്യ caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        സ്ഥിരം auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> തർക്കം {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                എങ്കിൽ (totalWidth > maxWidth) തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        വേണ്ടി (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        എങ്കിൽ (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            തിരികെ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        വേണ്ടി (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            സ്ഥിരം size_t width = lhsRecord.lhsp->width();
            സ്ഥിരം size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            സ്ഥിരം size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            എങ്കിൽ (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_വാക്കുകൾ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        സ്ഥിരം size_t alignedEntryWidth = VL_വാക്കുകൾ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        എങ്കിൽ (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            തിരികെ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    ശൂന്യം analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = അസത്യം;
        m_caseHasDefault = അസത്യം;
        m_case_NCase_ഇനങ്ങൾ = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = അസത്യം;

        Ast_കണ്ണി* സ്ഥിരം caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        എങ്കിൽ (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = സത്യം;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            സ്ഥിരം VNUser1InUse user1InUse;
            വേണ്ടി (Ast_കണ്ണി* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_കണ്ണി_Assign* സ്ഥിരം assp = checkDecoderAssign(prevp);
                എങ്കിൽ (!assp) നിർത്തുക;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                സ്ഥിരം തർക്കം multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    തിരികെ refp->varScopep()->user1SetOnce();
                });
                എങ്കിൽ (multipleAssignments) നിർത്തുക;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        തർക്കം canBeDecoder = സത്യം;
        വേണ്ടി (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            വേണ്ടി (Ast_കണ്ണി* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                എങ്കിൽ (!neverItem(nodep, VN_AS(condp, കണ്ണി_വ്യഞ്ജകം))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                എങ്കിൽ (!VN_IS(condp, Const)) {
                    m_caseOpaque = സത്യം;
                    canBeDecoder = അസത്യം;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            എങ്കിൽ (cip->isDefault()) m_caseHasDefault = സത്യം;
            // Count case items
            ++m_case_NCase_ഇനങ്ങൾ;
            // Check if it fits the decoder pattern, if still possible
            എങ്കിൽ (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        എങ്കിൽ (m_caseOpaque) തിരികെ;

        // If small enough, analyse details
        എങ്കിൽ (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        എങ്കിൽ (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    Ast_കണ്ണി_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, V3SharedTmps& tmps) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();

        // If there is only one LHS, just use the result
        എങ്കിൽ (m_caseDecoderRecords.size() == 1) {
            സ്ഥിരം LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = lhsRecord.lhsp->width();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp
                = exprp->width() == width ? exprp : പുതിയ AstSel{flp, exprp, 0, width};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = lhsRecord.lhsp->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (lhsRecord.nCaseAssigns) {
                തിരികെ പുതിയ AstAssign{flp, lhsp, rhsp};
            } അല്ലെങ്കിൽ എങ്കിൽ (lhsRecord.nCaseAssignDlys) {
                തിരികെ പുതിയ AstAssignDly{flp, lhsp, rhsp};
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* സ്ഥിരം tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tempWritep = പുതിയ AstVarRef{flp, tempVscp, VAccess::WRITE};
        Ast_കണ്ണി_Stmt* സ്ഥിരം resultp = പുതിയ AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        വേണ്ടി (സ്ഥിരം LhsRecord& lhsRecord : m_caseDecoderRecords) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = lhsRecord.lhsp->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = lhsRecord.offset;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tempReadp = പുതിയ AstVarRef{flp, tempVscp, VAccess::READ};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstSel{flp, tempReadp, lsb, width};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = lhsRecord.lhsp->clone_വൃക്ഷം_Pure(അസത്യം);
            എങ്കിൽ (lhsRecord.nCaseAssigns) {
                resultp->addNext(പുതിയ AstAssign{flp, lhsp, rhsp});
            } അല്ലെങ്കിൽ എങ്കിൽ (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(പുതിയ AstAssignDly{flp, lhsp, rhsp});
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        തിരികെ resultp;
    }

    Ast_കണ്ണി_Stmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstConst* സ്ഥിരം tablep
            = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, static_cast<പൂർണ്ണസംഖ്യ>(m_caseTableWidth), 0};
        സ്ഥിരം uint32_t tableEntries = 1U << nodep->exprp()->width();

        സ്ഥിരം തർക്കം isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        എങ്കിൽ (isTinyTable) {
            ++m_stats.caseTableTiny;
        } അല്ലെങ്കിൽ {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        സ്ഥിരം uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_വാക്കുകൾ_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        വേണ്ടി (സ്ഥിരം LhsRecord& lhsRecord : m_caseDecoderRecords) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhsWidth = lhsRecord.lhsp->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            എങ്കിൽ (lhsRecord.preDefaultp) {
                AstConst* സ്ഥിരം rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                വേണ്ടി (uint32_t index = 0; index < tableEntries; ++index) {
                    സ്ഥിരം uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->അക്കം().opSelInto(rhsp->അക്കം(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            വേണ്ടി (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* സ്ഥിരം rhsp = [&]() -> AstConst* {
                    വേണ്ടി (Ast_കണ്ണി* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_കണ്ണി_Assign* സ്ഥിരം ap = VN_AS(stmtp, കണ്ണി_Assign);
                        എങ്കിൽ (lhsRecord.lhsp->same_വൃക്ഷം(ap->lhsp())) തിരികെ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    തിരികെ VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                എങ്കിൽ (cip->isDefault()) {
                    വേണ്ടി (uint32_t index = 0; index < tableEntries; ++index) {
                        സ്ഥിരം uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->അക്കം().opSelInto(rhsp->അക്കം(), tableOffset, lhsWidth);
                    }
                    തുടരുക;
                }

                // Iterate case conditions in reverse order
                വേണ്ടി (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    എങ്കിൽ (neverItem(nodep, condp)) തുടരുക;  // If item never matches, ignore it
                    സ്ഥിരം auto& match = matchPattern(nodep, condp);
                    സ്ഥിരം uint32_t matchMask = match.first.toUInt();
                    സ്ഥിരം uint32_t matchBits = match.second.toUInt();
                    സ്ഥിരം uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    വേണ്ടി (uint32_t i = inverseMask; സത്യം; i = (i - 1) & inverseMask) {
                        സ്ഥിരം uint32_t index = i | matchBits;
                        സ്ഥിരം uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->അക്കം().opSelInto(rhsp->അക്കം(), tableOffset, lhsWidth);
                        എങ്കിൽ (!i) നിർത്തുക;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tableRefp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
            എങ്കിൽ (isTinyTable) തിരികെ tablep;
            AstVarRef* സ്ഥിരം refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->delete_വൃക്ഷം(), tablep);  // V3ConstPool::find clones
            തിരികെ refp;
        }();

        // Create the lookup index
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം caseExprp
            = പുതിയ AstExtend{flp, nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം), 32};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം scalep = പുതിയ AstConst{flp, entryWidth};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tableLsbp = പുതിയ AstMul{flp, scalep, caseExprp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം tableSelp
            = പുതിയ AstSel{flp, tableRefp, tableLsbp, static_cast<പൂർണ്ണസംഖ്യ>(m_caseDecoderEntryWidth)};

        // Connect outputs
        തിരികെ connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    Ast_കണ്ണി_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_വരി* സ്ഥിരം flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, Ast_കണ്ണി*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        വേണ്ടി (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            എങ്കിൽ (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                തുടരുക;
            }
            വേണ്ടി (Ast_കണ്ണി* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* സ്ഥിരം iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                എങ്കിൽ (neverItem(nodep, iconstp)) തുടരുക;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        സ്ഥിരം തർക്കം provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        എങ്കിൽ (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        സ്ഥിരം പൂർണ്ണസംഖ്യ decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        സ്ഥിരം പൂർണ്ണസംഖ്യ condWidth = nodep->exprp()->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ matchWidth = 2 * VL_വാക്കുകൾ_I(condWidth) * VL_EDATASIZE;
        AstConst* സ്ഥിരം matchp
            = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, decoderEnries * matchWidth, 0};
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < decoderEnries; ++i) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            എങ്കിൽ (AstConst* സ്ഥിരം condp = clauses[i].first) {
                സ്ഥിരം auto& match = matchPattern(nodep, condp);
                matchp->അക്കം().opSelInto(match.first, entryLsb, condWidth);
                matchp->അക്കം().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                തുടരുക;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_സംഖ്യ അക്കം_Zero{flp, condWidth, 0};
            matchp->അക്കം().opSelInto(അക്കം_Zero, entryLsb, condWidth);
            matchp->അക്കം().opSelInto(അക്കം_Zero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* സ്ഥിരം rangep = പുതിയ AstRange{flp, decoderEnries - 1, 0};
        Ast_കണ്ണി_DType* സ്ഥിരം entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        Ast_കണ്ണി_DType* സ്ഥിരം tableDtypep = പുതിയ AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* സ്ഥിരം tablep = പുതിയ AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < decoderEnries; ++i) {
            Ast_കണ്ണി* സ്ഥിരം stmtsp = clauses[i].second;
            AstConst* സ്ഥിരം entryp = പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{},
                                                  static_cast<പൂർണ്ണസംഖ്യ>(m_caseDecoderEntryWidth), 0};
            വേണ്ടി (സ്ഥിരം LhsRecord& lhsRecord : m_caseDecoderRecords) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                വേണ്ടി (Ast_കണ്ണി* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_കണ്ണി_Assign* സ്ഥിരം assignp = VN_AS(stmtp, കണ്ണി_Assign);
                    എങ്കിൽ (!lhsp->same_വൃക്ഷം(assignp->lhsp())) തുടരുക;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    നിർത്തുക;
                }
                // Not assigned in this clause, so use the pre-assigned default
                എങ്കിൽ (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->അക്കം().opSelInto(valConstp->അക്കം(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* സ്ഥിരം matchRefp = V3ConstPool::find(matchp);
        AstVarRef* സ്ഥിരം tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->delete_വൃക്ഷം(), matchp);
        VL_DO_DANGLING(tablep->delete_വൃക്ഷം(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം caseExprp = nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        AstMatchMasked* സ്ഥിരം indexp = പുതിയ AstMatchMasked{flp, caseExprp, matchRefp};
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം entryp = പുതിയ AstArraySel{flp, tableRefp, indexp};

        തിരികെ connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_കണ്ണി* convertCaseFastRecurse(Ast_കണ്ണി_വ്യഞ്ജകം* cexprp, പൂർണ്ണസംഖ്യ msb, uint32_t upper_മൂല്യം) സ്ഥിരം {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        എങ്കിൽ (msb < 0) തിരികെ m_caseDetails.records[upper_മൂല്യം].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        സ്ഥിരം uint32_t upperValue0 = upper_മൂല്യം;
        സ്ഥിരം uint32_t upperValue1 = upper_മൂല്യം | (1UL << msb);
        Ast_കണ്ണി* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        Ast_കണ്ണി* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        എങ്കിൽ (tree0p == tree1p) തിരികെ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            തർക്കം same = സത്യം;
            വേണ്ടി (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                എങ്കിൽ (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = അസത്യം;
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (same) {
                VL_DO_DANGLING(tree1p->delete_വൃക്ഷം(), tree1p);
                തിരികെ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        എങ്കിൽ (tree0p && tree0p->backp()) tree0p = tree0p->clone_വൃക്ഷം(സത്യം);
        എങ്കിൽ (tree1p && tree1p->backp()) tree1p = tree1p->clone_വൃക്ഷം(സത്യം);
        // Create the If statement
        File_വരി* സ്ഥിരം flp = cexprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = പുതിയ AstSel{flp, cexprp->clone_വൃക്ഷം_Pure(അസത്യം), msb, 1};
        AstIf* സ്ഥിരം ifp = പുതിയ AstIf{flp, condp, tree1p, tree0p};
        തിരികെ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_കണ്ണി* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        സ്ഥിരം പൂർണ്ണസംഖ്യ caseWidth = nodep->exprp()->width();
        Ast_കണ്ണി* സ്ഥിരം ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        തിരികെ ifrootp && ifrootp->backp() ? ifrootp->clone_വൃക്ഷം(സത്യം) : ifrootp;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* newCaseCompare(സ്ഥിരം AstCase* casep, Ast_കണ്ണി_വ്യഞ്ജകം* itemExprp) {
        File_വരി* സ്ഥിരം flp = itemExprp->fileline();
        എങ്കിൽ (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(itemExprp), itemExprp);
            തിരികെ പുതിയ AstConst{flp, AstConst::BitFalse{}};
        }

        എങ്കിൽ (AstCond* സ്ഥിരം condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം testp = condp->condp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(തള്ളുക_Deletep(itemExprp), itemExprp, condp);
            തിരികെ പുതിയ AstCond{flp, testp, thenp, elsep};
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം caseExprp = casep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        എങ്കിൽ (AstInsideRange* സ്ഥിരം itemRangep = VN_CAST(itemExprp, InsideRange)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(തള്ളുക_Deletep(itemExprp), itemExprp, itemRangep);
            തിരികെ resultp;
        }

        എങ്കിൽ (AstConst* സ്ഥിരം itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            എങ്കിൽ (itemConstp->അക്കം().isFourState()) {
                സ്ഥിരം auto& match = matchPattern(casep, itemConstp);
                സ്ഥിരം V3_സംഖ്യ& matchMask = match.first;
                സ്ഥിരം V3_സംഖ്യ& matchBits = match.second;
                VL_DO_DANGLING2(തള്ളുക_Deletep(itemExprp), itemExprp, itemConstp);
                തിരികെ AstEq::newTyped(flp, പുതിയ AstConst{flp, matchBits},
                                       പുതിയ AstAnd{flp, caseExprp, പുതിയ AstConst{flp, matchMask}});
            }
        }

        തിരികെ AstEq::newTyped(flp, caseExprp, itemExprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_കണ്ണി* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        തർക്കം hasDefault = അസത്യം;
        വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            File_വരി* സ്ഥിരം flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            എങ്കിൽ (itemp->isDefault()) {
                itemp->addCondsp(പുതിയ AstConst{flp, AstConst::BitTrue{}});
                hasDefault = സത്യം;
                തുടരുക;
            }

            // Regular clause. Construct the condition expression.
            Ast_കണ്ണി_വ്യഞ്ജകം* newCondp = nullptr;
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), കണ്ണി_വ്യഞ്ജകം);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                എങ്കിൽ (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(itemExprp), itemExprp);
                    തുടരുക;
                }

                // Compute the term to add to the condition expression
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? പുതിയ AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            എങ്കിൽ (!newCondp) newCondp = പുതിയ AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        എങ്കിൽ (!hasDefault) {
            എങ്കിൽ (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* സ്ഥിരം lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                തള്ളുക_Deletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(പുതിയ AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } അല്ലെങ്കിൽ {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(പുതിയ AstCaseItem{
                    nodep->fileline(), പുതിയ AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        പൂർണ്ണസംഖ്യ depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        വേണ്ടി (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            Ast_കണ്ണി* സ്ഥിരം istmtsp = itemp->stmtsp();
            എങ്കിൽ (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                എങ്കിൽ (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                എങ്കിൽ (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* സ്ഥിരം newp = പുതിയ AstIf{itemp->fileline(), ifexprp->clone_വൃക്ഷം_Pure(സത്യം)};
                    എങ്കിൽ (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } അല്ലെങ്കിൽ {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } അല്ലെങ്കിൽ {  // Continue group, modify if condition to OR in this new condition
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        പുതിയ AstOr{ifexprp->fileline(), condp, ifexprp->clone_വൃക്ഷം_Pure(സത്യം)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_കണ്ണി_വ്യഞ്ജകം* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                എങ്കിൽ (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_വൃക്ഷം(), itemexprp);
                    itemexprp = പുതിയ AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* സ്ഥിരം newp = പുതിയ AstIf{itemp->fileline(), itemexprp, istmtsp};
                എങ്കിൽ (itemnextp) {
                    itemnextp->addElsesp(newp);
                } അല്ലെങ്കിൽ {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        തിരികെ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_കണ്ണി* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        സ്ഥിരം തർക്കം useTable = [&]() {
            // Not if disabled
            എങ്കിൽ (!v3Global.opt.fCaseTable()) തിരികെ അസത്യം;
            // Not if analysis tells us we can't
            എങ്കിൽ (!m_caseTableWidth) തിരികെ അസത്യം;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            എങ്കിൽ (m_caseTableWidth <= CASE_TABLE_TINY_BITS) തിരികെ സത്യം;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            സ്ഥിരം size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            എങ്കിൽ (branches < CASE_TABLE_MIN_BRANCHES) തിരികെ അസത്യം;
            തിരികെ സത്യം;
        }();
        എങ്കിൽ (useTable) തിരികെ convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        സ്ഥിരം തർക്കം useDecoder = [&]() {
            // Not if disabled
            എങ്കിൽ (!v3Global.opt.fCaseDecoder()) തിരികെ അസത്യം;
            // Not if not a decoder pattern
            എങ്കിൽ (m_caseDecoderRecords.empty()) തിരികെ അസത്യം;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            സ്ഥിരം size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            എങ്കിൽ (branches < CASE_TABLE_MIN_BRANCHES) തിരികെ അസത്യം;
            തിരികെ സത്യം;
        }();
        എങ്കിൽ (useDecoder) തിരികെ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        സ്ഥിരം തർക്കം use_Fast_Bit_വൃക്ഷം = [&]() {
            // Not if disabled
            എങ്കിൽ (!v3Global.opt.f_Case_വൃക്ഷം()) തിരികെ അസത്യം;
            // Can't do it without the detailed analysis
            എങ്കിൽ (!m_caseDetailsValid) തിരികെ അസത്യം;
            // Can't do it if not exhaustive
            എങ്കിൽ (!m_caseDetails.exhaustive) തിരികെ അസത്യം;
            // Not worth doing if there are few conditions
            എങ്കിൽ (m_caseNConditions <= 3) തിരികെ അസത്യം;
            // Avoid e.g. priority expanders from going crazy in expansion
            സ്ഥിരം size_t caseWidth = nodep->exprp()->width();
            എങ്കിൽ (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) തിരികെ അസത്യം;
            // Otherwise use the bit tree
            തിരികെ സത്യം;
        }();
        എങ്കിൽ (use_Fast_Bit_വൃക്ഷം) തിരികെ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        എങ്കിൽ (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, സത്യം);
        }
        തിരികെ convertCaseGeneric(nodep);
    }

    // VISITORS
    ശൂന്യം visit(AstCase* nodep) override {
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
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            എങ്കിൽ (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_വൃക്ഷം(), stmtp);
            } അല്ലെങ്കിൽ {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }

    ശൂന്യം visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ശൂന്യം visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildren(nodep); }

പൊതു:
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

ശൂന്യം V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("case", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
ശൂന്യം V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
