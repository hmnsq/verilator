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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3Case.h"

#అన్తర్భావ "V3ConstPool.h"
#అన్తర్భావ "V3SharedTmps.h"
#అన్తర్భావ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

తరగతి CaseLintVisitor final : బహిరంగ VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    స్థిరరాశి Ast_కణుపు* m_casep = శూన్యసూచిక;

    // METHODS
    మూస <రకం_పేరు Case_అంశం>
    స్థిర శూన్య detect_గుణిజం_Defaults(Case_అంశం* itemsp) {
        తార్కిక తగిలింది_Default = అబద్ధం;
        కొరకు (Case_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<Case_అంశం>(itemp->nextp())) {
            ఒకవేళ (!itemp->isDefault()) కొనసాగు;
            ఒకవేళ (తగిలింది_Default) itemp->v3error("Multiple default statements in case statement.");
            తగిలింది_Default = నిజం;
        }
    }

    మూస <రకం_పేరు Case_అంశం>
    శూన్య checkXZinNonCaseX(Ast_కణుపు* casep, Ast_కణుపు_సమాసం* exprp, Case_అంశం* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        కొరకు (Case_అంశం* itemp = itemsp; itemp; itemp = Ast_కణుపు::as<Case_అంశం>(itemp->nextp())) {
            iterate_And_తదుపరి_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    శూన్య సందర్శించు(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_గుణిజం_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    శూన్య సందర్శించు(AstCase* nodep) override {
        ఒకవేళ (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_గుణిజం_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        ఒకవేళ (!nodep->అంకె().isFourState()) తిరిగి;

        // Error if generate case
        ఒకవేళ (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            తిరిగి;
        }

        // Otherwise must be a case statement
        స్థిరరాశి AstCase* స్థిరరాశి casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        ఒకవేళ (casep->casex()) తిరిగి;

        ఒకవేళ (casep->casez() || casep->caseInside()) {
            ఒకవేళ (nodep->అంకె().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            తిరిగి;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    స్పష్టమైన CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    స్పష్టమైన CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = అప్రమేయ;

బహిరంగ:
    స్థిర శూన్య apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    స్థిర శూన్య apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

తరగతి CaseVisitor final : బహిరంగ VNVisitor {
    // Maximum width for detailed analysis
    constexpr స్థిర పూర్ణాంకం CASE_DETAILS_MAX_వెడల్పు = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr స్థిర పూర్ణాంకం CASE_ENCODER_సమూహం_లోతు = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr స్థిర కొలత_t CASE_పట్టిక_TINY_బిట్లు = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr స్థిర కొలత_t CASE_పట్టిక_MAX_బిట్లు = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr స్థిర కొలత_t CASE_పట్టిక_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    నిర్మాణం Case_రికార్డు final {
        Ast_Case_అంశం* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_కణుపు* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    నిర్మాణం Lhs_రికార్డు final {
        Ast_కణుపు_సమాసం* lhsp = శూన్యసూచిక;  // LHS of the assignment
        Ast_కణుపు_కేటాయించు* preDefaultp = శూన్యసూచిక;  // Default assignment *before the case statement*
        కొలత_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        కొలత_t n_Case_కేటాయించు_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        కొలత_t స్థానభ్రంశం = 0;  // Offset in the table for this LHS

        స్థిర కొలత_t s_తదుపరి_Id;  // Static unique Id counter
        కొలత_t id = ++s_తదుపరి_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    నిర్మాణం Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_పట్టిక_సాధారణ;  // Cases using table method with normal table
        VDouble0 case_పట్టిక_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    స్థిరరాశి Ast_కణుపు* m_alwaysp = శూన్యసూచిక;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_పట్టిక_బహిర్గతం_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_బహిర్గతం_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = శూన్యసూచిక;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    తార్కిక m_caseOpaque = అబద్ధం;  // Case statement is opaque (non-packed, or non-const conditions)
    తార్కిక m_caseHasDefault = అబద్ధం;  // Indicates the case statement has a default case
    కొలత_t m_case_NCase_వస్తువులు = 0;  // Number of AstCaseItems in the case statement
    కొలత_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_కణుపు_సమాసం>, Lhs_రికార్డు> m_case_Lhs_రికార్డులు;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::సదిశ<Lhs_రికార్డు> m_case_Decoder_రికార్డులు;
    కొలత_t m_case_Decoder_నమోదు_వెడల్పు = 0;  // Width of each entry in the decoder table
    కొలత_t m_case_పట్టిక_వెడల్పు = 0;  // Total width of the case table - 0 means can't optimize
    తార్కిక m_caseDetailsValid = అబద్ధం;  // Indicates m_caseDetails is valid
    నిర్మాణం final {
        తార్కిక exhaustive = అబద్ధం;  // Proven exhaustive
        తార్కిక exhaustiveOverEnumOnly = అబద్ధం;  // Exhaustive over enum values only
        తార్కిక noOverlaps = అబద్ధం;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_రికార్డు, 1U << CASE_DETAILS_MAX_వెడల్పు> రికార్డులు;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    స్థిర తార్కిక never_అంశం(స్థిరరాశి AstCase* casep, స్థిరరాశి Ast_కణుపు_సమాసం* అంశం_Exprp) {
        స్థిరరాశి AstConst* స్థిరరాశి constp = VN_CAST(అంశం_Exprp, Const);
        ఒకవేళ (!constp) తిరిగి అబద్ధం;
        ఒకవేళ (casep->casex() || casep->caseInside()) తిరిగి అబద్ధం;
        ఒకవేళ (casep->casez()) తిరిగి constp->అంకె().isAnyX();
        తిరిగి constp->అంకె().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    స్థిర std::pair<V3_సంఖ్య, V3_సంఖ్య> సరిపోలిక_నమూనా(స్థిరరాశి AstCase* /*casep*/,
                                                      స్థిరరాశి AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_సంఖ్య, V3_సంఖ్య> pair_ముసుగు_బిట్లు{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->వెడల్పు(), 0),
            std::forward_as_tuple(condp->fileline(), condp->వెడల్పు(), 0)};
        pair_ముసుగు_బిట్లు.మొదటి.op_బిట్లు_Non_XZ(condp->అంకె());
        pair_ముసుగు_బిట్లు.రెండవ.op_బిట్లు_One(condp->అంకె());
        తిరిగి pair_ముసుగు_బిట్లు;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    స్థిర Ast_కణుపు_కేటాయించు* check_Decoder_కేటాయించు(Ast_కణుపు* stmtp) {
        // Only Assign and AssignDly are supported
        ఒకవేళ (!VN_IS(stmtp, కేటాయించు) && !VN_IS(stmtp, కేటాయించు_Dly)) తిరిగి శూన్యసూచిక;
        Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = VN_AS(stmtp, కణుపు_కేటాయించు);
        // Only if no timing control
        ఒకవేళ (assp->timingControlp()) తిరిగి శూన్యసూచిక;
        // Only if assigning a constant
        ఒకవేళ (!VN_IS(assp->rhsp(), Const)) తిరిగి శూన్యసూచిక;
        // Only if it's a packed value
        Ast_కణుపు_DType* స్థిరరాశి dtypep = assp->rhsp()->dtypep();
        ఒకవేళ (dtypep->isString() || dtypep->isDouble()) తిరిగి శూన్యసూచిక;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        ఒకవేళ (assp->lhsp()->exists([](AstVarRef* refp) { తిరిగి refp->access().isReadOrRW(); })) {
            తిరిగి శూన్యసూచిక;
        }
        // This is an assignment that fits the decoder pattern
        తిరిగి assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    తార్కిక analyze_Decoder_Case_అంశం(Ast_Case_అంశం* cip) {
        // AstVarScope::user1() -> bool: true if written to
        స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
        కొరకు (Ast_కణుపు* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = check_Decoder_కేటాయించు(stmtp);
            ఒకవేళ (!assp) తిరిగి అబద్ధం;
            // Must assign each LHS exactly once - RHS is Const
            స్థిరరాశి తార్కిక గుణిజం_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                తిరిగి refp->varScopep()->user1_సమితి_Once();
            });
            ఒకవేళ (గుణిజం_Assignments) తిరిగి అబద్ధం;
            // Update LhsRecord
            Lhs_రికార్డు& lhs_రికార్డు = m_case_Lhs_రికార్డులు[*assp->lhsp()];
            ఒకవేళ (!lhs_రికార్డు.lhsp) lhs_రికార్డు.lhsp = assp->lhsp();
            lhs_రికార్డు.nCaseAssigns += VN_IS(assp, కేటాయించు);
            lhs_రికార్డు.n_Case_కేటాయించు_Dlys += VN_IS(assp, కేటాయించు_Dly);
        }
        తిరిగి నిజం;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    స్థిర స్థిరరాశి AstEnumDType* getEnumCompletionCheckDType(స్థిరరాశి AstCase* స్థిరరాశి nodep) {
        ఒకవేళ (!nodep->uniquePragma() && !nodep->unique0Pragma()) తిరిగి శూన్యసూచిక;
        స్థిరరాశి AstEnumDType* స్థిరరాశి enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_గమ్యం_Enump(), EnumDType);
        ఒకవేళ (!enumDtp) తిరిగి శూన్యసూచిక;  // Case isn't enum
        స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = enumDtp->subDTypep()->basicp();
        ఒకవేళ (!basicp) తిరిగి శూన్యసూచిక;  // Not simple type (perhaps IEEE illegal)
        తిరిగి enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    తార్కిక checkExhaustiveEnum(స్థిరరాశి AstCase* స్థిరరాశి nodep, స్థిరరాశి AstEnumDType* స్థిరరాశి enump) {
        స్థిరరాశి uint32_t అంకె_Cases = 1UL << nodep->exprp()->వెడల్పు();
        తార్కిక fullyCovered = నిజం;
        string missing_వస్తువులు;
        కొరకు (Ast_Enum_అంశం* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_అంశం)) {
            స్థిరరాశి స్వయంచాలక& సరిపోలిక = సరిపోలిక_నమూనా(nodep, VN_AS(eip->valuep(), Const));
            స్థిరరాశి uint32_t ముసుగు = సరిపోలిక.మొదటి.గమ్యం_UInt();
            స్థిరరాశి uint32_t బిట్లు = సరిపోలిక.రెండవ.గమ్యం_UInt();

            // Check all cases to see if they cover this enum value/pattern
            కొరకు (uint32_t i = 0; i < అంకె_Cases; ++i) {
                ఒకవేళ ((i & ముసుగు) != బిట్లు) కొనసాగు;  // This case is not for this enum value
                ఒకవేళ (m_caseDetails.రికార్డులు[i].itemp) కొనసాగు;  // Covered case
                ఒకవేళ (!missing_వస్తువులు.ఖాళీలు()) missing_వస్తువులు += ", ";
                missing_వస్తువులు += eip->prettyNameQ();
                fullyCovered = అబద్ధం;
                ఆపు;
            }
        }
        ఒకవేళ (!missing_వస్తువులు.ఖాళీలు() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_వస్తువులు);
        }
        తిరిగి fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    తార్కిక checkExhaustivePacked(AstCase* nodep) {
        స్థిరరాశి uint32_t అంకె_Cases = 1UL << nodep->exprp()->వెడల్పు();
        కొరకు (uint32_t i = 0; i < అంకె_Cases; ++i) {
            ఒకవేళ (m_caseDetails.రికార్డులు[i].itemp) కొనసాగు;  // Covered case
            ఒకవేళ (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            తిరిగి అబద్ధం;
        }

        // It's an exhaustive case statement
        తిరిగి నిజం;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    శూన్య analyzeCaseDetails(AstCase* nodep) {
        స్థిరరాశి uint32_t అంకె_విలువలు = 1UL << nodep->exprp()->వెడల్పు();
        // Clear case records
        కొరకు (uint32_t i = 0; i < అంకె_విలువలు; ++i) {
            m_caseDetails.రికార్డులు[i].itemp = శూన్యసూచిక;
            m_caseDetails.రికార్డులు[i].constp = శూన్యసూచిక;
            m_caseDetails.రికార్డులు[i].stmtsp = శూన్యసూచిక;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        తార్కిక reportedOverlap = అబద్ధం;
        తార్కిక hasDefault = అబద్ధం;
        m_caseDetails.noOverlaps = నిజం;
        కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {

            // Default case
            ఒకవేళ (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                కొరకు (uint32_t i = 0; i < అంకె_విలువలు; ++i) {
                    Case_రికార్డు& case_రికార్డు = m_caseDetails.రికార్డులు[i];
                    ఒకవేళ (!case_రికార్డు.itemp) {
                        case_రికార్డు.itemp = itemp;
                        case_రికార్డు.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = నిజం;
                కొనసాగు;
            }

            కొరకు (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                ఒకవేళ (never_అంశం(nodep, iconstp)) కొనసాగు;

                స్థిరరాశి స్వయంచాలక& సరిపోలిక = సరిపోలిక_నమూనా(nodep, iconstp);
                స్థిరరాశి uint32_t ముసుగు = సరిపోలిక.మొదటి.గమ్యం_UInt();
                స్థిరరాశి uint32_t బిట్లు = సరిపోలిక.రెండవ.గమ్యం_UInt();

                తార్కిక దొరికింది_New_Case = అబద్ధం;
                స్థిరరాశి AstConst* మొదటి_Overlap_Constp = శూన్యసూచిక;
                uint32_t మొదటి_Overlap_విలువ = 0;
                కొరకు (uint32_t i = 0; i < అంకె_విలువలు; ++i) {
                    ఒకవేళ ((i & ముసుగు) != బిట్లు) కొనసాగు;

                    Case_రికార్డు& case_రికార్డు = m_caseDetails.రికార్డులు[i];

                    // If this is the first case that covers this value, record it
                    ఒకవేళ (!case_రికార్డు.itemp) {
                        case_రికార్డు.itemp = itemp;
                        case_రికార్డు.constp = iconstp;
                        case_రికార్డు.stmtsp = itemp->stmtsp();
                        దొరికింది_New_Case = నిజం;
                        కొనసాగు;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    ఒకవేళ (case_రికార్డు.itemp == itemp) కొనసాగు;

                    // Otherwise record the first overlapping case
                    ఒకవేళ (!మొదటి_Overlap_Constp) {
                        మొదటి_Overlap_Constp = case_రికార్డు.constp;
                        మొదటి_Overlap_విలువ = i;
                        m_caseDetails.noOverlaps = అబద్ధం;
                    }
                }

                // Only report first overlap
                ఒకవేళ (reportedOverlap || !మొదటి_Overlap_Constp) కొనసాగు;

                // Report first overlap
                ఒకవేళ (nodep->ప్రాధాన్యత_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    ఒకవేళ (!దొరికింది_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << మొదటి_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << మొదటి_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = నిజం;
                    }
                } లేకపోతే {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_నమూనా;
                    ఒకవేళ (iconstp->అంకె().isAnyXZ()) {
                        example_నమూనా << " (example pattern 0x" << std::hex << మొదటి_Overlap_విలువ
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_నమూనా.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << మొదటి_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << మొదటి_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = నిజం;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = అబద్ధం;
        m_caseDetails.exhaustive = hasDefault;
        ఒకవేళ (!hasDefault) {
            ఒకవేళ (స్థిరరాశి AstEnumDType* స్థిరరాశి enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                స్థిరరాశి తార్కిక exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } లేకపోతే {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = నిజం;
    }

    శూన్య analyze_Decoder_నమూనా(AstCase* nodep) {
        // Check each LHS record
        కొరకు (స్వయంచాలక it = m_case_Lhs_రికార్డులు.cbegin(); it != m_case_Lhs_రికార్డులు.cend();) {
            స్థిరరాశి Lhs_రికార్డు& lhs_రికార్డు = it->రెండవ;

            // Delete records that have no assignments in any case item (only pre-defaults)
            ఒకవేళ (!lhs_రికార్డు.nCaseAssigns && !lhs_రికార్డు.n_Case_కేటాయించు_Dlys) {
                it = m_case_Lhs_రికార్డులు.erase(it);
                కొనసాగు;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            ఒకవేళ (lhs_రికార్డు.nCaseAssigns && lhs_రికార్డు.n_Case_కేటాయించు_Dlys) తిరిగి;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            ఒకవేళ (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                ఒకవేళ (lhs_రికార్డు.nCaseAssigns == m_case_NCase_వస్తువులు) కొనసాగు;
                ఒకవేళ (lhs_రికార్డు.n_Case_కేటాయించు_Dlys == m_case_NCase_వస్తువులు) కొనసాగు;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_కణుపు* స్థిరరాశి preDefaultp = lhs_రికార్డు.preDefaultp;
            ఒకవేళ (!preDefaultp) తిరిగి;
            // And the pre-default needs to be the same type
            ఒకవేళ (lhs_రికార్డు.nCaseAssigns && !VN_IS(preDefaultp, కేటాయించు)) తిరిగి;
            ఒకవేళ (lhs_రికార్డు.n_Case_కేటాయించు_Dlys && !VN_IS(preDefaultp, కేటాయించు_Dly)) తిరిగి;
        }
        // All cases check out, can optimize if there are some entries left
        ఒకవేళ (m_case_Lhs_రికార్డులు.ఖాళీలు()) తిరిగి;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_రికార్డులు.reserve(m_case_Lhs_రికార్డులు.కొలత());
        కొరకు (స్థిరరాశి స్వయంచాలక& అంశం_2 : m_case_Lhs_రికార్డులు) m_case_Decoder_రికార్డులు.emplace_వెనుక(అంశం_2.రెండవ);
        std::క్రమీకరించు(m_case_Decoder_రికార్డులు.begin(), m_case_Decoder_రికార్డులు.చివర(),
                  [](స్థిరరాశి Lhs_రికార్డు& a, స్థిరరాశి Lhs_రికార్డు& b) {
                      // Sort by width, then id
                      స్థిరరాశి పూర్ణాంకం a_వెడల్పు = a.lhsp->వెడల్పు();
                      స్థిరరాశి పూర్ణాంకం b_వెడల్పు = b.lhsp->వెడల్పు();
                      ఒకవేళ (a_వెడల్పు != b_వెడల్పు) తిరిగి a_వెడల్పు < b_వెడల్పు;
                      తిరిగి a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        స్థిరరాశి పూర్ణాంకం case_వెడల్పు = nodep->exprp()->వెడల్పు();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        స్థిరరాశి స్వయంచాలక సరిపోతుంది_హద్దు = [&](కొలత_t నమోదు_వెడల్పు, కొలత_t max_వెడల్పు) -> తార్కిక {
            కొలత_t మొత్తం_వెడల్పు = నమోదు_వెడల్పు;
            // Multiply cases - iterative to avoid overflow
            కొరకు (పూర్ణాంకం i = 0; i < case_వెడల్పు; ++i) {
                మొత్తం_వెడల్పు <<= 1;
                ఒకవేళ (మొత్తం_వెడల్పు > max_వెడల్పు) తిరిగి అబద్ధం;
            }
            తిరిగి నిజం;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_నమోదు_వెడల్పు = 0;
        కొరకు (Lhs_రికార్డు& lhs_రికార్డు : m_case_Decoder_రికార్డులు) {
            lhs_రికార్డు.స్థానభ్రంశం = m_case_Decoder_నమోదు_వెడల్పు;
            m_case_Decoder_నమోదు_వెడల్పు += lhs_రికార్డు.lhsp->వెడల్పు();
        }
        // If it fits, we will pack it tightly
        ఒకవేళ (సరిపోతుంది_హద్దు(m_case_Decoder_నమోదు_వెడల్పు, CASE_పట్టిక_TINY_బిట్లు)) {
            m_case_పట్టిక_వెడల్పు = m_case_Decoder_నమోదు_వెడల్పు << case_వెడల్పు;  // Can optimize
            తిరిగి;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_నమోదు_వెడల్పు = 0;
        కొరకు (Lhs_రికార్డు& lhs_రికార్డు : m_case_Decoder_రికార్డులు) {
            స్థిరరాశి కొలత_t వెడల్పు = lhs_రికార్డు.lhsp->వెడల్పు();
            స్థిరరాశి కొలత_t lsb_మాట = VL_BITWORD_E(m_case_Decoder_నమోదు_వెడల్పు);
            స్థిరరాశి కొలత_t msb_మాట = VL_BITWORD_E(m_case_Decoder_నమోదు_వెడల్పు + వెడల్పు - 1);
            ఒకవేళ (lsb_మాట != msb_మాట) {
                m_case_Decoder_నమోదు_వెడల్పు = VL_పదాలు_I(m_case_Decoder_నమోదు_వెడల్పు) * VL_EDATASIZE;
            }
            lhs_రికార్డు.స్థానభ్రంశం = m_case_Decoder_నమోదు_వెడల్పు;
            m_case_Decoder_నమోదు_వెడల్పు += వెడల్పు;
        }
        // Check the table fits max size
        స్థిరరాశి కొలత_t aligned_నమోదు_వెడల్పు = VL_పదాలు_I(m_case_Decoder_నమోదు_వెడల్పు) * VL_EDATASIZE;
        ఒకవేళ (సరిపోతుంది_హద్దు(aligned_నమోదు_వెడల్పు, CASE_పట్టిక_MAX_బిట్లు)) {
            m_case_పట్టిక_వెడల్పు = aligned_నమోదు_వెడల్పు << case_వెడల్పు;  // Can optimize
            తిరిగి;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    శూన్య analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = అబద్ధం;
        m_caseHasDefault = అబద్ధం;
        m_case_NCase_వస్తువులు = 0;
        m_caseNConditions = 0;
        m_case_Decoder_రికార్డులు.clear();
        m_case_Decoder_నమోదు_వెడల్పు = 0;
        m_case_పట్టిక_వెడల్పు = 0;
        m_case_Lhs_రికార్డులు.clear();
        m_caseDetailsValid = అబద్ధం;

        Ast_కణుపు* స్థిరరాశి caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        ఒకవేళ (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = నిజం;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            స్థిరరాశి VNUser1_లోపల_Use user1_లోపల_Use;
            కొరకు (Ast_కణుపు* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_కణుపు_కేటాయించు* స్థిరరాశి assp = check_Decoder_కేటాయించు(prevp);
                ఒకవేళ (!assp) ఆపు;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                స్థిరరాశి తార్కిక గుణిజం_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    తిరిగి refp->varScopep()->user1_సమితి_Once();
                });
                ఒకవేళ (గుణిజం_Assignments) ఆపు;
                // Store pre-default assignment
                Lhs_రికార్డు& lhs_రికార్డు = m_case_Lhs_రికార్డులు[*assp->lhsp()];
                lhs_రికార్డు.lhsp = assp->lhsp();
                lhs_రికార్డు.preDefaultp = assp;
            }
        }

        // Check each case item
        తార్కిక canBeDecoder = నిజం;
        కొరకు (Ast_Case_అంశం* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_అంశం)) {
            // Check conditions
            కొరకు (Ast_కణుపు* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                ఒకవేళ (!never_అంశం(nodep, VN_AS(condp, కణుపు_సమాసం))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                ఒకవేళ (!VN_IS(condp, Const)) {
                    m_caseOpaque = నిజం;
                    canBeDecoder = అబద్ధం;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            ఒకవేళ (cip->isDefault()) m_caseHasDefault = నిజం;
            // Count case items
            ++m_case_NCase_వస్తువులు;
            // Check if it fits the decoder pattern, if still possible
            ఒకవేళ (canBeDecoder) canBeDecoder = analyze_Decoder_Case_అంశం(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        ఒకవేళ (m_caseOpaque) తిరిగి;

        // If small enough, analyse details
        ఒకవేళ (caseExprp->వెడల్పు() <= CASE_DETAILS_MAX_వెడల్పు) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        ఒకవేళ (canBeDecoder) analyze_Decoder_నమూనా(nodep);
    }

    Ast_కణుపు_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_కణుపు_సమాసం* exprp, V3SharedTmps& tmps) {
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        // If there is only one LHS, just use the result
        ఒకవేళ (m_case_Decoder_రికార్డులు.కొలత() == 1) {
            స్థిరరాశి Lhs_రికార్డు& lhs_రికార్డు = m_case_Decoder_రికార్డులు[0];
            స్థిరరాశి పూర్ణాంకం వెడల్పు = lhs_రికార్డు.lhsp->వెడల్పు();
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp
                = exprp->వెడల్పు() == వెడల్పు ? exprp : కొత్త AstSel{flp, exprp, 0, వెడల్పు};
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = lhs_రికార్డు.lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (lhs_రికార్డు.nCaseAssigns) {
                తిరిగి కొత్త Ast_కేటాయించు{flp, lhsp, rhsp};
            } లేకపోతే ఒకవేళ (lhs_రికార్డు.n_Case_కేటాయించు_Dlys) {
                తిరిగి కొత్త Ast_కేటాయించు_Dly{flp, lhsp, rhsp};
            } లేకపోతే {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* స్థిరరాశి తాత్కాలిక_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_నమోదు_వెడల్పు);
        Ast_కణుపు_సమాసం* స్థిరరాశి తాత్కాలిక_Writep = కొత్త AstVarRef{flp, తాత్కాలిక_Vscp, VAccess::WRITE};
        Ast_కణుపు_Stmt* స్థిరరాశి resultp = కొత్త Ast_కేటాయించు{flp, తాత్కాలిక_Writep, exprp};

        // For each LHS, select out the result
        కొరకు (స్థిరరాశి Lhs_రికార్డు& lhs_రికార్డు : m_case_Decoder_రికార్డులు) {
            స్థిరరాశి పూర్ణాంకం వెడల్పు = lhs_రికార్డు.lhsp->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం lsb = lhs_రికార్డు.స్థానభ్రంశం;
            Ast_కణుపు_సమాసం* స్థిరరాశి తాత్కాలిక_Readp = కొత్త AstVarRef{flp, తాత్కాలిక_Vscp, VAccess::READ};
            Ast_కణుపు_సమాసం* స్థిరరాశి rhsp = కొత్త AstSel{flp, తాత్కాలిక_Readp, lsb, వెడల్పు};
            Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = lhs_రికార్డు.lhsp->clone_వృక్షం_శుద్ధ(అబద్ధం);
            ఒకవేళ (lhs_రికార్డు.nCaseAssigns) {
                resultp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు{flp, lhsp, rhsp});
            } లేకపోతే ఒకవేళ (lhs_రికార్డు.n_Case_కేటాయించు_Dlys) {
                resultp->కూడు_తదుపరి(కొత్త Ast_కేటాయించు_Dly{flp, lhsp, rhsp});
            } లేకపోతే {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        తిరిగి resultp;
    }

    Ast_కణుపు_Stmt* convert_Case_పట్టిక(AstCase* nodep) {
        // Create the table constant
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        AstConst* స్థిరరాశి tablep
            = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, static_cast<పూర్ణాంకం>(m_case_పట్టిక_వెడల్పు), 0};
        స్థిరరాశి uint32_t పట్టిక_Entries = 1U << nodep->exprp()->వెడల్పు();

        స్థిరరాశి తార్కిక is_Tiny_పట్టిక = m_case_పట్టిక_వెడల్పు <= CASE_పట్టిక_TINY_బిట్లు;
        ఒకవేళ (is_Tiny_పట్టిక) {
            ++m_stats.case_పట్టిక_Tiny;
        } లేకపోతే {
            ++m_stats.case_పట్టిక_సాధారణ;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        స్థిరరాశి uint32_t నమోదు_వెడల్పు = is_Tiny_పట్టిక
                                        ? m_case_Decoder_నమోదు_వెడల్పు
                                        : VL_పదాలు_I(m_case_Decoder_నమోదు_వెడల్పు) * VL_EDATASIZE;
        కొరకు (స్థిరరాశి Lhs_రికార్డు& lhs_రికార్డు : m_case_Decoder_రికార్డులు) {
            స్థిరరాశి పూర్ణాంకం lhs_వెడల్పు = lhs_రికార్డు.lhsp->వెడల్పు();
            స్థిరరాశి పూర్ణాంకం lhs_స్థానభ్రంశం = lhs_రికార్డు.స్థానభ్రంశం;

            // Broadcast the pre-default assignment
            ఒకవేళ (lhs_రికార్డు.preDefaultp) {
                AstConst* స్థిరరాశి rhsp = VN_AS(lhs_రికార్డు.preDefaultp->rhsp(), Const);
                కొరకు (uint32_t సూచీ = 0; సూచీ < పట్టిక_Entries; ++సూచీ) {
                    స్థిరరాశి uint32_t పట్టిక_స్థానభ్రంశం = సూచీ * నమోదు_వెడల్పు + lhs_స్థానభ్రంశం;
                    tablep->అంకె().opSelInto(rhsp->అంకె(), పట్టిక_స్థానభ్రంశం, lhs_వెడల్పు);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            కొరకు (Ast_Case_అంశం* cip = VN_AS(nodep->itemsp()->lastp(), Case_అంశం); cip;
                 cip = VN_AS(cip->prevp(), Case_అంశం)) {
                // Find the RHS in this case
                AstConst* స్థిరరాశి rhsp = [&]() -> AstConst* {
                    కొరకు (Ast_కణుపు* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_కణుపు_కేటాయించు* స్థిరరాశి ap = VN_AS(stmtp, కణుపు_కేటాయించు);
                        ఒకవేళ (lhs_రికార్డు.lhsp->same_వృక్షం(ap->lhsp())) తిరిగి VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    తిరిగి VN_AS(lhs_రికార్డు.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                ఒకవేళ (cip->isDefault()) {
                    కొరకు (uint32_t సూచీ = 0; సూచీ < పట్టిక_Entries; ++సూచీ) {
                        స్థిరరాశి uint32_t పట్టిక_స్థానభ్రంశం = సూచీ * నమోదు_వెడల్పు + lhs_స్థానభ్రంశం;
                        tablep->అంకె().opSelInto(rhsp->అంకె(), పట్టిక_స్థానభ్రంశం, lhs_వెడల్పు);
                    }
                    కొనసాగు;
                }

                // Iterate case conditions in reverse order
                కొరకు (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    ఒకవేళ (never_అంశం(nodep, condp)) కొనసాగు;  // If item never matches, ignore it
                    స్థిరరాశి స్వయంచాలక& సరిపోలిక = సరిపోలిక_నమూనా(nodep, condp);
                    స్థిరరాశి uint32_t సరిపోలిక_ముసుగు = సరిపోలిక.మొదటి.గమ్యం_UInt();
                    స్థిరరాశి uint32_t సరిపోలిక_బిట్లు = సరిపోలిక.రెండవ.గమ్యం_UInt();
                    స్థిరరాశి uint32_t inverse_ముసుగు = ~సరిపోలిక_ముసుగు & ((1U << condp->వెడల్పు()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    కొరకు (uint32_t i = inverse_ముసుగు; నిజం; i = (i - 1) & inverse_ముసుగు) {
                        స్థిరరాశి uint32_t సూచీ = i | సరిపోలిక_బిట్లు;
                        స్థిరరాశి uint32_t పట్టిక_స్థానభ్రంశం = సూచీ * నమోదు_వెడల్పు + lhs_స్థానభ్రంశం;
                        tablep->అంకె().opSelInto(rhsp->అంకె(), పట్టిక_స్థానభ్రంశం, lhs_వెడల్పు);
                        ఒకవేళ (!i) ఆపు;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_కణుపు_సమాసం* స్థిరరాశి పట్టిక_Refp = [&]() -> Ast_కణుపు_సమాసం* {
            ఒకవేళ (is_Tiny_పట్టిక) తిరిగి tablep;
            AstVarRef* స్థిరరాశి refp = V3ConstPool::కనుగొను(tablep);
            VL_DO_DANGLING(tablep->delete_వృక్షం(), tablep);  // V3ConstPool::find clones
            తిరిగి refp;
        }();

        // Create the lookup index
        Ast_కణుపు_సమాసం* స్థిరరాశి caseExprp
            = కొత్త AstExtend{flp, nodep->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం), 32};
        Ast_కణుపు_సమాసం* స్థిరరాశి scalep = కొత్త AstConst{flp, నమోదు_వెడల్పు};
        Ast_కణుపు_సమాసం* స్థిరరాశి పట్టిక_Lsbp = కొత్త AstMul{flp, scalep, caseExprp};
        Ast_కణుపు_సమాసం* స్థిరరాశి పట్టిక_Selp
            = కొత్త AstSel{flp, పట్టిక_Refp, పట్టిక_Lsbp, static_cast<పూర్ణాంకం>(m_case_Decoder_నమోదు_వెడల్పు)};

        // Connect outputs
        తిరిగి connectDecoderOutputs(nodep, పట్టిక_Selp, m_పట్టిక_బహిర్గతం_Tmps);
    }

    Ast_కణుపు_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::సదిశ<std::pair<AstConst*, Ast_కణుపు*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        కొరకు (Ast_Case_అంశం* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_అంశం)) {
            ఒకవేళ (cip->isDefault()) {
                clauses.emplace_వెనుక(శూన్యసూచిక, cip->stmtsp());
                కొనసాగు;
            }
            కొరకు (Ast_కణుపు* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* స్థిరరాశి iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                ఒకవేళ (never_అంశం(nodep, iconstp)) కొనసాగు;
                clauses.emplace_వెనుక(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        స్థిరరాశి తార్కిక provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        ఒకవేళ (clauses.వెనుక().మొదటి && !provenExhaustive) clauses.emplace_వెనుక(శూన్యసూచిక, శూన్యసూచిక);

        // Number of entries in decoder table
        స్థిరరాశి పూర్ణాంకం decoderEnries = clauses.కొలత();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        స్థిరరాశి పూర్ణాంకం cond_వెడల్పు = nodep->exprp()->వెడల్పు();
        స్థిరరాశి పూర్ణాంకం సరిపోలిక_వెడల్పు = 2 * VL_పదాలు_I(cond_వెడల్పు) * VL_EDATASIZE;
        AstConst* స్థిరరాశి matchp
            = కొత్త AstConst{flp, AstConst::Widthed_విలువ{}, decoderEnries * సరిపోలిక_వెడల్పు, 0};
        కొరకు (పూర్ణాంకం i = 0; i < decoderEnries; ++i) {
            స్థిరరాశి పూర్ణాంకం నమోదు_Lsb = i * సరిపోలిక_వెడల్పు;

            // If the entry has a condition, use it's match bits and mask
            ఒకవేళ (AstConst* స్థిరరాశి condp = clauses[i].మొదటి) {
                స్థిరరాశి స్వయంచాలక& సరిపోలిక = సరిపోలిక_నమూనా(nodep, condp);
                matchp->అంకె().opSelInto(సరిపోలిక.మొదటి, నమోదు_Lsb, cond_వెడల్పు);
                matchp->అంకె().opSelInto(సరిపోలిక.రెండవ, నమోదు_Lsb + సరిపోలిక_వెడల్పు / 2, cond_వెడల్పు);
                కొనసాగు;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_సంఖ్య అంకె_Zero{flp, cond_వెడల్పు, 0};
            matchp->అంకె().opSelInto(అంకె_Zero, నమోదు_Lsb, cond_వెడల్పు);
            matchp->అంకె().opSelInto(అంకె_Zero, నమోదు_Lsb + సరిపోలిక_వెడల్పు / 2, cond_వెడల్పు);
        }

        // Create the table initializer
        AstRange* స్థిరరాశి rangep = కొత్త AstRange{flp, decoderEnries - 1, 0};
        Ast_కణుపు_DType* స్థిరరాశి నమోదు_Dtypep = nodep->కనుగొను_బిట్టు_DType(
            m_case_Decoder_నమోదు_వెడల్పు, m_case_Decoder_నమోదు_వెడల్పు, VSigning::UNSIGNED);
        Ast_కణుపు_DType* స్థిరరాశి పట్టిక_Dtypep = కొత్త AstUnpackArrayDType{flp, నమోదు_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->కూడు_Typesp(పట్టిక_Dtypep);
        AstConsArray* స్థిరరాశి tablep = కొత్త AstConsArray{flp, పట్టిక_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        కొరకు (పూర్ణాంకం i = 0; i < decoderEnries; ++i) {
            Ast_కణుపు* స్థిరరాశి stmtsp = clauses[i].రెండవ;
            AstConst* స్థిరరాశి entryp = కొత్త AstConst{flp, AstConst::Widthed_విలువ{},
                                                  static_cast<పూర్ణాంకం>(m_case_Decoder_నమోదు_వెడల్పు), 0};
            కొరకు (స్థిరరాశి Lhs_రికార్డు& lhs_రికార్డు : m_case_Decoder_రికార్డులు) {
                Ast_కణుపు_సమాసం* స్థిరరాశి lhsp = lhs_రికార్డు.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = శూన్యసూచిక;
                కొరకు (Ast_కణుపు* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_కణుపు_కేటాయించు* స్థిరరాశి assignp = VN_AS(stmtp, కణుపు_కేటాయించు);
                    ఒకవేళ (!lhsp->same_వృక్షం(assignp->lhsp())) కొనసాగు;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    ఆపు;
                }
                // Not assigned in this clause, so use the pre-assigned default
                ఒకవేళ (!valConstp) {
                    UASSERT_OBJ(lhs_రికార్డు.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_రికార్డు.preDefaultp->rhsp(), Const);
                }
                entryp->అంకె().opSelInto(valConstp->అంకె(), lhs_రికార్డు.స్థానభ్రంశం, lhsp->వెడల్పు());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* స్థిరరాశి సరిపోలిక_Refp = V3ConstPool::కనుగొను(matchp);
        AstVarRef* స్థిరరాశి పట్టిక_Refp = V3ConstPool::కనుగొను(tablep);
        VL_DO_DANGLING(matchp->delete_వృక్షం(), matchp);
        VL_DO_DANGLING(tablep->delete_వృక్షం(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_కణుపు_సమాసం* స్థిరరాశి caseExprp = nodep->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        Ast_సరిపోలిక_Masked* స్థిరరాశి indexp = కొత్త Ast_సరిపోలిక_Masked{flp, caseExprp, సరిపోలిక_Refp};
        Ast_కణుపు_సమాసం* స్థిరరాశి entryp = కొత్త AstArraySel{flp, పట్టిక_Refp, indexp};

        తిరిగి connectDecoderOutputs(nodep, entryp, m_decoder_బహిర్గతం_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_కణుపు* convertCaseFastRecurse(Ast_కణుపు_సమాసం* cexprp, పూర్ణాంకం msb, uint32_t పై_విలువ) స్థిరరాశి {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        ఒకవేళ (msb < 0) తిరిగి m_caseDetails.రికార్డులు[పై_విలువ].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        స్థిరరాశి uint32_t పై_Value0 = పై_విలువ;
        స్థిరరాశి uint32_t పై_Value1 = పై_విలువ | (1UL << msb);
        Ast_కణుపు* tree0p = convertCaseFastRecurse(cexprp, msb - 1, పై_Value0);
        Ast_కణుపు* tree1p = convertCaseFastRecurse(cexprp, msb - 1, పై_Value1);

        // If same logic on both sides, we can just return one of them
        ఒకవేళ (tree0p == tree1p) తిరిగి tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            తార్కిక same = నిజం;
            కొరకు (uint32_t a = పై_Value0, b = పై_Value1; a < పై_Value1; ++a, ++b) {
                ఒకవేళ (m_caseDetails.రికార్డులు[a].stmtsp != m_caseDetails.రికార్డులు[b].stmtsp) {
                    same = అబద్ధం;
                    ఆపు;
                }
            }
            ఒకవేళ (same) {
                VL_DO_DANGLING(tree1p->delete_వృక్షం(), tree1p);
                తిరిగి tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        ఒకవేళ (tree0p && tree0p->backp()) tree0p = tree0p->clone_వృక్షం(నిజం);
        ఒకవేళ (tree1p && tree1p->backp()) tree1p = tree1p->clone_వృక్షం(నిజం);
        // Create the If statement
        File_పంక్తి* స్థిరరాశి flp = cexprp->fileline();
        Ast_కణుపు_సమాసం* స్థిరరాశి condp = కొత్త AstSel{flp, cexprp->clone_వృక్షం_శుద్ధ(అబద్ధం), msb, 1};
        AstIf* స్థిరరాశి ifp = కొత్త AstIf{flp, condp, tree1p, tree0p};
        తిరిగి ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_కణుపు* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        స్థిరరాశి పూర్ణాంకం case_వెడల్పు = nodep->exprp()->వెడల్పు();
        Ast_కణుపు* స్థిరరాశి ifrootp = convertCaseFastRecurse(nodep->exprp(), case_వెడల్పు - 1, 0UL);
        తిరిగి ifrootp && ifrootp->backp() ? ifrootp->clone_వృక్షం(నిజం) : ifrootp;
    }

    Ast_కణుపు_సమాసం* newCaseCompare(స్థిరరాశి AstCase* casep, Ast_కణుపు_సమాసం* అంశం_Exprp) {
        File_పంక్తి* స్థిరరాశి flp = అంశం_Exprp->fileline();
        ఒకవేళ (never_అంశం(casep, అంశం_Exprp)) {
            VL_DO_DANGLING(నెట్టు_Deletep(అంశం_Exprp), అంశం_Exprp);
            తిరిగి కొత్త AstConst{flp, AstConst::బిట్టు_False{}};
        }

        ఒకవేళ (AstCond* స్థిరరాశి condp = VN_CAST(అంశం_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_కణుపు_సమాసం* స్థిరరాశి testp = condp->condp()->unlink_Fr_వెనుక();
            Ast_కణుపు_సమాసం* స్థిరరాశి thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_వెనుక());
            Ast_కణుపు_సమాసం* స్థిరరాశి elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_వెనుక());
            VL_DO_DANGLING2(నెట్టు_Deletep(అంశం_Exprp), అంశం_Exprp, condp);
            తిరిగి కొత్త AstCond{flp, testp, thenp, elsep};
        }

        Ast_కణుపు_సమాసం* స్థిరరాశి caseExprp = casep->exprp()->clone_వృక్షం_శుద్ధ(అబద్ధం);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        ఒకవేళ (AstInsideRange* స్థిరరాశి అంశం_Rangep = VN_CAST(అంశం_Exprp, InsideRange)) {
            Ast_కణుపు_సమాసం* స్థిరరాశి resultp = అంశం_Rangep->newAndFromInside(
                caseExprp, అంశం_Rangep->lhsp()->unlink_Fr_వెనుక(), అంశం_Rangep->rhsp()->unlink_Fr_వెనుక());
            VL_DO_DANGLING2(నెట్టు_Deletep(అంశం_Exprp), అంశం_Exprp, అంశం_Rangep);
            తిరిగి resultp;
        }

        ఒకవేళ (AstConst* స్థిరరాశి అంశం_Constp = VN_CAST(అంశం_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            ఒకవేళ (అంశం_Constp->అంకె().isFourState()) {
                స్థిరరాశి స్వయంచాలక& సరిపోలిక = సరిపోలిక_నమూనా(casep, అంశం_Constp);
                స్థిరరాశి V3_సంఖ్య& సరిపోలిక_ముసుగు = సరిపోలిక.మొదటి;
                స్థిరరాశి V3_సంఖ్య& సరిపోలిక_బిట్లు = సరిపోలిక.రెండవ;
                VL_DO_DANGLING2(నెట్టు_Deletep(అంశం_Exprp), అంశం_Exprp, అంశం_Constp);
                తిరిగి AstEq::newTyped(flp, కొత్త AstConst{flp, సరిపోలిక_బిట్లు},
                                       కొత్త AstAnd{flp, caseExprp, కొత్త AstConst{flp, సరిపోలిక_ముసుగు}});
            }
        }

        తిరిగి AstEq::newTyped(flp, caseExprp, అంశం_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_కణుపు* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        తార్కిక hasDefault = అబద్ధం;
        కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {

            File_పంక్తి* స్థిరరాశి flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            ఒకవేళ (itemp->isDefault()) {
                itemp->కూడు_Condsp(కొత్త AstConst{flp, AstConst::బిట్టు_True{}});
                hasDefault = నిజం;
                కొనసాగు;
            }

            // Regular clause. Construct the condition expression.
            Ast_కణుపు_సమాసం* newCondp = శూన్యసూచిక;
            కొరకు (Ast_కణుపు_సమాసం *అంశం_Exprp = itemp->condsp(), *nextp; అంశం_Exprp; అంశం_Exprp = nextp) {
                nextp = VN_AS(అంశం_Exprp->nextp(), కణుపు_సమాసం);
                అంశం_Exprp->unlink_Fr_వెనుక();

                // If case never matches, ignore it
                ఒకవేళ (never_అంశం(nodep, అంశం_Exprp)) {
                    VL_DO_DANGLING(నెట్టు_Deletep(అంశం_Exprp), అంశం_Exprp);
                    కొనసాగు;
                }

                // Compute the term to add to the condition expression
                Ast_కణుపు_సమాసం* స్థిరరాశి termp = newCaseCompare(nodep, అంశం_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? కొత్త AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            ఒకవేళ (!newCondp) newCondp = కొత్త AstConst{flp, AstConst::బిట్టు_False{}};
            itemp->కూడు_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        ఒకవేళ (!hasDefault) {
            ఒకవేళ (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_అంశం* స్థిరరాశి lastp = VN_AS(nodep->itemsp()->lastp(), Case_అంశం);
                నెట్టు_Deletep(lastp->condsp()->unlink_Fr_వెనుక_With_తదుపరి());
                lastp->కూడు_Condsp(కొత్త AstConst{lastp->fileline(), AstConst::బిట్టు_True{}});
            } లేకపోతే {
                // Otherwise needs an explicit empty default
                nodep->కూడు_Itemsp(కొత్త Ast_Case_అంశం{
                    nodep->fileline(), కొత్త AstConst{nodep->fileline(), AstConst::బిట్టు_True{}},
                    శూన్యసూచిక});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        పూర్ణాంకం లోతు = 0;
        AstIf* grouprootp = శూన్యసూచిక;
        AstIf* groupnextp = శూన్యసూచిక;
        AstIf* itemnextp = శూన్యసూచిక;
        కొరకు (Ast_Case_అంశం* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_అంశం)) {

            // Grab the statements from this item. May be empty.
            Ast_కణుపు* స్థిరరాశి istmtsp = itemp->stmtsp();
            ఒకవేళ (istmtsp) istmtsp->unlink_Fr_వెనుక_With_తదుపరి();

            // Expressioned clause
            Ast_కణుపు_సమాసం* స్థిరరాశి ifexprp = itemp->condsp()->unlink_Fr_వెనుక();
            {  // Prepare for next group
                ఒకవేళ (++లోతు > CASE_ENCODER_సమూహం_లోతు) లోతు = 1;
                ఒకవేళ (లోతు == 1) {  // First group or starting new group
                    itemnextp = శూన్యసూచిక;
                    AstIf* స్థిరరాశి newp = కొత్త AstIf{itemp->fileline(), ifexprp->clone_వృక్షం_శుద్ధ(నిజం)};
                    ఒకవేళ (groupnextp) {
                        groupnextp->కూడు_Elsesp(newp);
                    } లేకపోతే {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } లేకపోతే {  // Continue group, modify if condition to OR in this new condition
                    Ast_కణుపు_సమాసం* స్థిరరాశి condp = groupnextp->condp()->unlink_Fr_వెనుక();
                    groupnextp->condp(
                        కొత్త AstOr{ifexprp->fileline(), condp, ifexprp->clone_వృక్షం_శుద్ధ(నిజం)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_కణుపు_సమాసం* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                ఒకవేళ (లోతు == CASE_ENCODER_సమూహం_లోతు) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_వృక్షం(), itemexprp);
                    itemexprp = కొత్త AstConst{itemp->fileline(), AstConst::బిట్టు_True{}};
                }
                AstIf* స్థిరరాశి newp = కొత్త AstIf{itemp->fileline(), itemexprp, istmtsp};
                ఒకవేళ (itemnextp) {
                    itemnextp->కూడు_Elsesp(newp);
                } లేకపోతే {
                    groupnextp->కూడు_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        తిరిగి grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_కణుపు* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        స్థిరరాశి తార్కిక use_పట్టిక = [&]() {
            // Not if disabled
            ఒకవేళ (!v3Global.opt.f_Case_పట్టిక()) తిరిగి అబద్ధం;
            // Not if analysis tells us we can't
            ఒకవేళ (!m_case_పట్టిక_వెడల్పు) తిరిగి అబద్ధం;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            ఒకవేళ (m_case_పట్టిక_వెడల్పు <= CASE_పట్టిక_TINY_బిట్లు) తిరిగి నిజం;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            స్థిరరాశి కొలత_t branches = std::min<కొలత_t>(nodep->exprp()->వెడల్పు(), m_caseNConditions);
            ఒకవేళ (branches < CASE_పట్టిక_MIN_BRANCHES) తిరిగి అబద్ధం;
            తిరిగి నిజం;
        }();
        ఒకవేళ (use_పట్టిక) తిరిగి convert_Case_పట్టిక(nodep);

        // Determine if we should use the decoder method.
        స్థిరరాశి తార్కిక useDecoder = [&]() {
            // Not if disabled
            ఒకవేళ (!v3Global.opt.fCaseDecoder()) తిరిగి అబద్ధం;
            // Not if not a decoder pattern
            ఒకవేళ (m_case_Decoder_రికార్డులు.ఖాళీలు()) తిరిగి అబద్ధం;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            స్థిరరాశి కొలత_t branches = std::min<కొలత_t>(nodep->exprp()->వెడల్పు(), m_caseNConditions);
            ఒకవేళ (branches < CASE_పట్టిక_MIN_BRANCHES) తిరిగి అబద్ధం;
            తిరిగి నిజం;
        }();
        ఒకవేళ (useDecoder) తిరిగి convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        స్థిరరాశి తార్కిక use_Fast_బిట్టు_వృక్షం = [&]() {
            // Not if disabled
            ఒకవేళ (!v3Global.opt.f_Case_వృక్షం()) తిరిగి అబద్ధం;
            // Can't do it without the detailed analysis
            ఒకవేళ (!m_caseDetailsValid) తిరిగి అబద్ధం;
            // Can't do it if not exhaustive
            ఒకవేళ (!m_caseDetails.exhaustive) తిరిగి అబద్ధం;
            // Not worth doing if there are few conditions
            ఒకవేళ (m_caseNConditions <= 3) తిరిగి అబద్ధం;
            // Avoid e.g. priority expanders from going crazy in expansion
            స్థిరరాశి కొలత_t case_వెడల్పు = nodep->exprp()->వెడల్పు();
            ఒకవేళ (case_వెడల్పు >= 8 && m_caseNConditions <= (case_వెడల్పు + 1)) తిరిగి అబద్ధం;
            // Otherwise use the bit tree
            తిరిగి నిజం;
        }();
        ఒకవేళ (use_Fast_బిట్టు_వృక్షం) తిరిగి convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        ఒకవేళ (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_సంకేతం::LATCH, నిజం);
        }
        తిరిగి convertCaseGeneric(nodep);
    }

    // VISITORS
    శూన్య సందర్శించు(AstCase* nodep) override {
        UASSERT_OBJ(nodep->exprp()->is_శుద్ధ(), nodep,
                    "Impure case expression should have been removed by V3LiftExpr");

        CaseLintVisitor::apply(nodep);

        // Convert any children first
        iterateChildren(nodep);

        // Analyze this case statement
        analyzeCase(nodep);

        // Take the 'notParallelp' statements under the case statement created by V3Assert.
        // If the statement was proven to have no overlaps and all cases covered,
        // it can be removed. Otherwise insert the assertion after the case statement.
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_వెనుక_With_తదుపరి();
            ఒకవేళ (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_వృక్షం(), stmtp);
            } లేకపోతే {
                nodep->కూడు_తదుపరి_ఇక్కడ(stmtp);
            }
        }

        // Convert the case statement and replace the original
        ఒకవేళ (Ast_కణుపు* స్థిరరాశి replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } లేకపోతే {
            nodep->unlink_Fr_వెనుక();
        }
        VL_DO_DANGLING(nodep->delete_వృక్షం(), nodep);
    }

    శూన్య సందర్శించు(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    శూన్య సందర్శించు(Ast_కణుపు* nodep) override { iterateChildren(nodep); }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::కూడు_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::కూడు_Stat("Optimizations, Cases table normal", m_stats.case_పట్టిక_సాధారణ);
        V3Stats::కూడు_Stat("Optimizations, Cases table tiny", m_stats.case_పట్టిక_Tiny);
        V3Stats::కూడు_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::కూడు_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::కూడు_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

కొలత_t CaseVisitor::Lhs_రికార్డు::s_తదుపరి_Id = 0;

//######################################################################
// Case class functions

శూన్య V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_వృక్షం("case", 0, dump_వృక్షం_Either_Level() >= 3);
}
శూన్య V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
