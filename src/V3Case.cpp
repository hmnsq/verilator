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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Case.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ CaseLintVisitor final : ਜਨਤਕ VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    ਅਚਲ Ast_ਗੰਢ* m_casep = ਖਾਲੀ_ਸੂਚਕ;

    // METHODS
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Case_ਵਸਤੂ>
    ਸਥਿਰ ਖਾਲੀ detect_ਗੁਣਜ_Defaults(Case_ਵਸਤੂ* itemsp) {
        ਤਾਰਕਿਕ ਪ੍ਰਾਪਤੀ_Default = ਝੂਠ;
        ਲਈ (Case_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<Case_ਵਸਤੂ>(itemp->nextp())) {
            ਜੇ (!itemp->isDefault()) ਜਾਰੀ;
            ਜੇ (ਪ੍ਰਾਪਤੀ_Default) itemp->v3error("Multiple default statements in case statement.");
            ਪ੍ਰਾਪਤੀ_Default = ਸੱਚ;
        }
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Case_ਵਸਤੂ>
    ਖਾਲੀ checkXZinNonCaseX(Ast_ਗੰਢ* casep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Case_ਵਸਤੂ* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        ਲਈ (Case_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<Case_ਵਸਤੂ>(itemp->nextp())) {
            iterate_And_ਅਗਲਾ_Const_Null(itemp->condsp());
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstGenCase* nodep) override {
        // Detect multiple defaults
        detect_ਗੁਣਜ_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        ਜੇ (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detect_ਗੁਣਜ_Defaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        ਜੇ (!nodep->ਅੰਕ().isFourState()) ਵਾਪਸ;

        // Error if generate case
        ਜੇ (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            ਵਾਪਸ;
        }

        // Otherwise must be a case statement
        ਅਚਲ AstCase* ਅਚਲ casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        ਜੇ (casep->casex()) ਵਾਪਸ;

        ਜੇ (casep->casez() || casep->caseInside()) {
            ਜੇ (nodep->ਅੰਕ().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            ਵਾਪਸ;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    ਸਪਸ਼ਟ CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = ਮੂਲ;

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    ਸਥਿਰ ਖਾਲੀ apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

ਵਰਗ CaseVisitor final : ਜਨਤਕ VNVisitor {
    // Maximum width for detailed analysis
    constexpr ਸਥਿਰ ਪੂਰਨ CASE_DETAILS_MAX_ਚੌੜਾਈ = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr ਸਥਿਰ ਪੂਰਨ CASE_ENCODER_ਸਮੂਹ_ਡੂੰਘਾਈ = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr ਸਥਿਰ ਮਾਪ_t CASE_ਸਾਰਣੀ_TINY_ਬਿੱਟ = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr ਸਥਿਰ ਮਾਪ_t CASE_ਸਾਰਣੀ_MAX_ਬਿੱਟ = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr ਸਥਿਰ ਮਾਪ_t CASE_ਸਾਰਣੀ_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    ਬਣਤਰ Case_ਰਿਕਾਰਡ final {
        Ast_Case_ਵਸਤੂ* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        Ast_ਗੰਢ* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    ਬਣਤਰ Lhs_ਰਿਕਾਰਡ final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = ਖਾਲੀ_ਸੂਚਕ;  // LHS of the assignment
        Ast_ਗੰਢ_ਸੌਂਪੋ* preDefaultp = ਖਾਲੀ_ਸੂਚਕ;  // Default assignment *before the case statement*
        ਮਾਪ_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        ਮਾਪ_t n_Case_ਸੌਂਪੋ_Dlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        ਮਾਪ_t ਵਿੱਥ = 0;  // Offset in the table for this LHS

        ਸਥਿਰ ਮਾਪ_t s_ਅਗਲਾ_Id;  // Static unique Id counter
        ਮਾਪ_t id = ++s_ਅਗਲਾ_Id;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    ਬਣਤਰ Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 case_ਸਾਰਣੀ_ਸਾਧਾਰਨ;  // Cases using table method with normal table
        VDouble0 case_ਸਾਰਣੀ_Tiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    ਅਚਲ Ast_ਗੰਢ* m_alwaysp = ਖਾਲੀ_ਸੂਚਕ;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_ਸਾਰਣੀ_ਬਾਹਰਲਾ_Tmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoder_ਬਾਹਰਲਾ_Tmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    ਤਾਰਕਿਕ m_caseOpaque = ਝੂਠ;  // Case statement is opaque (non-packed, or non-const conditions)
    ਤਾਰਕਿਕ m_caseHasDefault = ਝੂਠ;  // Indicates the case statement has a default case
    ਮਾਪ_t m_case_NCase_ਚੀਜ਼ਾਂ = 0;  // Number of AstCaseItems in the case statement
    ਮਾਪ_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<Ast_ਗੰਢ_ਵਿਅੰਜਕ>, Lhs_ਰਿਕਾਰਡ> m_case_Lhs_ਰਿਕਾਰਡਾਂ;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::ਸਦਿਸ਼<Lhs_ਰਿਕਾਰਡ> m_case_Decoder_ਰਿਕਾਰਡਾਂ;
    ਮਾਪ_t m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ = 0;  // Width of each entry in the decoder table
    ਮਾਪ_t m_case_ਸਾਰਣੀ_ਚੌੜਾਈ = 0;  // Total width of the case table - 0 means can't optimize
    ਤਾਰਕਿਕ m_caseDetailsValid = ਝੂਠ;  // Indicates m_caseDetails is valid
    ਬਣਤਰ final {
        ਤਾਰਕਿਕ exhaustive = ਝੂਠ;  // Proven exhaustive
        ਤਾਰਕਿਕ exhaustiveOverEnumOnly = ਝੂਠ;  // Exhaustive over enum values only
        ਤਾਰਕਿਕ noOverlaps = ਝੂਠ;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<Case_ਰਿਕਾਰਡ, 1U << CASE_DETAILS_MAX_ਚੌੜਾਈ> ਰਿਕਾਰਡਾਂ;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    ਸਥਿਰ ਤਾਰਕਿਕ never_ਵਸਤੂ(ਅਚਲ AstCase* casep, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਸਤੂ_Exprp) {
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(ਵਸਤੂ_Exprp, Const);
        ਜੇ (!constp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (casep->casex() || casep->caseInside()) ਵਾਪਸ ਝੂਠ;
        ਜੇ (casep->casez()) ਵਾਪਸ constp->ਅੰਕ().isAnyX();
        ਵਾਪਸ constp->ਅੰਕ().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    ਸਥਿਰ std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ> ਮੇਲ_ਨਮੂਨਾ(ਅਚਲ AstCase* /*casep*/,
                                                      ਅਚਲ AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3_ਸੰਖਿਆ, V3_ਸੰਖਿਆ> pair_ਨਕਾਬ_ਬਿੱਟ{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->ਚੌੜਾਈ(), 0),
            std::forward_as_tuple(condp->fileline(), condp->ਚੌੜਾਈ(), 0)};
        pair_ਨਕਾਬ_ਬਿੱਟ.ਪਹਿਲਾ.op_ਬਿੱਟ_Non_XZ(condp->ਅੰਕ());
        pair_ਨਕਾਬ_ਬਿੱਟ.ਦੂਜਾ.op_ਬਿੱਟ_One(condp->ਅੰਕ());
        ਵਾਪਸ pair_ਨਕਾਬ_ਬਿੱਟ;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    ਸਥਿਰ Ast_ਗੰਢ_ਸੌਂਪੋ* check_Decoder_ਸੌਂਪੋ(Ast_ਗੰਢ* stmtp) {
        // Only Assign and AssignDly are supported
        ਜੇ (!VN_IS(stmtp, ਸੌਂਪੋ) && !VN_IS(stmtp, ਸੌਂਪੋ_Dly)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = VN_AS(stmtp, ਗੰਢ_ਸੌਂਪੋ);
        // Only if no timing control
        ਜੇ (assp->timingControlp()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Only if assigning a constant
        ਜੇ (!VN_IS(assp->rhsp(), Const)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Only if it's a packed value
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = assp->rhsp()->dtypep();
        ਜੇ (dtypep->isString() || dtypep->isDouble()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        ਜੇ (assp->lhsp()->exists([](AstVarRef* refp) { ਵਾਪਸ refp->access().isReadOrRW(); })) {
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // This is an assignment that fits the decoder pattern
        ਵਾਪਸ assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    ਤਾਰਕਿਕ analyze_Decoder_Case_ਵਸਤੂ(Ast_Case_ਵਸਤੂ* cip) {
        // AstVarScope::user1() -> bool: true if written to
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        ਲਈ (Ast_ਗੰਢ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = check_Decoder_ਸੌਂਪੋ(stmtp);
            ਜੇ (!assp) ਵਾਪਸ ਝੂਠ;
            // Must assign each LHS exactly once - RHS is Const
            ਅਚਲ ਤਾਰਕਿਕ ਗੁਣਜ_Assignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                ਵਾਪਸ refp->varScopep()->user1_ਸੈੱਟ_Once();
            });
            ਜੇ (ਗੁਣਜ_Assignments) ਵਾਪਸ ਝੂਠ;
            // Update LhsRecord
            Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ = m_case_Lhs_ਰਿਕਾਰਡਾਂ[*assp->lhsp()];
            ਜੇ (!lhs_ਰਿਕਾਰਡ.lhsp) lhs_ਰਿਕਾਰਡ.lhsp = assp->lhsp();
            lhs_ਰਿਕਾਰਡ.nCaseAssigns += VN_IS(assp, ਸੌਂਪੋ);
            lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys += VN_IS(assp, ਸੌਂਪੋ_Dly);
        }
        ਵਾਪਸ ਸੱਚ;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    ਸਥਿਰ ਅਚਲ AstEnumDType* getEnumCompletionCheckDType(ਅਚਲ AstCase* ਅਚਲ nodep) {
        ਜੇ (!nodep->uniquePragma() && !nodep->unique0Pragma()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstEnumDType* ਅਚਲ enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
        ਜੇ (!enumDtp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Case isn't enum
        ਅਚਲ AstBasicDType* ਅਚਲ basicp = enumDtp->subDTypep()->basicp();
        ਜੇ (!basicp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Not simple type (perhaps IEEE illegal)
        ਵਾਪਸ enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    ਤਾਰਕਿਕ checkExhaustiveEnum(ਅਚਲ AstCase* ਅਚਲ nodep, ਅਚਲ AstEnumDType* ਅਚਲ enump) {
        ਅਚਲ uint32_t ਅੰਕ_Cases = 1UL << nodep->exprp()->ਚੌੜਾਈ();
        ਤਾਰਕਿਕ fullyCovered = ਸੱਚ;
        string missing_ਚੀਜ਼ਾਂ;
        ਲਈ (Ast_Enum_ਵਸਤੂ* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), Enum_ਵਸਤੂ)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੇਲ = ਮੇਲ_ਨਮੂਨਾ(nodep, VN_AS(eip->valuep(), Const));
            ਅਚਲ uint32_t ਨਕਾਬ = ਮੇਲ.ਪਹਿਲਾ.ਮੰਜ਼ਿਲ_UInt();
            ਅਚਲ uint32_t ਬਿੱਟ = ਮੇਲ.ਦੂਜਾ.ਮੰਜ਼ਿਲ_UInt();

            // Check all cases to see if they cover this enum value/pattern
            ਲਈ (uint32_t i = 0; i < ਅੰਕ_Cases; ++i) {
                ਜੇ ((i & ਨਕਾਬ) != ਬਿੱਟ) ਜਾਰੀ;  // This case is not for this enum value
                ਜੇ (m_caseDetails.ਰਿਕਾਰਡਾਂ[i].itemp) ਜਾਰੀ;  // Covered case
                ਜੇ (!missing_ਚੀਜ਼ਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) missing_ਚੀਜ਼ਾਂ += ", ";
                missing_ਚੀਜ਼ਾਂ += eip->prettyNameQ();
                fullyCovered = ਝੂਠ;
                ਰੋਕੋ;
            }
        }
        ਜੇ (!missing_ਚੀਜ਼ਾਂ.ਖਾਲੀ_ਥਾਵਾਂ() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missing_ਚੀਜ਼ਾਂ);
        }
        ਵਾਪਸ fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    ਤਾਰਕਿਕ checkExhaustivePacked(AstCase* nodep) {
        ਅਚਲ uint32_t ਅੰਕ_Cases = 1UL << nodep->exprp()->ਚੌੜਾਈ();
        ਲਈ (uint32_t i = 0; i < ਅੰਕ_Cases; ++i) {
            ਜੇ (m_caseDetails.ਰਿਕਾਰਡਾਂ[i].itemp) ਜਾਰੀ;  // Covered case
            ਜੇ (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            ਵਾਪਸ ਝੂਠ;
        }

        // It's an exhaustive case statement
        ਵਾਪਸ ਸੱਚ;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    ਖਾਲੀ analyzeCaseDetails(AstCase* nodep) {
        ਅਚਲ uint32_t ਅੰਕ_ਮੁੱਲਾਂ = 1UL << nodep->exprp()->ਚੌੜਾਈ();
        // Clear case records
        ਲਈ (uint32_t i = 0; i < ਅੰਕ_ਮੁੱਲਾਂ; ++i) {
            m_caseDetails.ਰਿਕਾਰਡਾਂ[i].itemp = ਖਾਲੀ_ਸੂਚਕ;
            m_caseDetails.ਰਿਕਾਰਡਾਂ[i].constp = ਖਾਲੀ_ਸੂਚਕ;
            m_caseDetails.ਰਿਕਾਰਡਾਂ[i].stmtsp = ਖਾਲੀ_ਸੂਚਕ;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        ਤਾਰਕਿਕ reportedOverlap = ਝੂਠ;
        ਤਾਰਕਿਕ hasDefault = ਝੂਠ;
        m_caseDetails.noOverlaps = ਸੱਚ;
        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {

            // Default case
            ਜੇ (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                ਲਈ (uint32_t i = 0; i < ਅੰਕ_ਮੁੱਲਾਂ; ++i) {
                    Case_ਰਿਕਾਰਡ& case_ਰਿਕਾਰਡ = m_caseDetails.ਰਿਕਾਰਡਾਂ[i];
                    ਜੇ (!case_ਰਿਕਾਰਡ.itemp) {
                        case_ਰਿਕਾਰਡ.itemp = itemp;
                        case_ਰਿਕਾਰਡ.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = ਸੱਚ;
                ਜਾਰੀ;
            }

            ਲਈ (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                ਜੇ (never_ਵਸਤੂ(nodep, iconstp)) ਜਾਰੀ;

                ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੇਲ = ਮੇਲ_ਨਮੂਨਾ(nodep, iconstp);
                ਅਚਲ uint32_t ਨਕਾਬ = ਮੇਲ.ਪਹਿਲਾ.ਮੰਜ਼ਿਲ_UInt();
                ਅਚਲ uint32_t ਬਿੱਟ = ਮੇਲ.ਦੂਜਾ.ਮੰਜ਼ਿਲ_UInt();

                ਤਾਰਕਿਕ ਮਿਲਿਆ_New_Case = ਝੂਠ;
                ਅਚਲ AstConst* ਪਹਿਲਾ_Overlap_Constp = ਖਾਲੀ_ਸੂਚਕ;
                uint32_t ਪਹਿਲਾ_Overlap_ਮੁੱਲ = 0;
                ਲਈ (uint32_t i = 0; i < ਅੰਕ_ਮੁੱਲਾਂ; ++i) {
                    ਜੇ ((i & ਨਕਾਬ) != ਬਿੱਟ) ਜਾਰੀ;

                    Case_ਰਿਕਾਰਡ& case_ਰਿਕਾਰਡ = m_caseDetails.ਰਿਕਾਰਡਾਂ[i];

                    // If this is the first case that covers this value, record it
                    ਜੇ (!case_ਰਿਕਾਰਡ.itemp) {
                        case_ਰਿਕਾਰਡ.itemp = itemp;
                        case_ਰਿਕਾਰਡ.constp = iconstp;
                        case_ਰਿਕਾਰਡ.stmtsp = itemp->stmtsp();
                        ਮਿਲਿਆ_New_Case = ਸੱਚ;
                        ਜਾਰੀ;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    ਜੇ (case_ਰਿਕਾਰਡ.itemp == itemp) ਜਾਰੀ;

                    // Otherwise record the first overlapping case
                    ਜੇ (!ਪਹਿਲਾ_Overlap_Constp) {
                        ਪਹਿਲਾ_Overlap_Constp = case_ਰਿਕਾਰਡ.constp;
                        ਪਹਿਲਾ_Overlap_ਮੁੱਲ = i;
                        m_caseDetails.noOverlaps = ਝੂਠ;
                    }
                }

                // Only report first overlap
                ਜੇ (reportedOverlap || !ਪਹਿਲਾ_Overlap_Constp) ਜਾਰੀ;

                // Report first overlap
                ਜੇ (nodep->ਤਰਜੀਹ_Pragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    ਜੇ (!ਮਿਲਿਆ_New_Case) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << ਪਹਿਲਾ_Overlap_Constp->warnOther()
                                            << "... Location of previous condition\n"
                                            << ਪਹਿਲਾ_Overlap_Constp->warnContextPrimary());
                        reportedOverlap = ਸੱਚ;
                    }
                } ਨਹੀਂਤਾਂ {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream example_ਨਮੂਨਾ;
                    ਜੇ (iconstp->ਅੰਕ().isAnyXZ()) {
                        example_ਨਮੂਨਾ << " (example pattern 0x" << std::hex << ਪਹਿਲਾ_Overlap_ਮੁੱਲ
                                       << ")";
                    }
                    iconstp->v3warn(CASEOVERLAP,
                                    "Case conditions overlap"
                                        << example_ਨਮੂਨਾ.str() << "\n"
                                        << iconstp->warnContextPrimary() << '\n'
                                        << ਪਹਿਲਾ_Overlap_Constp->warnOther()
                                        << "... Location of overlapping condition\n"
                                        << ਪਹਿਲਾ_Overlap_Constp->warnContextSecondary());
                    reportedOverlap = ਸੱਚ;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = ਝੂਠ;
        m_caseDetails.exhaustive = hasDefault;
        ਜੇ (!hasDefault) {
            ਜੇ (ਅਚਲ AstEnumDType* ਅਚਲ enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                ਅਚਲ ਤਾਰਕਿਕ exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } ਨਹੀਂਤਾਂ {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = ਸੱਚ;
    }

    ਖਾਲੀ analyze_Decoder_ਨਮੂਨਾ(AstCase* nodep) {
        // Check each LHS record
        ਲਈ (ਸਵੈਚਾਲਿਤ it = m_case_Lhs_ਰਿਕਾਰਡਾਂ.cbegin(); it != m_case_Lhs_ਰਿਕਾਰਡਾਂ.cend();) {
            ਅਚਲ Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ = it->ਦੂਜਾ;

            // Delete records that have no assignments in any case item (only pre-defaults)
            ਜੇ (!lhs_ਰਿਕਾਰਡ.nCaseAssigns && !lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys) {
                it = m_case_Lhs_ਰਿਕਾਰਡਾਂ.erase(it);
                ਜਾਰੀ;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            ਜੇ (lhs_ਰਿਕਾਰਡ.nCaseAssigns && lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys) ਵਾਪਸ;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            ਜੇ (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                ਜੇ (lhs_ਰਿਕਾਰਡ.nCaseAssigns == m_case_NCase_ਚੀਜ਼ਾਂ) ਜਾਰੀ;
                ਜੇ (lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys == m_case_NCase_ਚੀਜ਼ਾਂ) ਜਾਰੀ;
            }

            // Otherwise it needs to have a pre-default assignment
            Ast_ਗੰਢ* ਅਚਲ preDefaultp = lhs_ਰਿਕਾਰਡ.preDefaultp;
            ਜੇ (!preDefaultp) ਵਾਪਸ;
            // And the pre-default needs to be the same type
            ਜੇ (lhs_ਰਿਕਾਰਡ.nCaseAssigns && !VN_IS(preDefaultp, ਸੌਂਪੋ)) ਵਾਪਸ;
            ਜੇ (lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys && !VN_IS(preDefaultp, ਸੌਂਪੋ_Dly)) ਵਾਪਸ;
        }
        // All cases check out, can optimize if there are some entries left
        ਜੇ (m_case_Lhs_ਰਿਕਾਰਡਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_case_Decoder_ਰਿਕਾਰਡਾਂ.reserve(m_case_Lhs_ਰਿਕਾਰਡਾਂ.ਮਾਪ());
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਵਸਤੂ_3 : m_case_Lhs_ਰਿਕਾਰਡਾਂ) m_case_Decoder_ਰਿਕਾਰਡਾਂ.emplace_ਪਿੱਠ(ਵਸਤੂ_3.ਦੂਜਾ);
        std::ਛਾਂਟੋ(m_case_Decoder_ਰਿਕਾਰਡਾਂ.begin(), m_case_Decoder_ਰਿਕਾਰਡਾਂ.ਸਿਰਾ(),
                  [](ਅਚਲ Lhs_ਰਿਕਾਰਡ& a, ਅਚਲ Lhs_ਰਿਕਾਰਡ& b) {
                      // Sort by width, then id
                      ਅਚਲ ਪੂਰਨ a_ਚੌੜਾਈ = a.lhsp->ਚੌੜਾਈ();
                      ਅਚਲ ਪੂਰਨ b_ਚੌੜਾਈ = b.lhsp->ਚੌੜਾਈ();
                      ਜੇ (a_ਚੌੜਾਈ != b_ਚੌੜਾਈ) ਵਾਪਸ a_ਚੌੜਾਈ < b_ਚੌੜਾਈ;
                      ਵਾਪਸ a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        ਅਚਲ ਪੂਰਨ case_ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਸਮਾਉਂਦਾ_ਹੱਦਬੰਦੀ = [&](ਮਾਪ_t ਇੰਦਰਾਜ_ਚੌੜਾਈ, ਮਾਪ_t max_ਚੌੜਾਈ) -> ਤਾਰਕਿਕ {
            ਮਾਪ_t ਕੁੱਲ_ਚੌੜਾਈ = ਇੰਦਰਾਜ_ਚੌੜਾਈ;
            // Multiply cases - iterative to avoid overflow
            ਲਈ (ਪੂਰਨ i = 0; i < case_ਚੌੜਾਈ; ++i) {
                ਕੁੱਲ_ਚੌੜਾਈ <<= 1;
                ਜੇ (ਕੁੱਲ_ਚੌੜਾਈ > max_ਚੌੜਾਈ) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ = 0;
        ਲਈ (Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ : m_case_Decoder_ਰਿਕਾਰਡਾਂ) {
            lhs_ਰਿਕਾਰਡ.ਵਿੱਥ = m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ;
            m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ += lhs_ਰਿਕਾਰਡ.lhsp->ਚੌੜਾਈ();
        }
        // If it fits, we will pack it tightly
        ਜੇ (ਸਮਾਉਂਦਾ_ਹੱਦਬੰਦੀ(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ, CASE_ਸਾਰਣੀ_TINY_ਬਿੱਟ)) {
            m_case_ਸਾਰਣੀ_ਚੌੜਾਈ = m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ << case_ਚੌੜਾਈ;  // Can optimize
            ਵਾਪਸ;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ = 0;
        ਲਈ (Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ : m_case_Decoder_ਰਿਕਾਰਡਾਂ) {
            ਅਚਲ ਮਾਪ_t ਚੌੜਾਈ = lhs_ਰਿਕਾਰਡ.lhsp->ਚੌੜਾਈ();
            ਅਚਲ ਮਾਪ_t lsb_ਸ਼ਬਦ = VL_BITWORD_E(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ);
            ਅਚਲ ਮਾਪ_t msb_ਸ਼ਬਦ = VL_BITWORD_E(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ + ਚੌੜਾਈ - 1);
            ਜੇ (lsb_ਸ਼ਬਦ != msb_ਸ਼ਬਦ) {
                m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ = VL_ਸ਼ਬਦ_I(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ) * VL_EDATASIZE;
            }
            lhs_ਰਿਕਾਰਡ.ਵਿੱਥ = m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ;
            m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ += ਚੌੜਾਈ;
        }
        // Check the table fits max size
        ਅਚਲ ਮਾਪ_t aligned_ਇੰਦਰਾਜ_ਚੌੜਾਈ = VL_ਸ਼ਬਦ_I(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ) * VL_EDATASIZE;
        ਜੇ (ਸਮਾਉਂਦਾ_ਹੱਦਬੰਦੀ(aligned_ਇੰਦਰਾਜ_ਚੌੜਾਈ, CASE_ਸਾਰਣੀ_MAX_ਬਿੱਟ)) {
            m_case_ਸਾਰਣੀ_ਚੌੜਾਈ = aligned_ਇੰਦਰਾਜ_ਚੌੜਾਈ << case_ਚੌੜਾਈ;  // Can optimize
            ਵਾਪਸ;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    ਖਾਲੀ analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = ਝੂਠ;
        m_caseHasDefault = ਝੂਠ;
        m_case_NCase_ਚੀਜ਼ਾਂ = 0;
        m_caseNConditions = 0;
        m_case_Decoder_ਰਿਕਾਰਡਾਂ.clear();
        m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ = 0;
        m_case_ਸਾਰਣੀ_ਚੌੜਾਈ = 0;
        m_case_Lhs_ਰਿਕਾਰਡਾਂ.clear();
        m_caseDetailsValid = ਝੂਠ;

        Ast_ਗੰਢ* ਅਚਲ caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        ਜੇ (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = ਸੱਚ;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
            ਲਈ (Ast_ਗੰਢ* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assp = check_Decoder_ਸੌਂਪੋ(prevp);
                ਜੇ (!assp) ਰੋਕੋ;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                ਅਚਲ ਤਾਰਕਿਕ ਗੁਣਜ_Assignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    ਵਾਪਸ refp->varScopep()->user1_ਸੈੱਟ_Once();
                });
                ਜੇ (ਗੁਣਜ_Assignments) ਰੋਕੋ;
                // Store pre-default assignment
                Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ = m_case_Lhs_ਰਿਕਾਰਡਾਂ[*assp->lhsp()];
                lhs_ਰਿਕਾਰਡ.lhsp = assp->lhsp();
                lhs_ਰਿਕਾਰਡ.preDefaultp = assp;
            }
        }

        // Check each case item
        ਤਾਰਕਿਕ canBeDecoder = ਸੱਚ;
        ਲਈ (Ast_Case_ਵਸਤੂ* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_ਵਸਤੂ)) {
            // Check conditions
            ਲਈ (Ast_ਗੰਢ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                ਜੇ (!never_ਵਸਤੂ(nodep, VN_AS(condp, ਗੰਢ_ਵਿਅੰਜਕ))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                ਜੇ (!VN_IS(condp, Const)) {
                    m_caseOpaque = ਸੱਚ;
                    canBeDecoder = ਝੂਠ;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            ਜੇ (cip->isDefault()) m_caseHasDefault = ਸੱਚ;
            // Count case items
            ++m_case_NCase_ਚੀਜ਼ਾਂ;
            // Check if it fits the decoder pattern, if still possible
            ਜੇ (canBeDecoder) canBeDecoder = analyze_Decoder_Case_ਵਸਤੂ(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        ਜੇ (m_caseOpaque) ਵਾਪਸ;

        // If small enough, analyse details
        ਜੇ (caseExprp->ਚੌੜਾਈ() <= CASE_DETAILS_MAX_ਚੌੜਾਈ) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        ਜੇ (canBeDecoder) analyze_Decoder_ਨਮੂਨਾ(nodep);
    }

    Ast_ਗੰਢ_Stmt* connectDecoderOutputs(AstCase* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, V3SharedTmps& tmps) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        // If there is only one LHS, just use the result
        ਜੇ (m_case_Decoder_ਰਿਕਾਰਡਾਂ.ਮਾਪ() == 1) {
            ਅਚਲ Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ = m_case_Decoder_ਰਿਕਾਰਡਾਂ[0];
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = lhs_ਰਿਕਾਰਡ.lhsp->ਚੌੜਾਈ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp
                = exprp->ਚੌੜਾਈ() == ਚੌੜਾਈ ? exprp : ਨਵਾਂ AstSel{flp, exprp, 0, ਚੌੜਾਈ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = lhs_ਰਿਕਾਰਡ.lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (lhs_ਰਿਕਾਰਡ.nCaseAssigns) {
                ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp};
            } ਨਹੀਂਤਾਂ ਜੇ (lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys) {
                ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{flp, lhsp, rhsp};
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* ਅਚਲ ਆਰਜ਼ੀ_Vscp = tmps.make(flp, m_scopep, m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਆਰਜ਼ੀ_Writep = ਨਵਾਂ AstVarRef{flp, ਆਰਜ਼ੀ_Vscp, VAccess::WRITE};
        Ast_ਗੰਢ_Stmt* ਅਚਲ resultp = ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਆਰਜ਼ੀ_Writep, exprp};

        // For each LHS, select out the result
        ਲਈ (ਅਚਲ Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ : m_case_Decoder_ਰਿਕਾਰਡਾਂ) {
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = lhs_ਰਿਕਾਰਡ.lhsp->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ lsb = lhs_ਰਿਕਾਰਡ.ਵਿੱਥ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਆਰਜ਼ੀ_Readp = ਨਵਾਂ AstVarRef{flp, ਆਰਜ਼ੀ_Vscp, VAccess::READ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstSel{flp, ਆਰਜ਼ੀ_Readp, lsb, ਚੌੜਾਈ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = lhs_ਰਿਕਾਰਡ.lhsp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
            ਜੇ (lhs_ਰਿਕਾਰਡ.nCaseAssigns) {
                resultp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, lhsp, rhsp});
            } ਨਹੀਂਤਾਂ ਜੇ (lhs_ਰਿਕਾਰਡ.n_Case_ਸੌਂਪੋ_Dlys) {
                resultp->ਜੋੜੋ_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ_Dly{flp, lhsp, rhsp});
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        ਵਾਪਸ resultp;
    }

    Ast_ਗੰਢ_Stmt* convert_Case_ਸਾਰਣੀ(AstCase* nodep) {
        // Create the table constant
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        AstConst* ਅਚਲ tablep
            = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, static_cast<ਪੂਰਨ>(m_case_ਸਾਰਣੀ_ਚੌੜਾਈ), 0};
        ਅਚਲ uint32_t ਸਾਰਣੀ_Entries = 1U << nodep->exprp()->ਚੌੜਾਈ();

        ਅਚਲ ਤਾਰਕਿਕ is_Tiny_ਸਾਰਣੀ = m_case_ਸਾਰਣੀ_ਚੌੜਾਈ <= CASE_ਸਾਰਣੀ_TINY_ਬਿੱਟ;
        ਜੇ (is_Tiny_ਸਾਰਣੀ) {
            ++m_stats.case_ਸਾਰਣੀ_Tiny;
        } ਨਹੀਂਤਾਂ {
            ++m_stats.case_ਸਾਰਣੀ_ਸਾਧਾਰਨ;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        ਅਚਲ uint32_t ਇੰਦਰਾਜ_ਚੌੜਾਈ = is_Tiny_ਸਾਰਣੀ
                                        ? m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ
                                        : VL_ਸ਼ਬਦ_I(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ) * VL_EDATASIZE;
        ਲਈ (ਅਚਲ Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ : m_case_Decoder_ਰਿਕਾਰਡਾਂ) {
            ਅਚਲ ਪੂਰਨ lhs_ਚੌੜਾਈ = lhs_ਰਿਕਾਰਡ.lhsp->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ lhs_ਵਿੱਥ = lhs_ਰਿਕਾਰਡ.ਵਿੱਥ;

            // Broadcast the pre-default assignment
            ਜੇ (lhs_ਰਿਕਾਰਡ.preDefaultp) {
                AstConst* ਅਚਲ rhsp = VN_AS(lhs_ਰਿਕਾਰਡ.preDefaultp->rhsp(), Const);
                ਲਈ (uint32_t ਸੂਚਕਾਂਕ = 0; ਸੂਚਕਾਂਕ < ਸਾਰਣੀ_Entries; ++ਸੂਚਕਾਂਕ) {
                    ਅਚਲ uint32_t ਸਾਰਣੀ_ਵਿੱਥ = ਸੂਚਕਾਂਕ * ਇੰਦਰਾਜ_ਚੌੜਾਈ + lhs_ਵਿੱਥ;
                    tablep->ਅੰਕ().opSelInto(rhsp->ਅੰਕ(), ਸਾਰਣੀ_ਵਿੱਥ, lhs_ਚੌੜਾਈ);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            ਲਈ (Ast_Case_ਵਸਤੂ* cip = VN_AS(nodep->itemsp()->lastp(), Case_ਵਸਤੂ); cip;
                 cip = VN_AS(cip->prevp(), Case_ਵਸਤੂ)) {
                // Find the RHS in this case
                AstConst* ਅਚਲ rhsp = [&]() -> AstConst* {
                    ਲਈ (Ast_ਗੰਢ* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ ap = VN_AS(stmtp, ਗੰਢ_ਸੌਂਪੋ);
                        ਜੇ (lhs_ਰਿਕਾਰਡ.lhsp->same_ਰੁੱਖ(ap->lhsp())) ਵਾਪਸ VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    ਵਾਪਸ VN_AS(lhs_ਰਿਕਾਰਡ.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                ਜੇ (cip->isDefault()) {
                    ਲਈ (uint32_t ਸੂਚਕਾਂਕ = 0; ਸੂਚਕਾਂਕ < ਸਾਰਣੀ_Entries; ++ਸੂਚਕਾਂਕ) {
                        ਅਚਲ uint32_t ਸਾਰਣੀ_ਵਿੱਥ = ਸੂਚਕਾਂਕ * ਇੰਦਰਾਜ_ਚੌੜਾਈ + lhs_ਵਿੱਥ;
                        tablep->ਅੰਕ().opSelInto(rhsp->ਅੰਕ(), ਸਾਰਣੀ_ਵਿੱਥ, lhs_ਚੌੜਾਈ);
                    }
                    ਜਾਰੀ;
                }

                // Iterate case conditions in reverse order
                ਲਈ (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    ਜੇ (never_ਵਸਤੂ(nodep, condp)) ਜਾਰੀ;  // If item never matches, ignore it
                    ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੇਲ = ਮੇਲ_ਨਮੂਨਾ(nodep, condp);
                    ਅਚਲ uint32_t ਮੇਲ_ਨਕਾਬ = ਮੇਲ.ਪਹਿਲਾ.ਮੰਜ਼ਿਲ_UInt();
                    ਅਚਲ uint32_t ਮੇਲ_ਬਿੱਟ = ਮੇਲ.ਦੂਜਾ.ਮੰਜ਼ਿਲ_UInt();
                    ਅਚਲ uint32_t inverse_ਨਕਾਬ = ~ਮੇਲ_ਨਕਾਬ & ((1U << condp->ਚੌੜਾਈ()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    ਲਈ (uint32_t i = inverse_ਨਕਾਬ; ਸੱਚ; i = (i - 1) & inverse_ਨਕਾਬ) {
                        ਅਚਲ uint32_t ਸੂਚਕਾਂਕ = i | ਮੇਲ_ਬਿੱਟ;
                        ਅਚਲ uint32_t ਸਾਰਣੀ_ਵਿੱਥ = ਸੂਚਕਾਂਕ * ਇੰਦਰਾਜ_ਚੌੜਾਈ + lhs_ਵਿੱਥ;
                        tablep->ਅੰਕ().opSelInto(rhsp->ਅੰਕ(), ਸਾਰਣੀ_ਵਿੱਥ, lhs_ਚੌੜਾਈ);
                        ਜੇ (!i) ਰੋਕੋ;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸਾਰਣੀ_Refp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
            ਜੇ (is_Tiny_ਸਾਰਣੀ) ਵਾਪਸ tablep;
            AstVarRef* ਅਚਲ refp = V3ConstPool::ਲੱਭੋ(tablep);
            VL_DO_DANGLING(tablep->delete_ਰੁੱਖ(), tablep);  // V3ConstPool::find clones
            ਵਾਪਸ refp;
        }();

        // Create the lookup index
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ caseExprp
            = ਨਵਾਂ AstExtend{flp, nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 32};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ scalep = ਨਵਾਂ AstConst{flp, ਇੰਦਰਾਜ_ਚੌੜਾਈ};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸਾਰਣੀ_Lsbp = ਨਵਾਂ AstMul{flp, scalep, caseExprp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸਾਰਣੀ_Selp
            = ਨਵਾਂ AstSel{flp, ਸਾਰਣੀ_Refp, ਸਾਰਣੀ_Lsbp, static_cast<ਪੂਰਨ>(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ)};

        // Connect outputs
        ਵਾਪਸ connectDecoderOutputs(nodep, ਸਾਰਣੀ_Selp, m_ਸਾਰਣੀ_ਬਾਹਰਲਾ_Tmps);
    }

    Ast_ਗੰਢ_Stmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::ਸਦਿਸ਼<std::pair<AstConst*, Ast_ਗੰਢ*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        ਲਈ (Ast_Case_ਵਸਤੂ* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), Case_ਵਸਤੂ)) {
            ਜੇ (cip->isDefault()) {
                clauses.emplace_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ, cip->stmtsp());
                ਜਾਰੀ;
            }
            ਲਈ (Ast_ਗੰਢ* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* ਅਚਲ iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                ਜੇ (never_ਵਸਤੂ(nodep, iconstp)) ਜਾਰੀ;
                clauses.emplace_ਪਿੱਠ(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        ਅਚਲ ਤਾਰਕਿਕ provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        ਜੇ (clauses.ਪਿੱਠ().ਪਹਿਲਾ && !provenExhaustive) clauses.emplace_ਪਿੱਠ(ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ);

        // Number of entries in decoder table
        ਅਚਲ ਪੂਰਨ decoderEnries = clauses.ਮਾਪ();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        ਅਚਲ ਪੂਰਨ cond_ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ ਮੇਲ_ਚੌੜਾਈ = 2 * VL_ਸ਼ਬਦ_I(cond_ਚੌੜਾਈ) * VL_EDATASIZE;
        AstConst* ਅਚਲ matchp
            = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, decoderEnries * ਮੇਲ_ਚੌੜਾਈ, 0};
        ਲਈ (ਪੂਰਨ i = 0; i < decoderEnries; ++i) {
            ਅਚਲ ਪੂਰਨ ਇੰਦਰਾਜ_Lsb = i * ਮੇਲ_ਚੌੜਾਈ;

            // If the entry has a condition, use it's match bits and mask
            ਜੇ (AstConst* ਅਚਲ condp = clauses[i].ਪਹਿਲਾ) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੇਲ = ਮੇਲ_ਨਮੂਨਾ(nodep, condp);
                matchp->ਅੰਕ().opSelInto(ਮੇਲ.ਪਹਿਲਾ, ਇੰਦਰਾਜ_Lsb, cond_ਚੌੜਾਈ);
                matchp->ਅੰਕ().opSelInto(ਮੇਲ.ਦੂਜਾ, ਇੰਦਰਾਜ_Lsb + ਮੇਲ_ਚੌੜਾਈ / 2, cond_ਚੌੜਾਈ);
                ਜਾਰੀ;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3_ਸੰਖਿਆ ਅੰਕ_Zero{flp, cond_ਚੌੜਾਈ, 0};
            matchp->ਅੰਕ().opSelInto(ਅੰਕ_Zero, ਇੰਦਰਾਜ_Lsb, cond_ਚੌੜਾਈ);
            matchp->ਅੰਕ().opSelInto(ਅੰਕ_Zero, ਇੰਦਰਾਜ_Lsb + ਮੇਲ_ਚੌੜਾਈ / 2, cond_ਚੌੜਾਈ);
        }

        // Create the table initializer
        AstRange* ਅਚਲ rangep = ਨਵਾਂ AstRange{flp, decoderEnries - 1, 0};
        Ast_ਗੰਢ_DType* ਅਚਲ ਇੰਦਰਾਜ_Dtypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(
            m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ, m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ, VSigning::UNSIGNED);
        Ast_ਗੰਢ_DType* ਅਚਲ ਸਾਰਣੀ_Dtypep = ਨਵਾਂ AstUnpackArrayDType{flp, ਇੰਦਰਾਜ_Dtypep, rangep};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(ਸਾਰਣੀ_Dtypep);
        AstConsArray* ਅਚਲ tablep = ਨਵਾਂ AstConsArray{flp, ਸਾਰਣੀ_Dtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        ਲਈ (ਪੂਰਨ i = 0; i < decoderEnries; ++i) {
            Ast_ਗੰਢ* ਅਚਲ stmtsp = clauses[i].ਦੂਜਾ;
            AstConst* ਅਚਲ entryp = ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{},
                                                  static_cast<ਪੂਰਨ>(m_case_Decoder_ਇੰਦਰਾਜ_ਚੌੜਾਈ), 0};
            ਲਈ (ਅਚਲ Lhs_ਰਿਕਾਰਡ& lhs_ਰਿਕਾਰਡ : m_case_Decoder_ਰਿਕਾਰਡਾਂ) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = lhs_ਰਿਕਾਰਡ.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (Ast_ਗੰਢ* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ assignp = VN_AS(stmtp, ਗੰਢ_ਸੌਂਪੋ);
                    ਜੇ (!lhsp->same_ਰੁੱਖ(assignp->lhsp())) ਜਾਰੀ;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    ਰੋਕੋ;
                }
                // Not assigned in this clause, so use the pre-assigned default
                ਜੇ (!valConstp) {
                    UASSERT_OBJ(lhs_ਰਿਕਾਰਡ.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhs_ਰਿਕਾਰਡ.preDefaultp->rhsp(), Const);
                }
                entryp->ਅੰਕ().opSelInto(valConstp->ਅੰਕ(), lhs_ਰਿਕਾਰਡ.ਵਿੱਥ, lhsp->ਚੌੜਾਈ());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* ਅਚਲ ਮੇਲ_Refp = V3ConstPool::ਲੱਭੋ(matchp);
        AstVarRef* ਅਚਲ ਸਾਰਣੀ_Refp = V3ConstPool::ਲੱਭੋ(tablep);
        VL_DO_DANGLING(matchp->delete_ਰੁੱਖ(), matchp);
        VL_DO_DANGLING(tablep->delete_ਰੁੱਖ(), tablep);

        // AstMatchMasked produces the index of the matching entry
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ caseExprp = nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਮੇਲ_Masked* ਅਚਲ indexp = ਨਵਾਂ Ast_ਮੇਲ_Masked{flp, caseExprp, ਮੇਲ_Refp};
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ entryp = ਨਵਾਂ AstArraySel{flp, ਸਾਰਣੀ_Refp, indexp};

        ਵਾਪਸ connectDecoderOutputs(nodep, entryp, m_decoder_ਬਾਹਰਲਾ_Tmps);
    }

    // TODO: should return AstNodeStmt after #6280
    Ast_ਗੰਢ* convertCaseFastRecurse(Ast_ਗੰਢ_ਵਿਅੰਜਕ* cexprp, ਪੂਰਨ msb, uint32_t ਉੱਤਲਾ_ਮੁੱਲ) ਅਚਲ {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        ਜੇ (msb < 0) ਵਾਪਸ m_caseDetails.ਰਿਕਾਰਡਾਂ[ਉੱਤਲਾ_ਮੁੱਲ].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        ਅਚਲ uint32_t ਉੱਤਲਾ_Value0 = ਉੱਤਲਾ_ਮੁੱਲ;
        ਅਚਲ uint32_t ਉੱਤਲਾ_Value1 = ਉੱਤਲਾ_ਮੁੱਲ | (1UL << msb);
        Ast_ਗੰਢ* tree0p = convertCaseFastRecurse(cexprp, msb - 1, ਉੱਤਲਾ_Value0);
        Ast_ਗੰਢ* tree1p = convertCaseFastRecurse(cexprp, msb - 1, ਉੱਤਲਾ_Value1);

        // If same logic on both sides, we can just return one of them
        ਜੇ (tree0p == tree1p) ਵਾਪਸ tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            ਤਾਰਕਿਕ same = ਸੱਚ;
            ਲਈ (uint32_t a = ਉੱਤਲਾ_Value0, b = ਉੱਤਲਾ_Value1; a < ਉੱਤਲਾ_Value1; ++a, ++b) {
                ਜੇ (m_caseDetails.ਰਿਕਾਰਡਾਂ[a].stmtsp != m_caseDetails.ਰਿਕਾਰਡਾਂ[b].stmtsp) {
                    same = ਝੂਠ;
                    ਰੋਕੋ;
                }
            }
            ਜੇ (same) {
                VL_DO_DANGLING(tree1p->delete_ਰੁੱਖ(), tree1p);
                ਵਾਪਸ tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        ਜੇ (tree0p && tree0p->backp()) tree0p = tree0p->clone_ਰੁੱਖ(ਸੱਚ);
        ਜੇ (tree1p && tree1p->backp()) tree1p = tree1p->clone_ਰੁੱਖ(ਸੱਚ);
        // Create the If statement
        File_ਸਤਰ* ਅਚਲ flp = cexprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = ਨਵਾਂ AstSel{flp, cexprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), msb, 1};
        AstIf* ਅਚਲ ifp = ਨਵਾਂ AstIf{flp, condp, tree1p, tree0p};
        ਵਾਪਸ ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    Ast_ਗੰਢ* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        ਅਚਲ ਪੂਰਨ case_ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();
        Ast_ਗੰਢ* ਅਚਲ ifrootp = convertCaseFastRecurse(nodep->exprp(), case_ਚੌੜਾਈ - 1, 0UL);
        ਵਾਪਸ ifrootp && ifrootp->backp() ? ifrootp->clone_ਰੁੱਖ(ਸੱਚ) : ifrootp;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newCaseCompare(ਅਚਲ AstCase* casep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਸਤੂ_Exprp) {
        File_ਸਤਰ* ਅਚਲ flp = ਵਸਤੂ_Exprp->fileline();
        ਜੇ (never_ਵਸਤੂ(casep, ਵਸਤੂ_Exprp)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਵਸਤੂ_Exprp), ਵਸਤੂ_Exprp);
            ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        }

        ਜੇ (AstCond* ਅਚਲ condp = VN_CAST(ਵਸਤੂ_Exprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ testp = condp->condp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thenp = newCaseCompare(casep, condp->thenp()->unlink_Fr_ਪਿੱਠ());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elsep = newCaseCompare(casep, condp->elsep()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING2(ਧੱਕੋ_Deletep(ਵਸਤੂ_Exprp), ਵਸਤੂ_Exprp, condp);
            ਵਾਪਸ ਨਵਾਂ AstCond{flp, testp, thenp, elsep};
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ caseExprp = casep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        ਜੇ (AstInsideRange* ਅਚਲ ਵਸਤੂ_Rangep = VN_CAST(ਵਸਤੂ_Exprp, InsideRange)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp = ਵਸਤੂ_Rangep->newAndFromInside(
                caseExprp, ਵਸਤੂ_Rangep->lhsp()->unlink_Fr_ਪਿੱਠ(), ਵਸਤੂ_Rangep->rhsp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING2(ਧੱਕੋ_Deletep(ਵਸਤੂ_Exprp), ਵਸਤੂ_Exprp, ਵਸਤੂ_Rangep);
            ਵਾਪਸ resultp;
        }

        ਜੇ (AstConst* ਅਚਲ ਵਸਤੂ_Constp = VN_CAST(ਵਸਤੂ_Exprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            ਜੇ (ਵਸਤੂ_Constp->ਅੰਕ().isFourState()) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ& ਮੇਲ = ਮੇਲ_ਨਮੂਨਾ(casep, ਵਸਤੂ_Constp);
                ਅਚਲ V3_ਸੰਖਿਆ& ਮੇਲ_ਨਕਾਬ = ਮੇਲ.ਪਹਿਲਾ;
                ਅਚਲ V3_ਸੰਖਿਆ& ਮੇਲ_ਬਿੱਟ = ਮੇਲ.ਦੂਜਾ;
                VL_DO_DANGLING2(ਧੱਕੋ_Deletep(ਵਸਤੂ_Exprp), ਵਸਤੂ_Exprp, ਵਸਤੂ_Constp);
                ਵਾਪਸ AstEq::newTyped(flp, ਨਵਾਂ AstConst{flp, ਮੇਲ_ਬਿੱਟ},
                                       ਨਵਾਂ AstAnd{flp, caseExprp, ਨਵਾਂ AstConst{flp, ਮੇਲ_ਨਕਾਬ}});
            }
        }

        ਵਾਪਸ AstEq::newTyped(flp, caseExprp, ਵਸਤੂ_Exprp);
    }

    // Convet case statement using generic if/else tree
    // CASEx(cexpr,ITEM(icond1,istmts1),ITEM(icond2,istmts2),ITEM(default,istmts3))
    // ->  IF((cexpr==icond1),istmts1,
    //                       IF((EQ (AND MASK cexpr) (AND MASK icond1)
    //                              ,istmts2, istmts3
    // TODO: should return AstNodeStmt after #6280
    Ast_ਗੰਢ* convertCaseGeneric(AstCase* nodep) {
        ++m_stats.caseGeneric;
        // We'll do this in two stages.
        // First stage, convert the conditions to the appropriate IF AND terms.
        ਤਾਰਕਿਕ hasDefault = ਝੂਠ;
        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {

            File_ਸਤਰ* ਅਚਲ flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            ਜੇ (itemp->isDefault()) {
                itemp->ਜੋੜੋ_Condsp(ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
                hasDefault = ਸੱਚ;
                ਜਾਰੀ;
            }

            // Regular clause. Construct the condition expression.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newCondp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ *ਵਸਤੂ_Exprp = itemp->condsp(), *nextp; ਵਸਤੂ_Exprp; ਵਸਤੂ_Exprp = nextp) {
                nextp = VN_AS(ਵਸਤੂ_Exprp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
                ਵਸਤੂ_Exprp->unlink_Fr_ਪਿੱਠ();

                // If case never matches, ignore it
                ਜੇ (never_ਵਸਤੂ(nodep, ਵਸਤੂ_Exprp)) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਵਸਤੂ_Exprp), ਵਸਤੂ_Exprp);
                    ਜਾਰੀ;
                }

                // Compute the term to add to the condition expression
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp = newCaseCompare(nodep, ਵਸਤੂ_Exprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? ਨਵਾਂ AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            ਜੇ (!newCondp) newCondp = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
            itemp->ਜੋੜੋ_Condsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        ਜੇ (!hasDefault) {
            ਜੇ (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                Ast_Case_ਵਸਤੂ* ਅਚਲ lastp = VN_AS(nodep->itemsp()->lastp(), Case_ਵਸਤੂ);
                ਧੱਕੋ_Deletep(lastp->condsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                lastp->ਜੋੜੋ_Condsp(ਨਵਾਂ AstConst{lastp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
            } ਨਹੀਂਤਾਂ {
                // Otherwise needs an explicit empty default
                nodep->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Case_ਵਸਤੂ{
                    nodep->fileline(), ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}},
                    ਖਾਲੀ_ਸੂਚਕ});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        ਪੂਰਨ ਡੂੰਘਾਈ = 0;
        AstIf* grouprootp = ਖਾਲੀ_ਸੂਚਕ;
        AstIf* groupnextp = ਖਾਲੀ_ਸੂਚਕ;
        AstIf* itemnextp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_Case_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Case_ਵਸਤੂ)) {

            // Grab the statements from this item. May be empty.
            Ast_ਗੰਢ* ਅਚਲ istmtsp = itemp->stmtsp();
            ਜੇ (istmtsp) istmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();

            // Expressioned clause
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ifexprp = itemp->condsp()->unlink_Fr_ਪਿੱਠ();
            {  // Prepare for next group
                ਜੇ (++ਡੂੰਘਾਈ > CASE_ENCODER_ਸਮੂਹ_ਡੂੰਘਾਈ) ਡੂੰਘਾਈ = 1;
                ਜੇ (ਡੂੰਘਾਈ == 1) {  // First group or starting new group
                    itemnextp = ਖਾਲੀ_ਸੂਚਕ;
                    AstIf* ਅਚਲ newp = ਨਵਾਂ AstIf{itemp->fileline(), ifexprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)};
                    ਜੇ (groupnextp) {
                        groupnextp->ਜੋੜੋ_Elsesp(newp);
                    } ਨਹੀਂਤਾਂ {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } ਨਹੀਂਤਾਂ {  // Continue group, modify if condition to OR in this new condition
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ condp = groupnextp->condp()->unlink_Fr_ਪਿੱਠ();
                    groupnextp->condp(
                        ਨਵਾਂ AstOr{ifexprp->fileline(), condp, ifexprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਸੱਚ)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                ਜੇ (ਡੂੰਘਾਈ == CASE_ENCODER_ਸਮੂਹ_ਡੂੰਘਾਈ) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->delete_ਰੁੱਖ(), itemexprp);
                    itemexprp = ਨਵਾਂ AstConst{itemp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                }
                AstIf* ਅਚਲ newp = ਨਵਾਂ AstIf{itemp->fileline(), itemexprp, istmtsp};
                ਜੇ (itemnextp) {
                    itemnextp->ਜੋੜੋ_Elsesp(newp);
                } ਨਹੀਂਤਾਂ {
                    groupnextp->ਜੋੜੋ_Thensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        ਵਾਪਸ grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    Ast_ਗੰਢ* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        ਅਚਲ ਤਾਰਕਿਕ use_ਸਾਰਣੀ = [&]() {
            // Not if disabled
            ਜੇ (!v3Global.opt.f_Case_ਸਾਰਣੀ()) ਵਾਪਸ ਝੂਠ;
            // Not if analysis tells us we can't
            ਜੇ (!m_case_ਸਾਰਣੀ_ਚੌੜਾਈ) ਵਾਪਸ ਝੂਠ;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            ਜੇ (m_case_ਸਾਰਣੀ_ਚੌੜਾਈ <= CASE_ਸਾਰਣੀ_TINY_ਬਿੱਟ) ਵਾਪਸ ਸੱਚ;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            ਅਚਲ ਮਾਪ_t branches = std::min<ਮਾਪ_t>(nodep->exprp()->ਚੌੜਾਈ(), m_caseNConditions);
            ਜੇ (branches < CASE_ਸਾਰਣੀ_MIN_BRANCHES) ਵਾਪਸ ਝੂਠ;
            ਵਾਪਸ ਸੱਚ;
        }();
        ਜੇ (use_ਸਾਰਣੀ) ਵਾਪਸ convert_Case_ਸਾਰਣੀ(nodep);

        // Determine if we should use the decoder method.
        ਅਚਲ ਤਾਰਕਿਕ useDecoder = [&]() {
            // Not if disabled
            ਜੇ (!v3Global.opt.fCaseDecoder()) ਵਾਪਸ ਝੂਠ;
            // Not if not a decoder pattern
            ਜੇ (m_case_Decoder_ਰਿਕਾਰਡਾਂ.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ ਝੂਠ;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            ਅਚਲ ਮਾਪ_t branches = std::min<ਮਾਪ_t>(nodep->exprp()->ਚੌੜਾਈ(), m_caseNConditions);
            ਜੇ (branches < CASE_ਸਾਰਣੀ_MIN_BRANCHES) ਵਾਪਸ ਝੂਠ;
            ਵਾਪਸ ਸੱਚ;
        }();
        ਜੇ (useDecoder) ਵਾਪਸ convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        ਅਚਲ ਤਾਰਕਿਕ use_Fast_ਦੋਆਧਾਰੀ_ਅੰਕ_ਰੁੱਖ = [&]() {
            // Not if disabled
            ਜੇ (!v3Global.opt.f_Case_ਰੁੱਖ()) ਵਾਪਸ ਝੂਠ;
            // Can't do it without the detailed analysis
            ਜੇ (!m_caseDetailsValid) ਵਾਪਸ ਝੂਠ;
            // Can't do it if not exhaustive
            ਜੇ (!m_caseDetails.exhaustive) ਵਾਪਸ ਝੂਠ;
            // Not worth doing if there are few conditions
            ਜੇ (m_caseNConditions <= 3) ਵਾਪਸ ਝੂਠ;
            // Avoid e.g. priority expanders from going crazy in expansion
            ਅਚਲ ਮਾਪ_t case_ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();
            ਜੇ (case_ਚੌੜਾਈ >= 8 && m_caseNConditions <= (case_ਚੌੜਾਈ + 1)) ਵਾਪਸ ਝੂਠ;
            // Otherwise use the bit tree
            ਵਾਪਸ ਸੱਚ;
        }();
        ਜੇ (use_Fast_ਦੋਆਧਾਰੀ_ਅੰਕ_ਰੁੱਖ) ਵਾਪਸ convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        ਜੇ (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3_Error_ਕੋਡ::LATCH, ਸੱਚ);
        }
        ਵਾਪਸ convertCaseGeneric(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        UASSERT_OBJ(nodep->exprp()->is_ਸ਼ੁੱਧ(), nodep,
                    "Impure case expression should have been removed by V3LiftExpr");

        CaseLintVisitor::apply(nodep);

        // Convert any children first
        iterateChildren(nodep);

        // Analyze this case statement
        analyzeCase(nodep);

        // Take the 'notParallelp' statements under the case statement created by V3Assert.
        // If the statement was proven to have no overlaps and all cases covered,
        // it can be removed. Otherwise insert the assertion after the case statement.
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtp = nodep->notParallelp()) {
            stmtp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->delete_ਰੁੱਖ(), stmtp);
            } ਨਹੀਂਤਾਂ {
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(stmtp);
            }
        }

        // Convert the case statement and replace the original
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CaseVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CaseVisitor() override {
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases decoder", m_stats.caseDecoder);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases table normal", m_stats.case_ਸਾਰਣੀ_ਸਾਧਾਰਨ);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases table tiny", m_stats.case_ਸਾਰਣੀ_Tiny);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases parallelized", m_stats.caseFast);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases complex", m_stats.caseGeneric);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Cases proven assertions", m_stats.provenAssertions);
    }
};

ਮਾਪ_t CaseVisitor::Lhs_ਰਿਕਾਰਡ::s_ਅਗਲਾ_Id = 0;

//######################################################################
// Case class functions

ਖਾਲੀ V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("case", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
ਖਾਲੀ V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
