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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Case.h"

#include "V3ConstPool.h"
#include "V3SharedTmps.h"
#include "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

class CaseLintVisitor final : public VNVisitorConst {
    // Under a CASE value node, if so the relevant case statement
    const AstNode* m_casep = nullptr;

    // METHODS
    template <typename CaseItem>
    static void detectMultipleDefaults(CaseItem* itemsp) {
        bool hitDefault = false;
        위해 (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            만약 (!itemp->isDefault()) continue;
            만약 (hitDefault) itemp->v3error("Multiple default statements in case statement.");
            hitDefault = true;
        }
    }

    template <typename CaseItem>
    void checkXZinNonCaseX(AstNode* casep, AstNodeExpr* exprp, CaseItem* itemsp) {
        VL_RESTORER(m_casep);
        m_casep = casep;
        iterateConst(exprp);
        위해 (CaseItem* itemp = itemsp; itemp; itemp = AstNode::as<CaseItem>(itemp->nextp())) {
            iterateAndNextConstNull(itemp->condsp());
        }
    }

    // VISITORS
    void visit(AstGenCase* nodep) override {
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }

    void visit(AstCase* nodep) override {
        만약 (nodep->casex()) {
            nodep->v3warn(CASEX, "Suggest casez (with ?'s) in place of casex (with X's)");
        }
        // Detect multiple defaults
        detectMultipleDefaults(nodep->itemsp());
        // Check for X/Z in non-casex statements
        checkXZinNonCaseX(nodep, nodep->exprp(), nodep->itemsp());
    }
    void visit(AstConst* nodep) override {
        만약 (!nodep->num().isFourState()) 반환;

        // Error if generate case
        만약 (VN_IS(m_casep, GenCase)) {
            nodep->v3error("Use of x/? constant in generate case statement, "
                           "(no such thing as 'generate casez')");
            반환;
        }

        // Otherwise must be a case statement
        const AstCase* const casep = VN_AS(m_casep, Case);

        // Don't sweat it, we already complained about casex in general
        만약 (casep->casex()) 반환;

        만약 (casep->casez() || casep->caseInside()) {
            만약 (nodep->num().isAnyX()) {
                nodep->v3warn(CASEWITHX, "Use of x constant in casez statement, "
                                         "(perhaps intended ?/z in constant)");
            }
            반환;
        }

        nodep->v3warn(CASEWITHX, "Use of x/? constant in case statement, "
                                 "(perhaps intended casex/casez)");
    }
    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

    // CONSTRUCTORS
    explicit CaseLintVisitor(AstCase* nodep) { iterateConst(nodep); }
    explicit CaseLintVisitor(AstGenCase* nodep) { iterateConst(nodep); }
    ~CaseLintVisitor() override = default;

public:
    static void apply(AstCase* nodep) { CaseLintVisitor{nodep}; }
    static void apply(AstGenCase* nodep) { CaseLintVisitor{nodep}; }
};

//######################################################################
// Case state, as a visitor of each AstNode

class CaseVisitor final : public VNVisitor {
    // Maximum width for detailed analysis
    constexpr static 정수 CASE_DETAILS_MAX_WIDTH = 16;
    // Levels of priority to be ORed together in top IF tree
    constexpr static 정수 CASE_ENCODER_GROUP_DEPTH = 8;
    // Maximum size for tiny lookup tables - materialized in code
    constexpr static size_t CASE_TABLE_TINY_BITS = 32;  // Up to 2 instructions to materialize
    // Maximum size for normal lookup tables - stored in constant pool
    constexpr static size_t CASE_TABLE_MAX_BITS = 1ULL << 16;  // 64Kbits / 8KBytes
    // Minimum number of the branches a table must replace to be worth a load
    constexpr static size_t CASE_TABLE_MIN_BRANCHES = 3;

    // TYPES
    // Record for each case value
    struct CaseRecord final {
        AstCaseItem* itemp;  // Case item that covers value
        AstConst* constp;  // Expression within 'itemp' that covers value (nullptr for default)
        AstNode* stmtsp;  // Statements of 'itemp' (might be nullptr if case is empty)
    };

    // Record for each LHS of a decoder pattern
    struct LhsRecord final {
        AstNodeExpr* lhsp = nullptr;  // LHS of the assignment
        AstNodeAssign* preDefaultp = nullptr;  // Default assignment *before the case statement*
        size_t nCaseAssigns = 0;  // Number of AstAssigns to this LHS in case clauses
        size_t nCaseAssignDlys = 0;  // Number of AstAssignDlys to this LHS in case clauses
        size_t offset = 0;  // Offset in the table for this LHS

        static size_t s_nextId;  // Static unique Id counter
        size_t id = ++s_nextId;  // Unique Id for sorting
    };

    // NODE STATE:
    // AstVarScope::user1() -> bool: true if written to, only in parts of analysis phase

    // STATE
    // Statistics tracking, as a struct so can be passed to 'const' methods
    struct Stats final {
        VDouble0 caseDecoder;  // Cases using decoder method
        VDouble0 caseTableNormal;  // Cases using table method with normal table
        VDouble0 caseTableTiny;  // Cases using table method with tiny table
        VDouble0 caseFast;  // Cases using fast bit tree method
        VDouble0 caseGeneric;  // Cases using generic if/else tree method
        VDouble0 provenAssertions;  // Assertions proven to hold
    } m_stats;
    const AstNode* m_alwaysp = nullptr;  // Always in which case is located
    // Temporary variables, shared by instances
    // For table lookup results
    V3SharedTmps m_tableOutTmps{"__VcaseTableOut", VVarType::MODULETEMP};
    // For decoder results
    V3SharedTmps m_decoderOutTmps{"__VcaseDecoderOut", VVarType::MODULETEMP};
    AstScope* m_scopep = nullptr;  // Current scope

