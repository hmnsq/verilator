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
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_casep = ಶೂನ್ಯಸೂಚಕ;

    // METHODS
    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Case_ಅಂಶ>
    ಸ್ಥಿರ ಶೂನ್ಯ detect_ಅಪವರ್ತ್ಯ_Defaults(Case_ಅಂಶ* itemsp) {
        ತಾರ್ಕಿಕ ದೊರಕಿತು_Default = ಸುಳ್ಳು;
        ಗಾಗಿ (Case_ಅಂಶ* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<Case_ಅಂಶ>(itemp->nextp())) {
            ಆದರೆ (!itemp->isDefault()) ಮುಂದುವರಿ;
            ಆದರೆ (ದೊರಕಿತು_Default) itemp->v3error("Multiple default statements in case statement.");
            ದೊರಕಿತು_Default = ನಿಜ;
        }
    }

    ಮಾದರಿ <ಬಗೆ_ಹೆಸರು Case_ಅಂಶ>
    ಶೂನ್ಯ checkXZinNonCaseX(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Case_ಅಂಶ* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        ಗಾಗಿ (Case_ಅಂಶ* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<Case_ಅಂಶ>(itemp->nextp())) {
            iterate_And_ಮುಂದಿನ_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_ಅಪವರ್ತ್ಯ_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        ಆದರೆ (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_ಅಪವರ್ತ್ಯ_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ ಭೇಟಿ(AstConst* nodep) override {
        ಆದರೆ (!nodep->ಅಂಕೆ().isFourState()) ಹಿಂತಿರುಗಿ;

        // Error if generate case
        ಆದರೆ (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            ಹಿಂತಿರುಗಿ;
        }

        // Otherwise must be a case statement
        ಸ್ಥಿರಾಂಕ AstCase* ಸ್ಥಿರಾಂಕ casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        ಆದರೆ (casep->casex()) ಹಿಂತಿರುಗಿ;

        ಆದರೆ (casep->casez() || casep->caseInside()) {
            ಆದರೆ (nodep->ಅಂಕೆ().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            ಹಿಂತಿರುಗಿ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    ಸ್ಪಷ್ಟ CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ಸ್ಥಿರ ಶೂನ್ಯ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

ವರ್ಗ CaseVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // Maximum width for detailed analysis
    constexpr ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ CASE_DETAILS_MAX_ಅಗಲ = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ CASE_ENCODER_ಗುಂಪು_ಆಳ = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ಸ್ಥಿರ ಗಾತ್ರ_t CASE_ಕೋಷ್ಟಕ_TINY_ಬಿಟ್ಟುಗಳು = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ಸ್ಥಿರ ಗಾತ್ರ_t CASE_ಕೋಷ್ಟಕ_MAX_ಬಿಟ್ಟುಗಳು = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ಸ್ಥಿರ ಗಾತ್ರ_t CASE_ಕೋಷ್ಟಕ_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ರಚನೆ Case_ದಾಖಲೆ final {
        Ast_Case_ಅಂಶ* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ಗಂಟು* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ರಚನೆ Lhs_ದಾಖಲೆ final {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = ಶೂನ್ಯಸೂಚಕ;  // LHS of the assignment
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* preDefaultp = ಶೂನ್ಯಸೂಚಕ;  // Default assignment *before the case statement*
        ಗಾತ್ರ_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        ಗಾತ್ರ_t n_Case_ನಿಯೋಜಿಸು_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        ಗಾತ್ರ_t ಸ್ಥಾನಾಂತರ = 0;  // Offset in the table for this LHS

        ಸ್ಥಿರ ಗಾತ್ರ_t s_ಮುಂದಿನ_Id;  // Static unique Id counter
        ಗಾತ್ರ_t id = ++s_ಮುಂದಿನ_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ರಚನೆ Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_ಕೋಷ್ಟಕ_ಸಾಮಾನ್ಯ;  // Cases using table method with normal table
        VDouble0 case_ಕೋಷ್ಟಕ_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* m_alwaysp = ಶೂನ್ಯಸೂಚಕ;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_ಹೊರಹೋಗು_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = ಶೂನ್ಯಸೂಚಕ;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    ತಾರ್ಕಿಕ m_caseOpaque = ಸುಳ್ಳು;  // Case statement is opaque (non-packed, or non-const conditions)
    ತಾರ್ಕಿಕ m_caseHasDefault = ಸುಳ್ಳು;  // Indicates the case statement has a default case
    ಗಾತ್ರ_t m_case_NCase_ವಸ್ತುಗಳು = 0;  // Number of AstCaseItems in the case statement
    ಗಾತ್ರ_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ>, Lhs_ದಾಖಲೆ> m_case_Lhs_ದಾಖಲೆಗಳು;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::ಸದಿಶ<Lhs_ದಾಖಲೆ> m_case_Decoder_ದಾಖಲೆಗಳು;
    ಗಾತ್ರ_t m_case_Decoder_ನಮೂದು_ಅಗಲ = 0;  // Width of each entry in the decoder table
    ಗಾತ್ರ_t m_case_ಕೋಷ್ಟಕ_ಅಗಲ = 0;  // Total width of the case table - 0 means can't optimize
    ತಾರ್ಕಿಕ m_caseDetailsValid = ಸುಳ್ಳು;  // Indicates m_caseDetails is valid
    ರಚನೆ final {
        ತಾರ್ಕಿಕ exhaustive = ಸುಳ್ಳು;  // Proven exhaustive
        ತಾರ್ಕಿಕ exhaustiveOverEnumOnly = ಸುಳ್ಳು;  // Exhaustive over enum values only
        ತಾರ್ಕಿಕ noOverlaps = ಸುಳ್ಳು;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_ದಾಖಲೆ, 1U << CASE_DETAILS_MAX_ಅಗಲ> ದಾಖಲೆಗಳು;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ಸ್ಥಿರ ತಾರ್ಕಿಕ never_ಅಂಶ(ಸ್ಥಿರಾಂಕ AstCase* casep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಂಶ_Exprp) {
        ಸ್ಥಿರಾಂಕ AstConst* ಸ್ಥಿರಾಂಕ constp = VN_CAST(ಅಂಶ_Exprp, Const);
        ಆದರೆ (!constp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (casep->casex() || casep->caseInside()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        ಆದರೆ (casep->casez()) ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().isAnyX();
        ಹಿಂತಿರುಗಿ constp->ಅಂಕೆ().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ಸ್ಥಿರ std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ> ಹೊಂದಿಕೆ_ನಮೂನೆ(ಸ್ಥಿರಾಂಕ AstCase* /*casep*/,
                                                      ಸ್ಥಿರಾಂಕ AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_ಸಂಖ್ಯೆ, V3_ಸಂಖ್ಯೆ> pair_ಮುಖವಾಡ_ಬಿಟ್ಟುಗಳು{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->ಅಗಲ(), 0),
            std::forward_as_tuple(condp->fileline(), condp->ಅಗಲ(), 0)};
        pair_ಮುಖವಾಡ_ಬಿಟ್ಟುಗಳು.ಮೊದಲ.op_ಬಿಟ್ಟುಗಳು_Non_XZ(condp->ಅಂಕೆ());
        pair_ಮುಖವಾಡ_ಬಿಟ್ಟುಗಳು.ಎರಡನೇ.op_ಬಿಟ್ಟುಗಳು_One(condp->ಅಂಕೆ());
        ಹಿಂತಿರುಗಿ pair_ಮುಖವಾಡ_ಬಿಟ್ಟುಗಳು;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ಸ್ಥಿರ Ast_ಗಂಟು_ನಿಯೋಜಿಸು* check_Decoder_ನಿಯೋಜಿಸು(Ast_ಗಂಟು* stmtp) {
        // Only Assign and AssignDly are supported
        ಆದರೆ (!VN_IS(stmtp, ನಿಯೋಜಿಸು) && !VN_IS(stmtp, ನಿಯೋಜಿಸು_Dly)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = VN_AS(stmtp, ಗಂಟು_ನಿಯೋಜಿಸು);
        // Only if no timing control
        ಆದರೆ (assp->timingControlp()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Only if assigning a constant
        ಆದರೆ (!VN_IS(assp->rhsp(), Const)) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Only if it's a packed value
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ dtypep = assp->rhsp()->dtypep();
        ಆದರೆ (dtypep->isString() || dtypep->isDouble()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        ಆದರೆ (assp->lhsp()->exists([](AstVarRef* refp) { ಹಿಂತಿರುಗಿ refp->access().isReadOrRW(); })) {
            ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        }
        // This is an assignment that fits the decoder pattern
        ಹಿಂತಿರುಗಿ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    ತಾರ್ಕಿಕ analyze_Decoder_Case_ಅಂಶ(Ast_Case_ಅಂಶ* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
        ಗಾಗಿ (Ast_ಗಂಟು* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = check_Decoder_ನಿಯೋಜಿಸು(stmtp);
            ಆದರೆ (!assp) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Must assign each LHS exactly once - RHS is Const
            ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅಪವರ್ತ್ಯ_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                ಹಿಂತಿರುಗಿ refp->varScopep()->user1_ಗಣ_Once();
            });
            ಆದರೆ (ಅಪವರ್ತ್ಯ_Assignments) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Update LhsRecord
            Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ = m_case_Lhs_ದಾಖಲೆಗಳು[*assp->lhsp()];
            ಆದರೆ (!lhs_ದಾಖಲೆ.lhsp) lhs_ದಾಖಲೆ.lhsp = assp->lhsp();
            lhs_ದಾಖಲೆ.nCaseAssigns += VN_IS(assp, ನಿಯೋಜಿಸು);
            lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys += VN_IS(assp, ನಿಯೋಜಿಸು_Dly);
        }
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ AstEnumDType* getEnumCompletionCheckDType(ಸ್ಥಿರಾಂಕ AstCase* ಸ್ಥಿರಾಂಕ nodep) {
        ಆದರೆ (!nodep->uniquePragma() && !nodep->unique0Pragma()) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;
        ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_ಗಮ್ಯ_Enump(), EnumDType);
        ಆದರೆ (!enumDtp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Case isn't enum
        ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = enumDtp->subDTypep()->basicp();
        ಆದರೆ (!basicp) ಹಿಂತಿರುಗಿ ಶೂನ್ಯಸೂಚಕ;  // Not simple type (perhaps IEEE illegal)
        ಹಿಂತಿರುಗಿ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    ತಾರ್ಕಿಕ checkExhaustiveEnum(ಸ್ಥಿರಾಂಕ AstCase* ಸ್ಥಿರಾಂಕ nodep, ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ enump) {
        ಸ್ಥಿರಾಂಕ uint32_t ಅಂಕೆ_Cases = 1UL << nodep->exprp()->ಅಗಲ();
        ತಾರ್ಕಿಕ fullyCovered = ನಿಜ;
        string missing_ವಸ್ತುಗಳು;
        ಗಾಗಿ (Ast_Enum_ಅಂಶ* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_ಅಂಶ)) {
            ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ_ನಮೂನೆ(nodep, VN_AS(eip->valuep(), Const));
            ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = ಹೊಂದಿಕೆ.ಮೊದಲ.ಗಮ್ಯ_UInt();
            ಸ್ಥಿರಾಂಕ uint32_t ಬಿಟ್ಟುಗಳು = ಹೊಂದಿಕೆ.ಎರಡನೇ.ಗಮ್ಯ_UInt();

            // Check all cases to see if they cover this enum value/pattern
            ಗಾಗಿ (uint32_t i = 0; i < ಅಂಕೆ_Cases; ++i) {
                ಆದರೆ ((i & ಮುಖವಾಡ) != ಬಿಟ್ಟುಗಳು) ಮುಂದುವರಿ;  // This case is not for this enum value
                ಆದರೆ (m_caseDetails.ದಾಖಲೆಗಳು[i].itemp) ಮುಂದುವರಿ;  // Covered case
                ಆದರೆ (!missing_ವಸ್ತುಗಳು.ಖಾಲಿಗಳು()) missing_ವಸ್ತುಗಳು += ", ";
                missing_ವಸ್ತುಗಳು += eip->prettyNameQ();
                fullyCovered = ಸುಳ್ಳು;
                ನಿಲ್ಲಿಸು;
            }
        }
        ಆದರೆ (!missing_ವಸ್ತುಗಳು.ಖಾಲಿಗಳು() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_ವಸ್ತುಗಳು);
        }
        ಹಿಂತಿರುಗಿ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    ತಾರ್ಕಿಕ checkExhaustivePacked(AstCase* nodep) {
        ಸ್ಥಿರಾಂಕ uint32_t ಅಂಕೆ_Cases = 1UL << nodep->exprp()->ಅಗಲ();
        ಗಾಗಿ (uint32_t i = 0; i < ಅಂಕೆ_Cases; ++i) {
            ಆದರೆ (m_caseDetails.ದಾಖಲೆಗಳು[i].itemp) ಮುಂದುವರಿ;  // Covered case
            ಆದರೆ (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
        }

        // It's an exhaustive case statement
        ಹಿಂತಿರುಗಿ ನಿಜ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    ಶೂನ್ಯ analyzeCaseDetails(AstCase* nodep) {
        ಸ್ಥಿರಾಂಕ uint32_t ಅಂಕೆ_ಮೌಲ್ಯಗಳು = 1UL << nodep->exprp()->ಅಗಲ();
        // Clear case records
        ಗಾಗಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
            m_caseDetails.ದಾಖಲೆಗಳು[i].itemp = ಶೂನ್ಯಸೂಚಕ;
            m_caseDetails.ದಾಖಲೆಗಳು[i].constp = ಶೂನ್ಯಸೂಚಕ;
            m_caseDetails.ದಾಖಲೆಗಳು[i].stmtsp = ಶೂನ್ಯಸೂಚಕ;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        ತಾರ್ಕಿಕ reportedOverlap = ಸುಳ್ಳು;
        ತಾರ್ಕಿಕ hasDefault = ಸುಳ್ಳು;
        m_caseDetails.noOverlaps = ನಿಜ;
        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {

            // Default case
            ಆದರೆ (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                ಗಾಗಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
                    Case_ದಾಖಲೆ& case_ದಾಖಲೆ = m_caseDetails.ದಾಖಲೆಗಳು[i];
                    ಆದರೆ (!case_ದಾಖಲೆ.itemp) {
                        case_ದಾಖಲೆ.itemp = itemp;
                        case_ದಾಖಲೆ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = ನಿಜ;
                ಮುಂದುವರಿ;
            }

            ಗಾಗಿ (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                ಆದರೆ (never_ಅಂಶ(nodep, iconstp)) ಮುಂದುವರಿ;

                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ_ನಮೂನೆ(nodep, iconstp);
                ಸ್ಥಿರಾಂಕ uint32_t ಮುಖವಾಡ = ಹೊಂದಿಕೆ.ಮೊದಲ.ಗಮ್ಯ_UInt();
                ಸ್ಥಿರಾಂಕ uint32_t ಬಿಟ್ಟುಗಳು = ಹೊಂದಿಕೆ.ಎರಡನೇ.ಗಮ್ಯ_UInt();

                ತಾರ್ಕಿಕ ಸಿಕ್ಕಿತು_New_Case = ಸುಳ್ಳು;
                ಸ್ಥಿರಾಂಕ AstConst* ಮೊದಲ_Overlap_Constp = ಶೂನ್ಯಸೂಚಕ;
                uint32_t ಮೊದಲ_Overlap_ಮೌಲ್ಯ = 0;
                ಗಾಗಿ (uint32_t i = 0; i < ಅಂಕೆ_ಮೌಲ್ಯಗಳು; ++i) {
                    ಆದರೆ ((i & ಮುಖವಾಡ) != ಬಿಟ್ಟುಗಳು) ಮುಂದುವರಿ;

                    Case_ದಾಖಲೆ& case_ದಾಖಲೆ = m_caseDetails.ದಾಖಲೆಗಳು[i];

                    // If this is the first case that covers this value, record it
                    ಆದರೆ (!case_ದಾಖಲೆ.itemp) {
                        case_ದಾಖಲೆ.itemp = itemp;
                        case_ದಾಖಲೆ.constp = iconstp;
                        case_ದಾಖಲೆ.stmtsp = itemp->stmtsp();
                        ಸಿಕ್ಕಿತು_New_Case = ನಿಜ;
                        ಮುಂದುವರಿ;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    ಆದರೆ (case_ದಾಖಲೆ.itemp == itemp) ಮುಂದುವರಿ;

                    // Otherwise record the first overlapping case
                    ಆದರೆ (!ಮೊದಲ_Overlap_Constp) {
                        ಮೊದಲ_Overlap_Constp = case_ದಾಖಲೆ.constp;
                        ಮೊದಲ_Overlap_ಮೌಲ್ಯ = i;
                        m_caseDetails.noOverlaps = ಸುಳ್ಳು;
                    }
                }

                // Only report first overlap
                ಆದರೆ (reportedOverlap || !ಮೊದಲ_Overlap_Constp) ಮುಂದುವರಿ;

                // Report first overlap
                ಆದರೆ (nodep->ಆದ್ಯತೆ_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    ಆದರೆ (!ಸಿಕ್ಕಿತು_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << ಮೊದಲ_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << ಮೊದಲ_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = ನಿಜ;
                    }
                } ಇಲ್ಲದಿದ್ದರೆ {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_ನಮೂನೆ;
                    ಆದರೆ (iconstp->ಅಂಕೆ().isAnyXZ()) {
                        example_ನಮೂನೆ << " (example pattern 0x" << std::hex << ಮೊದಲ_Overlap_ಮೌಲ್ಯ
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_ನಮೂನೆ.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << ಮೊದಲ_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << ಮೊದಲ_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = ನಿಜ;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = ಸುಳ್ಳು;
        m_caseDetails.exhaustive = hasDefault;
        ಆದರೆ (!hasDefault) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstEnumDType* ಸ್ಥಿರಾಂಕ enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } ಇಲ್ಲದಿದ್ದರೆ {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = ನಿಜ;
    }

    ಶೂನ್ಯ analyze_Decoder_ನಮೂನೆ(AstCase* nodep) {
        // Check each LHS record
        ಗಾಗಿ (ಸ್ವಯಂಚಾಲಿತ it = m_case_Lhs_ದಾಖಲೆಗಳು.cbegin(); it != m_case_Lhs_ದಾಖಲೆಗಳು.cend();) {
            ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ = it->ಎರಡನೇ;

            // Delete records that have no assignments in any case item (only pre-defaults)
            ಆದರೆ (!lhs_ದಾಖಲೆ.nCaseAssigns && !lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys) {
                it = m_case_Lhs_ದಾಖಲೆಗಳು.erase(it);
                ಮುಂದುವರಿ;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            ಆದರೆ (lhs_ದಾಖಲೆ.nCaseAssigns && lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys) ಹಿಂತಿರುಗಿ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            ಆದರೆ (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                ಆದರೆ (lhs_ದಾಖಲೆ.nCaseAssigns == m_case_NCase_ವಸ್ತುಗಳು) ಮುಂದುವರಿ;
                ಆದರೆ (lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys == m_case_NCase_ವಸ್ತುಗಳು) ಮುಂದುವರಿ;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ preDefaultp = lhs_ದಾಖಲೆ.preDefaultp;
            ಆದರೆ (!preDefaultp) ಹಿಂತಿರುಗಿ;
            // And the pre-default needs to be the same type
            ಆದರೆ (lhs_ದಾಖಲೆ.nCaseAssigns && !VN_IS(preDefaultp, ನಿಯೋಜಿಸು)) ಹಿಂತಿರುಗಿ;
            ಆದರೆ (lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys && !VN_IS(preDefaultp, ನಿಯೋಜಿಸು_Dly)) ಹಿಂತಿರುಗಿ;
        }
        // All cases check out, can optimize if there are some entries left
        ಆದರೆ (m_case_Lhs_ದಾಖಲೆಗಳು.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_ದಾಖಲೆಗಳು.reserve(m_case_Lhs_ದಾಖಲೆಗಳು.ಗಾತ್ರ_2());
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಅಂಶ_2 : m_case_Lhs_ದಾಖಲೆಗಳು) m_case_Decoder_ದಾಖಲೆಗಳು.emplace_ಹಿಂಭಾಗ(ಅಂಶ_2.ಎರಡನೇ);
        std::ವಿಂಗಡಿಸು(m_case_Decoder_ದಾಖಲೆಗಳು.begin(), m_case_Decoder_ದಾಖಲೆಗಳು.ಕೊನೆ(),
                  [](ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& a, ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& b) {
                      // Sort by width, then id
                      ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ a_ಅಗಲ = a.lhsp->ಅಗಲ();
                      ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ b_ಅಗಲ = b.lhsp->ಅಗಲ();
                      ಆದರೆ (a_ಅಗಲ != b_ಅಗಲ) ಹಿಂತಿರುಗಿ a_ಅಗಲ < b_ಅಗಲ;
                      ಹಿಂತಿರುಗಿ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ case_ಅಗಲ = nodep->exprp()->ಅಗಲ();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಹೊಂದುತ್ತದೆ_ಎಲ್ಲೆ = [&](ಗಾತ್ರ_t ನಮೂದು_ಅಗಲ, ಗಾತ್ರ_t max_ಅಗಲ) -> ತಾರ್ಕಿಕ {
            ಗಾತ್ರ_t ಒಟ್ಟು_ಅಗಲ = ನಮೂದು_ಅಗಲ;
            // Multiply cases - iterative to avoid overflow
            ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < case_ಅಗಲ; ++i) {
                ಒಟ್ಟು_ಅಗಲ <<= 1;
                ಆದರೆ (ಒಟ್ಟು_ಅಗಲ > max_ಅಗಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            }
            ಹಿಂತಿರುಗಿ ನಿಜ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_ನಮೂದು_ಅಗಲ = 0;
        ಗಾಗಿ (Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ : m_case_Decoder_ದಾಖಲೆಗಳು) {
            lhs_ದಾಖಲೆ.ಸ್ಥಾನಾಂತರ = m_case_Decoder_ನಮೂದು_ಅಗಲ;
            m_case_Decoder_ನಮೂದು_ಅಗಲ += lhs_ದಾಖಲೆ.lhsp->ಅಗಲ();
        }
        // If it fits, we will pack it tightly
        ಆದರೆ (ಹೊಂದುತ್ತದೆ_ಎಲ್ಲೆ(m_case_Decoder_ನಮೂದು_ಅಗಲ, CASE_ಕೋಷ್ಟಕ_TINY_ಬಿಟ್ಟುಗಳು)) {
            m_case_ಕೋಷ್ಟಕ_ಅಗಲ = m_case_Decoder_ನಮೂದು_ಅಗಲ << case_ಅಗಲ;  // Can optimize
            ಹಿಂತಿರುಗಿ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_ನಮೂದು_ಅಗಲ = 0;
        ಗಾಗಿ (Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ : m_case_Decoder_ದಾಖಲೆಗಳು) {
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t ಅಗಲ = lhs_ದಾಖಲೆ.lhsp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t lsb_ಶಬ್ದ = VL_BITWORD_E(m_case_Decoder_ನಮೂದು_ಅಗಲ);
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t msb_ಶಬ್ದ = VL_BITWORD_E(m_case_Decoder_ನಮೂದು_ಅಗಲ + ಅಗಲ - 1);
            ಆದರೆ (lsb_ಶಬ್ದ != msb_ಶಬ್ದ) {
                m_case_Decoder_ನಮೂದು_ಅಗಲ = VL_ಪದಗಳು_I(m_case_Decoder_ನಮೂದು_ಅಗಲ) * VL_EDATASIZE;
            }
            lhs_ದಾಖಲೆ.ಸ್ಥಾನಾಂತರ = m_case_Decoder_ನಮೂದು_ಅಗಲ;
            m_case_Decoder_ನಮೂದು_ಅಗಲ += ಅಗಲ;
        }
        // Check the table fits max size
        ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t aligned_ನಮೂದು_ಅಗಲ = VL_ಪದಗಳು_I(m_case_Decoder_ನಮೂದು_ಅಗಲ) * VL_EDATASIZE;
        ಆದರೆ (ಹೊಂದುತ್ತದೆ_ಎಲ್ಲೆ(aligned_ನಮೂದು_ಅಗಲ, CASE_ಕೋಷ್ಟಕ_MAX_ಬಿಟ್ಟುಗಳು)) {
            m_case_ಕೋಷ್ಟಕ_ಅಗಲ = aligned_ನಮೂದು_ಅಗಲ << case_ಅಗಲ;  // Can optimize
            ಹಿಂತಿರುಗಿ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    ಶೂನ್ಯ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = ಸುಳ್ಳು;
        m_caseHasDefault = ಸುಳ್ಳು;
        m_case_NCase_ವಸ್ತುಗಳು = 0;
        m_caseNConditions = 0;
        m_case_Decoder_ದಾಖಲೆಗಳು.clear();
        m_case_Decoder_ನಮೂದು_ಅಗಲ = 0;
        m_case_ಕೋಷ್ಟಕ_ಅಗಲ = 0;
        m_case_Lhs_ದಾಖಲೆಗಳು.clear();
        m_caseDetailsValid = ಸುಳ್ಳು;

        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        ಆದರೆ (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = ನಿಜ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ಸ್ಥಿರಾಂಕ VNUser1_ಒಳ_Use user1_ಒಳ_Use;
            ಗಾಗಿ (Ast_ಗಂಟು* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assp = check_Decoder_ನಿಯೋಜಿಸು(prevp);
                ಆದರೆ (!assp) ನಿಲ್ಲಿಸು;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ ಅಪವರ್ತ್ಯ_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    ಹಿಂತಿರುಗಿ refp->varScopep()->user1_ಗಣ_Once();
                });
                ಆದರೆ (ಅಪವರ್ತ್ಯ_Assignments) ನಿಲ್ಲಿಸು;
                // Store pre-default assignment
                Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ = m_case_Lhs_ದಾಖಲೆಗಳು[*assp->lhsp()];
                lhs_ದಾಖಲೆ.lhsp = assp->lhsp();
                lhs_ದಾಖಲೆ.preDefaultp = assp;
            }
        }

        // Check each case item
        ತಾರ್ಕಿಕ canBeDecoder = ನಿಜ;
        ಗಾಗಿ (Ast_Case_ಅಂಶ* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_ಅಂಶ)) {
            // Check conditions
            ಗಾಗಿ (Ast_ಗಂಟು* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                ಆದರೆ (!never_ಅಂಶ(nodep, VN_AS(condp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                ಆದರೆ (!VN_IS(condp, Const)) {
                    m_caseOpaque = ನಿಜ;
                    canBeDecoder = ಸುಳ್ಳು;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            ಆದರೆ (cip->isDefault()) m_caseHasDefault = ನಿಜ;
            // Count case items
            ++m_case_NCase_ವಸ್ತುಗಳು;
            // Check if it fits the decoder pattern, if still possible
            ಆದರೆ (canBeDecoder) canBeDecoder = analyze_Decoder_Case_ಅಂಶ(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        ಆದರೆ (m_caseOpaque) ಹಿಂತಿರುಗಿ;

        // If small enough, analyse details
        ಆದರೆ (caseExprp->ಅಗಲ() <= CASE_DETAILS_MAX_ಅಗಲ) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        ಆದರೆ (canBeDecoder) analyze_Decoder_ನಮೂನೆ(nodep);
    }

    Ast_ಗಂಟು_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, V3SharedTmps& tmps) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        // If there is only one LHS, just use the result
        ಆದರೆ (m_case_Decoder_ದಾಖಲೆಗಳು.ಗಾತ್ರ_2() == 1) {
            ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ = m_case_Decoder_ದಾಖಲೆಗಳು[0];
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = lhs_ದಾಖಲೆ.lhsp->ಅಗಲ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp
                = exprp->ಅಗಲ() == ಅಗಲ ? exprp : ಹೊಸ AstSel{flp, exprp, 0, ಅಗಲ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = lhs_ದಾಖಲೆ.lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (lhs_ದಾಖಲೆ.nCaseAssigns) {
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys) {
                ಹಿಂತಿರುಗಿ ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{flp, lhsp, rhsp};
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_ನಮೂದು_ಅಗಲ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Writep = ಹೊಸ AstVarRef{flp, ತಾತ್ಕಾಲಿಕ_Vscp, VAccess::WRITE};
        Ast_ಗಂಟು_Stmt* ಸ್ಥಿರಾಂಕ resultp = ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, ತಾತ್ಕಾಲಿಕ_Writep, exprp};

        // For each LHS, select out the result
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ : m_case_Decoder_ದಾಖಲೆಗಳು) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಅಗಲ = lhs_ದಾಖಲೆ.lhsp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lsb = lhs_ದಾಖಲೆ.ಸ್ಥಾನಾಂತರ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ತಾತ್ಕಾಲಿಕ_Readp = ಹೊಸ AstVarRef{flp, ತಾತ್ಕಾಲಿಕ_Vscp, VAccess::READ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ rhsp = ಹೊಸ AstSel{flp, ತಾತ್ಕಾಲಿಕ_Readp, lsb, ಅಗಲ};
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = lhs_ದಾಖಲೆ.lhsp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
            ಆದರೆ (lhs_ದಾಖಲೆ.nCaseAssigns) {
                resultp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು{flp, lhsp, rhsp});
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (lhs_ದಾಖಲೆ.n_Case_ನಿಯೋಜಿಸು_Dlys) {
                resultp->ಕೂಡಿಸು_ಮುಂದಿನ(ಹೊಸ Ast_ನಿಯೋಜಿಸು_Dly{flp, lhsp, rhsp});
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        ಹಿಂತಿರುಗಿ resultp;
    }

    Ast_ಗಂಟು_Stmt* convert_Case_ಕೋಷ್ಟಕ(AstCase* nodep) {
        // Create the table constant
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();
        AstConst* ಸ್ಥಿರಾಂಕ tablep
            = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, static_cast<ಪೂರ್ಣಾಂಕ>(m_case_ಕೋಷ್ಟಕ_ಅಗಲ), 0};
        ಸ್ಥಿರಾಂಕ uint32_t ಕೋಷ್ಟಕ_Entries = 1U << nodep->exprp()->ಅಗಲ();

        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ is_Tiny_ಕೋಷ್ಟಕ = m_case_ಕೋಷ್ಟಕ_ಅಗಲ <= CASE_ಕೋಷ್ಟಕ_TINY_ಬಿಟ್ಟುಗಳು;
        ಆದರೆ (is_Tiny_ಕೋಷ್ಟಕ) {
            ++m_stats.case_ಕೋಷ್ಟಕ_Tiny;
        } ಇಲ್ಲದಿದ್ದರೆ {
            ++m_stats.case_ಕೋಷ್ಟಕ_ಸಾಮಾನ್ಯ;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ಸ್ಥಿರಾಂಕ uint32_t ನಮೂದು_ಅಗಲ = is_Tiny_ಕೋಷ್ಟಕ
                                        ? m_case_Decoder_ನಮೂದು_ಅಗಲ
                                        : VL_ಪದಗಳು_I(m_case_Decoder_ನಮೂದು_ಅಗಲ) * VL_EDATASIZE;
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ : m_case_Decoder_ದಾಖಲೆಗಳು) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhs_ಅಗಲ = lhs_ದಾಖಲೆ.lhsp->ಅಗಲ();
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ lhs_ಸ್ಥಾನಾಂತರ = lhs_ದಾಖಲೆ.ಸ್ಥಾನಾಂತರ;

            // Broadcast the pre-default assignment
            ಆದರೆ (lhs_ದಾಖಲೆ.preDefaultp) {
                AstConst* ಸ್ಥಿರಾಂಕ rhsp = VN_AS(lhs_ದಾಖಲೆ.preDefaultp->rhsp(), Const);
                ಗಾಗಿ (uint32_t ಸೂಚ್ಯಂಕ = 0; ಸೂಚ್ಯಂಕ < ಕೋಷ್ಟಕ_Entries; ++ಸೂಚ್ಯಂಕ) {
                    ಸ್ಥಿರಾಂಕ uint32_t ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ = ಸೂಚ್ಯಂಕ * ನಮೂದು_ಅಗಲ + lhs_ಸ್ಥಾನಾಂತರ;
                    tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ, lhs_ಅಗಲ);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            ಗಾಗಿ (Ast_Case_ಅಂಶ* cip = VN_AS(nodep->itemsp()->lastp(), Case_ಅಂಶ); cip;
                 cip = VN_AS(cip->prevp(), Case_ಅಂಶ)) {
                // Find the RHS in this case
                AstConst* ಸ್ಥಿರಾಂಕ rhsp = [&]() -> AstConst* {
                    ಗಾಗಿ (Ast_ಗಂಟು* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ ap = VN_AS(stmtp, ಗಂಟು_ನಿಯೋಜಿಸು);
                        ಆದರೆ (lhs_ದಾಖಲೆ.lhsp->same_ವೃಕ್ಷ(ap->lhsp())) ಹಿಂತಿರುಗಿ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    ಹಿಂತಿರುಗಿ VN_AS(lhs_ದಾಖಲೆ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                ಆದರೆ (cip->isDefault()) {
                    ಗಾಗಿ (uint32_t ಸೂಚ್ಯಂಕ = 0; ಸೂಚ್ಯಂಕ < ಕೋಷ್ಟಕ_Entries; ++ಸೂಚ್ಯಂಕ) {
                        ಸ್ಥಿರಾಂಕ uint32_t ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ = ಸೂಚ್ಯಂಕ * ನಮೂದು_ಅಗಲ + lhs_ಸ್ಥಾನಾಂತರ;
                        tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ, lhs_ಅಗಲ);
                    }
                    ಮುಂದುವರಿ;
                }

                // Iterate case conditions in reverse order
                ಗಾಗಿ (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    ಆದರೆ (never_ಅಂಶ(nodep, condp)) ಮುಂದುವರಿ;  // If item never matches, ignore it
                    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ_ನಮೂನೆ(nodep, condp);
                    ಸ್ಥಿರಾಂಕ uint32_t ಹೊಂದಿಕೆ_ಮುಖವಾಡ = ಹೊಂದಿಕೆ.ಮೊದಲ.ಗಮ್ಯ_UInt();
                    ಸ್ಥಿರಾಂಕ uint32_t ಹೊಂದಿಕೆ_ಬಿಟ್ಟುಗಳು = ಹೊಂದಿಕೆ.ಎರಡನೇ.ಗಮ್ಯ_UInt();
                    ಸ್ಥಿರಾಂಕ uint32_t inverse_ಮುಖವಾಡ = ~ಹೊಂದಿಕೆ_ಮುಖವಾಡ & ((1U << condp->ಅಗಲ()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    ಗಾಗಿ (uint32_t i = inverse_ಮುಖವಾಡ; ನಿಜ; i = (i - 1) & inverse_ಮುಖವಾಡ) {
                        ಸ್ಥಿರಾಂಕ uint32_t ಸೂಚ್ಯಂಕ = i | ಹೊಂದಿಕೆ_ಬಿಟ್ಟುಗಳು;
                        ಸ್ಥಿರಾಂಕ uint32_t ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ = ಸೂಚ್ಯಂಕ * ನಮೂದು_ಅಗಲ + lhs_ಸ್ಥಾನಾಂತರ;
                        tablep->ಅಂಕೆ().opSelInto(rhsp->ಅಂಕೆ(), ಕೋಷ್ಟಕ_ಸ್ಥಾನಾಂತರ, lhs_ಅಗಲ);
                        ಆದರೆ (!i) ನಿಲ್ಲಿಸು;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_Refp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
            ಆದರೆ (is_Tiny_ಕೋಷ್ಟಕ) ಹಿಂತಿರುಗಿ tablep;
            AstVarRef* ಸ್ಥಿರಾಂಕ refp = V3ConstPool::ಕಂಡುಹಿಡಿ(tablep);
            VL_DO_DANGLING(tablep->delete_ವೃಕ್ಷ(), tablep);  // V3ConstPool::find clones
            ಹಿಂತಿರುಗಿ refp;
        }();

        // Create the lookup index
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ caseExprp
            = ಹೊಸ AstExtend{flp, nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), 32};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ scalep = ಹೊಸ AstConst{flp, ನಮೂದು_ಅಗಲ};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_Lsbp = ಹೊಸ AstMul{flp, scalep, caseExprp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_Selp
            = ಹೊಸ AstSel{flp, ಕೋಷ್ಟಕ_Refp, ಕೋಷ್ಟಕ_Lsbp, static_cast<ಪೂರ್ಣಾಂಕ>(m_case_Decoder_ನಮೂದು_ಅಗಲ)};

        // Connect outputs
        ಹಿಂತಿರುಗಿ connectDecoderOutputs(nodep, ಕೋಷ್ಟಕ_Selp, m_ಕೋಷ್ಟಕ_ಹೊರಹೋಗು_Tmps);
    }

    Ast_ಗಂಟು_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::ಸದಿಶ<std::pair<AstConst*, Ast_ಗಂಟು*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        ಗಾಗಿ (Ast_Case_ಅಂಶ* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_ಅಂಶ)) {
            ಆದರೆ (cip->isDefault()) {
                clauses.emplace_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ, cip->stmtsp());
                ಮುಂದುವರಿ;
            }
            ಗಾಗಿ (Ast_ಗಂಟು* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ಸ್ಥಿರಾಂಕ iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                ಆದರೆ (never_ಅಂಶ(nodep, iconstp)) ಮುಂದುವರಿ;
                clauses.emplace_ಹಿಂಭಾಗ(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        ಆದರೆ (clauses.ಹಿಂಭಾಗ().ಮೊದಲ && !provenExhaustive) clauses.emplace_ಹಿಂಭಾಗ(ಶೂನ್ಯಸೂಚಕ, ಶೂನ್ಯಸೂಚಕ);

        // Number of entries in decoder table
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ decoderEnries = clauses.ಗಾತ್ರ_2();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ cond_ಅಗಲ = nodep->exprp()->ಅಗಲ();
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಹೊಂದಿಕೆ_ಅಗಲ = 2 * VL_ಪದಗಳು_I(cond_ಅಗಲ) * VL_EDATASIZE;
        AstConst* ಸ್ಥಿರಾಂಕ matchp
            = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{}, decoderEnries * ಹೊಂದಿಕೆ_ಅಗಲ, 0};
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < decoderEnries; ++i) {
            ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ನಮೂದು_Lsb = i * ಹೊಂದಿಕೆ_ಅಗಲ;

            // If the entry has a condition, use it's match bits and mask
            ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ condp = clauses[i].ಮೊದಲ) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ_ನಮೂನೆ(nodep, condp);
                matchp->ಅಂಕೆ().opSelInto(ಹೊಂದಿಕೆ.ಮೊದಲ, ನಮೂದು_Lsb, cond_ಅಗಲ);
                matchp->ಅಂಕೆ().opSelInto(ಹೊಂದಿಕೆ.ಎರಡನೇ, ನಮೂದು_Lsb + ಹೊಂದಿಕೆ_ಅಗಲ / 2, cond_ಅಗಲ);
                ಮುಂದುವರಿ;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ_Zero{flp, cond_ಅಗಲ, 0};
            matchp->ಅಂಕೆ().opSelInto(ಅಂಕೆ_Zero, ನಮೂದು_Lsb, cond_ಅಗಲ);
            matchp->ಅಂಕೆ().opSelInto(ಅಂಕೆ_Zero, ನಮೂದು_Lsb + ಹೊಂದಿಕೆ_ಅಗಲ / 2, cond_ಅಗಲ);
        }

        // Create the table initializer
        AstRange* ಸ್ಥಿರಾಂಕ rangep = ಹೊಸ AstRange{flp, decoderEnries - 1, 0};
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ನಮೂದು_Dtypep = nodep->ಕಂಡುಹಿಡಿ_ಬಿಟ್_DType(
            m_case_Decoder_ನಮೂದು_ಅಗಲ, m_case_Decoder_ನಮೂದು_ಅಗಲ, VSigning::UNSIGNED);
        Ast_ಗಂಟು_DType* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_Dtypep = ಹೊಸ AstUnpackArrayDType{flp, ನಮೂದು_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->ಕೂಡಿಸು_Typesp(ಕೋಷ್ಟಕ_Dtypep);
        AstConsArray* ಸ್ಥಿರಾಂಕ tablep = ಹೊಸ AstConsArray{flp, ಕೋಷ್ಟಕ_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        ಗಾಗಿ (ಪೂರ್ಣಾಂಕ i = 0; i < decoderEnries; ++i) {
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtsp = clauses[i].ಎರಡನೇ;
            AstConst* ಸ್ಥಿರಾಂಕ entryp = ಹೊಸ AstConst{flp, AstConst::Widthed_ಮೌಲ್ಯ{},
                                                  static_cast<ಪೂರ್ಣಾಂಕ>(m_case_Decoder_ನಮೂದು_ಅಗಲ), 0};
            ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Lhs_ದಾಖಲೆ& lhs_ದಾಖಲೆ : m_case_Decoder_ದಾಖಲೆಗಳು) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ lhsp = lhs_ದಾಖಲೆ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = ಶೂನ್ಯಸೂಚಕ;
                ಗಾಗಿ (Ast_ಗಂಟು* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ಗಂಟು_ನಿಯೋಜಿಸು* ಸ್ಥಿರಾಂಕ assignp = VN_AS(stmtp, ಗಂಟು_ನಿಯೋಜಿಸು);
                    ಆದರೆ (!lhsp->same_ವೃಕ್ಷ(assignp->lhsp())) ಮುಂದುವರಿ;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    ನಿಲ್ಲಿಸು;
                }
                // Not assigned in this clause, so use the pre-assigned default
                ಆದರೆ (!valConstp) {
                    UASSERT_OBJ(lhs_ದಾಖಲೆ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_ದಾಖಲೆ.preDefaultp->rhsp(), Const);
                }
                entryp->ಅಂಕೆ().opSelInto(valConstp->ಅಂಕೆ(), lhs_ದಾಖಲೆ.ಸ್ಥಾನಾಂತರ, lhsp->ಅಗಲ());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ಸ್ಥಿರಾಂಕ ಹೊಂದಿಕೆ_Refp = V3ConstPool::ಕಂಡುಹಿಡಿ(matchp);
        AstVarRef* ಸ್ಥಿರಾಂಕ ಕೋಷ್ಟಕ_Refp = V3ConstPool::ಕಂಡುಹಿಡಿ(tablep);
        VL_DO_DANGLING(matchp->delete_ವೃಕ್ಷ(), matchp);
        VL_DO_DANGLING(tablep->delete_ವೃಕ್ಷ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ caseExprp = nodep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        Ast_ಹೊಂದಿಕೆ_Masked* ಸ್ಥಿರಾಂಕ indexp = ಹೊಸ Ast_ಹೊಂದಿಕೆ_Masked{flp, caseExprp, ಹೊಂದಿಕೆ_Refp};
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ entryp = ಹೊಸ AstArraySel{flp, ಕೋಷ್ಟಕ_Refp, indexp};

        ಹಿಂತಿರುಗಿ connectDecoderOutputs(nodep, entryp, m_decoder_ಹೊರಹೋಗು_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCaseFastRecurse(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* cexprp, ಪೂರ್ಣಾಂಕ msb, uint32_t ಊರ್ಧ್ವ_ಮೌಲ್ಯ) ಸ್ಥಿರಾಂಕ {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        ಆದರೆ (msb < 0) ಹಿಂತಿರುಗಿ m_caseDetails.ದಾಖಲೆಗಳು[ಊರ್ಧ್ವ_ಮೌಲ್ಯ].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ಸ್ಥಿರಾಂಕ uint32_t ಊರ್ಧ್ವ_Value0 = ಊರ್ಧ್ವ_ಮೌಲ್ಯ;
        ಸ್ಥಿರಾಂಕ uint32_t ಊರ್ಧ್ವ_Value1 = ಊರ್ಧ್ವ_ಮೌಲ್ಯ | (1UL << msb);
        Ast_ಗಂಟು* tree0p = convertCaseFastRecurse(cexprp, msb - 1, ಊರ್ಧ್ವ_Value0);
        Ast_ಗಂಟು* tree1p = convertCaseFastRecurse(cexprp, msb - 1, ಊರ್ಧ್ವ_Value1);

        // If same logic on both sides, we can just return one of them
        ಆದರೆ (tree0p == tree1p) ಹಿಂತಿರುಗಿ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            ತಾರ್ಕಿಕ same = ನಿಜ;
            ಗಾಗಿ (uint32_t a = ಊರ್ಧ್ವ_Value0, b = ಊರ್ಧ್ವ_Value1; a < ಊರ್ಧ್ವ_Value1; ++a, ++b) {
                ಆದರೆ (m_caseDetails.ದಾಖಲೆಗಳು[a].stmtsp != m_caseDetails.ದಾಖಲೆಗಳು[b].stmtsp) {
                    same = ಸುಳ್ಳು;
                    ನಿಲ್ಲಿಸು;
                }
            }
            ಆದರೆ (same) {
                VL_DO_DANGLING(tree1p->delete_ವೃಕ್ಷ(), tree1p);
                ಹಿಂತಿರುಗಿ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        ಆದರೆ (tree0p && tree0p->backp()) tree0p = tree0p->clone_ವೃಕ್ಷ(ನಿಜ);
        ಆದರೆ (tree1p && tree1p->backp()) tree1p = tree1p->clone_ವೃಕ್ಷ(ನಿಜ);
        // Create the If statement
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = cexprp->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = ಹೊಸ AstSel{flp, cexprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು), msb, 1};
        AstIf* ಸ್ಥಿರಾಂಕ ifp = ಹೊಸ AstIf{flp, condp, tree1p, tree0p};
        ಹಿಂತಿರುಗಿ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ case_ಅಗಲ = nodep->exprp()->ಅಗಲ();
        Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ ifrootp = convertCaseFastRecurse(nodep->exprp(), case_ಅಗಲ - 1, 0UL);
        ಹಿಂತಿರುಗಿ ifrootp && ifrootp->backp() ? ifrootp->clone_ವೃಕ್ಷ(ನಿಜ) : ifrootp;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newCaseCompare(ಸ್ಥಿರಾಂಕ AstCase* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಅಂಶ_Exprp) {
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = ಅಂಶ_Exprp->fileline();
        ಆದರೆ (never_ಅಂಶ(casep, ಅಂಶ_Exprp)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಅಂಶ_Exprp), ಅಂಶ_Exprp);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}};
        }

        ಆದರೆ (AstCond* ಸ್ಥಿರಾಂಕ condp = VN_CAST(ಅಂಶ_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ testp = condp->condp()->unlink_Fr_ಹಿಂಭಾಗ();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_ಹಿಂಭಾಗ());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(ಅಂಶ_Exprp), ಅಂಶ_Exprp, condp);
            ಹಿಂತಿರುಗಿ ಹೊಸ AstCond{flp, testp, thenp, elsep};
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ caseExprp = casep->exprp()->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ಸುಳ್ಳು);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        ಆದರೆ (AstInsideRange* ಸ್ಥಿರಾಂಕ ಅಂಶ_Rangep = VN_CAST(ಅಂಶ_Exprp, InsideRange)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ resultp = ಅಂಶ_Rangep->newAndFromInside(
                caseExprp, ಅಂಶ_Rangep->lhsp()->unlink_Fr_ಹಿಂಭಾಗ(), ಅಂಶ_Rangep->rhsp()->unlink_Fr_ಹಿಂಭಾಗ());
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(ಅಂಶ_Exprp), ಅಂಶ_Exprp, ಅಂಶ_Rangep);
            ಹಿಂತಿರುಗಿ resultp;
        }

        ಆದರೆ (AstConst* ಸ್ಥಿರಾಂಕ ಅಂಶ_Constp = VN_CAST(ಅಂಶ_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            ಆದರೆ (ಅಂಶ_Constp->ಅಂಕೆ().isFourState()) {
                ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ಹೊಂದಿಕೆ = ಹೊಂದಿಕೆ_ನಮೂನೆ(casep, ಅಂಶ_Constp);
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಹೊಂದಿಕೆ_ಮುಖವಾಡ = ಹೊಂದಿಕೆ.ಮೊದಲ;
                ಸ್ಥಿರಾಂಕ V3_ಸಂಖ್ಯೆ& ಹೊಂದಿಕೆ_ಬಿಟ್ಟುಗಳು = ಹೊಂದಿಕೆ.ಎರಡನೇ;
                VL_DO_DANGLING2(ತಳ್ಳು_Deletep(ಅಂಶ_Exprp), ಅಂಶ_Exprp, ಅಂಶ_Constp);
                ಹಿಂತಿರುಗಿ AstEq::newTyped(flp, ಹೊಸ AstConst{flp, ಹೊಂದಿಕೆ_ಬಿಟ್ಟುಗಳು},
                                       ಹೊಸ AstAnd{flp, caseExprp, ಹೊಸ AstConst{flp, ಹೊಂದಿಕೆ_ಮುಖವಾಡ}});
            }
        }

        ಹಿಂತಿರುಗಿ AstEq::newTyped(flp, caseExprp, ಅಂಶ_Exprp);
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
        ತಾರ್ಕಿಕ hasDefault = ಸುಳ್ಳು;
        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {

            File_ಸಾಲು* ಸ್ಥಿರಾಂಕ flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            ಆದರೆ (itemp->isDefault()) {
                itemp->ಕೂಡಿಸು_Condsp(ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_True{}});
                hasDefault = ನಿಜ;
                ಮುಂದುವರಿ;
            }

            // Regular clause. Construct the condition expression.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newCondp = ಶೂನ್ಯಸೂಚಕ;
            ಗಾಗಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *ಅಂಶ_Exprp = itemp->condsp(), *nextp; ಅಂಶ_Exprp; ಅಂಶ_Exprp = nextp) {
                nextp = VN_AS(ಅಂಶ_Exprp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                ಅಂಶ_Exprp->unlink_Fr_ಹಿಂಭಾಗ();

                // If case never matches, ignore it
                ಆದರೆ (never_ಅಂಶ(nodep, ಅಂಶ_Exprp)) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(ಅಂಶ_Exprp), ಅಂಶ_Exprp);
                    ಮುಂದುವರಿ;
                }

                // Compute the term to add to the condition expression
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ termp = newCaseCompare(nodep, ಅಂಶ_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? ಹೊಸ AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            ಆದರೆ (!newCondp) newCondp = ಹೊಸ AstConst{flp, AstConst::ಬಿಟ್_False{}};
            itemp->ಕೂಡಿಸು_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        ಆದರೆ (!hasDefault) {
            ಆದರೆ (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_ಅಂಶ* ಸ್ಥಿರಾಂಕ lastp = VN_AS(nodep->itemsp()->lastp(), Case_ಅಂಶ);
                ತಳ್ಳು_Deletep(lastp->condsp()->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ());
                lastp->ಕೂಡಿಸು_Condsp(ಹೊಸ AstConst{lastp->fileline(), AstConst::ಬಿಟ್_True{}});
            } ಇಲ್ಲದಿದ್ದರೆ {
                // Otherwise needs an explicit empty default
                nodep->ಕೂಡಿಸು_Itemsp(ಹೊಸ Ast_Case_ಅಂಶ{
                    nodep->fileline(), ಹೊಸ AstConst{nodep->fileline(), AstConst::ಬಿಟ್_True{}},
                    ಶೂನ್ಯಸೂಚಕ});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        ಪೂರ್ಣಾಂಕ ಆಳ = 0;
        AstIf* grouprootp = ಶೂನ್ಯಸೂಚಕ;
        AstIf* groupnextp = ಶೂನ್ಯಸೂಚಕ;
        AstIf* itemnextp = ಶೂನ್ಯಸೂಚಕ;
        ಗಾಗಿ (Ast_Case_ಅಂಶ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ಅಂಶ)) {

            // Grab the statements from this item. May be empty.
            Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ istmtsp = itemp->stmtsp();
            ಆದರೆ (istmtsp) istmtsp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();

            // Expressioned clause
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ ifexprp = itemp->condsp()->unlink_Fr_ಹಿಂಭಾಗ();
            {  // Prepare for next group
                ಆದರೆ (++ಆಳ > CASE_ENCODER_ಗುಂಪು_ಆಳ) ಆಳ = 1;
                ಆದರೆ (ಆಳ == 1) {  // First group or starting new group
                    itemnextp = ಶೂನ್ಯಸೂಚಕ;
                    AstIf* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{itemp->fileline(), ifexprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)};
                    ಆದರೆ (groupnextp) {
                        groupnextp->ಕೂಡಿಸು_Elsesp(newp);
                    } ಇಲ್ಲದಿದ್ದರೆ {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } ಇಲ್ಲದಿದ್ದರೆ {  // Continue group, modify if condition to OR in this new condition
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಸ್ಥಿರಾಂಕ condp = groupnextp->condp()->unlink_Fr_ಹಿಂಭಾಗ();
                    groupnextp->condp(
                        ಹೊಸ AstOr{ifexprp->fileline(), condp, ifexprp->clone_ವೃಕ್ಷ_ಶುದ್ಧ(ನಿಜ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                ಆದರೆ (ಆಳ == CASE_ENCODER_ಗುಂಪು_ಆಳ) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_ವೃಕ್ಷ(), itemexprp);
                    itemexprp = ಹೊಸ AstConst{itemp->fileline(), AstConst::ಬಿಟ್_True{}};
                }
                AstIf* ಸ್ಥಿರಾಂಕ newp = ಹೊಸ AstIf{itemp->fileline(), itemexprp, istmtsp};
                ಆದರೆ (itemnextp) {
                    itemnextp->ಕೂಡಿಸು_Elsesp(newp);
                } ಇಲ್ಲದಿದ್ದರೆ {
                    groupnextp->ಕೂಡಿಸು_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        ಹಿಂತಿರುಗಿ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ಗಂಟು* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ use_ಕೋಷ್ಟಕ = [&]() {
            // Not if disabled
            ಆದರೆ (!v3Global.opt.f_Case_ಕೋಷ್ಟಕ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Not if analysis tells us we can't
            ಆದರೆ (!m_case_ಕೋಷ್ಟಕ_ಅಗಲ) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            ಆದರೆ (m_case_ಕೋಷ್ಟಕ_ಅಗಲ <= CASE_ಕೋಷ್ಟಕ_TINY_ಬಿಟ್ಟುಗಳು) ಹಿಂತಿರುಗಿ ನಿಜ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t branches = std::min<ಗಾತ್ರ_t>(nodep->exprp()->ಅಗಲ(), m_caseNConditions);
            ಆದರೆ (branches < CASE_ಕೋಷ್ಟಕ_MIN_BRANCHES) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }();
        ಆದರೆ (use_ಕೋಷ್ಟಕ) ಹಿಂತಿರುಗಿ convert_Case_ಕೋಷ್ಟಕ(nodep);

        // Determine if we should use the decoder method.
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ useDecoder = [&]() {
            // Not if disabled
            ಆದರೆ (!v3Global.opt.fCaseDecoder()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Not if not a decoder pattern
            ಆದರೆ (m_case_Decoder_ದಾಖಲೆಗಳು.ಖಾಲಿಗಳು()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t branches = std::min<ಗಾತ್ರ_t>(nodep->exprp()->ಅಗಲ(), m_caseNConditions);
            ಆದರೆ (branches < CASE_ಕೋಷ್ಟಕ_MIN_BRANCHES) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }();
        ಆದರೆ (useDecoder) ಹಿಂತಿರುಗಿ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ಸ್ಥಿರಾಂಕ ತಾರ್ಕಿಕ use_Fast_ಬಿಟ್_ವೃಕ್ಷ = [&]() {
            // Not if disabled
            ಆದರೆ (!v3Global.opt.f_Case_ವೃಕ್ಷ()) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Can't do it without the detailed analysis
            ಆದರೆ (!m_caseDetailsValid) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Can't do it if not exhaustive
            ಆದರೆ (!m_caseDetails.exhaustive) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Not worth doing if there are few conditions
            ಆದರೆ (m_caseNConditions <= 3) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Avoid e.g. priority expanders from going crazy in expansion
            ಸ್ಥಿರಾಂಕ ಗಾತ್ರ_t case_ಅಗಲ = nodep->exprp()->ಅಗಲ();
            ಆದರೆ (case_ಅಗಲ >= 8 && m_caseNConditions <= (case_ಅಗಲ + 1)) ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
            // Otherwise use the bit tree
            ಹಿಂತಿರುಗಿ ನಿಜ;
        }();
        ಆದರೆ (use_Fast_ಬಿಟ್_ವೃಕ್ಷ) ಹಿಂತಿರುಗಿ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        ಆದರೆ (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_ಸಂಕೇತ::LATCH, ನಿಜ);
        }
        ಹಿಂತಿರುಗಿ convertCaseGeneric(nodep);
    }

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstCase* nodep) override {
        UASSERT_OBJ(nodep->exprp()->is_ಶುದ್ಧ(), nodep,
                    "Impure case expression should have been removed by V3LiftExpr");

        CaseLintVisitor::apply(nodep);

        // Convert any children first
        iterateChildren(nodep);

        // Analyze this case statement
        analyzeCase(nodep);

        // Take the 'notParallelp' statements under the case statement created by V3Assert.
        // If the statement was proven to have no overlaps and all cases covered,
        // it can be removed. Otherwise insert the assertion after the case statement.
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_ಹಿಂಭಾಗ_With_ಮುಂದಿನ();
            ಆದರೆ (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_ವೃಕ್ಷ(), stmtp);
            } ಇಲ್ಲದಿದ್ದರೆ {
                nodep->ಕೂಡಿಸು_ಮುಂದಿನ_ಇಲ್ಲಿ(stmtp);
            }
        }

        // Convert the case statement and replace the original
        ಆದರೆ (Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            nodep->unlink_Fr_ಹಿಂಭಾಗ();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು* nodep) override { iterateChildren(nodep); }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    ಸ್ಪಷ್ಟ CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases table normal", m_stats.case_ಕೋಷ್ಟಕ_ಸಾಮಾನ್ಯ);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases table tiny", m_stats.case_ಕೋಷ್ಟಕ_Tiny);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::ಕೂಡಿಸು_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

ಗಾತ್ರ_t CaseVisitor::Lhs_ದಾಖಲೆ::s_ಮುಂದಿನ_Id = 0;

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