    // STATE - per AstCase. Update by 'analyzeCase', treat 'const' otherwise
    bool m_caseOpaque = false;  // Case statement is opaque (non-packed, or non-const conditions)
    bool m_caseHasDefault = false;  // Indicates the case statement has a default case
    size_t m_caseNCaseItems = 0;  // Number of AstCaseItems in the case statement
    size_t m_caseNConditions = 0;  // Number of conditions in the case statement
    // Map from LHSs of decoder pattern to corresponding LhsRecord.
    std::unordered_map<VNRef<AstNodeExpr>, LhsRecord> m_caseLhsRecords;
    // Values of 'm_caseLhsRecords' in sorted order, if case statement is a decoder pattern
    std::vector<LhsRecord> m_caseDecoderRecords;
    size_t m_caseDecoderEntryWidth = 0;  // Width of each entry in the decoder table
    size_t m_caseTableWidth = 0;  // Total width of the case table - 0 means can't optimize
    bool m_caseDetailsValid = false;  // Indicates m_caseDetails is valid
    struct final {
        bool exhaustive = false;  // Proven exhaustive
        bool exhaustiveOverEnumOnly = false;  // Exhaustive over enum values only
        bool noOverlaps = false;  // Proven no overlaps between cases
        // Map from value (index) to the CaseRecord that covers this value
        std::array<CaseRecord, 1U << CASE_DETAILS_MAX_WIDTH> records;
    } m_caseDetails;

    // METHODS

    // Xs in case or casez are impossible due to two state simulations.
    // Returns true if the item is never possible
    static bool neverItem(const AstCase* casep, const AstNodeExpr* itemExprp) {
        const AstConst* const constp = VN_CAST(itemExprp, Const);
        만약 (!constp) 반환 false;
        만약 (casep->casex() || casep->caseInside()) 반환 false;
        만약 (casep->casez()) 반환 constp->num().isAnyX();
        반환 constp->num().isFourState();
    }

    // Returns the matching mask and bits for a case item constant condition
    // TODO: with 'neverItem' checked, this is alway the same for all types of cases
    //       4-state will have to fix this up
    static std::pair<V3Number, V3Number> matchPattern(const AstCase* /*casep*/,
                                                      const AstConst* condp) {
        // As one to encourage NRVO/move
        std::pair<V3Number, V3Number> pairMaskBits{
            std::piecewise_construct,  //
            std::forward_as_tuple(condp->fileline(), condp->width(), 0),
            std::forward_as_tuple(condp->fileline(), condp->width(), 0)};
        pairMaskBits.first.opBitsNonXZ(condp->num());
        pairMaskBits.second.opBitsOne(condp->num());
        반환 pairMaskBits;
    }

    // If the given statement is an assignment that fits the decoder pattern,
    // return it, otherwise return nullptr
    static AstNodeAssign* checkDecoderAssign(AstNode* stmtp) {
        // Only Assign and AssignDly are supported
        만약 (!VN_IS(stmtp, Assign) && !VN_IS(stmtp, AssignDly)) 반환 nullptr;
        AstNodeAssign* const assp = VN_AS(stmtp, NodeAssign);
        // Only if no timing control
        만약 (assp->timingControlp()) 반환 nullptr;
        // Only if assigning a constant
        만약 (!VN_IS(assp->rhsp(), Const)) 반환 nullptr;
        // Only if it's a packed value
        AstNodeDType* const dtypep = assp->rhsp()->dtypep();
        만약 (dtypep->isString() || dtypep->isDouble()) 반환 nullptr;
        // Only if the LHS has no reads (can be relaxed, but need to prove there is no r/w hazard)
        만약 (assp->lhsp()->exists([](AstVarRef* refp) { 반환 refp->access().isReadOrRW(); })) {
            반환 nullptr;
        }
        // This is an assignment that fits the decoder pattern
        반환 assp;
    }

    // Analyze if the given case item fits the decoder pattern, return true iff so.
    // Updates 'm_caseLhsRecords'.
    bool analyzeDecoderCaseItem(AstCaseItem* cip) {
        // AstVarScope::user1() -> bool: true if written to
        const VNUser1InUse user1InUse;
        위해 (AstNode* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            // Must be an assignment that fits the decoder pattern
            AstNodeAssign* const assp = checkDecoderAssign(stmtp);
            만약 (!assp) 반환 false;
            // Must assign each LHS exactly once - RHS is Const
            const bool multipleAssignments = assp->lhsp()->exists([](AstVarRef* refp) {  //
                반환 refp->varScopep()->user1SetOnce();
            });
            만약 (multipleAssignments) 반환 false;
            // Update LhsRecord
            LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
            만약 (!lhsRecord.lhsp) lhsRecord.lhsp = assp->lhsp();
            lhsRecord.nCaseAssigns += VN_IS(assp, Assign);
            lhsRecord.nCaseAssignDlys += VN_IS(assp, AssignDly);
        }
        반환 true;
    }

    // Determine whether we should check case items are complete
    // Returns enum's dtype if should check, nullptr if shouldn't
    static const AstEnumDType* getEnumCompletionCheckDType(const AstCase* const nodep) {
        만약 (!nodep->uniquePragma() && !nodep->unique0Pragma()) 반환 nullptr;
        const AstEnumDType* const enumDtp
            = VN_CAST(nodep->exprp()->dtypep()->skipRefToEnump(), EnumDType);
        만약 (!enumDtp) 반환 nullptr;  // Case isn't enum
        const AstBasicDType* const basicp = enumDtp->subDTypep()->basicp();
        만약 (!basicp) 반환 nullptr;  // Not simple type (perhaps IEEE illegal)
        반환 enumDtp;
    }

    // Check and warn if case items are not complete over the given enum type.
    // Returns true iff the case items cover all enum values/patterns.
    bool checkExhaustiveEnum(const AstCase* const nodep, const AstEnumDType* const enump) {
        const uint32_t numCases = 1UL << nodep->exprp()->width();
        bool fullyCovered = true;
        string missingItems;
        위해 (AstEnumItem* eip = enump->itemsp(); eip; eip = VN_AS(eip->nextp(), EnumItem)) {
            const auto& match = matchPattern(nodep, VN_AS(eip->valuep(), Const));
            const uint32_t mask = match.first.toUInt();
            const uint32_t bits = match.second.toUInt();

            // Check all cases to see if they cover this enum value/pattern
            위해 (uint32_t i = 0; i < numCases; ++i) {
                만약 ((i & mask) != bits) continue;  // This case is not for this enum value
                만약 (m_caseDetails.records[i].itemp) continue;  // Covered case
                만약 (!missingItems.empty()) missingItems += ", ";
                missingItems += eip->prettyNameQ();
                fullyCovered = false;
                break;
            }
        }
        만약 (!missingItems.empty() && !nodep->unique0Pragma()) {
            nodep->v3warn(CASEINCOMPLETE, "Enum item(s) not covered by case: " << missingItems);
        }
        반환 fullyCovered;  // enum is fully covered
    }

    // Check and warn if case items are not complete over all possible values.
    // Returns true iff the case items cover all values of the case expression.
    bool checkExhaustivePacked(AstCase* nodep) {
        const uint32_t numCases = 1UL << nodep->exprp()->width();
        위해 (uint32_t i = 0; i < numCases; ++i) {
            만약 (m_caseDetails.records[i].itemp) continue;  // Covered case
            만약 (!nodep->unique0Pragma()) {
                nodep->v3warn(CASEINCOMPLETE,
                              "Case values incompletely covered (example pattern 0x" << std::hex
                                                                                     << i << ")");
            }
            // TODO: warn for more than one uncovered case, not just the first
            반환 false;
        }

        // It's an exhaustive case statement
        반환 true;
    }

    // Analyze each value in the case statement. Updates 'm_caseDetails' and issues warnings.
    void analyzeCaseDetails(AstCase* nodep) {
        const uint32_t numValues = 1UL << nodep->exprp()->width();
        // Clear case records
        위해 (uint32_t i = 0; i < numValues; ++i) {
            m_caseDetails.records[i].itemp = nullptr;
            m_caseDetails.records[i].constp = nullptr;
            m_caseDetails.records[i].stmtsp = nullptr;
        }
        // Now pick up the values for each assignment
        // We can cheat and use uint32_t's because we only support narrow case's
        bool reportedOverlap = false;
        bool hasDefault = false;
        m_caseDetails.noOverlaps = true;
        위해 (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Default case
            만약 (itemp->isDefault()) {
                // Default was moved to be the last item by V3LinkDot. Fill remaining cases
                위해 (uint32_t i = 0; i < numValues; ++i) {
                    CaseRecord& caseRecord = m_caseDetails.records[i];
                    만약 (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.stmtsp = itemp->stmtsp();
                    }
                }
                hasDefault = true;
                continue;
            }

            위해 (AstConst* iconstp = VN_AS(itemp->condsp(), Const); iconstp;
                 iconstp = VN_AS(iconstp->nextp(), Const)) {
                // Some items can never match due to 2-state simulation
                만약 (neverItem(nodep, iconstp)) continue;

                const auto& match = matchPattern(nodep, iconstp);
                const uint32_t mask = match.first.toUInt();
                const uint32_t bits = match.second.toUInt();

                bool foundNewCase = false;
                const AstConst* firstOverlapConstp = nullptr;
                uint32_t firstOverlapValue = 0;
                위해 (uint32_t i = 0; i < numValues; ++i) {
                    만약 ((i & mask) != bits) continue;

                    CaseRecord& caseRecord = m_caseDetails.records[i];

                    // If this is the first case that covers this value, record it
                    만약 (!caseRecord.itemp) {
                        caseRecord.itemp = itemp;
                        caseRecord.constp = iconstp;
                        caseRecord.stmtsp = itemp->stmtsp();
                        foundNewCase = true;
                        continue;
                    }

                    // There is an overlap. If it's within the same CaseItem, it is legal
                    만약 (caseRecord.itemp == itemp) continue;

                    // Otherwise record the first overlapping case
                    만약 (!firstOverlapConstp) {
                        firstOverlapConstp = caseRecord.constp;
                        firstOverlapValue = i;
                        m_caseDetails.noOverlaps = false;
                    }
                }

                // Only report first overlap
                만약 (reportedOverlap || !firstOverlapConstp) continue;

                // Report first overlap
                만약 (nodep->priorityPragma()) {
                    // If this is a priority case, we only want to complain if every possible
                    // value for this item is already hit by some other item. This is true if
                    // 'foundNewCase' is false. 'firstOverlapConstp' is null when the only
                    // covering item is this item itself, which is legal overlap within one
                    // item.
                    만약 (!foundNewCase) {
                        iconstp->v3warn(CASEOVERLAP,
                                        "Case item ignored: every matching value is covered "
                                        "by an earlier condition\n"
                                            << iconstp->warnContextPrimary() << '\n'
                                            << firstOverlapConstp->warnOther()
                                            << "... Location of previous condition\n"
                                            << firstOverlapConstp->warnContextPrimary());
                        reportedOverlap = true;
                    }
                } 아니면 {
                    // If this case statement doesn't have the priority keyword,
                    // we want to warn on any overlap.
                    std::ostringstream examplePattern;
                    만약 (iconstp->num().isAnyXZ()) {
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
                    reportedOverlap = true;
                }
            }
        }

        // If there was no default, check exhaustiveness
        m_caseDetails.exhaustiveOverEnumOnly = false;
        m_caseDetails.exhaustive = hasDefault;
        만약 (!hasDefault) {
            만약 (const AstEnumDType* const enump = getEnumCompletionCheckDType(nodep)) {
                // Only checks enum values are covered, not all bit patterns of the case expression
                const bool exhaustiveOverEnum = checkExhaustiveEnum(nodep, enump);
                m_caseDetails.exhaustiveOverEnumOnly = exhaustiveOverEnum;
                m_caseDetails.exhaustive = exhaustiveOverEnum;
            } 아니면 {
                m_caseDetails.exhaustive = checkExhaustivePacked(nodep);
            }
        }

        // Records now valid
        m_caseDetailsValid = true;
    }

    void analyzeDecoderPattern(AstCase* nodep) {
        // Check each LHS record
        위해 (auto it = m_caseLhsRecords.cbegin(); it != m_caseLhsRecords.cend();) {
            const LhsRecord& lhsRecord = it->second;

            // Delete records that have no assignments in any case item (only pre-defaults)
            만약 (!lhsRecord.nCaseAssigns && !lhsRecord.nCaseAssignDlys) {
                it = m_caseLhsRecords.erase(it);
                continue;
            }
            ++it;

            // If mixed assignments, it's not a decoder pattern
            만약 (lhsRecord.nCaseAssigns && lhsRecord.nCaseAssignDlys) 반환;

            // If assigned in all branches, it's good - but only if every table entry will be
            // covered, i.e. the case has a default, or is exhaustive over all bit patterns.
            // Enum-only exhaustiveness is not enough: out-of-enum values leave entries
            // uncovered.
            만약 (m_caseHasDefault
                || (m_caseDetailsValid && m_caseDetails.exhaustive
                    && !m_caseDetails.exhaustiveOverEnumOnly)) {
                만약 (lhsRecord.nCaseAssigns == m_caseNCaseItems) continue;
                만약 (lhsRecord.nCaseAssignDlys == m_caseNCaseItems) continue;
            }

            // Otherwise it needs to have a pre-default assignment
            AstNode* const preDefaultp = lhsRecord.preDefaultp;
            만약 (!preDefaultp) 반환;
            // And the pre-default needs to be the same type
            만약 (lhsRecord.nCaseAssigns && !VN_IS(preDefaultp, Assign)) 반환;
            만약 (lhsRecord.nCaseAssignDlys && !VN_IS(preDefaultp, AssignDly)) 반환;
        }
        // All cases check out, can optimize if there are some entries left
        만약 (m_caseLhsRecords.empty()) 반환;

        // Gather all the LhsRecords and sort them - there is a copy here, it's ok, won't be many
        m_caseDecoderRecords.reserve(m_caseLhsRecords.size());
        위해 (const auto& item : m_caseLhsRecords) m_caseDecoderRecords.emplace_back(item.second);
        std::sort(m_caseDecoderRecords.begin(), m_caseDecoderRecords.end(),
                  [](const LhsRecord& a, const LhsRecord& b) {
                      // Sort by width, then id
                      const 정수 aWidth = a.lhsp->width();
                      const 정수 bWidth = b.lhsp->width();
                      만약 (aWidth != bWidth) 반환 aWidth < bWidth;
                      반환 a.id < b.id;
                  });

        // We can either create a single lookup table for all LHSs, or one for each LHS.
        // With a single table, we need to select out of the lookup via a temporary variable.
        // With one table per LHS, we need to do multiple loads. The table is likely to incur a
        // D-cache miss on large designs, so we choose single table.

        const 정수 caseWidth = nodep->exprp()->width();

        // Safely check if table with 'entryWidth' entries would fit within 'maxWidth' bits
        const auto fitsLimit = [&](size_t entryWidth, size_t maxWidth) -> bool {
            size_t totalWidth = entryWidth;
            // Multiply cases - iterative to avoid overflow
            위해 (정수 i = 0; i < caseWidth; ++i) {
                totalWidth <<= 1;
                만약 (totalWidth > maxWidth) 반환 false;
            }
            반환 true;
        };

        // Check if the whole table would fit in a tiny table packed tightly
        m_caseDecoderEntryWidth = 0;
        위해 (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += lhsRecord.lhsp->width();
        }
        // If it fits, we will pack it tightly
        만약 (fitsLimit(m_caseDecoderEntryWidth, CASE_TABLE_TINY_BITS)) {
            m_caseTableWidth = m_caseDecoderEntryWidth << caseWidth;  // Can optimize
            반환;
        }

        // Tabel will be bigish. To avoid expensive bit swizzling, align each entry to a
        // word boundary if it would cross a word boundary.
        m_caseDecoderEntryWidth = 0;
        위해 (LhsRecord& lhsRecord : m_caseDecoderRecords) {
            const size_t width = lhsRecord.lhsp->width();
            const size_t lsbWord = VL_BITWORD_E(m_caseDecoderEntryWidth);
            const size_t msbWord = VL_BITWORD_E(m_caseDecoderEntryWidth + width - 1);
            만약 (lsbWord != msbWord) {
                m_caseDecoderEntryWidth = VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
            }
            lhsRecord.offset = m_caseDecoderEntryWidth;
            m_caseDecoderEntryWidth += width;
        }
        // Check the table fits max size
        const size_t alignedEntryWidth = VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        만약 (fitsLimit(alignedEntryWidth, CASE_TABLE_MAX_BITS)) {
            m_caseTableWidth = alignedEntryWidth << caseWidth;  // Can optimize
            반환;
        }

        // Can optimize as AstMatchMasked, no other info needed
    }

    // Analyze case statement. Updates 'm_case*' members. Reports warnings.
    void analyzeCase(AstCase* nodep) {
        // Reset all analysis results
        m_caseOpaque = false;
        m_caseHasDefault = false;
        m_caseNCaseItems = 0;
        m_caseNConditions = 0;
        m_caseDecoderRecords.clear();
        m_caseDecoderEntryWidth = 0;
        m_caseTableWidth = 0;
        m_caseLhsRecords.clear();
        m_caseDetailsValid = false;

        AstNode* const caseExprp = nodep->exprp();

        // Mark opaque if not a packed value - TODO: can this be a class?
        만약 (caseExprp->isDouble() || caseExprp->isString()) m_caseOpaque = true;

        // Gather pre-default assignments of decoder pattern
        {
            // AstVarScope::user1() -> bool: true if written to
            const VNUser1InUse user1InUse;
            위해 (AstNode* prevp = nodep->prevp(); prevp; prevp = prevp->prevp()) {
                AstNodeAssign* const assp = checkDecoderAssign(prevp);
                만약 (!assp) break;  // Stop if not a decoder assignment
                // Stop if multiple assignments
                const bool multipleAssignments = assp->lhsp()->exists([&](AstVarRef* refp) {  //
                    반환 refp->varScopep()->user1SetOnce();
                });
                만약 (multipleAssignments) break;
                // Store pre-default assignment
                LhsRecord& lhsRecord = m_caseLhsRecords[*assp->lhsp()];
                lhsRecord.lhsp = assp->lhsp();
                lhsRecord.preDefaultp = assp;
            }
        }

        // Check each case item
        bool canBeDecoder = true;
        위해 (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            // Check conditions
            위해 (AstNode* condp = cip->condsp(); condp; condp = condp->nextp()) {
                // Count conditions that can actually match.
                만약 (!neverItem(nodep, VN_AS(condp, NodeExpr))) ++m_caseNConditions;
                // Mark opaque if non-constant condition
                만약 (!VN_IS(condp, Const)) {
                    m_caseOpaque = true;
                    canBeDecoder = false;  // Can't be a decoder if opaque
                }
            }
            // Check if it has a default case
            만약 (cip->isDefault()) m_caseHasDefault = true;
            // Count case items
            ++m_caseNCaseItems;
            // Check if it fits the decoder pattern, if still possible
            만약 (canBeDecoder) canBeDecoder = analyzeDecoderCaseItem(cip);
        }

        // Nothing else to do if not a packed type, or non-const conditions
        만약 (m_caseOpaque) 반환;

        // If small enough, analyse details
        만약 (caseExprp->width() <= CASE_DETAILS_MAX_WIDTH) analyzeCaseDetails(nodep);

        // Check if it actually fits a full decoder pattern
        만약 (canBeDecoder) analyzeDecoderPattern(nodep);
    }

    AstNodeStmt* connectDecoderOutputs(AstCase* nodep, AstNodeExpr* exprp, V3SharedTmps& tmps) {
        FileLine* const flp = nodep->fileline();

        // If there is only one LHS, just use the result
        만약 (m_caseDecoderRecords.size() == 1) {
            const LhsRecord& lhsRecord = m_caseDecoderRecords[0];
            const 정수 width = lhsRecord.lhsp->width();
            AstNodeExpr* const rhsp
                = exprp->width() == width ? exprp : new AstSel{flp, exprp, 0, width};
            AstNodeExpr* const lhsp = lhsRecord.lhsp->cloneTreePure(false);
            만약 (lhsRecord.nCaseAssigns) {
                반환 new AstAssign{flp, lhsp, rhsp};
            } 아니면 만약 (lhsRecord.nCaseAssignDlys) {
                반환 new AstAssignDly{flp, lhsp, rhsp};
            } 아니면 {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }

        // There are multiple LHSs, store the lookup result in a temporary
        AstVarScope* const tempVscp = tmps.make(flp, m_scopep, m_caseDecoderEntryWidth);
        AstNodeExpr* const tempWritep = new AstVarRef{flp, tempVscp, VAccess::WRITE};
        AstNodeStmt* const resultp = new AstAssign{flp, tempWritep, exprp};

        // For each LHS, select out the result
        위해 (const LhsRecord& lhsRecord : m_caseDecoderRecords) {
            const 정수 width = lhsRecord.lhsp->width();
            const 정수 lsb = lhsRecord.offset;
            AstNodeExpr* const tempReadp = new AstVarRef{flp, tempVscp, VAccess::READ};
            AstNodeExpr* const rhsp = new AstSel{flp, tempReadp, lsb, width};
            AstNodeExpr* const lhsp = lhsRecord.lhsp->cloneTreePure(false);
            만약 (lhsRecord.nCaseAssigns) {
                resultp->addNext(new AstAssign{flp, lhsp, rhsp});
            } 아니면 만약 (lhsRecord.nCaseAssignDlys) {
                resultp->addNext(new AstAssignDly{flp, lhsp, rhsp});
            } 아니면 {
                nodep->v3fatalSrc("Unknown assignment type");
            }
        }
        반환 resultp;
    }

    AstNodeStmt* convertCaseTable(AstCase* nodep) {
        // Create the table constant
        FileLine* const flp = nodep->fileline();
        AstConst* const tablep
            = new AstConst{flp, AstConst::WidthedValue{}, static_cast<정수>(m_caseTableWidth), 0};
        const uint32_t tableEntries = 1U << nodep->exprp()->width();

        const bool isTinyTable = m_caseTableWidth <= CASE_TABLE_TINY_BITS;
        만약 (isTinyTable) {
            ++m_stats.caseTableTiny;
        } 아니면 {
            ++m_stats.caseTableNormal;
        }

        // Populate the table. Align entries to a word boundary to avoid bit swizzling at runtime.
        const uint32_t entryWidth = isTinyTable
                                        ? m_caseDecoderEntryWidth
                                        : VL_WORDS_I(m_caseDecoderEntryWidth) * VL_EDATASIZE;
        위해 (const LhsRecord& lhsRecord : m_caseDecoderRecords) {
            const 정수 lhsWidth = lhsRecord.lhsp->width();
            const 정수 lhsOffset = lhsRecord.offset;

            // Broadcast the pre-default assignment
            만약 (lhsRecord.preDefaultp) {
                AstConst* const rhsp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                위해 (uint32_t index = 0; index < tableEntries; ++index) {
                    const uint32_t tableOffset = index * entryWidth + lhsOffset;
                    tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                }
            }

            // Populate table based on each case item. In reverse order so earlier items win
            위해 (AstCaseItem* cip = VN_AS(nodep->itemsp()->lastp(), CaseItem); cip;
                 cip = VN_AS(cip->prevp(), CaseItem)) {
                // Find the RHS in this case
                AstConst* const rhsp = [&]() -> AstConst* {
                    위해 (AstNode* stmtp = cip->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                        AstNodeAssign* const ap = VN_AS(stmtp, NodeAssign);
                        만약 (lhsRecord.lhsp->sameTree(ap->lhsp())) 반환 VN_AS(ap->rhsp(), Const);
                    }
                    // Not assigned in this case, use the pre-assigned default
                    반환 VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }();

                // If default, broadcast it
                만약 (cip->isDefault()) {
                    위해 (uint32_t index = 0; index < tableEntries; ++index) {
                        const uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                    }
                    continue;
                }

                // Iterate case conditions in reverse order
                위해 (AstConst* condp = VN_AS(cip->condsp()->lastp(), Const); condp;
                     condp = VN_AS(condp->prevp(), Const)) {
                    만약 (neverItem(nodep, condp)) continue;  // If item never matches, ignore it
                    const auto& match = matchPattern(nodep, condp);
                    const uint32_t matchMask = match.first.toUInt();
                    const uint32_t matchBits = match.second.toUInt();
                    const uint32_t inverseMask = ~matchMask & ((1U << condp->width()) - 1);
                    // This iterates through all integers that are a subset of the inverse mask,
                    // i.e.: all don't care values masked out
                    위해 (uint32_t i = inverseMask; true; i = (i - 1) & inverseMask) {
                        const uint32_t index = i | matchBits;
                        const uint32_t tableOffset = index * entryWidth + lhsOffset;
                        tablep->num().opSelInto(rhsp->num(), tableOffset, lhsWidth);
                        만약 (!i) break;
                    }
                }
            }
        }

        // Create the lookup table reference, in the constant pool unless using an inline table
        AstNodeExpr* const tableRefp = [&]() -> AstNodeExpr* {
            만약 (isTinyTable) 반환 tablep;
            AstVarRef* const refp = V3ConstPool::find(tablep);
            VL_DO_DANGLING(tablep->deleteTree(), tablep);  // V3ConstPool::find clones
            반환 refp;
        }();

        // Create the lookup index
        AstNodeExpr* const caseExprp
            = new AstExtend{flp, nodep->exprp()->cloneTreePure(false), 32};
        AstNodeExpr* const scalep = new AstConst{flp, entryWidth};
        AstNodeExpr* const tableLsbp = new AstMul{flp, scalep, caseExprp};
        AstNodeExpr* const tableSelp
            = new AstSel{flp, tableRefp, tableLsbp, static_cast<정수>(m_caseDecoderEntryWidth)};

        // Connect outputs
        반환 connectDecoderOutputs(nodep, tableSelp, m_tableOutTmps);
    }

    AstNodeStmt* convertCaseDecoder(AstCase* nodep) {
        ++m_stats.caseDecoder;

        FileLine* const flp = nodep->fileline();

        // Gather all the case conditions, paird with their statements. A 'nullptr' condition
        // matches anything (the default case, or the catch-all added below).
        std::vector<std::pair<AstConst*, AstNode*>> clauses;  // (condition, item statements)
        clauses.reserve(m_caseNConditions + 1);
        위해 (AstCaseItem* cip = nodep->itemsp(); cip; cip = VN_AS(cip->nextp(), CaseItem)) {
            만약 (cip->isDefault()) {
                clauses.emplace_back(nullptr, cip->stmtsp());
                continue;
            }
            위해 (AstNode* condp = cip->condsp(); condp; condp = condp->nextp()) {
                AstConst* const iconstp = VN_AS(condp, Const);
                // Skip items that can never match in 2-state simulation (e.g. X in casez)
                만약 (neverItem(nodep, iconstp)) continue;
                clauses.emplace_back(iconstp, cip->stmtsp());
            }
        }
        // If the case has no default item and is not provably exhaustive, unmatched selector
        // values fall back to the pre-defaults. Represent that with a catch-all clause (null
        // condition and no statements, so every LHS uses its pre-default). 'analyzeDecoderPattern'
        // guarantees every LHS has a pre-default in this case.
        const bool provenExhaustive = m_caseDetailsValid && m_caseDetails.exhaustive
                                      && !m_caseDetails.exhaustiveOverEnumOnly;
        만약 (clauses.back().first && !provenExhaustive) clauses.emplace_back(nullptr, nullptr);

        // Number of entries in decoder table
        const 정수 decoderEnries = clauses.size();

        // Build the match table: a {matchBits, matchMask} packed pair per clause, shared by all
        // LHSs. Each field is rounded up to a whole EDATA word boundary to avoid bit swizzling
        // at runtime. We use a packed value so the runtime function can take a VlWide pointer
        // without templating on the array size.
        const 정수 condWidth = nodep->exprp()->width();
        const 정수 matchWidth = 2 * VL_WORDS_I(condWidth) * VL_EDATASIZE;
        AstConst* const matchp
            = new AstConst{flp, AstConst::WidthedValue{}, decoderEnries * matchWidth, 0};
        위해 (정수 i = 0; i < decoderEnries; ++i) {
            const 정수 entryLsb = i * matchWidth;

            // If the entry has a condition, use it's match bits and mask
            만약 (AstConst* const condp = clauses[i].first) {
                const auto& match = matchPattern(nodep, condp);
                matchp->num().opSelInto(match.first, entryLsb, condWidth);
                matchp->num().opSelInto(match.second, entryLsb + matchWidth / 2, condWidth);
                continue;
            }

            // Otherwise use zero for mask and bits, which matches anything
            V3Number numZero{flp, condWidth, 0};
            matchp->num().opSelInto(numZero, entryLsb, condWidth);
            matchp->num().opSelInto(numZero, entryLsb + matchWidth / 2, condWidth);
        }

        // Create the table initializer
        AstRange* const rangep = new AstRange{flp, decoderEnries - 1, 0};
        AstNodeDType* const entryDtypep = nodep->findBitDType(
            m_caseDecoderEntryWidth, m_caseDecoderEntryWidth, VSigning::UNSIGNED);
        AstNodeDType* const tableDtypep = new AstUnpackArrayDType{flp, entryDtypep, rangep};
        v3Global.rootp()->typeTablep()->addTypesp(tableDtypep);
        AstConsArray* const tablep = new AstConsArray{flp, tableDtypep};

        // Build a single value table for all LHSs: one entry per clause, packing each LHS's value
        // at its offset. The entry width is the table packing computed by 'analyzeDecoderPattern'
        // Rounded up to a whole EDATA word boundary to avoid bit swizzling at runtime.
        위해 (정수 i = 0; i < decoderEnries; ++i) {
            AstNode* const stmtsp = clauses[i].second;
            AstConst* const entryp = new AstConst{flp, AstConst::WidthedValue{},
                                                  static_cast<정수>(m_caseDecoderEntryWidth), 0};
            위해 (const LhsRecord& lhsRecord : m_caseDecoderRecords) {
                AstNodeExpr* const lhsp = lhsRecord.lhsp;
                // Find the value assigned to this LHS in the clause's statements
                AstConst* valConstp = nullptr;
                위해 (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    AstNodeAssign* const assignp = VN_AS(stmtp, NodeAssign);
                    만약 (!lhsp->sameTree(assignp->lhsp())) continue;
                    valConstp = VN_AS(assignp->rhsp(), Const);
                    break;
                }
                // Not assigned in this clause, so use the pre-assigned default
                만약 (!valConstp) {
                    UASSERT_OBJ(lhsRecord.preDefaultp, nodep,
                                "Decoder LHS unassigned in case item without a pre-default");
                    valConstp = VN_AS(lhsRecord.preDefaultp->rhsp(), Const);
                }
                entryp->num().opSelInto(valConstp->num(), lhsRecord.offset, lhsp->width());
            }
            tablep->valuep(i, entryp);
        }

        // Create the tables
        AstVarRef* const matchRefp = V3ConstPool::find(matchp);
        AstVarRef* const tableRefp = V3ConstPool::find(tablep);
        VL_DO_DANGLING(matchp->deleteTree(), matchp);
        VL_DO_DANGLING(tablep->deleteTree(), tablep);

        // AstMatchMasked produces the index of the matching entry
        AstNodeExpr* const caseExprp = nodep->exprp()->cloneTreePure(false);
        AstMatchMasked* const indexp = new AstMatchMasked{flp, caseExprp, matchRefp};
        AstNodeExpr* const entryp = new AstArraySel{flp, tableRefp, indexp};

        반환 connectDecoderOutputs(nodep, entryp, m_decoderOutTmps);
    }

    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCaseFastRecurse(AstNodeExpr* cexprp, 정수 msb, uint32_t upperValue) const {
        // Base case: If reached the last bit, upperValue equals an exact value, just return
        // the statements from that CaseItem. Note: Not cloning here as the caller will do
        // an identity check.
        만약 (msb < 0) 반환 m_caseDetails.records[upperValue].stmtsp;

        // Recursive case:
        // Make left and right subtrees assuming cexpr[msb] is 0 and 1 respectively
        const uint32_t upperValue0 = upperValue;
        const uint32_t upperValue1 = upperValue | (1UL << msb);
        AstNode* tree0p = convertCaseFastRecurse(cexprp, msb - 1, upperValue0);
        AstNode* tree1p = convertCaseFastRecurse(cexprp, msb - 1, upperValue1);

        // If same logic on both sides, we can just return one of them
        만약 (tree0p == tree1p) 반환 tree0p;

        // We could have a "checkerboard" with A B A B, we can use the same IF on both edges
        {
            bool same = true;
            위해 (uint32_t a = upperValue0, b = upperValue1; a < upperValue1; ++a, ++b) {
                만약 (m_caseDetails.records[a].stmtsp != m_caseDetails.records[b].stmtsp) {
                    same = false;
                    break;
                }
            }
            만약 (same) {
                VL_DO_DANGLING(tree1p->deleteTree(), tree1p);
                반환 tree0p;
            }
        }

        // Must have differing logic. Test the bit and convert to an If.

        // Clone if needed
        만약 (tree0p && tree0p->backp()) tree0p = tree0p->cloneTree(true);
        만약 (tree1p && tree1p->backp()) tree1p = tree1p->cloneTree(true);
        // Create the If statement
        FileLine* const flp = cexprp->fileline();
        AstNodeExpr* const condp = new AstSel{flp, cexprp->cloneTreePure(false), msb, 1};
        AstIf* const ifp = new AstIf{flp, condp, tree1p, tree0p};
        반환 ifp;
    }

    // CASEx(cexpr,....
    // ->  tree of IF(msb,  IF(msb-1, 11, 10)
    //                      IF(msb-1, 01, 00))
    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCaseFast(AstCase* nodep) {
        ++m_stats.caseFast;
        const 정수 caseWidth = nodep->exprp()->width();
        AstNode* const ifrootp = convertCaseFastRecurse(nodep->exprp(), caseWidth - 1, 0UL);
        반환 ifrootp && ifrootp->backp() ? ifrootp->cloneTree(true) : ifrootp;
    }

    AstNodeExpr* newCaseCompare(const AstCase* casep, AstNodeExpr* itemExprp) {
        FileLine* const flp = itemExprp->fileline();
        만약 (neverItem(casep, itemExprp)) {
            VL_DO_DANGLING(pushDeletep(itemExprp), itemExprp);
            반환 new AstConst{flp, AstConst::BitFalse{}};
        }

        만약 (AstCond* const condp = VN_CAST(itemExprp, Cond)) {
            // Bounds checks can leave X-valued branches in case items. Compare each
            // branch using case semantics before lowering to two-state expressions.
            AstNodeExpr* const testp = condp->condp()->unlinkFrBack();
            AstNodeExpr* const thenp = newCaseCompare(casep, condp->thenp()->unlinkFrBack());
            AstNodeExpr* const elsep = newCaseCompare(casep, condp->elsep()->unlinkFrBack());
            VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, condp);
            반환 new AstCond{flp, testp, thenp, elsep};
        }

        AstNodeExpr* const caseExprp = casep->exprp()->cloneTreePure(false);
        // InsideRange: Similar logic in V3Width::visit(AstInside)
        만약 (AstInsideRange* const itemRangep = VN_CAST(itemExprp, InsideRange)) {
            AstNodeExpr* const resultp = itemRangep->newAndFromInside(
                caseExprp, itemRangep->lhsp()->unlinkFrBack(), itemRangep->rhsp()->unlinkFrBack());
            VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, itemRangep);
            반환 resultp;
        }

        만약 (AstConst* const itemConstp = VN_CAST(itemExprp, Const)) {
            // neverItem has already rejected X/Z values that cannot match.
            만약 (itemConstp->num().isFourState()) {
                const auto& match = matchPattern(casep, itemConstp);
                const V3Number& matchMask = match.first;
                const V3Number& matchBits = match.second;
                VL_DO_DANGLING2(pushDeletep(itemExprp), itemExprp, itemConstp);
                반환 AstEq::newTyped(flp, new AstConst{flp, matchBits},
                                       new AstAnd{flp, caseExprp, new AstConst{flp, matchMask}});
            }
        }

        반환 AstEq::newTyped(flp, caseExprp, itemExprp);
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
        bool hasDefault = false;
        위해 (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            FileLine* const flp = itemp->fileline();

            // Default clause. Just make true, we'll optimize it away later
            만약 (itemp->isDefault()) {
                itemp->addCondsp(new AstConst{flp, AstConst::BitTrue{}});
                hasDefault = true;
                continue;
            }

            // Regular clause. Construct the condition expression.
            AstNodeExpr* newCondp = nullptr;
            위해 (AstNodeExpr *itemExprp = itemp->condsp(), *nextp; itemExprp; itemExprp = nextp) {
                nextp = VN_AS(itemExprp->nextp(), NodeExpr);
                itemExprp->unlinkFrBack();

                // If case never matches, ignore it
                만약 (neverItem(nodep, itemExprp)) {
                    VL_DO_DANGLING(pushDeletep(itemExprp), itemExprp);
                    continue;
                }

                // Compute the term to add to the condition expression
                AstNodeExpr* const termp = newCaseCompare(nodep, itemExprp);

                // 'Or' new term with previous terms
                newCondp = newCondp ? new AstLogOr{flp, newCondp, termp} : termp;
            }
            // Replace expression in tree. Needs to be non-null, so add a constant false if
            // needed
            만약 (!newCondp) newCondp = new AstConst{flp, AstConst::BitFalse{}};
            itemp->addCondsp(newCondp);
        }

        // If there was no default, make one up, this greatly simplifies below code
        // and constant propagation will just eliminate it for us later.
        만약 (!hasDefault) {
            만약 (m_caseDetailsValid && m_caseDetails.exhaustive
                && !m_caseDetails.exhaustiveOverEnumOnly) {
                // Case is proven exhaustive, so the last is reached only when its test holds,
                // make it unconditional
                AstCaseItem* const lastp = VN_AS(nodep->itemsp()->lastp(), CaseItem);
                pushDeletep(lastp->condsp()->unlinkFrBackWithNext());
                lastp->addCondsp(new AstConst{lastp->fileline(), AstConst::BitTrue{}});
            } 아니면 {
                // Otherwise needs an explicit empty default
                nodep->addItemsp(new AstCaseItem{
                    nodep->fileline(), new AstConst{nodep->fileline(), AstConst::BitTrue{}},
                    nullptr});
            }
        }

        // Now build the IF statement tree
        // The tree can be quite huge.  Pull every group of 8 out, and make a OR tree.
        // This reduces the depth for the bottom elements, at the cost of
        // some of the top elements.  If we ever have profiling data, we
        // should pull out the most common item from here and instead make
        // it the first IF branch.
        정수 depth = 0;
        AstIf* grouprootp = nullptr;
        AstIf* groupnextp = nullptr;
        AstIf* itemnextp = nullptr;
        위해 (AstCaseItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), CaseItem)) {

            // Grab the statements from this item. May be empty.
            AstNode* const istmtsp = itemp->stmtsp();
            만약 (istmtsp) istmtsp->unlinkFrBackWithNext();

            // Expressioned clause
            AstNodeExpr* const ifexprp = itemp->condsp()->unlinkFrBack();
            {  // Prepare for next group
                만약 (++depth > CASE_ENCODER_GROUP_DEPTH) depth = 1;
                만약 (depth == 1) {  // First group or starting new group
                    itemnextp = nullptr;
                    AstIf* const newp = new AstIf{itemp->fileline(), ifexprp->cloneTreePure(true)};
                    만약 (groupnextp) {
                        groupnextp->addElsesp(newp);
                    } 아니면 {
                        grouprootp = newp;
                    }
                    groupnextp = newp;
                } 아니면 {  // Continue group, modify if condition to OR in this new condition
                    AstNodeExpr* const condp = groupnextp->condp()->unlinkFrBack();
                    groupnextp->condp(
                        new AstOr{ifexprp->fileline(), condp, ifexprp->cloneTreePure(true)});
                }
            }
            {  // Make the new lower IF and attach in the tree
                AstNodeExpr* itemexprp = ifexprp;
                VL_DANGLING(ifexprp);
                만약 (depth == CASE_ENCODER_GROUP_DEPTH) {  // End of group - can skip the condition
                    VL_DO_DANGLING(itemexprp->deleteTree(), itemexprp);
                    itemexprp = new AstConst{itemp->fileline(), AstConst::BitTrue{}};
                }
                AstIf* const newp = new AstIf{itemp->fileline(), itemexprp, istmtsp};
                만약 (itemnextp) {
                    itemnextp->addElsesp(newp);
                } 아니면 {
                    groupnextp->addThensp(newp);  // First in a new group
                }
                itemnextp = newp;
            }
        }
        반환 grouprootp;
    }

    // Convert the given case statement to a representation not using AstCase
    // TODO: should return AstNodeStmt after #6280
    AstNode* convertCase(AstCase* nodep) {
        // Determine if we should use the lookup table method
        const bool useTable = [&]() {
            // Not if disabled
            만약 (!v3Global.opt.fCaseTable()) 반환 false;
            // Not if analysis tells us we can't
            만약 (!m_caseTableWidth) 반환 false;
            // Always if tiny - it is materialized inline, so there is no load to amortize
            만약 (m_caseTableWidth <= CASE_TABLE_TINY_BITS) 반환 true;
            // For a normal (constant-pool) table, weigh the indexed load against the branch
            // lowering it would replace. That lowering's depth is bounded by the selector
            // width (a balanced bit tree tests ~one bit per level) and by the number of
            // distinct values (a generic if/else does ~one compare per value). A few compares
            // are cheaper than a load that is likely to be a cache miss, so only table once that
            // depth is exceeded.
            const size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            만약 (branches < CASE_TABLE_MIN_BRANCHES) 반환 false;
            반환 true;
        }();
        만약 (useTable) 반환 convertCaseTable(nodep);

        // Determine if we should use the decoder method.
        const bool useDecoder = [&]() {
            // Not if disabled
            만약 (!v3Global.opt.fCaseDecoder()) 반환 false;
            // Not if not a decoder pattern
            만약 (m_caseDecoderRecords.empty()) 반환 false;
            // Only worth it once the branch lowering it would replace is deep enough (see
            // useTable)
            const size_t branches = std::min<size_t>(nodep->exprp()->width(), m_caseNConditions);
            만약 (branches < CASE_TABLE_MIN_BRANCHES) 반환 false;
            반환 true;
        }();
        만약 (useDecoder) 반환 convertCaseDecoder(nodep);

        // Determine if we should use the fast bitwise branching tree method
        const bool useFastBitTree = [&]() {
            // Not if disabled
            만약 (!v3Global.opt.fCaseTree()) 반환 false;
            // Can't do it without the detailed analysis
            만약 (!m_caseDetailsValid) 반환 false;
            // Can't do it if not exhaustive
            만약 (!m_caseDetails.exhaustive) 반환 false;
            // Not worth doing if there are few conditions
            만약 (m_caseNConditions <= 3) 반환 false;
            // Avoid e.g. priority expanders from going crazy in expansion
            const size_t caseWidth = nodep->exprp()->width();
            만약 (caseWidth >= 8 && m_caseNConditions <= (caseWidth + 1)) 반환 false;
            // Otherwise use the bit tree
            반환 true;
        }();
        만약 (useFastBitTree) 반환 convertCaseFast(nodep);

        // Convert using the generic if/else tree method
        // If a case statement is exhaustive, presume signals involved aren't forming a latch
        // TODO: this is broken, but it is as was before
        만약 (m_alwaysp && (!m_caseDetailsValid || m_caseDetails.exhaustive)) {
            m_alwaysp->fileline()->warnOff(V3ErrorCode::LATCH, true);
        }
        반환 convertCaseGeneric(nodep);
    }

    // VISITORS
    void visit(AstCase* nodep) override {
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
        만약 (AstNode* const stmtp = nodep->notParallelp()) {
            stmtp->unlinkFrBackWithNext();
            만약 (m_caseDetailsValid && m_caseDetails.exhaustive && m_caseDetails.noOverlaps) {
                ++m_stats.provenAssertions;
                VL_DO_DANGLING(stmtp->deleteTree(), stmtp);
            } 아니면 {
                nodep->addNextHere(stmtp);
            }
        }

        // Convert the case statement and replace the original
        만약 (AstNode* const replacementp = convertCase(nodep)) {
            nodep->replaceWith(replacementp);
        } 아니면 {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }

    void visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }

    void visit(AstAlways* nodep) override {
        VL_RESTORER(m_alwaysp);
        m_alwaysp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
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

void V3Case::caseAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { CaseVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("case", 0, dumpTreeEitherLevel() >= 3);
}
void V3Case::caseLint(AstGenCase* nodep) {
    UINFO(4, __FUNCTION__ << ": ");
    CaseLintVisitor::apply(nodep);
}
