// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Expression width calculations
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
// V3Width's Transformations:
//      Top down traversal:
//          Determine width of sub-expressions
//              width() = # bits upper expression wants, 0 for anything-goes
//              widthUnsized() = # bits for unsized constant, or 0 if it's sized
//              widthMin() = Alternative acceptable width for linting, or width() if sized
//              Determine this subop's width, can be either:
//                  Fixed width X
//                  Unsized, min width X   ('d5 is unsized, min 3 bits.)
//              Pass up:
//                  width() = # bits this expression generates
//                  widthSized() = true if all constants sized, else false
//          Compute size of this expression
//          Lint warn about mismatches
//              If expr size != subop fixed, bad
//              If expr size  < subop unsized minimum, bad
//              If expr size != subop, edit netlist
//                      For == and similar ops, if multibit underneath, add a REDOR
//                      If subop larger, add a EXTRACT
//                      If subop smaller, add a EXTEND
//          Pass size to sub-expressions if required (+/-* etc)
//              FINAL = true.
//              Subexpressions lint and extend as needed
//
//*************************************************************************
// Signedness depends on:
//      Decimal numbers are signed
//      Based numbers are unsigned unless 's' prefix
//      Comparison results are unsigned
//      Bit&Part selects are unsigned, even if whole
//      Concatenates are unsigned
//      Ignore signedness of self-determined:
//              shift rhs, ** rhs, x?: lhs, concat and replicate members
//      Else, if any operand unsigned, output unsigned
//
// Real number rules:
//      Real numbers are real (duh)
//      Reals convert to integers by rounding
//      Reals init to 0.0
//      Logicals convert compared to zero
//      If any operand is real, result is real
//*************************************************************************
// V3Width is the only visitor that uses vup.  We could switch to using userp,
// though note some iterators operate on next() and so would need to pass the
// same value on each nextp().
//*************************************************************************
// See notes in internal.txt about misuse of iterateAndNext and use of
// iterateSubtreeReturnEdits.
//*************************************************************************

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3Width.h"

#ಅನ್ತರ್ಭಾವ "V3Assert.h"
#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Begin.h"
#ಅನ್ತರ್ಭಾವ "V3Const.h"
#ಅನ್ತರ್ಭಾವ "V3ConstPool.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3LinkLValue.h"
#ಅನ್ತರ್ಭಾವ "V3MemberMap.h"
#ಅನ್ತರ್ಭಾವ "V3Number.h"
#ಅನ್ತರ್ಭಾವ "V3Randomize.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"
#ಅನ್ತರ್ಭಾವ "V3UniqueNames.h"
#ಅನ್ತರ್ಭಾವ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#ಅನ್ತರ್ಭಾವ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ವರ್ಗ ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ಧ್ರುವ Ast_ಗಂಟು*, AstClass*> m_cache;

    AstClass* findCached(Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ auto it = m_cache.find(nodep);
        ಯದಿ (it != m_cache.end()) ಪ್ರತ್ಯಾಗಮ it->second;

        AstClass* classp = nullptr;
        ಯದಿ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } ಅನ್ಯಥಾ ಯದಿ (AstClass* ಧ್ರುವ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ಅನ್ಯಥಾ ಯದಿ (AstClassPackage* ಧ್ರುವ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ಅನ್ಯಥಾ {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ಪ್ರತ್ಯಾಗಮ classp;
    }

ಸಾರ್ವಜನಿಕ:
    AstClass* find(Ast_ಗಂಟು* nodep) {
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) ಪ್ರತ್ಯಾಗಮ classp;
        ಯದಿ (AstClassPackage* ಧ್ರುವ packagep = VN_CAST(nodep, ClassPackage)) {
            ಪ್ರತ್ಯಾಗಮ packagep->classp();
        }
        ಪ್ರತ್ಯಾಗಮ findCached(nodep);
    }
};

//######################################################################

ಗಣನಾ Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, ಧ್ರುವ Stage& rhs) {
    ಪ್ರತ್ಯಾಗಮ str << ("-PFB"[static_cast<ಪೂರ್ಣಾಙ್ಕ>(rhs)]);
}

ಗಣನಾ Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, ಧ್ರುವ Determ& rhs) {
    ಸ್ಥಿರ constexpr ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ s_det[] = {"SELF", "CNTX", "ASSN"};
    ಪ್ರತ್ಯಾಗಮ str << s_det[rhs];
}

ಗಣನಾ StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, ಸಂದೇಶ) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               ಸಂದೇಶ)

//######################################################################
// Width state, as a visitor of each AstNode

ವರ್ಗ WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ಗಂಟು_DType* ಧ್ರುವ m_dtypep;  // Parent's data type to resolve to
    ಧ್ರುವ Stage m_stage;  // If true, report errors
    ಧ್ರುವ StreamUse m_streamUse;  // Current expression may be a stream concat

ಸಾರ್ವಜನಿಕ:
    WidthVP(Ast_ಗಂಟು_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    WidthVP(Determ determ, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
        : m_dtypep{nullptr}
        , m_stage{stage}
        , m_streamUse{streamUse} {
        ಯದಿ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { ಪ್ರತ್ಯಾಗಮ ಏತತ್; }
    ತಾರ್ಕಿಕ selfDtm() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dtypep == nullptr; }
    StreamUse streamUse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_streamUse; }
    Ast_ಗಂಟು_DType* dtypep() ಧ್ರುವ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ಪ್ರತ್ಯಾಗಮ m_dtypep;
    }
    Ast_ಗಂಟು_DType* dtypeNullp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_dtypep; }
    Ast_ಗಂಟು_DType* dtypeNullSkipRefp() ಧ್ರುವ {
        Ast_ಗಂಟು_DType* dtp = dtypeNullp();
        ಯದಿ (dtp) dtp = dtp->skipRefp();
        ಪ್ರತ್ಯಾಗಮ dtp;
    }
    Ast_ಗಂಟು_DType* dtypeOverridep(Ast_ಗಂಟು_DType* ಧ್ರುವ defaultp) ಧ್ರುವ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ಯದಿ (!m_dtypep) ಪ್ರತ್ಯಾಗಮ defaultp;
        ಯದಿ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ಪ್ರತ್ಯಾಗಮ defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ಪ್ರತ್ಯಾಗಮ m_dtypep;
    }
    ಪೂರ್ಣಾಙ್ಕ width() ಧ್ರುವ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ಪ್ರತ್ಯಾಗಮ m_dtypep->width();
    }
    ಪೂರ್ಣಾಙ್ಕ widthMin() ಧ್ರುವ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ಪ್ರತ್ಯಾಗಮ m_dtypep->widthMin();
    }
    ತಾರ್ಕಿಕ prelim() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stage & PRELIM; }
    ತಾರ್ಕಿಕ final() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_stage & FINAL; }
    ಶೂನ್ಯ dump(std::ostream& str) ಧ್ರುವ {
        ಯದಿ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ಅನ್ಯಥಾ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, ಧ್ರುವ WidthVP* vup) {
    ಯದಿ (vup) vup->dump(str);
    ಪ್ರತ್ಯಾಗಮ str;
}

//######################################################################

ವರ್ಗ WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    ಶೂನ್ಯ clearWidthRecurse(Ast_ಗಂಟು* nodep) {
        ಪ್ರತಿ (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(ಅಸತ್ಯ);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) clearWidthRecurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) clearWidthRecurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) clearWidthRecurse(refp);
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    ಆಭಾಸ ~WidthClearVisitor() = ಸ್ವಭಾವ;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_ಗಂಟು_iterate_instead_of_Ast_ಗಂಟು_accept

//######################################################################

ವರ್ಗ WidthVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    using PatVecMap = std::map<ಪೂರ್ಣಾಙ್ಕ, AstPatMember*>;
    using DTypeMap = std::map<ಧ್ರುವ std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ಧ್ರುವ AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    ತಾರ್ಕಿಕ m_underFork = ಅಸತ್ಯ;  // Visiting under a fork
    ತಾರ್ಕಿಕ m_underSExpr = ಅಸತ್ಯ;  // Visiting under a sequence expression
    ತಾರ್ಕಿಕ m_underPackedArray = ಅಸತ್ಯ;  // Visiting under a AstPackArrayDType
    ತಾರ್ಕಿಕ m_underMemberSel = ಅಸತ್ಯ;  // Viting under a MemberSel
    Ast_ಗಂಟು* m_seqUnsupp = nullptr;  // Property has unsupported node
    ತಾರ್ಕಿಕ m_hasSExpr = ಅಸತ್ಯ;  // Property has a sequence expression
    ಧ್ರುವ AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    ಧ್ರುವ AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_ಗಂಟು_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_ಗಂಟು_Module* m_modep = nullptr;  // Current module
    ಧ್ರುವ AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_ಗಂಟು_Procedure* m_procedurep = nullptr;  // Current final/always
    ಧ್ರುವ AstWith* m_withp = nullptr;  // Current 'with' statement
    ಧ್ರುವ AstFunc* m_funcp = nullptr;  // Current function
    ಧ್ರುವ AstAttrOf* m_attrp = nullptr;  // Current attribute
    ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    ಧ್ರುವ ತಾರ್ಕಿಕ m_paramsOnly;  // Computing parameter value; limit operation
    ಧ್ರುವ ತಾರ್ಕಿಕ m_doGenerate;  // Do errors later inside generate statement
    ತಾರ್ಕಿಕ m_streamConcat = ಅಸತ್ಯ;  // True if visiting arguments of stream concatenation
    ತಾರ್ಕಿಕ m_inParamOverride = ಅಸತ್ಯ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ಧ್ರುವ Ast_ಗಂಟು_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<ಧ್ರುವ Ast_ಗಂಟು_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<ಧ್ರುವ AstVar*> m_curModVars;  // Variables declared in current module

    ಸ್ಥಿರ constexpr ಪೂರ್ಣಾಙ್ಕ ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ಗಣನಾ ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ಗಣನಾ ವರ್ಗ ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ಸ್ಥಿರ StreamUse streamUseForDeterm(Determ determ) {
        ಪ್ರತ್ಯಾಗಮ determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    ಸ್ಥಿರ ಶೂನ್ಯ packIfUnpacked(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nodep) {
        ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements_ಅಂಕೆ = unpackDTypep->arrayUnpackedElements();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ unpackMinBits = elements_ಅಂಕೆ * unpackDTypep->subDTypep()->widthMin();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ unpackBits = elements_ಅಂಕೆ * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(ನೂತನ AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ಸ್ಥಿರ Ast_ಗಂಟು_DType* fromDTypep(Ast_ಗಂಟು* fromp) {
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = VN_CAST(fromp, ಗಂಟು_DType))
            ಪ್ರತ್ಯಾಗಮ dtypep->skipRefOrNullp();
        ಪ್ರತ್ಯಾಗಮ fromp ? fromp->dtypep() : nullptr;
    }
    // VISITORS
    //   Naming:  width_O{outputtype}_L{lhstype}_R{rhstype}_W{widthing}_S{signing}
    //          Where type:
    //                  _O1=boolean (width 1 unsigned)
    //                  _Ou=unsigned
    //                  _Os=signed
    //                  _Ous=unsigned or signed
    //                  _Or=real
    //                  _Ox=anything

    // Widths: 1 bit out, lhs 1 bit; Real: converts via compare with 0
    ಶೂನ್ಯ visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    ಶೂನ್ಯ visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    ಶೂನ್ಯ visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    ಶೂನ್ಯ visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    ಶೂನ್ಯ visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    ಶೂನ್ಯ visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    ಶೂನ್ಯ visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    ಶೂನ್ಯ visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    ಶೂನ್ಯ visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    ಶೂನ್ಯ visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    ಶೂನ್ಯ visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    ಶೂನ್ಯ visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    ಶೂನ್ಯ visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    ಶೂನ್ಯ visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    ಶೂನ್ಯ visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    ಶೂನ್ಯ visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ಶೂನ್ಯ visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ಶೂನ್ಯ visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ಶೂನ್ಯ visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    ಶೂನ್ಯ visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    ಶೂನ್ಯ visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    ಶೂನ್ಯ visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    ಶೂನ್ಯ visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    ಶೂನ್ಯ visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    ಶೂನ್ಯ visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    ಶೂನ್ಯ visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    ಶೂನ್ಯ visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    ಶೂನ್ಯ visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    ಶೂನ್ಯ visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    ಶೂನ್ಯ visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    ಶೂನ್ಯ visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    ಶೂನ್ಯ visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    ಶೂನ್ಯ visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    ಶೂನ್ಯ visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    ಶೂನ್ಯ visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    ಶೂನ್ಯ visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    ಶೂನ್ಯ visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    ಶೂನ್ಯ visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    ಶೂನ್ಯ visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, ಸತ್ಯ, ಅಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstSub* nodep) override { visit_add_sub_replace(nodep, ಸತ್ಯ, ಅಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, ಸತ್ಯ, ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstMul* nodep) override { visit_add_sub_replace(nodep, ಸತ್ಯ, ಅಸತ್ಯ); }
    // These can't promote to real
    ಶೂನ್ಯ visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, ಅಸತ್ಯ, ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, ಅಸತ್ಯ, ಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ); }
    ಶೂನ್ಯ visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, ಅಸತ್ಯ, ಸತ್ಯ); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    ಶೂನ್ಯ visit(AstNegate* nodep) override { visit_negate_not(nodep, ಸತ್ಯ); }
    // Unary never real
    ಶೂನ್ಯ visit(AstNot* nodep) override { visit_negate_not(nodep, ಅಸತ್ಯ); }

    // Real: inputs and output real
    ಶೂನ್ಯ visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    ಶೂನ್ಯ visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    ಶೂನ್ಯ visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    ಶೂನ್ಯ visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    ಶೂನ್ಯ visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    ಶೂನ್ಯ visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    ಶೂನ್ಯ visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    ಶೂನ್ಯ visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    ಶೂನ್ಯ visit(AstShiftL* nodep) override { visit_shift(nodep); }
    ಶೂನ್ಯ visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    ಶೂನ್ಯ visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    ಶೂನ್ಯ visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    ಶೂನ್ಯ visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    ಶೂನ್ಯ visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    ಶೂನ್ಯ visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    ಶೂನ್ಯ visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ಯದಿ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ಶೂನ್ಯ visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ಯದಿ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    ಶೂನ್ಯ visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ಯದಿ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ಶೂನ್ಯ visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ಯದಿ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ಶೂನ್ಯ visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ಯದಿ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ಶೂನ್ಯ visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ಶೂನ್ಯ visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ಯದಿ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } ಅನ್ಯಥಾ {
                nodep->dtypeSetInteger();
            }
        }
    }
    ಶೂನ್ಯ visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    ಶೂನ್ಯ visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    ಶೂನ್ಯ visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    ಶೂನ್ಯ visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    ಶೂನ್ಯ visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    ಶೂನ್ಯ visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            ನೂತನ AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isOverridePattern(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (ಧ್ರುವ AstPattern* ಧ್ರುವ patternp = VN_CAST(nodep, Pattern)) {
            ಪ್ರತ್ಯಾಗಮ !patternp->childDTypep();
        }
        ಧ್ರುವ AstCond* ಧ್ರುವ condp = VN_CAST(nodep, Cond);
        ಪ್ರತ್ಯಾಗಮ condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    ಶೂನ್ಯ visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ errors = V3Error::error_ಎಣಿಕೆ();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = ಅಸತ್ಯ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->condp(), Const);
            ಧ್ರುವ ತಾರ್ಕಿಕ known = constp && !constp->ಅಂಕೆ().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ಯದಿ (!known && V3Error::error_ಎಣಿಕೆ() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            userIterateAndNext(nodep->thenp(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            userIterateAndNext(nodep->elsep(), WidthVP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_ಗಂಟು_DType* ಧ್ರುವ thenDTypep = nodep->thenp()->dtypep();
            Ast_ಗಂಟು_DType* ಧ್ರುವ elseDTypep = nodep->elsep()->dtypep();
            Ast_ಗಂಟು_DType* ಧ್ರುವ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ಗಂಟು_DType* ಧ್ರುವ elseDTypeSkippedp = elseDTypep->skipRefp();
            ಯದಿ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->thenp()->is_Class_Handle_ಮೌಲ್ಯ()
                       || nodep->elsep()->is_Class_Handle_ಮೌಲ್ಯ()) {
                Ast_ಗಂಟು_DType* commonClassTypep = nullptr;
                ಯದಿ (nodep->thenp()->is_Class_Handle_ಮೌಲ್ಯ() && nodep->elsep()->is_Class_Handle_ಮೌಲ್ಯ()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ಗಂಟು::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ಯದಿ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ಅನ್ಯಥಾ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ಅನ್ಯಥಾ ಯದಿ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ಯದಿ (Ast_ಗಂಟು::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ಅನ್ಯಥಾ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ಯದಿ (!Ast_ಗಂಟು::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ಅನ್ಯಥಾ ಯದಿ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } ಅನ್ಯಥಾ ಯದಿ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ ತಾರ್ಕಿಕ isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                ತಾರ್ಕಿಕ handled = ಅಸತ್ಯ;
                ಧ್ರುವ ತಾರ್ಕಿಕ isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ಗಂಟು_DType* ಧ್ರುವ thenEnumDTypep = thenDTypep->skipRefToEnump();
                ಯದಿ (VN_IS(thenDTypeSkippedp, ಗಂಟು_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_ವೃಕ್ಷ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = ಸತ್ಯ;
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_ವೃಕ್ಷ(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ಯದಿ (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(ನೂತನ File_ಸಾಲು{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, ಸತ್ಯ);
                    }
                }
                ಯದಿ (!handled) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(thenDTypep->width(), elseDTypep->width());
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    ಧ್ರುವ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ಧ್ರುವ ತಾರ್ಕಿಕ isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ಯದಿ ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ಅನ್ಯಥಾ ಯದಿ ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ಅನ್ಯಥಾ {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* ಧ್ರುವ newp = ನೂತನ AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Concat handling
        ಯದಿ (m_vup->prelim()) {
            ಯದಿ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ಯದಿ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } ಅನ್ಯಥಾ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ಯದಿ (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ಯದಿ (ಧ್ರುವ AstReplicate* ಧ್ರುವ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ಯದಿ (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (ಧ್ರುವ AstReplicate* ಧ್ರುವ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ಯದಿ (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
        }
        ಯದಿ (m_vup->final()) {
            ಯದಿ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* rhsp = nodep->rhsp()->unlinkFrBack();
                ಯದಿ (!lhsp->isString()) lhsp = ನೂತನ AstCvtPackString{lhsp->fileline(), lhsp};
                ಯದಿ (!rhsp->isString()) rhsp = ನೂತನ AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ಶೂನ್ಯ visit(AstConcatN* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        ಯದಿ (m_vup->final()) {
            nodep->didWidth(ಸತ್ಯ);
            ಯದಿ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ಶೂನ್ಯ visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    ಶೂನ್ಯ visit(AstDelay* nodep) override {
        ಯದಿ (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            ಯದಿ (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ಯದಿ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->fileline()->timingOn()) {
            ಯದಿ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ಅನ್ಯಥಾ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ಯದಿ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    ಶೂನ್ಯ visit(AstFork* nodep) override {
        ಯದಿ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ಧ್ರುವ newp = ನೂತನ AstBegin{nodep->fileline(), nodep->name(), nullptr, ಅಸತ್ಯ};
            nodep->replaceWith(newp);
            ಯದಿ (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            ಯದಿ (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            ಯದಿ (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = ಸತ್ಯ;
            iterateChildren(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    ಶೂನ್ಯ visitWaitOrDisableFork(Ast_ಗಂಟು* nodep) {
        ಯದಿ (nodep->fileline()->timingOn()) {
            ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ಶೂನ್ಯ visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ಶೂನ್ಯ visit(AstToLowerN* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ಶೂನ್ಯ visit(AstToUpperN* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ಶೂನ್ಯ visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->countp(), Const);
            ಯದಿ (constp) {
                ಯದಿ (constp->ಅಂಕೆ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ಅನ್ಯಥಾ {
                    times = constp->toSInt();
                }
            }

            Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypeNullSkipRefp();
            ಯದಿ (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ಯದಿ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ಯದಿ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* ಧ್ರುವ newp = ನೂತನ AstPattern{
                        nodep->fileline(),
                        ನೂತನ AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            ಯದಿ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ಯದಿ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ಅನ್ಯಥಾ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ಯದಿ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ಗಂಟು* ಧ್ರುವ newp
                    = ನೂತನ AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                ಯದಿ (!constp) nodep->v3error("Replication value isn't a constant.");
                ಯದಿ (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    (nodep->srcp()->width() * times), (nodep->srcp()->widthMin() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        ಯದಿ (m_vup->final()) {
            ಯದಿ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ಶೂನ್ಯ visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        ಯದಿ (m_vup->final()) {
            ಯದಿ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Dist_Biop* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Dist_Triop* nodep) override {
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    ತಾರ್ಕಿಕ streamImplicitUseAllowed(ಧ್ರುವ Ast_ಗಂಟು_Stream* nodep) ಧ್ರುವ {
        ಯದಿ (m_streamConcat) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_vup && m_vup->streamUse() != STREAM_USE_NONE) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;

        ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ backp = nodep->backp();
        ಯದಿ (!backp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;  // Error elsewhere

        ಪ್ರತ್ಯಾಗಮ VN_IS(backp, ಗಂಟು_Assign);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            m_streamConcat = ಸತ್ಯ;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = ಅಸತ್ಯ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->rhsp(), Const)) {
                ಯದಿ (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ಅನ್ಯಥಾ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            ಯದಿ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ಗಂಟು_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ಗಂಟು_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } ಅನ್ಯಥಾ ಯದಿ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } ಅನ್ಯಥಾ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ಯದಿ (m_vup->final()) {
            ಯದಿ (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            ಯದಿ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                ಗಂಟು_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    ಶೂನ್ಯ visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), ಸತ್ಯ,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), ಸತ್ಯ,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ಯದಿ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->elementsConst();
            ಯದಿ (nodep->fromBracket() && nodep->ಎಡ_Const() > nodep->ಬಲ_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ಅನ್ಯಥಾ ಯದಿ (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ಧ್ರುವ ತಾರ್ಕಿಕ inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ಧ್ರುವ ತಾರ್ಕಿಕ inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ಧ್ರುವ ತಾರ್ಕಿಕ inTypeTable = !m_modep;
            ಧ್ರುವ Ast_ಗಂಟು* basep = nodep->backp();
            ಯಾವತ್ (VN_IS(basep, Range)) basep = basep->backp();
            ಯದಿ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ಎಡ_Const() << ":" << nodep->ಬಲ_Const()
                                            << "]");
            }
        }
    }

    ಶೂನ್ಯ visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), ಅಸತ್ಯ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            ಪೂರ್ಣಾಙ್ಕ width = nodep->widthConst();
            ಯದಿ (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                ಪ್ರತ್ಯಾಗಮ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ಧ್ರುವ ತಾರ್ಕಿಕ isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ಧ್ರುವ ತಾರ್ಕಿಕ inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ಯದಿ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                ತಳ್ಳು_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(ನೂತನ AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ಯದಿ (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                ಯದಿ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ಅಸತ್ಯ /*noerror*/);
                } ಅನ್ಯಥಾ {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << width << " bits to only "
                                                         << nodep->fromp()->width()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            ಪೂರ್ಣಾಙ್ಕ frommsb = nodep->fromp()->width() - 1;
            ಪೂರ್ಣಾಙ್ಕ fromlsb = 0;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            ಯದಿ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ಅನ್ಯಥಾ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ಗಂಟು_DType* ಧ್ರುವ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            ಯದಿ (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                ಯದಿ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->width() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->width() / elw)
                            << (nodep->lsbp()->width() != nodep->lsbp()->widthMin()
                                    ? " or " + cvtToStr(nodep->lsbp()->widthMin() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            ಯದಿ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ಯದಿ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } ಅನ್ಯಥಾ ಯದಿ (!inParameterizedTemplate) {
                    ಯದಿ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ಅನ್ಯಥಾ {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ಯದಿ (!isWriteSelect) {
                    // Extend it
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ extendTo = nodep->msbConst() + 1;
                    Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ಅಸತ್ಯ /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ಯದಿ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                ಅಸತ್ಯ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    ಶೂನ್ಯ visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ಯದಿ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->bitp(), Const)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bit = constp->toSInt();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    ಯದಿ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            //
            ಪೂರ್ಣಾಙ್ಕ frommsb;
            ಪೂರ್ಣಾಙ್ಕ fromlsb;
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ಯದಿ (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ಯದಿ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ಅನ್ಯಥಾ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ಗಂಟು_DType* ಧ್ರುವ selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ಯದಿ (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                ಯದಿ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ಯದಿ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ಯದಿ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ಯದಿ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                ಅಸತ್ಯ /*NOWARN*/);
            }
        }
    }

    ಶೂನ್ಯ visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ಯದಿ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->ಕೀಲಿ_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ಶೂನ್ಯ visit(AstAlias* nodep) override {
        ಯದಿ (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        ಧ್ರುವ auto check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok = [ಏತತ್](ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp) {
            ಯದಿ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }

            ಧ್ರುವ AstVarRef* ಧ್ರುವ varRefp = VN_CAST(exprp, VarRef);
            ಯದಿ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            AstVar* ಧ್ರುವ varp = varRefp->varp();
            ಯದಿ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಯದಿ (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                m_aliasedVars.ಸೇರಿಸು(varp);
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        };

        check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok(nodep->itemsp());
        Ast_ಗಂಟು_DType* ಧ್ರುವ firstItemDtypep = nodep->itemsp()->dtypep();
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_ಅಭಿವ್ಯಕ್ತಿ_Ok(VN_AS(itemp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            ಯದಿ (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    ಶೂನ್ಯ visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಧ್ರುವ AstWildcardArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ಯದಿ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ಯದಿ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ಶೂನ್ಯ visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ಯದಿ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ಗಂಟು_DType* ಧ್ರುವ newDtp
                = ನೂತನ AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          ನೂತನ AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ಯದಿ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ಧ್ರುವ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ಯದಿ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(ನೂತನ AstConsArray{
                        nodep->fileline(), newDtp,
                        ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ ಯದಿ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ಯದಿ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ಧ್ರುವ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ಧ್ರುವ newp = ನೂತನ AstConsArray{nodep->fileline(), newDtp};
                ಧ್ರುವ VNumRange range = nodep->declRange();
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಧ್ರುವ selp = V3Width::widthSelNoIterEdit(nodep);
        ಯದಿ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    ಶೂನ್ಯ visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಧ್ರುವ selp = V3Width::widthSelNoIterEdit(nodep);
        ಯದಿ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    ಶೂನ್ಯ visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಧ್ರುವ selp = V3Width::widthSelNoIterEdit(nodep);
        ಯದಿ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    ಶೂನ್ಯ visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು* ಧ್ರುವ selp = V3Width::widthSelNoIterEdit(nodep);
        ಯದಿ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    ಶೂನ್ಯ visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (m_vup && m_vup->prelim()) {
            ಯದಿ (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ಅನ್ಯಥಾ ಯದಿ (nodep->ಅಂಕೆ().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->ಅಂಕೆ().isString()) {
                nodep->dtypeSetString();
            } ಅನ್ಯಥಾ ಯದಿ (nodep->ಅಂಕೆ().isDouble()) {
                nodep->dtypeChgWidth(nodep->ಅಂಕೆ().width(), nodep->ಅಂಕೆ().sized()
                                                               ? nodep->ಅಂಕೆ().width()
                                                               : nodep->ಅಂಕೆ().widthToFit());
            } ಅನ್ಯಥಾ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->ಅಂಕೆ().width(),
                    nodep->ಅಂಕೆ().sized() ? nodep->ಅಂಕೆ().width() : nodep->ಅಂಕೆ().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->ಅಂಕೆ().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    ಶೂನ್ಯ visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        ಯದಿ (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    ಶೂನ್ಯ visit(AstFell* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstFalling* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstFuture* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    ಶೂನ್ಯ visit(AstPast* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            nodep->dtypeFrom(nodep->exprp());
            ಯದಿ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ ಸತ್ಯ);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->ticksp(), Const);
                ಯದಿ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                } ಅನ್ಯಥಾ ಯದಿ (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                } ಅನ್ಯಥಾ ಯದಿ (V3AssertCommon::check_Sva_ಮೌಲ್ಯ_Limit(nodep, constp, "$past tick value")) {
                    ಯದಿ (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    ಶೂನ್ಯ visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ಧ್ರುವ AstConst* ಧ್ರುವ minConstp = VN_CAST(nodep->countp(), Const);
            ಯದಿ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ಅನ್ಯಥಾ ಯದಿ (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ಅನ್ಯಥಾ ಯದಿ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ಯದಿ (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ಧ್ರುವ AstConst* ಧ್ರುವ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ಯದಿ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ಅನ್ಯಥಾ ಯದಿ (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ಅನ್ಯಥಾ ಯದಿ (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* propp = nodep->propp();
            ಯದಿ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ಯದಿ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ಧ್ರುವ ತಾರ್ಕಿಕ loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ಧ್ರುವ ತಾರ್ಕಿಕ hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ಯದಿ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ಗಂಟು* ಧ್ರುವ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಧ್ರುವ AstConst* ಧ್ರುವ loConstp = VN_CAST(nodep->loBoundp(), Const);
            ಧ್ರುವ AstConst* ಧ್ರುವ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ಯದಿ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ಯದಿ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ಯದಿ (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ಅನ್ಯಥಾ ಯದಿ (loConstp) {
                V3AssertCommon::check_Sva_ಮೌಲ್ಯ_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ಯದಿ (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_ಮೌಲ್ಯ_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ಯದಿ (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ತಾರ್ಕಿಕ hasPropertyOp = propp->isMultiCycleSva();
            ಯದಿ (!hasPropertyOp) {
                propp->foreach([&](ಧ್ರುವ Ast_ಗಂಟು* np) {
                    ಯದಿ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = ಸತ್ಯ;
                    }
                });
            }
            ಯದಿ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstRising* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstRose* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    ಶೂನ್ಯ visit(AstSampled* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    ಶೂನ್ಯ visit(AstSetuphold* nodep) override {
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        ಯದಿ (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        ಯದಿ (nodep->deldatap()) {
            ಯದಿ (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } ಅನ್ಯಥಾ {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ಯದಿ (!newp) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->replaceWith(ನೂತನ AstAlways{newp});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_ಸಾಲು* ಧ್ರುವ flp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ evp,
                                         Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ delp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = delp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = evp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        UASSERT_OBJ(VN_IS(lhsp, ಗಂಟು_Var_Ref) || VN_IS(lhsp, ಗಂಟು_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        ಯದಿ (Ast_ಗಂಟು_Var_Ref* varRefp = VN_CAST(lhsp, ಗಂಟು_Var_Ref)) {
            ಯದಿ (varRefp->varp()->direction() == VDirection::ಒಳಹರಿವು) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                ಪ್ರತ್ಯಾಗಮ nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        ಯದಿ (Ast_ಗಂಟು_Pre_Sel* selp = VN_CAST(lhsp, ಗಂಟು_Pre_Sel)) {
            ಯದಿ (Ast_ಗಂಟು_Var_Ref* varRefp = VN_CAST(selp->fromp(), ಗಂಟು_Var_Ref)) {
                ಯದಿ (varRefp->varp()->direction() == VDirection::ಒಳಹರಿವು) {
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                    ಪ್ರತ್ಯಾಗಮ nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstAssignW{flp, lhsp, rhsp};
    }

    ಶೂನ್ಯ visit(AstStable* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstSteady* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ಸತ್ಯ);
            nodep->dtypeSetBit();
        }
    }

    ಶೂನ್ಯ visit(Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    ಶೂನ್ಯ visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    ಶೂನ್ಯ visit(AstUntil* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ಯದಿ (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            ಯದಿ (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    ಶೂನ್ಯ visit(AstRand* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            ಯದಿ (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } ಅನ್ಯಥಾ {
                nodep->dtypeSetInt();  // Says the spec
            }
            ಯದಿ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    ಶೂನ್ಯ visit(AstSEventually* nodep) override {
        ಯದಿ (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstSGotoRep* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ಯದಿ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } ಅನ್ಯಥಾ {
                width_Check_Rep_ಎಣಿಕೆ(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstSNonConsRep* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            ಯದಿ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } ಅನ್ಯಥಾ {
                width_Check_Rep_ಎಣಿಕೆ(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    ಶೂನ್ಯ width_Check_Rep_ಎಣಿಕೆ(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->countp(), Const);
        ಯದಿ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ಅನ್ಯಥಾ ಯದಿ (constp->dtypep()->isSigned() && constp->ಅಂಕೆ().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ಅನ್ಯಥಾ ಯದಿ (constp->ಅಂಕೆ().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ಅನ್ಯಥಾ ಯದಿ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    ಶೂನ್ಯ widthCheckGotoRepRange(T_Rep* nodep, ಧ್ರುವ ವರ್ಣ* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ಧ್ರುವ AstConst* ಧ್ರುವ minConstp = VN_CAST(nodep->countp(), Const);
        ಧ್ರುವ AstConst* ಧ್ರುವ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ಯದಿ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    ಶೂನ್ಯ visit(AstSThroughout* nodep) override {
        m_hasSExpr = ಸತ್ಯ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = ಅಸತ್ಯ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = ಸತ್ಯ;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ಸತ್ಯ;
        m_hasSExpr = ಸತ್ಯ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            ಯದಿ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ಸತ್ಯ;
        m_hasSExpr = ಸತ್ಯ;
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            ಪ್ರತಿ (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstURandomRange* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        ಯದಿ (VN_IS(nodep->backp(), IsUnbounded)) ಪ್ರತ್ಯಾಗಮ;  // Ok, leave
        ಯದಿ (VN_IS(nodep->backp(), BracketArrayDType)) ಪ್ರತ್ಯಾಗಮ;  // Ok, leave
        ಯದಿ (VN_IS(nodep->backp(), InsideRange)) ಪ್ರತ್ಯಾಗಮ;  // Ok, leave
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ varp = VN_CAST(nodep->backp(), Var)) {
            ಯದಿ (varp->isParam()) ಪ್ರತ್ಯಾಗಮ;  // Ok, leave
        }
        Ast_ಗಂಟು* backp = nodep->backp();
        ಯದಿ (VN_IS(backp, Sub)) backp = backp->backp();
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ selp = VN_CAST(backp, SelExtract)) {
            ಯದಿ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ selp = VN_CAST(backp, SelBit)) {
            ಯದಿ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) ಪ್ರತ್ಯಾಗಮ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    ಶೂನ್ಯ visit(AstInferredDisable* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) nodep->dtypeSetBit();
    }
    ಶೂನ್ಯ visit(AstIsUnbounded* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ಯದಿ (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ಯದಿ (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    ಶೂನ್ಯ visit(AstCLog2* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    ಶೂನ್ಯ widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = nodep->valuep();
        ಧ್ರುವ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
            ಯದಿ (!constp->ಅಂಕೆ().isNegative()) ಪ್ರತ್ಯಾಗಮ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Continue with the default weight
        valuep->replaceWith(ನೂತನ AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    ಶೂನ್ಯ widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    ಶೂನ್ಯ visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ಧ್ರುವ cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ಯದಿ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ಯದಿ (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ಯದಿ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ಯದಿ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } ಅನ್ಯಥಾ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = nodep->valuep();
            ಯದಿ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(ನೂತನ AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            }
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->valuep(), Const);
            ಯದಿ (!constp || !constp->ಅಂಕೆ().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ಶೂನ್ಯ visit(AstCoverOption* nodep) override {
        ಯದಿ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        ಯದಿ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        ಯದಿ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    ಶೂನ್ಯ visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        ಯದಿ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    ಶೂನ್ಯ widthCovergroupRanges(Ast_ಗಂಟು* rangesp, ಪೂರ್ಣಾಙ್ಕ fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ಧ್ರುವ auto fill = [&](Ast_ಗಂಟು* nodep) {
            ಯದಿ (!fillWidth) ಪ್ರತ್ಯಾಗಮ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = VN_AS(nodep, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ಯದಿ (AstInsideRange* ಧ್ರುವ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ಅನ್ಯಥಾ {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    ಶೂನ್ಯ visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    ಶೂನ್ಯ visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        ಯದಿ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ಯದಿ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    ಶೂನ್ಯ visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        ಯದಿ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ಯದಿ (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    ಶೂನ್ಯ visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ countp = nodep->countp()) {
            ಯದಿ (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } ಅನ್ಯಥಾ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    ಶೂನ್ಯ widthCoverFilter(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* filterp, ಧ್ರುವ ವರ್ಣ* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        File_ಸಾಲು* ಧ್ರುವ fl = filterp->fileline();
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ resultp = filterp->dtypep()->skipRefp();
        ಯದಿ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                ನೂತನ AstNeqD{fl, filterp, ನೂತನ AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ಅನ್ಯಥಾ ಯದಿ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(ನೂತನ AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(filterp), filterp);
        } ಅನ್ಯಥಾ {
            fixWidthReduce(filterp);
        }
    }
    ಶೂನ್ಯ visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    ಶೂನ್ಯ visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    ಶೂನ್ಯ visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ಯದಿ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            checkCvtUS(nodep->lhsp(), ಅಸತ್ಯ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ಯದಿ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->ಅಂಕೆ().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ಗಂಟು* newp = nullptr;  // No change
            ಯದಿ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ನೂತನ AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ಅನ್ಯಥಾ ಯದಿ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = ನೂತನ AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } ಅನ್ಯಥಾ ಯದಿ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ನೂತನ AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            ಯದಿ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Bits* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = V3_ಸಂಖ್ಯೆ::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಎಣಿಕೆ_Ones* nodep) override {
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ widthMin = V3_ಸಂಖ್ಯೆ::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ಶೂನ್ಯ visit(AstCvtPackString* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dt = nodep->lhsp()->dtypep()) {
            ಯದಿ (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    ಶೂನ್ಯ visit(AstCvtPackedToArray* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ visit(AstCvtArrayToArray* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ visit(AstCvtArrayToPacked* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToQueue* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    ಶೂನ್ಯ visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ಯದಿ (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    ಶೂನ್ಯ visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ಧ್ರುವ ದ್ವಿಗುಣ timePrescale = constp->ಅಂಕೆ().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ಧ್ರುವ ದ್ವಿಗುಣ time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstEventControl* nodep) override {
        ಯದಿ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->fileline()->timingOn()) {
            ಯದಿ (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ಅನ್ಯಥಾ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ಯದಿ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ಸ್ಥಿರ string typenameExtends(ಧ್ರುವ AstClass* classp) {
        // An interface class may extend several, so those are not named
        ಯದಿ (!classp || classp->isInterfaceClass()) ಪ್ರತ್ಯಾಗಮ "";
        ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ಯದಿ (extendsp->isImplements()) ಅನುವರ್ತ;
            ಧ್ರುವ AstClass* ಧ್ರುವ basep = extendsp->classOrNullp();
            ಪ್ರತ್ಯಾಗಮ basep ? " extends class " + basep->dtypeName(ಸತ್ಯ) : "";
        }
        ಪ್ರತ್ಯಾಗಮ "";
    }
    ಶೂನ್ಯ visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        ಯದಿ (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ವಿಕಲ್ಪ (nodep->attrType()) {
        ಪಕ್ಷ VAttrType::DIM_DIMENSIONS:
        ಪಕ್ಷ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ಧ್ರುವ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(ಸತ್ಯ);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VAttrType::DIM_BITS_OR_ಸಂಖ್ಯೆ: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ಯದಿ (VN_IS(nodep->fromp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            } ಅನ್ಯಥಾ {
                Ast_ಗಂಟು* newp = ನೂತನ AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಪಕ್ಷ VAttrType::DIM_BITS:
        ಪಕ್ಷ VAttrType::DIM_ಮೇಲಿನ:
        ಪಕ್ಷ VAttrType::DIM_INCREMENT:
        ಪಕ್ಷ VAttrType::DIM_ಎಡ:
        ಪಕ್ಷ VAttrType::DIM_ಕೆಳಗಿನ:
        ಪಕ್ಷ VAttrType::DIM_ಬಲ:
        ಪಕ್ಷ VAttrType::DIM_SIZE: {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ಯದಿ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ಧ್ರುವ Ast_ಗಂಟು_Module* ಧ್ರುವ ownModp = v3Global.rootp()->containingModule(dtypep);
                ಯದಿ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }

            ಯದಿ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                ವಿಕಲ್ಪ (nodep->attrType()) {
                ಪಕ್ಷ VAttrType::DIM_SIZE: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    Ast_ಗಂಟು* ಧ್ರುವ newp
                        = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(ಸತ್ಯ);
                    newp->protect(ಅಸತ್ಯ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ VAttrType::DIM_ಎಡ:
                ಪಕ್ಷ VAttrType::DIM_ಕೆಳಗಿನ: {
                    Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ VAttrType::DIM_ಬಲ:
                ಪಕ್ಷ VAttrType::DIM_ಮೇಲಿನ: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sizep
                        = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(ಸತ್ಯ);
                    sizep->protect(ಅಸತ್ಯ);
                    Ast_ಗಂಟು* ಧ್ರುವ newp
                        = ನೂತನ AstSub{nodep->fileline(), sizep,
                                     ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ VAttrType::DIM_INCREMENT: {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                        = ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಭಙ್ಗ;
                }
                ಪಕ್ಷ VAttrType::DIM_BITS: {
                    ಯದಿ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ಅನ್ಯಥಾ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ಭಙ್ಗ;
                }
                ಸ್ವಭಾವ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(ಸತ್ಯ);
                ಧ್ರುವ uint32_t msbdim = dimpair.first + dimpair.second;
                ಯದಿ (!nodep->dimp() || msbdim < 1) {
                    ಯದಿ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
                        Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstShiftL{
                            nodep->fileline(), ನೂತನ AstLenN{nodep->fileline(), fromp},
                            ನೂತನ AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    } ಅನ್ಯಥಾ {
                        Ast_ಗಂಟು_DType* ಧ್ರುವ baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dim = 1;
                        AstConst* ಧ್ರುವ newp = dimension_ಮೌಲ್ಯ(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    }
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep->dimp(), Const)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* ಧ್ರುವ newp
                        = dimension_ಮೌಲ್ಯ(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                } ಅನ್ಯಥಾ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ಧ್ರುವ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dimp = nodep->dimp()->unlinkFrBack();
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                }
            }
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ಗಂಟು_DType* ಧ್ರುವ protoDtp = nodep->fromp()->dtypep();
            Ast_ಗಂಟು_DType* ಧ್ರುವ declDtp = [&]() {
                ಯದಿ (m_ftaskp->fvarp()) ಪ್ರತ್ಯಾಗಮ m_ftaskp->fvarp()->dtypep();
                Ast_ಗಂಟು_DType* ಧ್ರುವ voidp = ನೂತನ AstVoidDType{m_ftaskp->fileline()};
                ತಳ್ಳು_Deletep(voidp);  // Note voidp used past here
                ಪ್ರತ್ಯಾಗಮ voidp;
            }();
            ಯದಿ (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warnMore()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnMore()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(ಸತ್ಯ);
            ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            ಭಙ್ಗ;
        }
        ಪಕ್ಷ VAttrType::TYPEID:
            ಯದಿ (Ast_ಗಂಟು_DType* dtypep = VN_CAST(nodep->fromp(), ಗಂಟು_DType)) {
                nodep->dtypep(dtypep);
            } ಅನ್ಯಥಾ {
                nodep->dtypep(VN_AS(nodep->fromp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->dtypep());
            }
            ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ಭಙ್ಗ;
        ಸ್ವಭಾವ: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ಭಙ್ಗ;
        }
        }
    }
    ಶೂನ್ಯ visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    ಶೂನ್ಯ visit(AstCellArrayRef* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(ಸತ್ಯ);
    }

    ಶೂನ್ಯ visit(AstCellRef* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;

        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cellExprp = VN_CAST(nodep->cellp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(ಸತ್ಯ);
    }

    // DTYPES
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Array_DType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed

        ಯದಿ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ಧ್ರುವ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ಯದಿ (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* ಧ್ರುವ newp = ನೂತನ AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ಯದಿ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = ಸತ್ಯ;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            ಯದಿ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypep = nodep->subDTypep()->skipRefp();
            ಯದಿ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            ಯದಿ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ಧ್ರುವ Ast_ಗಂಟು_DType* baseDTypep = nodep->subDTypep();
                ಯಾವತ್ (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = nodep->basicp();
                ಯದಿ (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(ಸತ್ಯ));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstAssocArrayDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ಕೀಲಿ_DTypep(iterateEditMoveDTypep(nodep, nodep->ಕೀಲಿ_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ಗಂಟು_DType* ಧ್ರುವ childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_ಗಂಟು* ಧ್ರುವ elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_ಗಂಟು* newp;
        ಯದಿ (VN_IS(elementsp, Unbounded)) {
            newp = ನೂತನ AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_ವೃಕ್ಷ(), elementsp);
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ keyp = VN_CAST(elementsp, ಗಂಟು_DType)) {
            newp = ನೂತನ AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } ಅನ್ಯಥಾ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_ಸಾಲು* ಧ್ರುವ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, ಸತ್ಯ);
            // Must be expression that is constant, but we'll determine that later
            newp = ನೂತನ AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                ನೂತನ AstRange{nodep->fileline(), ನೂತನ AstConst(elementsp->fileline(), 0),
                             ನೂತನ AstSub{elementsNewFl, VN_AS(elementsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                                        ನೂತನ AstConst(elementsNewFl, 1)},
                             ಸತ್ಯ}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    ಶೂನ್ಯ visit(AstConstraintRefDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    ಶೂನ್ಯ visit(AstDynArrayDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstQueueDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        ಯದಿ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_ವೃಕ್ಷ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstVoidDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstUnsizedArrayDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        ಯದಿ (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstWildcardArrayDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstBasicDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        ಯದಿ (nodep->generic()) ಪ್ರತ್ಯಾಗಮ;  // Already perfect
        ಯದಿ (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->widthForce(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstConstDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(AstRefDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(ಸತ್ಯ);
        ಯದಿ (nodep->typeofp()) {  // type(typeofp_expression)
            ಯದಿ (Ast_ಗಂಟು_DType* typeofDtp = VN_CAST(nodep->typeofp(), ಗಂಟು_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ಅನ್ಯಥಾ {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_ಗಂಟು* ಧ್ರುವ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_ವೃಕ್ಷ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        ಯದಿ (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), nullptr);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), nullptr);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        ಯದಿ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ಧ್ರುವ ತಾರ್ಕಿಕ inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            ಯದಿ (inTemplateModule) {
                nodep->doingWidth(ಅಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(ಅಸತ್ಯ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    ಶೂನ್ಯ widthDTypeNameParams(ಧ್ರುವ AstTypedef* nodep) {
        ಧ್ರುವ VDTypeNameScopes* ಧ್ರುವ scopesp = VDTypeNameScopes::currentp();
        ಯದಿ (!scopesp) ಪ್ರತ್ಯಾಗಮ;  // Computing parameters, as by V3Param, so naming as found
        Ast_ಗಂಟು* scopep = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ಯದಿ (ಧ್ರುವ std::vector<Ast_ಗಂಟು*>* ಧ್ರುವ paramsp = scopesp->paramsp(scopep)) {
                ಪ್ರತಿ (Ast_ಗಂಟು* ಧ್ರುವ paramp : *paramsp) {
                    ಯದಿ (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstTypedef* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        ಯದಿ (auto* ಧ್ರುವ refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ಧ್ರುವ ತಾರ್ಕಿಕ declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ಯದಿ (declares) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->dtypep();
            ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ sdtypep = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ಅನ್ಯಥಾ ಯದಿ (AstEnumDType* ಧ್ರುವ edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    ಶೂನ್ಯ visit(AstParamTypeDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ಯದಿ (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    ಶೂನ್ಯ visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = VN_CAST(nodep->lhsp(), ಗಂಟು_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } ಅನ್ಯಥಾ {
            ಯದಿ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(ನೂತನ AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು_DType* ಧ್ರುವ toDtp = nodep->ತುದಿ()->dtypep()->skipRefToEnump();
        Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        ಧ್ರುವ auto castable = Ast_ಗಂಟು::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_ಗಂಟು* newp;
        ಯದಿ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ಧ್ರುವ enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_ಸಾಲು* ಧ್ರುವ fl_novalue = ನೂತನ File_ಸಾಲು{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, ಸತ್ಯ);
            newp = ನೂತನ AstCond{
                fl, testp,
                ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl,
                                ನೂತನ AstAssign{fl_novalue, nodep->ತುದಿ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                ನೂತನ AstConst{fl, AstConst::Signed32{}, 1}},
                ನೂತನ AstConst{fl, AstConst::Signed32{}, 0}};
        } ಅನ್ಯಥಾ ಯದಿ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
                fl,
                ನೂತನ AstAssign{fl, nodep->ತುದಿ()->unlinkFrBack(),
                              ನೂತನ AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                ನೂತನ AstConst{fl, AstConst::Signed32{}, 1}};
        } ಅನ್ಯಥಾ ಯದಿ (castable == VCastable::INCOMPATIBLE) {
            newp = ನೂತನ AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ಅನ್ಯಥಾ {
            newp = ನೂತನ AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    ಶೂನ್ಯ visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ refp = VN_CAST(nodep->dtp(), ಗಂಟು_DType)) {
            refp->unlinkFrBack();
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCast* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ಯದಿ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ಯದಿ (AstSigned* ಧ್ರುವ fromp = VN_CAST(nodep->fromp(), Signed)) {
                ಯದಿ (VN_IS(fromp->lhsp(), ಗಂಟು_Stream)) {
                    Ast_ಗಂಟು* ಧ್ರುವ lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_ವೃಕ್ಷ(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ಗಂಟು_DType* ಧ್ರುವ toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            ಧ್ರುವ auto castable = Ast_ಗಂಟು::computeCastable(toDtp, fromDtp, nodep->fromp());
            ತಾರ್ಕಿಕ bad = ಅಸತ್ಯ;
            ಯದಿ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ಅನ್ಯಥಾ ಯದಿ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = ಸತ್ಯ;
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
            ಯದಿ (bad) {
            } ಅನ್ಯಥಾ ಯದಿ (AstBasicDType* ಧ್ರುವ basicp = toDtp->basicp()) {
                ಯದಿ (!basicp->isString() && fromDtp->isString()) {
                    newp = ನೂತನ AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } ಅನ್ಯಥಾ ಯದಿ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ origDTypep = nodep->dtypep();
                    ಯದಿ (!VN_IS(fromDtp, StreamDType)) {
                        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ಅನ್ಯಥಾ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 ಅಸತ್ಯ);
                }
                ಯದಿ (newp) {
                } ಅನ್ಯಥಾ ಯದಿ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ಯದಿ (nodep->fromp()->isSigned()) {
                        newp = ನೂತನ AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } ಅನ್ಯಥಾ {
                        newp = ನೂತನ AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } ಅನ್ಯಥಾ ಯದಿ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = ನೂತನ AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } ಅನ್ಯಥಾ ಯದಿ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = ನೂತನ AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ಅನ್ಯಥಾ ಯದಿ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = ನೂತನ AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } ಅನ್ಯಥಾ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toDtp, QueueDType)) {
                ಯದಿ (VN_IS(fromDtp, BasicDType)) {
                    newp = ನೂತನ AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    ಪೂರ್ಣಾಙ್ಕ srcElementBits = 1;
                    ಪೂರ್ಣಾಙ್ಕ dstElementBits = 1;
                    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    ಧ್ರುವ AstQueueDType* ಧ್ರುವ dstQueueDtp = VN_AS(toDtp, QueueDType);
                    ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = ನೂತನ AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  ಅಸತ್ಯ,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            ಯದಿ (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ಯದಿ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, ಅಸತ್ಯ, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(ಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(ಸತ್ಯ);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            ಪೂರ್ಣಾಙ್ಕ width = nodep->rhsp()->toSInt();
            ಯದಿ (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        ಯದಿ (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ಗಂಟು* ಧ್ರುವ underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    ಶೂನ್ಯ visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    ಶೂನ್ಯ castSized(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* underp, ಪೂರ್ಣಾಙ್ಕ width) {
        ಧ್ರುವ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ಯದಿ (!underDtp) underDtp = underp->dtypep()->basicp();
        ಯದಿ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ calcWidth = std::max(width, underDtp->width());
            Ast_ಗಂಟು_DType* ಧ್ರುವ calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         ಅಸತ್ಯ, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ಗಂಟು_DType* ಧ್ರುವ outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), outDtp, EXTEND_EXP,
                            ಅಸತ್ಯ);
            VL_DANGLING(underp);
        }
    }
    ಶೂನ್ಯ visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(Ast_Constraint_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    ಶೂನ್ಯ visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ತಳ್ಳು_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->doingWidth(ಸತ್ಯ);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ಯದಿ (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.ತಳ್ಳು_back(nodep);
        ಯದಿ (nodep->attrsp()) {
            nodep->attrsp()->foreach([ಏತತ್, nodep](AstAttrOf* attrp) {
                ಯದಿ (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ಯದಿ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ಗಂಟು_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        ಯದಿ (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ಗಂಟು_DType* dtp = nodep->dtypep();
            Ast_ಗಂಟು_DType* np = nullptr;
            ಯಾವತ್ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ಯದಿ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ಅನ್ಯಥಾ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ಧ್ರುವ newp
                        = ನೂತನ AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ಯದಿ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ಅನ್ಯಥಾ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } ಅನ್ಯಥಾ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ಯದಿ (AstWildcardArrayDType* ಧ್ರುವ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ಯದಿ (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(ಸತ್ಯ);
        // Parameters if implicit untyped inherit from what they are assigned to
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        ತಾರ್ಕಿಕ didchk = ಅಸತ್ಯ;
        ಧ್ರುವ ತಾರ್ಕಿಕ implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ಯದಿ (implicitParam) {
            ಯದಿ (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                ಯದಿ (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } ಅನ್ಯಥಾ ಯದಿ (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } ಅನ್ಯಥಾ {
                    ಪೂರ್ಣಾಙ್ಕ width = 0;
                    Ast_ಗಂಟು_DType* ಧ್ರುವ ಮೌಲ್ಯ_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(ಮೌಲ್ಯ_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ಧ್ರುವ AstBasicDType* ಧ್ರುವ ಮೌಲ್ಯ_Bdtypep = ಮೌಲ್ಯ_DTypep->basicp();
                    ತಾರ್ಕಿಕ issigned = ಅಸತ್ಯ;
                    ಯದಿ (bdtypep->isNosign()) {
                        ಯದಿ (ಮೌಲ್ಯ_Bdtypep && ಮೌಲ್ಯ_Bdtypep->isSigned()) issigned = ಸತ್ಯ;
                    } ಅನ್ಯಥಾ {
                        issigned = bdtypep->isSigned();
                    }
                    ಯದಿ (ಮೌಲ್ಯ_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ಯದಿ (ಧ್ರುವ auto* ಧ್ರುವ constp = VN_CAST(nodep->valuep(), Const)) {
                            ಯದಿ (constp->ಅಂಕೆ().isString()) {
                                width = constp->ಅಂಕೆ().toString().length() * 8;
                            }
                        }
                        ಯದಿ (width < 8) width = 8;
                    } ಅನ್ಯಥಾ ಯದಿ (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } ಅನ್ಯಥಾ {
                        ಯದಿ (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ಧ್ರುವ ತಾರ್ಕಿಕ is_ಮೌಲ್ಯ_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ಯದಿ (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_ಮೌಲ್ಯ_Fourstate));
                    } ಅನ್ಯಥಾ {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_ಮೌಲ್ಯ_Fourstate);
                    }
                    didchk = ಸತ್ಯ;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ಅನ್ಯಥಾ {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } ಅನ್ಯಥಾ ಯದಿ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ಯದಿ (nodep->isNet()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ಯದಿ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ಯದಿ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(ಸತ್ಯ);
        nodep->doingWidth(ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Var_Ref* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (!nodep->varp()) {
            ಯದಿ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, ಅಸತ್ಯ,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ಯದಿ (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        ಯದಿ (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ಯದಿ (VN_IS(nodep->backp(), ಗಂಟು_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ಯದಿ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(ಸತ್ಯ);
        ಯದಿ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->access().isWriteOrRW() && nodep->varp()->is_ಒಳಹರಿವು()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ಧ್ರುವ ತಾರ್ಕಿಕ hierRef = !m_curModVars.ಎಣಿಕೆ(nodep->varp());
            ಧ್ರುವ ತಾರ್ಕಿಕ netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            ಧ್ರುವ ತಾರ್ಕಿಕ contAssign = VN_IS(nodep->abovep(), AssignW);
            ಧ್ರುವ ತಾರ್ಕಿಕ pinConn = VN_IS(nodep->abovep(), Pin);
            ಯದಿ (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ಯದಿ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(ಸತ್ಯ);
    }

    ಶೂನ್ಯ visit(AstEnumDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ಗಂಟು_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ಗಂಟು_DType* ಧ್ರುವ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ಯದಿ (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->findIntDType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->widthFromSub(nodep->subDTypep());
        // Assign widths
        userIterateAndNext(nodep->itemsp(), WidthVP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_ಸಂಖ್ಯೆ ಅಂಕೆ(nodep, nodep->width(), 0);
        ಧ್ರುವ V3_ಸಂಖ್ಯೆ one{nodep, nodep->width(), 1};
        ತಾರ್ಕಿಕ wrapAround = ಅಸತ್ಯ;
        std::map<ಧ್ರುವ V3_ಸಂಖ್ಯೆ, AstEnumItem*> inits;
        ಪ್ರತಿ (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ಯದಿ (itemp->valuep()) {
                ಯದಿ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_ವೃಕ್ಷ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ಯದಿ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_ವೃಕ್ಷ();
                    ಅನುವರ್ತ;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = ಅಸತ್ಯ;  // Explicit value resets wrap-around tracking
            }
            ಯದಿ (!itemp->valuep()) {
                ಯದಿ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ಯದಿ (ಅಂಕೆ.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(ನೂತನ AstConst{itemp->fileline(), ಅಂಕೆ});
            }

            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(itemp->valuep(), Const);
            ಯದಿ (constp->ಅಂಕೆ().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            ಅಂಕೆ.opAssign(constp->ಅಂಕೆ());
            // Look for duplicates
            ಧ್ರುವ auto pair = inits.emplace(ಅಂಕೆ, itemp);
            ಯದಿ (!pair.second) {  // IEEE says illegal
                ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ಧ್ರುವ ತಾರ್ಕಿಕ prevNeg = constp->ಅಂಕೆ().isNegative();
            ಅಂಕೆ.opAdd(one, constp->ಅಂಕೆ());
            ಯದಿ (basicp->isSigned()) {
                wrapAround = !prevNeg && ಅಂಕೆ.isNegative();
            } ಅನ್ಯಥಾ {
                wrapAround = ಅಂಕೆ.isEqZero();
            }
        }
    }
    ಶೂನ್ಯ visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ಯದಿ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ತಾರ್ಕಿಕ warnOn = ಸತ್ಯ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nodep->valuep();
            ಯದಿ (ಧ್ರುವ AstAdd* ಧ್ರುವ anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ಯದಿ (anodep->fileline()->equal_First_ಸಾಲು_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(valuep, Const)) {
                ಯದಿ (static_cast<ಪೂರ್ಣಾಙ್ಕ>(constp->ಅಂಕೆ().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = ಅಸತ್ಯ;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(ನೂತನ AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    ಶೂನ್ಯ visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ಯದಿ (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_ಗಂಟು* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            ಪ್ರತಿ (; enump; enump = enump->backp()) {
                ಯದಿ (VN_IS(enump, EnumDType)) ಭಙ್ಗ;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons* nodep) override {}  // Widthed when created
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Cons_Item* nodep) override {}  // Widthed when created
    ಶೂನ್ಯ visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ಪ್ರತಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ಗಂಟು_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        ಯದಿ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } ಅನ್ಯಥಾ ಯದಿ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } ಅನ್ಯಥಾ {
            // Take width as maximum across all items
            ಪೂರ್ಣಾಙ್ಕ width = nodep->exprp()->width();
            ಪೂರ್ಣಾಙ್ಕ mwidth = nodep->exprp()->widthMin();
            ತಾರ್ಕಿಕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        ಪ್ರತಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            ಯದಿ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } ಅನ್ಯಥಾ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ಯದಿ (m_constraintp) {
            ತಾರ್ಕಿಕ canLower = ಸತ್ಯ;
            ಪ್ರತಿ (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                ಯದಿ (VN_IS(ditemp->rangep(), InsideRange)) ಅನುವರ್ತ;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                ಯದಿ (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = ಅಸತ್ಯ;
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (canLower) ಪ್ರತ್ಯಾಗಮ;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        ಪ್ರತಿ (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inewp
                = insideItem(nodep, nodep->exprp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), itemp->rangep());
            ಯದಿ (!inewp) ಅನುವರ್ತ;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ cmpp
                = ನೂತನ AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            ನೂತನ AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, ಸತ್ಯ);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, ಸತ್ಯ);
            inewp = ನೂತನ AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? ನೂತನ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ಯದಿ (!newp) newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    ಶೂನ್ಯ visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ಪ್ರತಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ಗಂಟು_DType* expDTypep = nullptr;

        ಯದಿ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } ಅನ್ಯಥಾ ಯದಿ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } ಅನ್ಯಥಾ {
            // Take width as maximum across all items
            ಪೂರ್ಣಾಙ್ಕ width = nodep->exprp()->width();
            ಪೂರ್ಣಾಙ್ಕ mwidth = nodep->exprp()->widthMin();
            ತಾರ್ಕಿಕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            ಧ್ರುವ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        ಪ್ರತಿ (Ast_ಗಂಟು *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ಯದಿ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } ಅನ್ಯಥಾ ಯದಿ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp;
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಅಭಿವ್ಯಕ್ತಿ_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ಯದಿ (!m_constraintp && !nodep->exprp()->isPure()) {
            File_ಸಾಲು* ಧ್ರುವ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ಗಂಟು_DType* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Dtp = nodep->exprp()->dtypep();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ w = ಅಭಿವ್ಯಕ್ತಿ_Dtp->width();
            Ast_ಗಂಟು_DType* ಧ್ರುವ tempDTypep
                = ಅಭಿವ್ಯಕ್ತಿ_Dtp->widthSized() ? ಅಭಿವ್ಯಕ್ತಿ_Dtp
                                        : nodep->findBitOrLogicDType(w, w, ಅಭಿವ್ಯಕ್ತಿ_Dtp->numeric(),
                                                                     ಅಭಿವ್ಯಕ್ತಿ_Dtp->isFourstate());
            AstVar* ಧ್ರುವ varp
                = ನೂತನ AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = ನೂತನ AstVarRef{fl, varp, VAccess::READ};
            ಅಭಿವ್ಯಕ್ತಿ_Stmtp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{fl,
                                        ನೂತನ AstAssign{fl, ನೂತನ AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            ಯದಿ (m_ftaskp) {
                varp->funcLocal(ಸತ್ಯ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ಅನ್ಯಥಾ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        } ಅನ್ಯಥಾ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        ಪ್ರತಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);  // Will be unlinking
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* inewp;
            ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Stmtp) {
                inewp = insideItem(nodep, ಅಭಿವ್ಯಕ್ತಿ_Stmtp, itemp);
                ಅಭಿವ್ಯಕ್ತಿ_Stmtp = nullptr;
            } ಅನ್ಯಥಾ {
                inewp = insideItem(nodep, exprp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), itemp);
            }
            ಯದಿ (!inewp) ಅನುವರ್ತ;
            newp = newp ? ನೂತನ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Stmtp) VL_DO_DANGLING(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->delete_ವೃಕ್ಷ(), ಅಭಿವ್ಯಕ್ತಿ_Stmtp);
        ಯದಿ (!newp) newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* insideItem(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* itemp) {
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ itemDtp = itemp->dtypep()->skipRefp();
        ಯದಿ (AstInsideRange* ಧ್ರುವ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ಪ್ರತ್ಯಾಗಮ irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ inewp = ನೂತನ AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ inewp;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_ವೃಕ್ಷ(), exprp);
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    ಶೂನ್ಯ visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ಯದಿ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ಅನ್ಯಥಾ {
            ಯದಿ (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            ಯದಿ (m_vup->final()) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expWidth = expDTypep->width();
                ಧ್ರುವ ತಾರ್ಕಿಕ waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                ಧ್ರುವ ತಾರ್ಕಿಕ waiveRhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && expWidth >= nodep->rhsp()->widthMin();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    ಶೂನ್ಯ visit(AstIfaceRefDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ಯದಿ (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_UOr_Struct_DType* nodep) override {
        ಯದಿ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(ಅಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(ಸತ್ಯ);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ಯದಿ (ಧ್ರುವ AstUnionDType* ಧ್ರುವ unionp = VN_CAST(nodep, UnionDType)) {
            ಯದಿ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(ಅಸತ್ಯ);
        // Error checks
        ಪ್ರತಿ (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtp = itemp->subDTypep()->skipRefp();
            ಯದಿ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ಯದಿ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ತಳ್ಳು_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        ಧ್ರುವ ತಾರ್ಕಿಕ isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ಧ್ರುವ ತಾರ್ಕಿಕ inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ಯದಿ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            ಪೂರ್ಣಾಙ್ಕ lsb = 0;
            ಪೂರ್ಣಾಙ್ಕ width = 0;
            ತಾರ್ಕಿಕ first = ಸತ್ಯ;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            ಪ್ರತಿ (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            ಪ್ರತಿ (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ಯದಿ (itemp->isFourstate()) nodep->isFourstate(ಸತ್ಯ);
                itemp->lsb(lsb);
                ಯದಿ (VN_IS(nodep, UnionDType)) {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    ಯದಿ (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } ಅನ್ಯಥಾ {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = ಅಸತ್ಯ;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } ಅನ್ಯಥಾ {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(ಅಸತ್ಯ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    ಶೂನ್ಯ visit(AstThisRef* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        ಯದಿ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    ಶೂನ್ಯ visit(AstClassRefDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstClassOrPackageRef* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        userIterateChildren(nodep, nullptr);
    }
    ಶೂನ್ಯ visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ಶೂನ್ಯ visit(AstClassExtends* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    ಶೂನ್ಯ visit(AstMemberDType* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ಯದಿ (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    ಶೂನ್ಯ visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = ಸತ್ಯ;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        ಧ್ರುವ AstMemberSel* ಧ್ರುವ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ಯದಿ (AstClocking* ಧ್ರುವ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ಯದಿ (AstVar* ಧ್ರುವ varp = memberSelClocking(nodep, clockingp)) {
                ಯದಿ (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_ವೃಕ್ಷ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ಯದಿ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                ಯದಿ (AstIfaceRefDType* ಧ್ರುವ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(ಸತ್ಯ);  // Must not visit again: will confuse scopes
                ಪ್ರತ್ಯಾಗಮ;
            }
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, ಗಂಟು_UOr_Struct_DType)) {
            ಯದಿ (memberSelStruct(nodep, adtypep)) ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (AstClassRefDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ಯದಿ (memberSelClass(nodep, adtypep)) ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (AstIfaceRefDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ಯದಿ (Ast_ಗಂಟು* foundp = memberSelIface(nodep, adtypep)) {
                ಯದಿ (AstClocking* ಧ್ರುವ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(foundp, Var)) {
                    ಯದಿ (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* ಧ್ರುವ ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ಧ್ರುವ AstVarRef* ಧ್ರುವ refp = VN_CAST(nodep->fromp(), VarRef);
                    ಯದಿ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ಧ್ರುವ newp = ನೂತನ AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(ಸತ್ಯ);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(ಸತ್ಯ);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (AstModport* ಧ್ರುವ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ಧ್ರುವ newDtypep = ನೂತನ AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (AstCell* ಧ್ರುವ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ಯದಿ (VN_IS(cellp->modp(), Iface)) {
                        ಧ್ರುವ string viftopName = cellp->name() + "__Viftop";
                        Ast_ಗಂಟು_Module* ಧ್ರುವ parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ಯದಿ (AstVar* ಧ್ರುವ vp = VN_CAST(itemp, Var)) {
                                ಯದಿ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ಭಙ್ಗ;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ಯದಿ (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_ಗಂಟು_DType* subDtypep = viftopVarp->dtypep();
                        ಯದಿ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ಯದಿ (AstIfaceRefDType* ಧ್ರುವ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ಧ್ರುವ newDtypep = ನೂತನ AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(ಸತ್ಯ);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(ಸತ್ಯ);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                }
                ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp = VN_CAST(foundp, ಗಂಟು_FTask)) {
                    AstMethodCall* ಧ್ರುವ newp = ನೂತನ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }
    ತಾರ್ಕಿಕ memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ಯದಿ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* ಧ್ರುವ newp = ನೂತನ AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            visit(newp);
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ಧ್ರುವ first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        ಪ್ರತಿ (AstClass* classp = first_classp; classp;) {
            ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ foundp = m_memberMap.findMember(classp, nodep->name())) {
                ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(foundp, Var)) {
                    ಯದಿ (!varp->didWidth()) userIterate(varp, nullptr);
                    ಯದಿ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ಧ್ರುವ varRefp
                            = ನೂತನ AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                    }
                    ಯದಿ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(ಸತ್ಯ);
                    ಯದಿ (nodep->fromp()->same_ವೃಕ್ಷ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಯದಿ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(ನೂತನ AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಯದಿ (AstEnumItemRef* ಧ್ರುವ adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಯದಿ (Ast_ಗಂಟು_FTask* ftaskp = VN_CAST(foundp, ಗಂಟು_FTask)) {
                    AstMethodCall* newp = ನೂತನ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                ಯದಿ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }

        VSpellCheck speller;
        ಪ್ರತಿ (AstClass* classp = first_classp; classp;) {
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.ತಳ್ಳು_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // Caller handles error
    }
    ತಾರ್ಕಿಕ modportAllowsItem(Ast_ಗಂಟು* nodep, ಧ್ರುವ AstIfaceRefDType* adtypep, ಧ್ರುವ Ast_ಗಂಟು* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ಧ್ರುವ AstModport* ಧ್ರುವ modportp = adtypep->modportp();
        ಯದಿ (!modportp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(itemp, Var)) {
            ಯದಿ (varp->isParam()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(itemp, ಗಂಟು_FTask) && !VN_IS(itemp, Clocking)) {
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        ಯದಿ (pair.second) {
            ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.ಸೇರಿಸು(mpItemp->name());
            }
        }
        ಯದಿ (pair.first->second.ಎಣಿಕೆ(itemp->name())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    Ast_ಗಂಟು* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ಗಂಟು_Module* ಧ್ರುವ ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (itemp->name() == nodep->name()) {
                ಯದಿ (!modportAllowsItem(nodep, adtypep, itemp)) {
                    ಪ್ರತ್ಯಾಗಮ nullptr;  // Caller handles error
                }
                ಪ್ರತ್ಯಾಗಮ itemp;
            }
            ಯದಿ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ತಳ್ಳು_Candidate(itemp->prettyName());
            }
        }
        ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ಪ್ರತ್ಯಾಗಮ nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        ಪ್ರತಿ (Ast_ಗಂಟು* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ಯದಿ (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                ಯದಿ (citemp->varp()->name() == nodep->name()) ಪ್ರತ್ಯಾಗಮ citemp->varp();
                speller.ತಳ್ಳು_Candidate(citemp->varp()->prettyName());
            }
        }
        ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        ಪ್ರತ್ಯಾಗಮ nullptr;  // Caller handles error
    }
    ತಾರ್ಕಿಕ memberSelStruct(AstMemberSel* nodep, Ast_ಗಂಟು_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        ಯದಿ (AstMemberDType* ಧ್ರುವ memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            ಯದಿ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ಅನ್ಯಥಾ ಯದಿ (adtypep->packed()) {
                AstSel* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(ಸತ್ಯ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ಅನ್ಯಥಾ {
                AstStructSel* ಧ್ರುವ newp = ನೂತನ AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(ಸತ್ಯ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    ಶೂನ್ಯ visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* ಧ್ರುವ basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        ಯದಿ (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } ಅನ್ಯಥಾ ಯದಿ (AstEnumDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstAssocArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstWildcardArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstDynArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstQueueDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstClassRefDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstIfaceRefDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (AstConstraintRefDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } ಅನ್ಯಥಾ ಯದಿ (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_ಗಂಟು_FTask_Ref* nodep, ತಾರ್ಕಿಕ required, ತಾರ್ಕಿಕ arbReturn,
                              Ast_ಗಂಟು_DType* returnDtp, Ast_ಗಂಟು_DType* indexDtp,
                              Ast_ಗಂಟು_DType* ಮೌಲ್ಯ_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ಯದಿ (AstWith* ಧ್ರುವ withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->ಮೌಲ್ಯ_Arg_Refp()->dtypep(ಮೌಲ್ಯ_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            ಪ್ರತ್ಯಾಗಮ withp;
        }
        ಯದಿ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    ಶೂನ್ಯ methodOkArguments(Ast_ಗಂಟು_FTask_Ref* nodep, ಪೂರ್ಣಾಙ್ಕ minArg, ಪೂರ್ಣಾಙ್ಕ maxArg) {
        ಪೂರ್ಣಾಙ್ಕ narg = 0;
        ಯದಿ (AstWith* ಧ್ರುವ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(withp->unlinkFrBack()), withp);
        }
        ಪ್ರತಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ಧ್ರುವ ತಾರ್ಕಿಕ ok = (narg >= minArg) && (narg <= maxArg);
        ಯದಿ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            ಪ್ರತಿ (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    ನೂತನ AstArg{nodep->fileline(), "",
                               ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            ಪ್ರತಿ (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ಯಾವತ್ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_ವೃಕ್ಷ(), argp);
            }
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* methodArg(AstMethodCall* nodep, ಪೂರ್ಣಾಙ್ಕ arg) {
        AstArg* argp = nodep->argsp();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ಪ್ರತ್ಯಾಗಮ argp->exprp();
    }

    ಶೂನ್ಯ methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ಯದಿ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            ಯದಿ (nodep->name() == "num") {
                ಪೂರ್ಣಾಙ್ಕ ವಸ್ತುಗಳು = 0;
                ಪ್ರತಿ (Ast_ಗಂಟು* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++ವಸ್ತುಗಳು;
                newp = ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{}, ವಸ್ತುಗಳು);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "first") {
                ಧ್ರುವ AstEnumItem* itemp = adtypep->itemsp();
                ಯದಿ (!itemp) {
                    newp = ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ಅನ್ಯಥಾ {
                    newp = VN_AS(itemp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "last") {
                ಧ್ರುವ AstEnumItem* itemp = adtypep->itemsp();
                ಯಾವತ್ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                ಯದಿ (!itemp) {
                    newp = ನೂತನ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ಅನ್ಯಥಾ {
                    newp = VN_AS(itemp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ಯದಿ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Bad case");
            }

            ಯದಿ (nodep->name() != "name" && nodep->argsp()) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, ಸತ್ಯ);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ಯದಿ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ಧ್ರುವ uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                ಯದಿ (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* ಧ್ರುವ newp = ನೂತನ AstMethodCall{
                        nodep->fileline(),
                        ನೂತನ AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                } ಅನ್ಯಥಾ ಯದಿ (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ತಳ್ಳು_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* ಧ್ರುವ clonep = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    VN_AS(stepp, Const)->ಅಂಕೆ().setLong(1);
                    AstConst* ಧ್ರುವ constp = ನೂತನ AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* ಧ್ರುವ argp = ನೂತನ AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* ಧ್ರುವ newp
                        = ನೂತನ AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ಯದಿ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Wildcard_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಯದಿ (!nodep->argsp()) {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ಅನ್ಯಥಾ {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Wildcard_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "map") {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಯದಿ (newp) {
            newp->protect(ಅಸತ್ಯ);
            newp->didWidth(ಸತ್ಯ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ಯದಿ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ಯದಿ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(), BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Assoc_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(), BOTH);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Assoc_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಯದಿ (!nodep->argsp()) {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } ಅನ್ಯಥಾ {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->ಕೀಲಿ_DTypep(),
                                  BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Assoc_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep, adtypep);
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಧ್ರುವ withp = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ಧ್ರುವ withp = methodWithClause(
                nodep, ಅಸತ್ಯ, ಸತ್ಯ, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ಯದಿ (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->ಕೀಲಿ_DTypep()));
            } ಅನ್ಯಥಾ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ಧ್ರುವ withp = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->findBitDType(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ಧ್ರುವ withp = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->findBitDType(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->ಕೀಲಿ_DTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "map") {
            AstWith* ಧ್ರುವ withp = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                                    adtypep->ಕೀಲಿ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಯದಿ (newp) {
            newp->protect(ಅಸತ್ಯ);
            newp->didWidth(ಸತ್ಯ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* method_Call_Assoc_Index_ಅಭಿವ್ಯಕ್ತಿ(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_ಗಂಟು* ಧ್ರುವ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->ಕೀಲಿ_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ಪ್ರತ್ಯಾಗಮ nodep->argsp()->exprp();
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* method_Call_Wildcard_Index_ಅಭಿವ್ಯಕ್ತಿ(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ಗಂಟು* ಧ್ರುವ index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        ಪ್ರತ್ಯಾಗಮ nodep->argsp()->exprp();
    }
    ಶೂನ್ಯ methodCallLValueRecurse(AstMethodCall* nodep, Ast_ಗಂಟು* childp, ಧ್ರುವ VAccess& access) {
        ಯದಿ (AstCMethodHard* ಧ್ರುವ ichildp = VN_CAST(childp, CMethodHard)) {
            ತಾರ್ಕಿಕ methAt = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ methAtBack = ಅಸತ್ಯ;
            ತಾರ್ಕಿಕ methAtWrite = ಅಸತ್ಯ;
            ವಿಕಲ್ಪ (ichildp->method()) {
            ಪಕ್ಷ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = ಸತ್ಯ;
                ಭಙ್ಗ;
            ಪಕ್ಷ VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = ಸತ್ಯ;
                ಭಙ್ಗ;
            ಪಕ್ಷ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            ಪಕ್ಷ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            ಪಕ್ಷ VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = ಸತ್ಯ;
                ಭಙ್ಗ;
            ಸ್ವಭಾವ: ಭಙ್ಗ;
            }
            ಯದಿ (methAt || methAtBack || methAtWrite) {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ಯದಿ (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ಯದಿ (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } ಅನ್ಯಥಾ ಯದಿ (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(childp, ಗಂಟು_Var_Ref)) {
            varrefp->access(access);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstMemberSel* ಧ್ರುವ ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstStructSel* ಧ್ರುವ ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Sel* ಧ್ರುವ ichildp = VN_CAST(childp, ಗಂಟು_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ಅನ್ಯಥಾ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_ಗಂಟು_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        ಯದಿ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            ಯದಿ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, ಅಸತ್ಯ, ಸತ್ಯ, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ಧ್ರುವ withp = methodWithClause(
                nodep, ಅಸತ್ಯ, ಸತ್ಯ, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            ಯದಿ (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } ಅನ್ಯಥಾ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    ಶೂನ್ಯ methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ಯದಿ (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ ((newp = methodCallArray(nodep, adtypep))) {
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಯದಿ (newp) {
            newp->protect(ಅಸತ್ಯ);
            newp->didWidth(ಸತ್ಯ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        ಯದಿ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            ಯದಿ (!nodep->argsp()) {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } ಅನ್ಯಥಾ {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Queue_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep);
                ಯದಿ (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ತೆಗೆ_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ index_exprp = method_Call_Queue_Index_ಅಭಿವ್ಯಕ್ತಿ(nodep);
            AstArg* ಧ್ರುವ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ಯದಿ (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ತಳ್ಳು_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_ಸೇರಿಸು, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ಧ್ರುವ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ ((newp = methodCallArray(nodep, adtypep))) {
        } ಅನ್ಯಥಾ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ಯದಿ (newp) {
            newp->protect(ಅಸತ್ಯ);
            newp->didWidth(ಸತ್ಯ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* method_Call_Queue_Index_ಅಭಿವ್ಯಕ್ತಿ(AstMethodCall* nodep) {
        Ast_ಗಂಟು* ಧ್ರುವ index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        ಪ್ರತ್ಯಾಗಮ nodep->argsp()->exprp();
    }
    ಶೂನ್ಯ methodCallWarnTiming(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ nodep, ಧ್ರುವ std::string& className) {
        ಯದಿ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ಅನ್ಯಥಾ ಯದಿ (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    ಶೂನ್ಯ methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ಧ್ರುವ ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), ಗಂಟು_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ಯದಿ (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (adtypep->isVirtual()) {
                ಪ್ರತಿ (Ast_ಗಂಟು* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ಯದಿ (AstVar* ಧ್ರುವ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            ಯದಿ (ftaskp->isStatic()) {
                AstArg* ಧ್ರುವ argsp = nodep->argsp();
                ಯದಿ (argsp) argsp->unlinkFrBackWithNext();
                Ast_ಗಂಟು_FTask_Ref* newp = nullptr;
                ಯದಿ (VN_IS(ftaskp, Task)) {
                    newp = ನೂತನ AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ಅನ್ಯಥಾ {
                    newp = ನೂತನ AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            } ಅನ್ಯಥಾ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ಯದಿ (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    ಶೂನ್ಯ handleRandomizeArgs(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ nodep, AstClass* ಧ್ರುವ classp) {
        ತಾರ್ಕಿಕ hasNonNullArgs = ಅಸತ್ಯ;
        AstConst* nullp = nullptr;
        ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp = argp->exprp();
            ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(exprp, Const)) {
                ಯದಿ (constp->ಅಂಕೆ().isNull()) {
                    nullp = constp;
                    ಅನುವರ್ತ;
                }
            }
            hasNonNullArgs = ಸತ್ಯ;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ಯದಿ (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp = methodCallp->fromp();
                ಯಾವತ್ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ಧ್ರುವ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ಯದಿ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
                ಅನುವರ್ತ;
            }
            ಯಾವತ್ (exprp) {
                ಯದಿ (AstMemberSel* ಧ್ರುವ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ಅನ್ಯಥಾ {
                    ಯದಿ (AstVarRef* ಧ್ರುವ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ಅನ್ಯಥಾ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                ಯದಿ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ಯದಿ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ಅನ್ಯಥಾ ಯದಿ (classp->existsMember([&](ಧ್ರುವ AstClass*, ಧ್ರುವ AstVar* memberVarp) {
                           ಪ್ರತ್ಯಾಗಮ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ಅನ್ಯಥಾ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_ವೃಕ್ಷ(), argp);
            }
        }
        ಯದಿ (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    ಶೂನ್ಯ methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ಧ್ರುವ first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        ಯದಿ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            ಪ್ರತಿ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ಯದಿ (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        ಪ್ರತಿ (AstClass* classp = first_classp; classp;) {
            ಯದಿ (nodep->fileline()->timingOn()) {
                ಯದಿ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ಧ್ರುವ packagep = getItemPackage(classp);
                    // Check if it's std
                    ಯದಿ (packagep && packagep->name() == "std") {
                        ಯದಿ (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } ಅನ್ಯಥಾ ಯದಿ (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), ಗಂಟು_FTask)) {
                userIterate(ftaskp, nullptr);
                ಯದಿ (ftaskp->isStatic()) {
                    AstArg* ಧ್ರುವ argsp = nodep->argsp();
                    ಯದಿ (argsp) argsp->unlinkFrBackWithNext();
                    Ast_ಗಂಟು_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ಯದಿ (!m_vup) {  // Called as task
                        newp = ನೂತನ AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ಅನ್ಯಥಾ {
                        newp = ನೂತನ AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                } ಅನ್ಯಥಾ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ಯದಿ (VN_IS(ftaskp, Task)) {
                        ಯದಿ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    ಯದಿ (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(ಸತ್ಯ);
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                AstCMethodHard* ಧ್ರುವ newp
                    = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(ಸತ್ಯ);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(ಸತ್ಯ);
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                AstCMethodHard* ಧ್ರುವ newp
                    = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(ಸತ್ಯ);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            ಪ್ರತಿ (AstClass* classp = first_classp; classp;) {
                ಪ್ರತಿ (Ast_ಗಂಟು* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ಯದಿ (VN_IS(itemp, ಗಂಟು_FTask)) speller.ತಳ್ಳು_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            ಧ್ರುವ string suggest = speller.best_Candidate_ಸಂದೇಶ(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    ಶೂನ್ಯ methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        ಯದಿ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            ಯದಿ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } ಅನ್ಯಥಾ {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(ಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ಯದಿ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } ಅನ್ಯಥಾ {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(ಸತ್ಯ);
    }
    ಶೂನ್ಯ methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        ಗಣನಾ : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        ಯದಿ (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        ಯದಿ (methodId) {
            AstWith* ಧ್ರುವ withp
                = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            ಯದಿ (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ಧ್ರುವ newp
                    = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ಯದಿ (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(ಅಸತ್ಯ);
                newp->didWidth(ಸತ್ಯ);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ arrayRef = nodep->fromp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ selector = ನೂತನ AstArraySel{fl, arrayRef, i};
                    ಯದಿ (!newp) {
                        newp = selector;
                    } ಅನ್ಯಥಾ {
                        ವಿಕಲ್ಪ (methodId) {
                        ಪಕ್ಷ ARRAY_OR: newp = ನೂತನ AstOr{fl, newp, selector}; ಭಙ್ಗ;
                        ಪಕ್ಷ ARRAY_AND: newp = ನೂತನ AstAnd{fl, newp, selector}; ಭಙ್ಗ;
                        ಪಕ್ಷ ARRAY_XOR: newp = ನೂತನ AstXor{fl, newp, selector}; ಭಙ್ಗ;
                        ಪಕ್ಷ ARRAY_SUM: newp = ನೂತನ AstAdd{fl, newp, selector}; ಭಙ್ಗ;
                        ಪಕ್ಷ ARRAY_PRODUCT: newp = ನೂತನ AstMul{fl, newp, selector}; ಭಙ್ಗ;
                        ಸ್ವಭಾವ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        } ಅನ್ಯಥಾ ಯದಿ (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(ಅಸತ್ಯ);
            newp->didWidth(ಸತ್ಯ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    ಶೂನ್ಯ methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        ಯದಿ (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* ಧ್ರುವ callp = ನೂತನ AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    ಶೂನ್ಯ methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        ಯದಿ (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "compare" || nodep->name() == "icompare") {
            ಧ್ರುವ ತಾರ್ಕಿಕ ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ಧ್ರುವ argp = nodep->argsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhs = nodep->fromp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhs = argp->exprp()->unlinkFrBack();
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ಧ್ರುವ arg0p = nodep->argsp();
            AstArg* ಧ್ರುವ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* ಧ್ರುವ varrefp
                = ನೂತನ AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAssign{
                nodep->fileline(), fromp, ನೂತನ AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ತಳ್ಳು_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* ಧ್ರುವ arg0p = nodep->argsp();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* ಧ್ರುವ arg0p = nodep->argsp();
            AstArg* ಧ್ರುವ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->fromp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ thsp = arg1p->exprp()->unlinkFrBack();
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ಯದಿ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ಅನ್ಯಥಾ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_ಗಂಟು_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ಯದಿ (AstQueueDType* ಧ್ರುವ queuep = m_queueDTypeIndexed[indexDTypep]) {
            ಪ್ರತ್ಯಾಗಮ queuep;
        } ಅನ್ಯಥಾ {
            AstQueueDType* ಧ್ರುವ newp
                = ನೂತನ AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            ಪ್ರತ್ಯಾಗಮ newp;
        }
    }

    ಶೂನ್ಯ visit(AstNew* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        AstClass* classp = nullptr;
        ತಾರ್ಕಿಕ assign = ಅಸತ್ಯ;
        ಯದಿ (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = ಸತ್ಯ;
            Ast_ಗಂಟು* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            ಯದಿ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ಯದಿ (AstClass* ಧ್ರುವ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ಧ್ರುವ adtypep
                        = ನೂತನ AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } ಅನ್ಯಥಾ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            ಯದಿ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ಗಂಟು_DType)
                           ? ("data type "s + VN_AS(warnp, ಗಂಟು_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ಅನ್ಯಥಾ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ಅನ್ಯಥಾ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        ಯದಿ (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    ಶೂನ್ಯ visit(AstNewCopy* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        AstClassRefDType* ಧ್ರುವ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ಯದಿ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ಯದಿ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    ಶೂನ್ಯ visit(AstNewDynamic* nodep) override {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        AstDynArrayDType* ಧ್ರುವ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ಯದಿ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ಯದಿ (!VN_IS(nodep->backp(), ಗಂಟು_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->dtypep(adtypep);
        ಯದಿ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ಯದಿ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ಧ್ರುವ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ಯದಿ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }

    ಶೂನ್ಯ visit(Ast_Tagged_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ಶೂನ್ಯ visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ಶೂನ್ಯ visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    ಶೂನ್ಯ visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    ಶೂನ್ಯ visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    ಶೂನ್ಯ visit(AstPattern* nodep) override {
        ಯದಿ (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = ಅಸತ್ಯ;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ಯದಿ (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), ಗಂಟು_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                ಪ್ರತಿ (Ast_ಗಂಟು *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            ಪ್ರತ್ಯಾಗಮ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = ಅಸತ್ಯ;
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(9, "PATTERN " << nodep);
        ಯದಿ (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ಯದಿ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ಗಂಟು_DType* dtypep = nodep->dtypep();
        ಯದಿ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ಯದಿ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ಪ್ರತ್ಯಾಗಮ;
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ಯದಿ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ times = visitPatMemberRep(patp);
                ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i < times; i++) {
                    AstPatMember* ಧ್ರುವ newp = patp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ಯದಿ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    ಯಾವತ್ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ movep = VN_AS(patp->lhssp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_ಗಂಟು* newkeyp = nullptr;
                        ಯದಿ (patp->keyp()) newkeyp = patp->keyp()->clone_ವೃಕ್ಷ(ಸತ್ಯ);
                        AstPatMember* ಧ್ರುವ newp
                            = ನೂತನ AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ಧ್ರುವ nextp = VN_AS(patp->nextp(), PatMember);
                ಯದಿ (patp->isDefault()) {
                    ಯದಿ (defaultp) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            ಯಾವತ್ (ಧ್ರುವ AstConstDType* ಧ್ರುವ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            ಯದಿ (auto* ಧ್ರುವ vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } ಅನ್ಯಥಾ ಯದಿ (auto* ಧ್ರುವ vdtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ಧ್ರುವ consp = ನೂತನ AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ಧ್ರುವ consp = ನೂತನ AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } ಅನ್ಯಥಾ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಯದಿ (defaultp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(defaultp), defaultp);
        }
    }
    ಶೂನ್ಯ patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<ಧ್ರುವ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ಧ್ರುವ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ಯಾವತ್ (patp) {
                ಕುರು {
                    ಯದಿ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ಯದಿ (ಧ್ರುವ AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            ಯದಿ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                ಭಙ್ಗ;
                            } ಅನ್ಯಥಾ {
                                ಧ್ರುವ std::pair<PatMap::iterator, ತಾರ್ಕಿಕ> ret
                                    = patmap.emplace(memp, patp);
                                ಯದಿ (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_DType* nodedtypep = VN_CAST(patp->keyp(), ಗಂಟು_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            ಧ್ರುವ string dtype = nodedtypep->dtypep()->prettyDTypeName(ಸತ್ಯ);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ಅನ್ಯಥಾ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ಅನ್ಯಥಾ ಯದಿ (memp) {
                        // constant expr
                        ಧ್ರುವ std::pair<PatMap::iterator, ತಾರ್ಕಿಕ> ret = patmap.emplace(memp, patp);
                        ಯದಿ (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ಅನ್ಯಥಾ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ಯಾವತ್ (ಅಸತ್ಯ);

                // Next
                ಯದಿ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        ಯದಿ (vdtypep->packed()) {
            ಪ್ರತಿ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ಧ್ರುವ auto it = patmap.find(memp);
                ಯದಿ (it == patmap.end()) {  // default or default_type assignment
                    ಯದಿ (AstStructDType* ಧ್ರುವ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ಅನ್ಯಥಾ {
                        AstPatMember* ಧ್ರುವ patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = ಮೌಲ್ಯ_Concat_pattern_Struct(patp, newp, memp, nodep);
                        ಯದಿ (patp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(patp), patp);
                    }
                } ಅನ್ಯಥಾ {  // member assignment
                    newp = ಮೌಲ್ಯ_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } ಅನ್ಯಥಾ {  // Unpacked
            AstConsStruct* ಧ್ರುವ consp = ನೂತನ AstConsStruct{nodep->fileline(), vdtypep};
            ಪ್ರತಿ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ಧ್ರುವ auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                ಯದಿ (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    ತಳ್ಳು_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ತಳ್ಳು_Deletep(patp);  // patp used below
                } ಅನ್ಯಥಾ {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
                consp->addItemsp(ನೂತನ AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ಯದಿ (newp) {
            nodep->replaceWith(newp);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp,
                                                 AstPattern* nodep, ಧ್ರುವ DTypeMap& dtypemap) {
        ಪ್ರತಿ (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ಯದಿ (AstStructDType* ಧ್ರುವ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ಅನ್ಯಥಾ {
                AstPatMember* ಧ್ರುವ patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = ಮೌಲ್ಯ_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                ಯದಿ (patp) VL_DO_DANGLING(ತಳ್ಳು_Deletep(patp), patp);
            }
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ಧ್ರುವ DTypeMap& dtypemap) {
        ಧ್ರುವ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(ಸತ್ಯ);
        ಧ್ರುವ auto it = dtypemap.find(memp_DType);
        ಯದಿ (it != dtypemap.end()) {
            // default_value for data_type
            ಪ್ರತ್ಯಾಗಮ it->second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        ಯದಿ (defaultp) {
            // default_value for any unmatched member yet
            ಪ್ರತ್ಯಾಗಮ defaultp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        ಯದಿ (memp->valuep()) {
            ಪ್ರತ್ಯಾಗಮ ನೂತನ AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                                    ನೂತನ AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಮೌಲ್ಯ_Concat_pattern_Struct(AstPatMember* patp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        ಯದಿ (patp) {
            patp->dtypep(memp);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಯದಿ (!newp) {
                newp = valuep;
            } ಅನ್ಯಥಾ {
                AstConcat* ಧ್ರುವ concatp = ನೂತನ AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, ಸತ್ಯ);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, ಸತ್ಯ);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ಗಂಟು_DType* elemDTypep,
                                       ಧ್ರುವ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = defaultp->lhssp()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        Ast_ಗಂಟು_DType* ಧ್ರುವ elemDTypeSkipRefp = elemDTypep->skipRefp();
        ಧ್ರುವ AstStructDType* ಧ್ರುವ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ unpackedStruct = structp && !structp->packed();

        ಯದಿ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಪ್ರತ್ಯಾಗಮ defaultp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        ಯದಿ (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಪ್ರತ್ಯಾಗಮ defaultp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }
        ಯದಿ (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        ತಾರ್ಕಿಕ wholeElement = unpackedStruct;
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ ಮೌಲ್ಯ_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !ಮೌಲ್ಯ_DTypep->skipRefp()->isIntegralOrPacked()
                                          : ಮೌಲ್ಯ_DTypep->similarDType(elemDTypeSkipRefp);
        }
        ಯದಿ (wholeElement) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            ಪ್ರತ್ಯಾಗಮ defaultp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        }

        AstPatMember* ಧ್ರುವ nestedDefaultp
            = ನೂತನ AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(ಸತ್ಯ);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        AstPattern* ಧ್ರುವ recursivePatternp = ನೂತನ AstPattern{defaultp->fileline(), nestedDefaultp};
        ಪ್ರತ್ಯಾಗಮ ನೂತನ AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* patternDefaultValuep(AstPatMember* defaultp, Ast_ಗಂಟು_DType* elemDTypep) {
        ಯದಿ (!defaultp) ಪ್ರತ್ಯಾಗಮ nullptr;
        AstPatMember* ಧ್ರುವ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(elemDefaultp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(elemDefaultp), elemDefaultp);
        ಪ್ರತ್ಯಾಗಮ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    ಶೂನ್ಯ patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        ಧ್ರುವ VNumRange range = arrayDtp->declRange();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements = range.elements();
        AstConsArray* ಧ್ರುವ newp = ನೂತನ AstConsArray{nodep->fileline(), arrayDtp};
        ಪೂರ್ಣಾಙ್ಕ entn = 0;  // Number of elements given so far, from the left
        ತಾರ್ಕಿಕ error = ಅಸತ್ಯ;  // Reported an error, so the value is not complete
        ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಧ್ರುವ uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ಯದಿ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_ಮೌಲ್ಯಗಳು(index, valuep);
                ಅನುವರ್ತ;
            }
            ಧ್ರುವ VCastable castable = Ast_ಗಂಟು::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ಯದಿ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = ಸತ್ಯ;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        ಯದಿ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = ಸತ್ಯ;
        }
        ಯದಿ (error) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } ಅನ್ಯಥಾ {
            nodep->replaceWith(newp);
        }
    }
    ಶೂನ್ಯ patternArray(AstPattern* nodep, Ast_ಗಂಟು_Array_DType* arrayDtp, AstPatMember* defaultp) {
        ಯದಿ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.ಎಡ() << " to " << range.ಬಲ());
        Ast_ಗಂಟು* newp = nullptr;
        ತಾರ್ಕಿಕ missed = ಅಸತ್ಯ;  // Some element missed, so not a complete value
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ entn = 0, ent = range.ಎಡ(); entn < range.elements();
             ++entn, ent += range.ಎಡ_To_ಬಲ_Inc()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
            ಧ್ರುವ auto it = patmap.find(ent);
            ಯದಿ (it == patmap.end()) {
                ಯದಿ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } ಅನ್ಯಥಾ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = ಸತ್ಯ;
                }
            } ಅನ್ಯಥಾ {
                AstPatMember* ಧ್ರುವ patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ಯದಿ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                    valuep = ನೂತನ AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            ಯದಿ (valuep) {
                ಯದಿ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ಯದಿ (!newp) newp = ನೂತನ AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ಅನ್ಯಥಾ {  // Packed. Convert to concat for now.
                    ಯದಿ (!newp) {
                        newp = valuep;
                    } ಅನ್ಯಥಾ {
                        AstConcat* ಧ್ರುವ concatp
                            = ನೂತನ AstConcat{valuep->fileline(), VN_AS(newp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ಯದಿ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ಯದಿ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(newp), newp);
            newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        ಯದಿ (newp) {
            nodep->replaceWith(newp);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        ಯದಿ (VN_IS(patp->keyp(), ಗಂಟು_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (patp->ಕೀಲಿ_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->ಕೀಲಿ_Exprp());  // May change
        AstConst* ಧ್ರುವ constp = VN_CAST(patp->ಕೀಲಿ_Exprp(), Const);
        ಯದಿ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ಪ್ರತ್ಯಾಗಮ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_ಗಂಟು_DType* ಕೀಲಿ_DTypep) {
        ಯದಿ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ಯದಿ (ಕೀಲಿ_DTypep->skipRefp()->isIntegralOrPacked() && patp->ಕೀಲಿ_Exprp()) {
            userIterate(patp->ಕೀಲಿ_Exprp(), WidthVP{ಕೀಲಿ_DTypep, PRELIM}.p());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(patp->ಕೀಲಿ_Exprp()->width(), ಕೀಲಿ_DTypep->width());
            Ast_ಗಂಟು_DType* ಧ್ರುವ contextDTypep
                = ಕೀಲಿ_DTypep->findBitOrLogicDType(width, width, ಕೀಲಿ_DTypep->numeric(), ಸತ್ಯ);
            iterateCheck(patp, "key", patp->ಕೀಲಿ_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, ಅಸತ್ಯ);
        }
        AstConst* ಧ್ರುವ constp = patternConstKeyp(patp);
        ಯದಿ (!constp) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (constp->ಅಂಕೆ().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, ಕೀಲಿ_DTypep, EXTEND_EXP, ಅಸತ್ಯ);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ convp = patp->ಕೀಲಿ_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ಪ್ರತ್ಯಾಗಮ VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ ಕೀಲಿ_DTypep = VN_AS(arrayDtp, AssocArrayDType)->ಕೀಲಿ_DTypep();
        ಯದಿ (!ಕೀಲಿ_DTypep->skipRefp()->isAggregateType() || !patp->ಕೀಲಿ_Exprp()) {
            ಪ್ರತ್ಯಾಗಮ patternScalarKeyp(patp, ಕೀಲಿ_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->ಕೀಲಿ_Exprp(), ಕೀಲಿ_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->ಕೀಲಿ_Exprp());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ foldedp = patp->ಕೀಲಿ_Exprp();
        ಯದಿ (!foldedp->is_Constant_ವೃಕ್ಷ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಯದಿ (foldedp->exists([](ಧ್ರುವ AstConst* constp) { ಪ್ರತ್ಯಾಗಮ constp->ಅಂಕೆ().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        ಪ್ರತ್ಯಾಗಮ patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_ಗಂಟು_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ಪ್ರತ್ಯಾಗಮ patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    ಶೂನ್ಯ sort_Cons_ವಸ್ತುಗಳು(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        ಯಾವತ್ (T_Item* ಧ್ರುವ itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.ತಳ್ಳು_back(itemp);
        }
        ಧ್ರುವ auto ಕೀಲಿ_Of = [](ಧ್ರುವ T_Item* itemp) -> ಧ್ರುವ V3_ಸಂಖ್ಯೆ& {
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ keyp = itemp->keyp();
            ಪ್ರತ್ಯಾಗಮ VN_AS(keyp, Const)->ಅಂಕೆ();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](ಧ್ರುವ T_Item* ap, ಧ್ರುವ T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ಪ್ರತ್ಯಾಗಮ ಕೀಲಿ_Of(ap).isLtXZ(ಕೀಲಿ_Of(bp));
        });
        ಧ್ರುವ T_Item* firstp = nullptr;  // First item with the current key
        ಪ್ರತಿ (T_Item* ಧ್ರುವ itemp : itemps) {
            ಯದಿ (firstp && ಕೀಲಿ_Of(firstp).isCaseEq(ಕೀಲಿ_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ಅನ್ಯಥಾ {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    ಶೂನ್ಯ patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_ಗಂಟು_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಧ್ರುವ auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
            ಯದಿ (keyp) {
                consp->addItemsp(ನೂತನ T_Item{patp->fileline(), keyp, valuep});
            } ಅನ್ಯಥಾ {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ಯದಿ (!assocDtp || !assocDtp->ಕೀಲಿ_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_ವಸ್ತುಗಳು<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newConsDynArrayOrQueue(File_ಸಾಲು* fileline, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಯದಿ (VN_IS(dtypep, DynArrayDType)) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConsDynArray{fileline};
        ಯದಿ (VN_IS(dtypep, QueueDType)) ಪ್ರತ್ಯಾಗಮ ನೂತನ AstConsQueue{fileline};
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    ಶೂನ್ಯ addConsItem(T_Cons* consp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ಧ್ರುವ ತಾರ್ಕಿಕ isElement
            = Ast_ಗಂಟು::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ಧ್ರುವ AstQueueDType* ಧ್ರುವ subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        ಯದಿ (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ಧ್ರುವ subp = Ast_ಗಂಟು::cast<T_Cons>(valuep);
            ಯದಿ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ಯದಿ (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(subp), subp);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (AstConsQueue* ಧ್ರುವ queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_ವಸ್ತುಗಳು(consp, queuep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            ಯದಿ (AstConsDynArray* ಧ್ರುವ dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_ವಸ್ತುಗಳು(consp, dynp);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }
        T_Item* ಧ್ರುವ itemp = ನೂತನ T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ಯದಿ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ಅನ್ಯಥಾ {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    ಶೂನ್ಯ addConsItem(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* consp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep) {
        ಯದಿ (AstConsDynArray* ಧ್ರುವ dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } ಅನ್ಯಥಾ {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    ಶೂನ್ಯ add_Cons_ವಸ್ತುಗಳು(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* consp, T_Sub* subp) {
        ಯಾವತ್ (auto* ಧ್ರುವ itemp = subp->itemsp()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(subp), subp);
    }
    ಶೂನ್ಯ patternDynArrayOrQueue(AstPattern* nodep, Ast_ಗಂಟು_DType* arrayp, AstPatMember* defaultp) {
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ಧ್ರುವ AstPatMember* ಧ್ರುವ firstp = VN_AS(nodep->itemsp(), PatMember);
        ಧ್ರುವ ತಾರ್ಕಿಕ keyed = defaultp || (firstp && firstp->keyp());
        ಯದಿ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಧ್ರುವ ವರ್ಣ* ಧ್ರುವ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ಯದಿ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಯದಿ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_ಮೌಲ್ಯ_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    ಶೂನ್ಯ patternBasic(AstPattern* nodep, Ast_ಗಂಟು_DType* vdtypep, AstPatMember* defaultp) {
        ಧ್ರುವ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ಧ್ರುವ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = nullptr;
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ent : range.seqHiToLo()) {
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
            ಧ್ರುವ auto it = patmap.find(ent);
            ಯದಿ (it == patmap.end()) {
                ಯದಿ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } ಅನ್ಯಥಾ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ಅನ್ಯಥಾ {
                AstPatMember* ಧ್ರುವ patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_ಮೌಲ್ಯ_Iterate(patp);
            }
            ಯದಿ (valuep) {  // Packed. Convert to concat for now.
                ಯದಿ (!newp) {
                    newp = valuep;
                } ಅನ್ಯಥಾ {
                    AstConcat* ಧ್ರುವ concatp = ನೂತನ AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ಯದಿ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        ಯದಿ (newp) {
            nodep->replaceWith(newp);
        } ಅನ್ಯಥಾ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pattern_Member_ಮೌಲ್ಯ_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = patp->lhssp()->unlinkFrBack();
        ಯದಿ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ಯದಿ (AstConst* ಧ್ರುವ newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ಪ್ರತ್ಯಾಗಮ valuep;
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkEventAssignment(ಧ್ರುವ Ast_ಗಂಟು_Assign* ಧ್ರುವ asgnp) {
        string unsupEvtAsgn;
        ಯದಿ (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ಯದಿ (VN_IS(asgnp->rhsp(), CReset)) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        ಯದಿ (!unsupEvtAsgn.empty()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warnMore()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warnMore()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ usesDynamicScheduler(Ast_ಗಂಟು* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ಯಾವತ್ (ಸತ್ಯ) {
            AstVarRef* ಧ್ರುವ vrefp = VN_CAST(nodep, VarRef);
            ಯದಿ (vrefp) ಪ್ರತ್ಯಾಗಮ usesDynamicScheduler(vrefp);
            ಯದಿ (VN_IS(nodep, MemberSel)) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Sel* selp = VN_CAST(nodep, ಗಂಟು_Sel)) {
                nodep = selp->fromp();
            } ಅನ್ಯಥಾ {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ usesDynamicScheduler(AstVarRef* vrefp) {
        ಪ್ರತ್ಯಾಗಮ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    ಶೂನ್ಯ visit(AstPatMember* nodep) override {
        Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    ಪೂರ್ಣಾಙ್ಕ visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        ಯದಿ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep->repp(), Const);
            ಯದಿ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ಅನ್ಯಥಾ {
                times = constp->toUInt();
            }
            ಯದಿ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_ವೃಕ್ಷ();  // Done with replicate before cloning
        }
        ಪ್ರತ್ಯಾಗಮ times;
    }

    ಶೂನ್ಯ visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ಗಂಟು* propp = nodep->propp();
            ಯಾವತ್ (AstLogNot* ಧ್ರುವ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ಧ್ರುವ untilp = VN_CAST(propp, Until);
            ಯದಿ (untilp && VN_IS(nodep->backp(), ಗಂಟು_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(ನೂತನ AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            ಯದಿ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    ತಾರ್ಕಿಕ firstNewStatementOkRecurse(Ast_ಗಂಟು* nodep) {
        ಯದಿ (AstVar* ಧ್ರುವ varp = VN_CAST(nodep, Var)) {
            ಯದಿ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (AstAssign* ಧ್ರುವ aitemp = VN_CAST(nodep, Assign)) {
            ಯದಿ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (AstInitialStaticStmt* ಧ್ರುವ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (!firstNewStatementOkRecurse(stmtp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಯದಿ (AstInitialAutomaticStmt* ಧ್ರುವ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ಯದಿ (!firstNewStatementOkRecurse(stmtp)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    ಶೂನ್ಯ handleCaseType(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, CaseItem* itemsp) {
        AstAttrOf* ಧ್ರುವ exprap = VN_CAST(exprp, AttrOf);
        ಯದಿ (!exprap) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (exprap->attrType() != VAttrType::TYPEID) ಪ್ರತ್ಯಾಗಮ;

        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << ಅಭಿವ್ಯಕ್ತಿ_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_ಸಾಲು* ಧ್ರುವ newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, ಸತ್ಯ);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, ಸತ್ಯ);  // Side effect of transform
        casep->fileline(newfl);
        ಪ್ರತಿ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<CaseItem>(itemp->nextp())) {
            ಯದಿ (itemp->isDefault()) ಅನುವರ್ತ;
            ತಾರ್ಕಿಕ hit = ಅಸತ್ಯ;
            ಪ್ರತಿ (Ast_ಗಂಟು* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ಧ್ರುವ AstAttrOf* ಧ್ರುವ condAttrp = VN_CAST(condp, AttrOf);
                ಯದಿ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ಅನ್ಯಥಾ {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ condDtp = condAttrp->dtypep();
                    ಯದಿ (Ast_ಗಂಟು::computeCastable(ಅಭಿವ್ಯಕ್ತಿ_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = ಸತ್ಯ;
                        ಭಙ್ಗ;
                    }
                }
            }
            ತಳ್ಳು_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(ನೂತನ AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(ನೂತನ AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    ಶೂನ್ಯ handleCase(Ast_ಗಂಟು* casep, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ಗಂಟು_DType* subDTypep = exprp->dtypep();
        ಪ್ರತಿ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<CaseItem>(itemp->nextp())) {
            ಪ್ರತಿ (Ast_ಗಂಟು* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ಯದಿ (condp->dtypep() == subDTypep) ಅನುವರ್ತ;

                ಯದಿ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } ಅನ್ಯಥಾ ಯದಿ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } ಅನ್ಯಥಾ {
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(subDTypep->width(), condp->width());
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    ಧ್ರುವ ತಾರ್ಕಿಕ issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        ಪ್ರತಿ (CaseItem* itemp = itemsp; itemp; itemp = Ast_ಗಂಟು::as<CaseItem>(itemp->nextp())) {
            ಪ್ರತಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    ಶೂನ್ಯ visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ಪ್ರತಿ (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            ಪ್ರತಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    ಶೂನ್ಯ visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        ಪ್ರತಿ (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            ಪ್ರತಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ಶೂನ್ಯ visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ಗಂಟು_DType* ಧ್ರುವ itemDTypep = nodep->findUInt32DType();
        ಪ್ರತಿ (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            ಪ್ರತಿ (Ast_ಗಂಟು *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    ಶೂನ್ಯ visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    ಶೂನ್ಯ visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    ಶೂನ್ಯ visit(Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Foreach* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        nodep->didWidth(ಸತ್ಯ);
        ಧ್ರುವ AstForeachHeader* ಧ್ರುವ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ಗಂಟು_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        ಪ್ರತಿ (Ast_ಗಂಟು *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            ಧ್ರುವ ತಾರ್ಕಿಕ empty = VN_IS(argsp, Empty);
            AstVar* ಧ್ರುವ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            ಯದಿ (varp) varp->usedLoopIdx(ಸತ್ಯ);
            ಯದಿ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ಯದಿ (VN_IS(fromDtp, ಗಂಟು_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } ಅನ್ಯಥಾ ಯದಿ (AstBasicDType* ಧ್ರುವ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ಯದಿ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->ಕೀಲಿ_DTypep());
            } ಅನ್ಯಥಾ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ಗಂಟು* ಧ್ರುವ bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        ಯದಿ (AstForeach* ಧ್ರುವ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Assign* nodep) override {
        // IEEE-2012 10.7, 11.8.2, 11.8.3, 11.5:  (Careful of 11.8.1 which is
        //                  only one step; final dtype depends on assign LHS.)
        //    Determine RHS type width and signing
        //    Propagate type down to *non-self-determined* operators
        //       Real propagates only across one operator if one side is real -
        //       handled in each visitor.
        //    Then LHS sign-extends only if *RHS* is signed
        assertAtStatement(nodep);
        {
            // UINFOTREE(1, nodep, "", "assin:");
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ಯದಿ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            userIterateAndNext(nodep->rhsp(), WidthVP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            streamAssignLower(nodep);
            Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ಯದಿ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDtp = lhsDTypep->skipRefp();
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ಯದಿ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ಯದಿ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ಗಂಟು_Var_Ref, Ast_ಗಂಟು_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ಗಂಟು_Var_Ref), VN_CAST(nodep->rhsp(), ಗಂಟು_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                ಯದಿ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ಗಂಟು_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ಯದಿ (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ controlp = nodep->timingControlp()) {
            ಯದಿ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ(), controlp);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } ಅನ್ಯಥಾ {
                ಯದಿ (nodep->fileline()->timingOn()) {
                    ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ಅನ್ಯಥಾ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_ವೃಕ್ಷ(), controlp);
            }
        }
        ಯದಿ (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ಯದಿ (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            AstMethodCall* ಧ್ರುವ newp
                = ನೂತನ AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (ಧ್ರುವ AstNewDynamic* ಧ್ರುವ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ಯದಿ (!dynp->rhsp()) {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(ಸತ್ಯ);
            newp->protect(ಅಸತ್ಯ);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ಯದಿ (AstCMethodHard* ಧ್ರುವ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            ಯದಿ (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } ಅನ್ಯಥಾ ಯದಿ (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } ಅನ್ಯಥಾ ಯದಿ (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } ಅನ್ಯಥಾ {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            ಯದಿ (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ msbp = slicep->pinsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* ಧ್ರುವ newp
                    = ನೂತನ AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(ಸತ್ಯ);
                newp->protect(ಅಸತ್ಯ);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
        }

        ಯದಿ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    ಶೂನ್ಯ visit(AstRSRule* nodep) override {
        ಯದಿ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    ಶೂನ್ಯ visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    ಶೂನ್ಯ visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isFormatNonNumericArg(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ಪ್ರತ್ಯಾಗಮ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, ಗಂಟು_UOr_Struct_DType)
                   && !VN_AS(dtypep, ಗಂಟು_UOr_Struct_DType)->packed());
    }

    ಶೂನ್ಯ checkFormatNumericArg(Ast_ಗಂಟು* argp, ವರ್ಣ ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ಯದಿ (std::tolower(ch) == 'd') ಪ್ರತ್ಯಾಗಮ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ಯದಿ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    ಶೂನ್ಯ visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    ಶೂನ್ಯ visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ಯದಿ (m_vup && !m_vup->prelim()) ಪ್ರತ್ಯಾಗಮ;  // Can be called as statement or function
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ಯದಿ (!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* oldExprsp = nodep->exprsp();
        ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        }
        ಯದಿ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        ಯಾವತ್ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
            ಯದಿ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            ಯದಿ (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                ಅನುವರ್ತ;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = argp->dtypep()->skipRefp();
            ಯದಿ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ಅನ್ಯಥಾ ಯದಿ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ಅನ್ಯಥಾ ಯದಿ (isFormatNonNumericArg(dtypep)) {
                ಯದಿ (AstClassRefDType* ಧ್ರುವ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ಯದಿ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(ಸತ್ಯ);
                    }
                } ಅನ್ಯಥಾ {
                    // Class handles inside containers print type names, not their members.
                    Ast_ಗಂಟು_DType* ಗಂಟು_Dtypep = dtypep;
                    ಯಾವತ್ (ಗಂಟು_Dtypep) {
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->skipRefp();
                        ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ uOrStructDTypep
                            = VN_CAST(ಗಂಟು_Dtypep, ಗಂಟು_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(ಸತ್ಯ);
                        }
                        ಗಂಟು_Dtypep = ಗಂಟು_Dtypep->subDTypep();
                    }
                }
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2()) {
                ಯದಿ (AstEnumDType* ಧ್ರುವ enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    ಅನುವರ್ತ;
                }
            }
            ಯದಿ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ಯದಿ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } ಅನ್ಯಥಾ {
                nodep->addExprsp(ನೂತನ AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    ಶೂನ್ಯ visit(AstCReset* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    ಶೂನ್ಯ visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ಶೂನ್ಯ visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    ಶೂನ್ಯ visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ಯದಿ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        ಯದಿ (!m_paramsOnly) {
            nodep->foreach([ಏತತ್](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            ಯದಿ (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ವಿಕಲ್ಪ (nodep->displayType()) {
            ಪಕ್ಷ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); ಭಙ್ಗ;
            ಪಕ್ಷ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); ಭಙ್ಗ;
            ಸ್ವಭಾವ: UASSERT_OBJ(ಅಸತ್ಯ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
        }
    }
    ಶೂನ್ಯ visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ಯದಿ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFError* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ visit(AstFEof* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    ಶೂನ್ಯ visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ಯದಿ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ಶೂನ್ಯ visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ಶೂನ್ಯ visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFGetC* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ಶೂನ್ಯ visit(AstFGetS* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ visit(AstFUngetC* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    ಶೂನ್ಯ visit(AstFRead* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ಯದಿ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ಯದಿ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    ಶೂನ್ಯ visit(AstFScanF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ visit(AstSScanF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ visit(Ast_ರಾಶಿ_Trace_F* nodep) override { nodep->dtypeSetString(); }
    ಶೂನ್ಯ visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstSystemF* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        ಧ್ರುವ Ast_ಗಂಟು_DType* subp = nullptr;
        ಯದಿ (ಧ್ರುವ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ಯದಿ (!adtypep->ಕೀಲಿ_DTypep()->skipRefp()->basicp()
                || !adtypep->ಕೀಲಿ_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ಅನ್ಯಥಾ {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ಯದಿ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstTestPlusArgs* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    ಶೂನ್ಯ visit(Ast_ಮೌಲ್ಯ_Plus_Args* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ಶೂನ್ಯ visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        ಯದಿ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ಯದಿ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ಯದಿ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ಯದಿ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    ಶೂನ್ಯ visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ಶೂನ್ಯ visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    ತಾರ್ಕಿಕ pinInstArrayElement(AstPin* nodep, ಧ್ರುವ AstRange* rangep, ಪೂರ್ಣಾಙ್ಕ idx) {
        Ast_ಗಂಟು_DType* ಧ್ರುವ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ಗಂಟು_DType* ಧ್ರುವ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ಯದಿ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // Types don't match, but all dimensions done, still need the usual checks
        ಯದಿ (!rangep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        ಧ್ರುವ AstRange* ಧ್ರುವ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        ಪೂರ್ಣಾಙ್ಕ subElems = 1;
        ಪ್ರತಿ (ಧ್ರುವ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements = rangep->elementsConst();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ from_ಎಡ = idx / subElems;

        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ಧ್ರುವ uint32_t conDims = conDTypep->dimensions(ಅಸತ್ಯ).second;
        ಧ್ರುವ uint32_t modDims = modDTypep->dimensions(ಅಸತ್ಯ).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ಯದಿ (arrp && conDims != modDims) {
            ಯದಿ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            // ArraySel index is counted from lo
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ bit = arrp->declRange().ascending() ? from_ಎಡ : elements - 1 - from_ಎಡ;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlinkFrBack();
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(fromp), fromp);
            } ಅನ್ಯಥಾ {
                AstArraySel* ಧ್ರುವ selp = ನೂತನ AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(ಸತ್ಯ);
                nodep->exprp(selp);
            }
            ಪ್ರತ್ಯಾಗಮ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ಯದಿ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Packed connection
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ modwidth = modDTypep->width();
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        ಯದಿ (conwidth == modwidth) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ಯದಿ (conwidth == modwidth * elements * subElems) {
            ಯದಿ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ಎಡ_Const() << ":"
                                                     << rangep->ಬಲ_Const() << "]");
            }
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ fromp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)->unlinkFrBack();
            AstSel* ಧ್ರುವ selp = ನೂತನ AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ಎಡ), selwidth};
            selp->didWidth(ಸತ್ಯ);
            nodep->exprp(selp);
            ಪ್ರತ್ಯಾಗಮ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }

    ಶೂನ್ಯ visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ಯದಿ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ಧ್ರುವ concatp = VN_CAST(nodep->exprp(), Replicate);
            ಧ್ರುವ AstConst* ಧ್ರುವ countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ಯದಿ (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* ಧ್ರುವ patternp = ನೂತನ AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } ಅನ್ಯಥಾ ಯದಿ (!m_paramsOnly) {
            ಯದಿ (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            ಯದಿ (!nodep->exprp()) {  // No-connect
                ಯದಿ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                ಪ್ರತ್ಯಾಗಮ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ಧ್ರುವ VDirection pinDirection = nodep->modVarp()->direction();
            ಯದಿ (VN_IS(nodep->exprp(), ಗಂಟು_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ಗಂಟು_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ಗಂಟು_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ಯದಿ (ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ಯದಿ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ಗಂಟು_DType* subDTypep = modDTypep;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ modwidth = modDTypep->width();
            ಪೂರ್ಣಾಙ್ಕ conwidth = conDTypep->width();
            ಯದಿ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } ಅನ್ಯಥಾ {
                ಯದಿ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ಯದಿ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ಯದಿ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        ಪ್ರತ್ಯಾಗಮ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                ಯದಿ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ಅನ್ಯಥಾ ಯದಿ (nodep->modVarp()->isTristate()) {
                    ಯದಿ (modwidth != conwidth) {
                        // Ideally should call pinReconnectSimple which would tolerate this
                        // then have a conversion warning
                        nodep->v3warn(E_UNSUPPORTED,
                                      "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                      << " to inout signal requires " << modwidth
                                                      << " bits, but connection's "
                                                      << nodep->exprp()->prettyTypeName()
                                                      << " generates " << conwidth << " bits.");
                        // otherwise would need some mess to force both sides to proper size
                    }
                } ಅನ್ಯಥಾ ಯದಿ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ಯದಿ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ಯದಿ (ಅಭಿವ್ಯಕ್ತಿ_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ಅನ್ಯಥಾ ಯದಿ ((ಅಭಿವ್ಯಕ್ತಿ_Arrayp && !modArrayp) || (!ಅಭಿವ್ಯಕ್ತಿ_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (ಅಭಿವ್ಯಕ್ತಿ_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ಅನ್ಯಥಾ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ಗಂಟು_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ಗಂಟು_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ಯದಿ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    ಶೂನ್ಯ visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ಯದಿ (!m_paramsOnly) {
            ಯದಿ (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_ಸಂದೇಶ(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ಯದಿ (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ಯದಿ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ತಳ್ಳು_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    ಶೂನ್ಯ visit(AstGatePin* nodep) override {
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ pinDTypep = nodep->exprp()->dtypep();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಅಂಕೆ_Insts = nodep->rangep()->elementsConst();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ modwidth = ಅಂಕೆ_Insts;
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ pinwidth = pinDTypep->width();
            ಯದಿ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_ಗಂಟು* ಧ್ರುವ newp
                    = ನೂತನ AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(ಅಂಕೆ_Insts)};
                nodep->replaceWith(newp);
            } ಅನ್ಯಥಾ {
                ಯದಿ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                userIterateAndNext(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlinkFrBack());
            }
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(ಸತ್ಯ);
            nodep->didWidth(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ಅನ್ಯಥಾ ಯದಿ (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ಯದಿ (nodep->classMethod() && nodep->name() == "new") {
            ಯದಿ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ಯದಿ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ಗಂಟು* firstp = nullptr;
            ಪ್ರತಿ (Ast_ಗಂಟು* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ಯದಿ (Ast_Stmt_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ sep = VN_CAST(itemp, Stmt_ಅಭಿವ್ಯಕ್ತಿ)) {
                    ಯದಿ (AstNew* ಧ್ರುವ newp = VN_CAST(sep->exprp(), New)) {
                        ಯದಿ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ಭಙ್ಗ;
                        }
                    }
                    ಅನುವರ್ತ;
                }
                ಯದಿ (firstNewStatementOkRecurse(itemp)) ಅನುವರ್ತ;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(ಸತ್ಯ);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ಯದಿ (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        ಯದಿ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        ಯದಿ (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(ಸತ್ಯ);
        nodep->doingWidth(ಅಸತ್ಯ);
        ಯದಿ (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ಯದಿ (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](ಧ್ರುವ AstVarRef* varrefp) {
                   ಪ್ರತ್ಯಾಗಮ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } ಅನ್ಯಥಾ ಯದಿ (AstAttrOf* ಧ್ರುವ attrp = VN_CAST(stmtp, AttrOf)) {
                ಯದಿ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                ಯದಿ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ಯದಿ (protop) {
            ಪ್ರತಿ (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                ಯದಿ (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ಭಙ್ಗ;
                } ಅನ್ಯಥಾ {
                    AstVar* ಧ್ರುವ portp = ports[i];
                    AstAttrOf* ಧ್ರುವ rprotop = protos[i];
                    Ast_ಗಂಟು_DType* ಧ್ರುವ declDtp = portp->dtypep();
                    Ast_ಗಂಟು_DType* ಧ್ರುವ protoDtp = rprotop->fromp()->dtypep();
                    ಯದಿ (portp->name() != rprotop->name()) {
                        protoDtp->v3warn(PROTOTYPEMIS,
                                         "In prototype for "
                                             << nodep->prettyNameQ() << ", argument " << (i + 1)
                                             << " named " << rprotop->prettyNameQ()
                                             << " mismatches out-of-block argument name "
                                             << portp->prettyNameQ() << "  (IEEE 1800-2023 8.24)\n"
                                             << protoDtp->warnContextPrimary() << '\n'
                                             << declDtp->warnOther()
                                             << "... Location of out-of-block declaration\n"
                                             << declDtp->warnContextSecondary());
                    } ಅನ್ಯಥಾ ಯದಿ (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warnMore() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnMore() << "... Declaration data type: "
                                << declDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warnContextPrimary() << '\n'
                                << declDtp->warnOther()
                                << "... Location of out-of-block declaration\n"
                                << declDtp->warnContextSecondary());
                    }
                }
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
        }
    }
    ಶೂನ್ಯ visit(AstConstraint* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstProperty* nodep) override {
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(ಸತ್ಯ);
            nodep->didWidth(ಸತ್ಯ);
            ಪ್ರತ್ಯಾಗಮ;
        }
        nodep->doingWidth(ಸತ್ಯ);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        ಪ್ರತಿ (Ast_ಗಂಟು* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ಯದಿ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } ಅನ್ಯಥಾ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(ಸತ್ಯ);
        nodep->doingWidth(ಅಸತ್ಯ);
    }
    ಶೂನ್ಯ visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    ಶೂನ್ಯ visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        nodep->dtypeSetBit();
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(stmtp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ಅನ್ಯಥಾ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(ಸತ್ಯ);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_ಗಂಟು* pkgItemp) {
        ಯಾವತ್ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ಪ್ರತ್ಯಾಗಮ VN_CAST(pkgItemp->backp(), Package);
    }
    ಶೂನ್ಯ visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_ಗಂಟು_FTask_Ref*>(nodep));
        ಯದಿ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ಯದಿ (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ಅಭಿವ್ಯಕ್ತಿ))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } ಅನ್ಯಥಾ {
            nodep->dtypeFrom(nodep->taskp());
        }
        ಯದಿ (nodep->fileline()->timingOn()) {
            Ast_ಗಂಟು_Module* ಧ್ರುವ classp = nodep->classOrPackagep();
            ಯದಿ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ಧ್ರುವ packagep = getItemPackage(classp);
                ಯದಿ (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ಸ್ಥಿರ ತಾರ್ಕಿಕ areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ಧ್ರುವ std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ಯದಿ (dims0.size() != dims1.size()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತಿ (size_t i = 0; i < dims0.size(); ++i) {
            ಯದಿ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    // Makes sure that port and pin have same size and same datatype
    ಶೂನ್ಯ checkUnpackedArrayArgs(AstVar* portp, Ast_ಗಂಟು* pinp) {
        ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ಯದಿ (AstUnpackArrayDType* ಧ್ರುವ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ಯದಿ (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                ಧ್ರುವ AstBasicDType* ಧ್ರುವ portBasicp = portDtypep->basicp();
                ಧ್ರುವ AstBasicDType* ಧ್ರುವ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ಧ್ರುವ ತಾರ್ಕಿಕ sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                ಯದಿ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } ಅನ್ಯಥಾ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    ಶೂನ್ಯ processFTaskRefArgs(Ast_ಗಂಟು_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        ಕುರು {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ಧ್ರುವ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ಯದಿ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
                ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
                AstArg* ಧ್ರುವ argp = tconnect.second;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
                ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ಯದಿ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argsp = nullptr;
                    ಯಾವತ್ (AstArg* ಧ್ರುವ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_ವೃಕ್ಷ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ಯದಿ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->ಅಂಕೆ().toString();
                    } ಅನ್ಯಥಾ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(argp), argp);
                    AstSFormatF* ಧ್ರುವ newp
                        = ನೂತನ AstSFormatF{nodep->fileline(), format, ಅಸತ್ಯ, argsp};
                    ಯದಿ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(ನೂತನ AstScopeName{newp->fileline(), ಸತ್ಯ});
                    }
                    handle.relink(ನೂತನ AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    ಗಚ್ಛ reloop;  // so exit early; next loop will correct it
                }  //
                ಅನ್ಯಥಾ ಯದಿ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ಯದಿ (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ಯಾವತ್ (ಅಸತ್ಯ);
        // Stage 2
        {
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
                AstVar* ಧ್ರುವ portp = tconnect.first;
                ಧ್ರುವ AstArg* ಧ್ರುವ argp = tconnect.second;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = argp->exprp();
                ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
                // Change data types based on above accept completion
                ಯದಿ (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                ಯದಿ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
                ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
                ಧ್ರುವ AstArg* ಧ್ರುವ argp = tconnect.second;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ pinp = argp->exprp();
                ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ಯದಿ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
                ಧ್ರುವ AstVar* ಧ್ರುವ portp = tconnect.first;
                ಧ್ರುವ AstArg* ಧ್ರುವ argp = tconnect.second;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* pinp = argp->exprp();
                ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
                Ast_ಗಂಟು_DType* ಧ್ರುವ portDTypep = portp->dtypep()->skipRefToEnump();
                ಯದಿ (VN_IS(pinp, EmptyQueue)) {
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    ಯದಿ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = ನೂತನ AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ pinDTypep = pinp->dtypep()->skipRefToEnump();
                ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ portIfacep
                    = VN_CAST(portDTypep->elemDTypep(ಸತ್ಯ), IfaceRefDType);
                ಧ್ರುವ VCastable ifaceCastable
                    = portIfacep ? Ast_ಗಂಟು::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ ಸತ್ಯ)
                                 : VCastable{VCastable::UNSUPPORTED};
                ಧ್ರುವ ತಾರ್ಕಿಕ matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                ಧ್ರುವ ತಾರ್ಕಿಕ pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->ಅಂಕೆ().isNull();
                ಯದಿ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ಯದಿ (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ಅನ್ಯಥಾ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ಅನ್ಯಥಾ ಯದಿ (portIfacep && portIfacep->isVirtual() && portp->is_ಒಳಹರಿವು() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ಅನ್ಯಥಾ ಯದಿ (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ಯದಿ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ಅನುವರ್ತ;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ಯದಿ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ಅನ್ಯಥಾ ಯದಿ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } ಅನ್ಯಥಾ {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    ಶೂನ್ಯ handleStdRandomizeArgs(Ast_ಗಂಟು_FTask_Ref* ಧ್ರುವ nodep) {
        AstConst* nullp = nullptr;
        ಪ್ರತಿ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = argp->exprp();
            ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(exprp, Const)) {
                ಯದಿ (constp->ಅಂಕೆ().isNull()) {
                    nullp = constp;
                    ಅನುವರ್ತ;
                }
            }
            ಯದಿ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ಅನುವರ್ತ;
            }
            ಯದಿ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ಅನುವರ್ತ;
            }
            ಯದಿ (ಧ್ರುವ AstCMethodHard* ಧ್ರುವ methodp = VN_CAST(exprp, CMethodHard)) {
                ಯದಿ (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    ಅನುವರ್ತ;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ಯದಿ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        ಯದಿ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(ಸತ್ಯ);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            ಪ್ರತ್ಯಾಗಮ;  // Handled in V3Randomize
        } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ಧ್ರುವ classp = VN_CAST(nodep->classOrPackagep(), Class);
            ಯದಿ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                Ast_ಗಂಟು_DType* ಧ್ರುವ adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                ಪ್ರತಿ (ಧ್ರುವ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                ಯದಿ (withp) nodep->withp(withp);
                nodep->didWidth(ಸತ್ಯ);
                ಪ್ರತ್ಯಾಗಮ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ಯದಿ (nodep->name() == "randomize") {
                AstClassRefDType* ಧ್ರುವ adtypep
                    = ನೂತನ AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, ಅಸತ್ಯ, ಅಸತ್ಯ, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                ಪ್ರತಿ (ಧ್ರುವ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(ಸತ್ಯ);
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                AstCExpr* ಧ್ರುವ newp
                    = ನೂತನ AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(ಸತ್ಯ);
                v3Global.useRandomizeMethods(ಸತ್ಯ);
                AstCExpr* ಧ್ರುವ newp
                    = ನೂತನ AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                UASSERT_OBJ(ಅಸತ್ಯ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ಯದಿ (nodep->didWidth()) {
            ಯದಿ (withp) nodep->withp(withp);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            ತಾರ್ಕಿಕ allow = ಅಸತ್ಯ;
            ಯದಿ (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                ಯದಿ (ಧ್ರುವ AstFuncRef* ಧ್ರುವ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstTaskRef* ಧ್ರುವ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ಯದಿ (!allow) {
                    ಧ್ರುವ AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    ಯದಿ (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ಧ್ರುವ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ಯದಿ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ಯದಿ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ಧ್ರುವ AstClass* ಧ್ರುವ cgClassp = m_containingClassFinder.find(nodep);
                ಯದಿ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ಧ್ರುವ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ಯದಿ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ಯದಿ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ಯದಿ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(ನೂತನ AstScopeName{nodep->fileline(), ಅಸತ್ಯ});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        ಯದಿ (withp) nodep->withp(withp);
        nodep->didWidth(ಸತ್ಯ);
        // See steps that follow in visit(AstFuncRef*)
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    ಶೂನ್ಯ visit(AstSenItem* nodep) override {
        ಯದಿ (nodep->isComboStar()) ಪ್ರತ್ಯಾಗಮ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ಯದಿ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* ಧ್ರುವ concatOrReplp = VN_CAST(nodep->sensp(), ಗಂಟು_Biop);
            auto* ಧ್ರುವ rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            ಯದಿ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* ಧ್ರುವ lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(ನೂತನ AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(ನೂತನ AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
        } ಅನ್ಯಥಾ {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            ಯದಿ (nodep->edgeType().anEdge()) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ಯದಿ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ಅನ್ಯಥಾ ಯದಿ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    ಶೂನ್ಯ visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ಧ್ರುವ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    ಶೂನ್ಯ visit(AstWait* nodep) override {
        ಯದಿ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_ವೃಕ್ಷ(), nodep);
            ಪ್ರತ್ಯಾಗಮ;
        }
        ಯದಿ (nodep->fileline()->timingOn()) {
            ಯದಿ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ varrefp = VN_CAST(nodep->condp(), ಗಂಟು_Var_Ref)) {
                    ಯದಿ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ಅನ್ಯಥಾ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } ಅನ್ಯಥಾ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
    }
    ಶೂನ್ಯ visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->ಮೌಲ್ಯ_Arg_Refp(), nullptr);
        ಯದಿ (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } ಅನ್ಯಥಾ {
            ಯದಿ (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } ಅನ್ಯಥಾ {
                ಯದಿ (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } ಅನ್ಯಥಾ {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            ಯದಿ (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } ಅನ್ಯಥಾ {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    ಶೂನ್ಯ visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ಯದಿ (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } ಅನ್ಯಥಾ {
            nodep->dtypeFrom(m_withp->ಮೌಲ್ಯ_Arg_Refp());
        }
    }
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) override {
        ಯದಿ (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    ಶೂನ್ಯ visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ಯದಿ (Ast_ಗಂಟು_DType* ಧ್ರುವ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ಅನ್ಯಥಾ ಯದಿ (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->width() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->width() << " bits -> " << nodep->width()
                                    << " bits" << endl);

        // Final processing
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, FINAL}.p());
        nodep->didWidth(ಸತ್ಯ);
    }
    ಶೂನ್ಯ visitClass(AstClass* nodep) {
        ಯದಿ (nodep->didWidthAndSet()) ಪ್ರತ್ಯಾಗಮ;

        // If the class is std::process
        ಯದಿ (nodep->name() == "process") {
            AstPackage* ಧ್ರುವ packagep = getItemPackage(nodep);
            ಯದಿ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ಯದಿ (AstVar* ಧ್ರುವ varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep = varp->getChildDTypep();
                    ಯದಿ (!varp->dtypep()) {
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* ಧ್ರುವ newdtypep = ನೂತನ AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ಯದಿ (Ast_ಗಂಟು_FTask* ಧ್ರುವ ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), ಗಂಟು_FTask)) {
                    ftaskp->setNeedProcess();
                    v3Global.setUsesTiming();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        userIterateAndNext(nodep->extendsp(), nullptr);
        userIterateChildren(nodep, nullptr);  // First size all members
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        ಯದಿ (AstClass* ಧ್ರುವ classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } ಅನ್ಯಥಾ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ಏತತ್](ಧ್ರುವ AstVar* varp) { m_curModVars.ಸೇರಿಸು(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    ಶೂನ್ಯ visit(Ast_ಗಂಟು* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    ಶೂನ್ಯ visit_Or_Lu64(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    ಶೂನ್ಯ visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            ಯದಿ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    ನೂತನ AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_ವೃಕ್ಷ(), nodep);
            }
        }
    }
    ಶೂನ್ಯ visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    ಶೂನ್ಯ visit_Os32_Lr(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    ಶೂನ್ಯ visit_Ou64_Lr(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    ಶೂನ್ಯ visit_log_not(AstLogNot* nodep) {
        // CALLER: LogNot
        // Width-check: lhs 1 bit
        // Real: Allowed; implicitly compares with zero
        // We calculate the width of the UNDER expression.
        // We then check its width to see if it's legal, and edit if not
        // We finally set the width of our output
        // IEEE-2012: Table 11-21 and 11.8.1 (same as RedAnd):
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ಯದಿ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ಧ್ರುವ newp
                    = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            }
        }
    }
    ಶೂನ್ಯ visit_log_and_or(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    ಶೂನ್ಯ visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit_red_and_or(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    ಶೂನ್ಯ visit_red_unknown(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    ತಾರ್ಕಿಕ isEquivalentDType(ಧ್ರುವ Ast_ಗಂಟು_DType* lhs, ಧ್ರುವ Ast_ಗಂಟು_DType* rhs) {
        // a) If two types match, they are equivalent.
        ಯದಿ (!lhs || !rhs) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        ಯದಿ (lhs == rhs) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        // If both are basic types, check if they are the same type
        ಯದಿ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ಧ್ರುವ auto* lb = VN_CAST(lhs, BasicDType);
            ಧ್ರುವ auto* rb = VN_CAST(rhs, BasicDType);
            ಯದಿ (lb->isString() != rb->isString()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ಯದಿ (lhsIsUnpackArray || rhsIsUnpackArray) {
            ಯದಿ (lhsIsUnpackArray && rhsIsUnpackArray) {
                ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ lhsp = VN_CAST(lhs, UnpackArrayDType);
                ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ rhsp = VN_CAST(rhs, UnpackArrayDType);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lsz = lhsp->elementsConst();
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rsz = rhsp->elementsConst();
                ಯದಿ (lsz >= 0 && rsz >= 0 && lsz != rsz) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
                ಪ್ರತ್ಯಾಗಮ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsQueue = VN_IS(lhs, QueueDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsQueue = VN_IS(rhs, QueueDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ಯದಿ (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ lhsp = VN_CAST(lhs, DynArrayDType)) {
                ಯದಿ (ಧ್ರುವ AstDynArrayDType* ಧ್ರುವ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ಪ್ರತ್ಯಾಗಮ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ lhsp = VN_CAST(lhs, QueueDType)) {
                ಯದಿ (ಧ್ರುವ AstQueueDType* ಧ್ರುವ rhsp = VN_CAST(rhs, QueueDType)) {
                    ಪ್ರತ್ಯಾಗಮ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ಯದಿ (ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ಯದಿ (ಧ್ರುವ AstAssocArrayDType* ಧ್ರುವ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ಪ್ರತ್ಯಾಗಮ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->ಕೀಲಿ_DTypep(), rhsp->ಕೀಲಿ_DTypep());
                }
            }

            ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ಯದಿ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ಯದಿ (lhs->width() != rhs->width()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (lhs->isFourstate() != rhs->isFourstate()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಯದಿ (lhs->isSigned() != rhs->isSigned()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ಯದಿ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ !(VN_IS(lhs, ಗಂಟು_UOr_Struct_DType) || VN_IS(rhs, ಗಂಟು_UOr_Struct_DType));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isAggregateType(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = nodep->dtypep();
        ಯದಿ (!dtypep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        dtypep = dtypep->skipRefp();
        ಯದಿ (!dtypep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ dtypep->isAggregateType();
    }

    ಶೂನ್ಯ visit_cmp_eq_gt(Ast_ಗಂಟು_Biop* nodep, ಧ್ರುವ ComparisonType cmpType) {
        // CALLER: AstEq, AstGt, ..., AstLtS
        // Real allowed if and only if real_lhs set
        // See IEEE-2012 11.4.4, and 11.8.1:
        //   Widths: 1 bit out, width is max of LHS or RHS
        //   Sign:  signed compare (not output) if both signed, compare is signed,
        //             width mismatches sign extend
        //             else, compare is unsigned, **zero-extends**
        //   Real:  If either real, other side becomes real and real compare
        //   TODO: chandle/class handle/iface handle: WildEq/WildNeq same as Eq/Neq
        //   TODO: chandle/class handle/iface handle only allowed to self-compare or against null
        //   TODO: chandle/class handle/iface handle no relational compares
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ಧ್ರುವ ತಾರ್ಕಿಕ lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            ಧ್ರುವ ತಾರ್ಕಿಕ rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            ಯದಿ (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ಅನ್ಯಥಾ ಯದಿ (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ಅನ್ಯಥಾ {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            ಧ್ರುವ ತಾರ್ಕಿಕ isAggrLhs = isAggregateType(nodep->lhsp());
            ಧ್ರುವ ತಾರ್ಕಿಕ isAggrRhs = isAggregateType(nodep->rhsp());

            ಯದಿ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDType = nodep->lhsp()->dtypep();
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsDType = nodep->rhsp()->dtypep();

                ಯದಿ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ಗಂಟು* ಧ್ರುವ newp
                        = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ಅನ್ಯಥಾ ಯದಿ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ಯದಿ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ಗಂಟು* ಧ್ರುವ newp
                        = ನೂತನ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                    ಪ್ರತ್ಯಾಗಮ;
                }
                ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ಅನ್ಯಥಾ ಯದಿ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ ತಾರ್ಕಿಕ signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ತಾರ್ಕಿಕ warnOn = ಸತ್ಯ;
                ಯದಿ (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ಯದಿ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = ಅಸತ್ಯ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ಯದಿ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = ಅಸತ್ಯ;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(width, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtypeSetBitOrLogic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::NORMAL_EQ
                                               || cmpType == ComparisonType::NORMAL_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    ಶೂನ್ಯ visit_cmp_real(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit_cmp_string(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ಶೂನ್ಯ visit_cmp_type(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            ಧ್ರುವ AstAttrOf* ಧ್ರುವ lhsap = VN_AS(nodep->lhsp(), AttrOf);
            ಧ್ರುವ AstAttrOf* ಧ್ರುವ rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDtp = lhsap->dtypep();
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ಧ್ರುವ ತಾರ್ಕಿಕ invert = VN_IS(nodep, NeqT);
            ಧ್ರುವ ತಾರ್ಕಿಕ identical
                = Ast_ಗಂಟು::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ಧ್ರುವ ತಾರ್ಕಿಕ eq = invert ^ identical;
            Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        }
    }

    ಶೂನ್ಯ visit_negate_not(Ast_ಗಂಟು_Uniop* nodep, ತಾರ್ಕಿಕ real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ಯದಿ (!real_ok) checkCvtUS(nodep->lhsp(), ಅಸತ್ಯ);
        }
        ಯದಿ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ಯದಿ (Ast_ಗಂಟು_Uniop* ಧ್ರುವ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                ಪ್ರತ್ಯಾಗಮ;
            }
        } ಅನ್ಯಥಾ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ಯದಿ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            // Some warning suppressions
            ತಾರ್ಕಿಕ lhsWarn = ಸತ್ಯ;
            ಯದಿ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ಯದಿ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ಅಸತ್ಯ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    ಶೂನ್ಯ visit_signed_unsigned(Ast_ಗಂಟು_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ಸತ್ಯ);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = nodep->lhsp()->width();
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ಶೂನ್ಯ visit_shift(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ಧ್ರುವ Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (ಶೂನ್ಯ)newp;  // Ununused
    }
    ಶೂನ್ಯ iterate_shift_prelim(Ast_ಗಂಟು_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ಅಸತ್ಯ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ಯದಿ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ಗಂಟು_Biop* iterate_shift_final(Ast_ಗಂಟು_Biop* nodep) {
        // Nodep maybe edited
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ಯದಿ (VN_IS(nodep, ShiftRS)) {
                ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            ತಾರ್ಕಿಕ warnOn = ಸತ್ಯ;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ಯದಿ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ಗಂಟು_Assign)) warnOn = ಅಸತ್ಯ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ಧ್ರುವ shiftp = VN_CAST(nodep->rhsp(), Const);
            ಯದಿ (shiftp && !shiftp->ಅಂಕೆ().isFourState() && shiftp->width() <= 32) {
                ಧ್ರುವ int64_t shiftVal = shiftp->ಅಂಕೆ().toSQuad();
                ಯದಿ (VN_IS(nodep, ShiftL)) {
                    ಯದಿ (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = ಅಸತ್ಯ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ಯದಿ (nodep->rhsp()->width() > 32) {
                ಯದಿ (shiftp && shiftp->ಅಂಕೆ().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_ಸಂಖ್ಯೆ ಅಂಕೆ{shiftp, 32, shiftp->ಅಂಕೆ()};
                    Ast_ಗಂಟು* ಧ್ರುವ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(ನೂತನ AstConst{shiftrhsp->fileline(), ಅಂಕೆ});
                    VL_DO_DANGLING(shiftrhsp->delete_ವೃಕ್ಷ(), shiftrhsp);
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ nodep;  // May edit
    }

    ಶೂನ್ಯ visit_boolexpr_and_or(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ಅಸತ್ಯ);
            checkCvtUS(nodep->rhsp(), ಅಸತ್ಯ);
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            ಧ್ರುವ ತಾರ್ಕಿಕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ಯದಿ (m_vup->final()) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ಶೂನ್ಯ visit_add_sub_replace(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ real_ok, ಧ್ರುವ ತಾರ್ಕಿಕ modOrDiv) {
        // CALLER: (real_ok=false) AddS, SubS, ...
        // CALLER: (real_ok=true) Add, Sub, ...
        // Widths: out width = lhs width = rhs width
        // Signed: Replace operator with signed operator, or signed to unsigned
        // Real: Replace operator with real operator
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        //
        // UINFOTREE(9, nodep, "rus " << m_vup, "rusin");
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            ಯದಿ (!real_ok) {
                checkCvtUS(nodep->lhsp(), ಅಸತ್ಯ);
                checkCvtUS(nodep->rhsp(), ಅಸತ್ಯ);
            }
            ಯದಿ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                ಧ್ರುವ ತಾರ್ಕಿಕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ಯದಿ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ಗಂಟು_DType* ಧ್ರುವ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ಯದಿ (Ast_ಗಂಟು_Biop* ಧ್ರುವ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            ತಾರ್ಕಿಕ lhsWarn = ಸತ್ಯ;
            ತಾರ್ಕಿಕ rhsWarn = ಸತ್ಯ;
            ಯದಿ (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ಯದಿ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = ಅಸತ್ಯ;
                ಯದಿ (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = ಅಸತ್ಯ;
                ಯದಿ (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = ಅಸತ್ಯ;  // do_increment + ...
                ಯದಿ (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = ಅಸತ್ಯ;  // ... + do_increment
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ಯದಿ (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = ಅಸತ್ಯ;
                ಯದಿ (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = ಅಸತ್ಯ;
            }
            // Final call, so make sure children check their sizes
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         rhsWarn);
        }
        // UINFOTREE(9, nodep, "", "rusou");
    }
    ಶೂನ್ಯ visit_real_add_sub(Ast_ಗಂಟು_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    ಶೂನ್ಯ visit_real_neg_ceil(Ast_ಗಂಟು_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_ಅಭಿವ್ಯಕ್ತಿ(nodep);
        ಯದಿ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    ಶೂನ್ಯ visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->ಅಭಿವ್ಯಕ್ತಿ_Format_2(), nodep, "Assumes constant format");
        ತಾರ್ಕಿಕ inPct = ಅಸತ್ಯ;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* argp = nodep->exprsp();
        string newFormat;
        ಪ್ರತಿ (ವರ್ಣ ch : nodep->text()) {
            ಯದಿ (!inPct && ch == '%') {
                inPct = ಸತ್ಯ;
                newFormat += ch;
            } ಅನ್ಯಥಾ ಯದಿ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ಅನ್ಯಥಾ ಯದಿ (!inPct) {  // Normal text
                newFormat += ch;
            } ಅನ್ಯಥಾ {
                inPct = ಅಸತ್ಯ;
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ nextp = argp ? VN_AS(argp->nextp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) : nullptr;
                AstSFormatArg* ಧ್ರುವ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subargp = fargp ? fargp->exprp() : argp;
                ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                ವಿಕಲ್ಪ (ch) {
                ಪಕ್ಷ '?':  // Unspecified by user, guess
                    ಯದಿ (dtypep->isDouble()) {
                        ch = 'g';
                    } ಅನ್ಯಥಾ ಯದಿ (dtypep->isString()) {
                        ch = 's';
                    } ಅನ್ಯಥಾ ಯದಿ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ಅನ್ಯಥಾ {
                        ch = 'p';
                    }
                    argp = nextp;
                    ಭಙ್ಗ;
                ಪಕ್ಷ '%': ಭಙ್ಗ;  // %% - just output a %
                ಪಕ್ಷ 'm': ಭಙ್ಗ;  // %m - auto insert "name"
                ಪಕ್ಷ 'l': ಭಙ್ಗ;  // %m - auto insert "library"
                ಪಕ್ಷ 'd':
                    ಯದಿ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ಯದಿ (subargp->isDouble()) spliceCvtS(subargp, ಸತ್ಯ, 64);
                        argp = nextp;
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'b':  // FALLTHRU
                ಪಕ್ಷ 'o':  // FALLTHRU
                ಪಕ್ಷ 'x':
                    ಯದಿ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 't':  // Decimal/float time
                    ಯದಿ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'e':  // FALLTHRU
                ಪಕ್ಷ 'f':  // FALLTHRU
                ಪಕ್ಷ 'g':
                    ಯದಿ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ಯದಿ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ಭಙ್ಗ;
                ಪಕ್ಷ 'p':  // FALLTHRU
                ಪಕ್ಷ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ಯದಿ (subargp && !fargp) {
                        ಯದಿ (AstEnumDType* ಧ್ರುವ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ಭಙ್ಗ;
                ಸ್ವಭಾವ:  // Most operators, just move to next argument
                    argp = nextp;
                    ಭಙ್ಗ;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    ಸ್ಥಿರ AstEnumDType* formatEnumDType(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        ಯದಿ (!enumDtp) {
            ಯದಿ (ಧ್ರುವ AstVarRef* ಧ್ರುವ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        ಪ್ರತ್ಯಾಗಮ enumDtp;
    }

    Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* newEnumCapture(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, AstEnumDType* dtypep) {
        File_ಸಾಲು* ಧ್ರುವ flp = valuep->fileline();
        AstVar* ಧ್ರುವ varp = ನೂತನ AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(ಸತ್ಯ);
        ಯದಿ (m_ftaskp) {
            varp->funcLocal(ಸತ್ಯ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ exprp = ನೂತನ Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt{
            flp, ನೂತನ AstAssign{flp, ನೂತನ AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            ನೂತನ AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ಪ್ರತ್ಯಾಗಮ exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep, AstEnumDType* dtypep) {
        ಯದಿ (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* keyp;
        ಯದಿ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        } ಅನ್ಯಥಾ {
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ);
        }
        ಧ್ರುವ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ಧ್ರುವ argp = ನೂತನ AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ಪ್ರತ್ಯಾಗಮ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    ತಾರ್ಕಿಕ widthBad(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು_DType* expDTypep) {
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expWidth = expDTypep->width();
        ಪೂರ್ಣಾಙ್ಕ expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ಯದಿ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ಯದಿ (expWidthMin == 0) expWidthMin = expWidth;
        ಯದಿ (nodep->dtypep()->width() == expWidth) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

    ಶೂನ್ಯ fixWidthExtend(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, Ast_ಗಂಟು_DType* expDTypep, ExtendRule extendRule) {
        // Fix the width mismatch by extending or truncating bits
        // *ONLY* call this from checkWidth()
        // Truncation is rarer, but can occur:  parameter [3:0] FOO = 64'h12312;
        // A(CONSTwide)+B becomes  A(CONSTwidened)+B
        // A(somewide)+B  becomes  A(TRUNC(somewide,width))+B
        //                    or   A(EXTRACT(somewide,width,0))+B
        // Sign extension depends on the type of the *present*
        // node, while the output dtype is the *expected* sign.
        // It is reasonable to have sign extension with unsigned output,
        // for example $unsigned(a)+$signed(b), the SIGNED(B) will be unsigned dtype out
        UINFO(4, "  widthExtend_(r=" << static_cast<ಪೂರ್ಣಾಙ್ಕ>(extendRule) << ") old: " << nodep);
        ಯದಿ (extendRule == EXTEND_OFF) ಪ್ರತ್ಯಾಗಮ;
        AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expWidth = expDTypep->width();
        ಯದಿ (constp && !constp->ಅಂಕೆ().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ{nodep, expWidth, constp->ಅಂಕೆ()};
            ಅಂಕೆ.isSigned(ಅಸತ್ಯ);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ಅನ್ಯಥಾ ಯದಿ (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(ಸತ್ಯ);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ಅನ್ಯಥಾ {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            ತಾರ್ಕಿಕ doSigned = ಅಸತ್ಯ;
            ವಿಕಲ್ಪ (extendRule) {
            ಪಕ್ಷ EXTEND_ZERO: doSigned = ಅಸತ್ಯ; ಭಙ್ಗ;
            ಪಕ್ಷ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ಭಙ್ಗ;
            ಪಕ್ಷ EXTEND_LHS: doSigned = nodep->isSigned(); ಭಙ್ಗ;
            ಸ್ವಭಾವ: nodep->v3fatalSrc("bad case");
            }
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp
                = (doSigned ? static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*>(ನೂತನ AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(ಸತ್ಯ);
            linker.relink(newp);
            nodep = newp;
        }
        ಯದಿ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ಯದಿ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } ಅನ್ಯಥಾ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    ಶೂನ್ಯ fixWidthReduce(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expWidth = 1;
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expSigned = ಅಸತ್ಯ;
        ಯದಿ (nodep->width() == expWidth) ಪ್ರತ್ಯಾಗಮ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಯದಿ (constp) {
            V3_ಸಂಖ್ಯೆ ಅಂಕೆ(nodep, expWidth);
            ಅಂಕೆ.opRedOr(constp->ಅಂಕೆ());
            ಅಂಕೆ.isSigned(expSigned);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), ಅಂಕೆ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ಅನ್ಯಥಾ {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            Ast_ಗಂಟು_DType* ಧ್ರುವ dtypeSkipp = nodep->dtypep()->skipRefp();
            ಯದಿ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = ನೂತನ AstNeq{nodep->fileline(),
                                  ನೂತನ AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ಅನ್ಯಥಾ {
                // But, AstRedOr can be faster on wide
                newp = ನೂತನ AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    ತಾರ್ಕಿಕ fixAutoExtend(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*& nodepr, ಪೂರ್ಣಾಙ್ಕ expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ಯದಿ (AstConst* ಧ್ರುವ constp = VN_CAST(nodepr, Const)) {
            ಯದಿ (constp->ಅಂಕೆ().autoExtend() && !constp->ಅಂಕೆ().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ(constp, expWidth);
                ಅಂಕೆ.opRepl(constp->ಅಂಕೆ(), expWidth);  // {width{'1}}
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{constp->fileline(), ಅಂಕೆ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ಅನ್ಯಥಾ ಯದಿ (!constp->ಅಂಕೆ().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->ಅಂಕೆ().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_ಸಂಖ್ಯೆ ಅಂಕೆ(constp, expWidth);
                ಅಂಕೆ.opExtendXZ(constp->ಅಂಕೆ(), constp->width());
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{constp->fileline(), ಅಂಕೆ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ವೃಕ್ಷ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;  // No change
    }
    ತಾರ್ಕಿಕ isBaseClassRecurse(ಧ್ರುವ AstClass* ಧ್ರುವ cls1p, ಧ್ರುವ AstClass* ಧ್ರುವ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ಯದಿ (cls1p == cls2p) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ಯದಿ (isBaseClassRecurse(cls1p, cextp->classp())) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ಸ್ಥಿರ string warnDTypes(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ Ast_ಗಂಟು_DType* lhsDtp,
                             ಧ್ರುವ Ast_ಗಂಟು_DType* rhsDtp, ಧ್ರುವ string& lhsName = "Left-hand",
                             ಧ್ರುವ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ಯದಿ (lhsState == rhsState) lhsState = rhsState = "";
        ಪ್ರತ್ಯಾಗಮ "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ಸ್ಥಿರ ತಾರ್ಕಿಕ unpackedArraySizesMatch(ಧ್ರುವ Ast_ಗಂಟು_DType* lhsDtp, ಧ್ರುವ Ast_ಗಂಟು_DType* rhsDtp) {
        ಯಾವತ್ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ಯದಿ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    ಶೂನ್ಯ checkUnpackedArrayAssignmentCompatible(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ T* ಧ್ರುವ lhsRefp,
                                                ಧ್ರುವ N* ಧ್ರುವ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::ಮೌಲ್ಯ || std::is_same<T, Ast_ಗಂಟು_Var_Ref>::ಮೌಲ್ಯ)
                && (std::is_same<N, AstVar>::ಮೌಲ್ಯ || std::is_same<N, Ast_ಗಂಟು_Var_Ref>::ಮೌಲ್ಯ),
            "Unsupported types provided.");
        ಯದಿ (!lhsRefp || !rhsRefp) ಪ್ರತ್ಯಾಗಮ;
        std::string lhsName;
        std::string rhsName;
        ಯದಿ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ಅನ್ಯಥಾ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDtp = lhsRefp->dtypep()->skipRefp();
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsDtp = rhsRefp->dtypep()->skipRefp();
        ಧ್ರುವ ತಾರ್ಕಿಕ isLhsAggregate = lhsDtp->isAggregateType();
        ಧ್ರುವ ತಾರ್ಕಿಕ isRhsAggregate = rhsDtp->isAggregateType();
        ಯದಿ (!isLhsAggregate && !isRhsAggregate) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            ಪ್ರತ್ಯಾಗಮ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(ಅಸತ್ಯ),
                                      rhsDim = rhsDtp->dimensions(ಅಸತ್ಯ);
        // Check if unpacked array dimensions are matching
        ಯದಿ (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ಪ್ರತ್ಯಾಗಮ;
        }

        ಧ್ರುವ Ast_ಗಂಟು_DType* lhsDtpIterp = lhsDtp;
        ಧ್ರುವ Ast_ಗಂಟು_DType* rhsDtpIterp = rhsDtp;
        ಪ್ರತಿ (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            ಯದಿ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ಪ್ರತ್ಯಾಗಮ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ಯದಿ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    ಶೂನ್ಯ checkClassAssign(ಧ್ರುವ Ast_ಗಂಟು* nodep, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* rhsp,
                          Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDTypep, ತಾರ್ಕಿಕ isOutputArg = ಅಸತ್ಯ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ lhsRawDTypep = lhsDTypep->skipRefp();
        ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ಯದಿ (ಧ್ರುವ AstClassRefDType* ಧ್ರುವ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ಧ್ರುವ AstClass* ಧ್ರುವ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ಧ್ರುವ AstClass* ಧ್ರುವ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ಯದಿ (isBaseClassRecurse(dstp, srcp)) ಪ್ರತ್ಯಾಗಮ;
            } ಅನ್ಯಥಾ ಯದಿ (rhsp->isNull()) {
                ಪ್ರತ್ಯಾಗಮ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ಯದಿ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                ನೂತನ AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), lhsDTypep});
        }
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ similarDTypeRecurse(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ node1p,
                                    ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ node2p) {
        ಪ್ರತ್ಯಾಗಮ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    ಶೂನ್ಯ iterateCheckFileDesc(Ast_ಗಂಟು* parentp, Ast_ಗಂಟು* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckReal(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    ಶೂನ್ಯ iterateCheckSigned8(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckSigned32(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    ಶೂನ್ಯ iterateCheckUInt32(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    ಶೂನ್ಯ iterateCheckDelay(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ಯದಿ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ಯದಿ (stage & FINAL) {
            Ast_ಗಂಟು_DType* expDTypep;
            ಯದಿ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } ಅನ್ಯಥಾ {
                File_ಸಾಲು* ಧ್ರುವ newFl = ನೂತನ File_ಸಾಲು{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, ಸತ್ಯ);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, ಅಸತ್ಯ);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckString(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    ಶೂನ್ಯ iterateCheckTyped(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp,
                           Ast_ಗಂಟು_DType* expDTypep, Stage stage) {
        ಯದಿ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        ಯದಿ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckTypedSelfPrelim(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp,
                                     Ast_ಗಂಟು_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ಯದಿ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        ಯದಿ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckIntegralSelf(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, ಸತ್ಯ);
    }
    ಶೂನ್ಯ iterateCheckSelf(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Determ determ,
                          Stage stage, ತಾರ್ಕಿಕ integralOnly = ಅಸತ್ಯ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ಯದಿ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) ? checkCvtUS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), integralOnly) : underp;
        Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckSizedSelf(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Determ determ,
                               Stage stage, ತಾರ್ಕಿಕ deferUntyped = ಅಸತ್ಯ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ಯದಿ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ) ? checkCvtUS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), ಅಸತ್ಯ) : underp;
        Ast_ಗಂಟು_DType* ಧ್ರುವ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ಗಂಟು_DType* ಧ್ರುವ checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ಯದಿ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (ಶೂನ್ಯ)underp;  // cppcheck
    }
    ಶೂನ್ಯ iterateCheckAssign(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* rhsp, Stage stage,
                            Ast_ಗಂಟು_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ಯದಿ (AstConst* constp = VN_CAST(rhsp, Const)) {
            ಯದಿ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ಯದಿ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ಯದಿ (basicp->width() == 8 && constp->ಅಂಕೆ().isFromString()) {
                        AstConsArray* newp = ನೂತನ AstConsArray{
                            constp->fileline(), lhsDTypep,
                            ನೂತನ AstConst{constp->fileline(), AstConst::Widthed_ಮೌಲ್ಯ{}, 8, 0}};
                        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ aindex : arrayp->declRange().seqLoToHi()) {
                            ಪೂರ್ಣಾಙ್ಕ cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_ಸಂಖ್ಯೆ selected{constp, 8};
                            selected.opSel(constp->ಅಂಕೆ(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ಯದಿ (!selected.isFourState()) {
                                ಯದಿ (ಧ್ರುವ uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 ನೂತನ AstConst{constp->fileline(),
                                                              AstConst::Widthed_ಮೌಲ್ಯ{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ತಳ್ಳು_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        ಧ್ರುವ ತಾರ್ಕಿಕ lhsStream = (VN_IS(parentp, ಗಂಟು_Assign)
                                && VN_IS(VN_AS(parentp, ಗಂಟು_Assign)->lhsp(), ಗಂಟು_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, ಸತ್ಯ, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (ಶೂನ್ಯ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* convertUnpackedOrder(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* exprp, Ast_ಗಂಟು_DType* dtypep) {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ಯದಿ (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) ಪ್ರತ್ಯಾಗಮ exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ isLogicalOperator(ಧ್ರುವ Ast_ಗಂಟು* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ಪ್ರತ್ಯಾಗಮ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    ಶೂನ್ಯ iterateCheckBool(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_ಗಂಟು_DType* ಧ್ರುವ underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        ಯದಿ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstNeqD{parentp->fileline(), VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                              ನೂತನ AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ಅನ್ಯಥಾ ಯದಿ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_ಗಂಟು* ಧ್ರುವ newp
                = ನೂತನ AstNeqN{parentp->fileline(), VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ),
                              ನೂತನ AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ಅನ್ಯಥಾ ಯದಿ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)), underp);  // Changed
        } ಅನ್ಯಥಾ ಯದಿ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(ನೂತನ AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(underp), underp);
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ತಾರ್ಕಿಕ bad = widthBad(underp, parentp->findBitDType());
            ಯದಿ (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->width(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->width()
                                             << (underp->width() != underp->widthMin()
                                                     ? " or " + cvtToStr(underp->widthMin())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            ಯದಿ (Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ ಅಭಿವ್ಯಕ್ತಿ_Stmtp = VN_CAST(underp, ಅಭಿವ್ಯಕ್ತಿ_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp());
                ಅಭಿವ್ಯಕ್ತಿ_Stmtp->dtypeFrom(ಅಭಿವ್ಯಕ್ತಿ_Stmtp->resultp());
            } ಅನ್ಯಥಾ ಯದಿ (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)), underp);  // Changed
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ checkEnum(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* parentp, ಧ್ರುವ Ast_ಗಂಟು_DType* targetDTypep,
                          ಧ್ರುವ Ast_ಗಂಟು_DType* sourceDTypep, ಧ್ರುವ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* sourceExprp) {
        ಯದಿ (ಧ್ರುವ AstEnumDType* ಧ್ರುವ targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            ಧ್ರುವ VCastable castable
                = Ast_ಗಂಟು::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            ಯದಿ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << targetDTypep->prettyDTypeNameQ() << " from "
                                             << sourceDTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warnMore()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_ಗಂಟು* iterateCheck(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side, Ast_ಗಂಟು* underp, Determ determ,
                          Stage stage, Ast_ಗಂಟು_DType* expDTypep, ExtendRule extendRule,
                          ತಾರ್ಕಿಕ warnOn = ಸತ್ಯ, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ಧ್ರುವ StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        ಯದಿ (rhsAccess.isReadOnly()) {
            ಯದಿ (Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_CAST(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        ಯದಿ (VN_IS(underp, ಗಂಟು_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ಅನ್ಯಥಾ ಯದಿ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ಅನ್ಯಥಾ ಯದಿ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } ಅನ್ಯಥಾ ಯದಿ (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ಗಂಟು* ಧ್ರುವ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ಅನ್ಯಥಾ ಯದಿ (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ಗಂಟು* ಧ್ರುವ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), ಸತ್ಯ, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ಅನ್ಯಥಾ ಯದಿ (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ಗಂಟು* ಧ್ರುವ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ underDtp = underp->dtypep()->skipRefp();
            ಯದಿ (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ಗಂಟು* ಧ್ರುವ newp
                    = ನೂತನ AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(underp), underp);
                underp = newp;
            }
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ expDtp = expDTypep->skipRefp();
            ಯದಿ (determ == ASSIGN && VN_IS(expDtp, ಗಂಟು_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ಯದಿ (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ expBasicp = expDTypep->basicp();
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ underBasicp = underp->dtypep()->basicp();
            ಯದಿ (expBasicp && underBasicp) {
                ಯದಿ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    ಯದಿ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ));
                    }
                    ಯದಿ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_ಗಂಟು_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ಯದಿ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } ಅನ್ಯಥಾ ಯದಿ (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ಯದಿ (underp->isSigned() != subDTypep->isSigned()
                        || underp->width() != subDTypep->width()) {
                        subDTypep = parentp->findBitOrLogicDType(
                            std::max(subDTypep->width(), underp->width()),
                            std::max(subDTypep->widthMin(), underp->widthMin()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                } ಅನ್ಯಥಾ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ), expDTypep, extendRule,
                                warnOn);
            } ಅನ್ಯಥಾ ಯದಿ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ಧ್ರುವ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ಯದಿ (VN_IS(underp, Const) && VN_AS(underp, Const)->ಅಂಕೆ().isNull()) {
                    // '= null' is ok
                } ಅನ್ಯಥಾ ಯದಿ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ಅನ್ಯಥಾ ಯದಿ (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } ಅನ್ಯಥಾ ಯದಿ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ಅನ್ಯಥಾ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } ಅನ್ಯಥಾ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        ಪ್ರತ್ಯಾಗಮ underp;
    }

    ಶೂನ್ಯ
    widthCheckSized(Ast_ಗಂಟು* parentp, ಧ್ರುವ ವರ್ಣ* side,
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* underp,  // Node to be checked or have typecast added in front of
                    Ast_ಗಂಟು_DType* expDTypep, ExtendRule extendRule, ತಾರ್ಕಿಕ warnOn = ಸತ್ಯ) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ expBasicp = expDTypep->basicp();
        ಧ್ರುವ AstBasicDType* ಧ್ರುವ underBasicp = underp->dtypep()->basicp();
        ಯದಿ (expDTypep == underp->dtypep()) {
            ಪ್ರತ್ಯಾಗಮ;  // Same type must match
        } ಅನ್ಯಥಾ ಯದಿ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ಯದಿ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ expWidth = expDTypep->width();
            ಪೂರ್ಣಾಙ್ಕ expWidthMin = expDTypep->widthMin();
            ಯದಿ (expWidthMin == 0) expWidthMin = expWidth;
            ಧ್ರುವ ತಾರ್ಕಿಕ bad = widthBad(underp, expDTypep);
            ಯದಿ ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                ಪ್ರತ್ಯಾಗಮ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ಯದಿ (VN_IS(parentp->backp(), CastSize)) warnOn = ಅಸತ್ಯ;
            ಯದಿ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = ಅಸತ್ಯ;
            ಯದಿ (VN_IS(underp, Const) && VN_AS(underp, Const)->ಅಂಕೆ().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = ಅಸತ್ಯ;
            }
            ಯದಿ ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = ಅಸತ್ಯ;
            }
            ಯದಿ (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    expWidth, underp->width(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << expWidth
                        << (expWidth != expWidthMin ? " or " + cvtToStr(expWidthMin) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->width()
                        << (underp->width() != underp->widthMin()
                                ? " or " + cvtToStr(underp->widthMin())
                                : "")
                        << " bits.");
            }
            ಯದಿ (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ಧ್ರುವ Ast_ಗಂಟು_Assign* assignp = VN_CAST(parentp, ಗಂಟು_Assign);
                ಧ್ರುವ AstPin* pinp = VN_CAST(parentp, Pin);
                ಯದಿ (assignp && VN_IS(assignp->lhsp(), ಗಂಟು_Stream)) {
                } ಅನ್ಯಥಾ ಯದಿ (pinp && pinp->modVarp()->direction() != VDirection::ಒಳಹರಿವು) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ಅನ್ಯಥಾ ಯದಿ (assignp && VN_IS(underp, ಗಂಟು_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* ಧ್ರುವ widenedp = ನೂತನ AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(ಸತ್ಯ);
                    // Shift left so zeros fill on the right
                    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ shiftedp = ನೂತನ AstShiftL{
                        underp->fileline(), widenedp,
                        ನೂತನ AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ಅನ್ಯಥಾ {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* checkCvtUS(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ತಾರ್ಕಿಕ fatal) {
        ಯದಿ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ಯದಿ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ಅನ್ಯಥಾ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, ಅಸತ್ಯ, 32);
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtD(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ಯದಿ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
            ಯದಿ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = ನೂತನ AstISToRD{nodep->fileline(), nodep};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ಪ್ರತ್ಯಾಗಮ newp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nodep;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtS(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, ತಾರ್ಕಿಕ warnOn, ಪೂರ್ಣಾಙ್ಕ width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ಯದಿ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ಯದಿ (constp->isDouble() && V3_ಸಂಖ್ಯೆ::epsilonIntegral(constp->ಅಂಕೆ().toDouble())) {
                    warnOn = ಅಸತ್ಯ;
                }
            }
            ಯದಿ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            ಪ್ರತ್ಯಾಗಮ newp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nodep;
        }
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* spliceCvtString(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ಯದಿ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ಪ್ರತ್ಯಾಗಮ newp;
        } ಅನ್ಯಥಾ {
            ಪ್ರತ್ಯಾಗಮ nodep;
        }
    }
    Ast_ಗಂಟು_Biop* replaceWithUOrSVersion(Ast_ಗಂಟು_Biop* nodep, ತಾರ್ಕಿಕ signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಯದಿ (signedFlavorNeeded == nodep->signedFlavor()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ವಿಕಲ್ಪ (nodep->type()) {
        ಪಕ್ಷ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪಕ್ಷ VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); ಪ್ರತ್ಯಾಗಮ nullptr;
        ಸ್ವಭಾವ: ಭಙ್ಗ;
        }
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* newp = nullptr;
        ವಿಕಲ್ಪ (nodep->type()) {
        ಪಕ್ಷ VNType::Gt: newp = ನೂತನ AstGtS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::GtS: newp = ನೂತನ AstGt{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Gte: newp = ನೂತನ AstGteS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::GteS: newp = ನೂತನ AstGte{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lt: newp = ನೂತನ AstLtS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::LtS: newp = ನೂತನ AstLt{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lte: newp = ನೂತನ AstLteS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::LteS: newp = ನೂತನ AstLte{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Div: newp = ನೂತನ AstDivS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::DivS: newp = ನೂತನ AstDiv{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::ModDiv: newp = ನೂತನ AstModDivS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::ModDivS: newp = ನೂತನ AstModDiv{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Mul: newp = ನೂತನ AstMulS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::MulS: newp = ನೂತನ AstMul{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::ShiftR: newp = ನೂತನ AstShiftRS{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::ShiftRS: newp = ನೂತನ AstShiftR{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಸ್ವಭಾವ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ಭಙ್ಗ;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_Biop* replaceWithDVersion(Ast_ಗಂಟು_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಯದಿ (nodep->doubleFlavor()) ಪ್ರತ್ಯಾಗಮ nullptr;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ವಿಕಲ್ಪ (nodep->type()) {
        ಪಕ್ಷ VNType::Add: newp = ನೂತನ AstAddD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Sub: newp = ನೂತನ AstSubD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Pow: newp = ನೂತನ AstPowD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Eq:
        ಪಕ್ಷ VNType::EqCase: newp = ನೂತನ AstEqD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Neq:
        ಪಕ್ಷ VNType::NeqCase: newp = ನೂತನ AstNeqD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Gt:
        ಪಕ್ಷ VNType::GtS: newp = ನೂತನ AstGtD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Gte:
        ಪಕ್ಷ VNType::GteS: newp = ನೂತನ AstGteD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lt:
        ಪಕ್ಷ VNType::LtS: newp = ನೂತನ AstLtD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lte:
        ಪಕ್ಷ VNType::LteS: newp = ನೂತನ AstLteD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Div:
        ಪಕ್ಷ VNType::DivS: newp = ನೂತನ AstDivD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Mul:
        ಪಕ್ಷ VNType::MulS: newp = ನೂತನ AstMulD{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಸ್ವಭಾವ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ಭಙ್ಗ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_Biop* replaceWithNVersion(Ast_ಗಂಟು_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ಯದಿ (nodep->stringFlavor()) ಪ್ರತ್ಯಾಗಮ nullptr;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        ವಿಕಲ್ಪ (nodep->type()) {
        ಪಕ್ಷ VNType::Eq:
        ಪಕ್ಷ VNType::EqCase:
        ಪಕ್ಷ VNType::EqWild: newp = ನೂತನ AstEqN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Neq:
        ಪಕ್ಷ VNType::NeqCase:
        ಪಕ್ಷ VNType::NeqWild: newp = ನೂತನ AstNeqN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Gt:
        ಪಕ್ಷ VNType::GtS: newp = ನೂತನ AstGtN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Gte:
        ಪಕ್ಷ VNType::GteS: newp = ನೂತನ AstGteN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lt:
        ಪಕ್ಷ VNType::LtS: newp = ನೂತನ AstLtN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಪಕ್ಷ VNType::Lte:
        ಪಕ್ಷ VNType::LteS: newp = ನೂತನ AstLteN{fl, lhsp, rhsp}; ಭಙ್ಗ;
        ಸ್ವಭಾವ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ಭಙ್ಗ;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_Uniop* replaceWithDVersion(Ast_ಗಂಟು_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ಯದಿ (nodep->doubleFlavor()) ಪ್ರತ್ಯಾಗಮ nullptr;
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_ಗಂಟು_Uniop* newp = nullptr;
        ವಿಕಲ್ಪ (nodep->type()) {
        ಪಕ್ಷ VNType::Negate: newp = ನೂತನ AstNegateD{fl, lhsp}; ಭಙ್ಗ;
        ಸ್ವಭಾವ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ಭಙ್ಗ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
        ಪ್ರತ್ಯಾಗಮ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    ಶೂನ್ಯ replaceWithSFormat(AstMethodCall* nodep, ಧ್ರುವ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ಧ್ರುವ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_ಗಂಟು* ಧ್ರುವ newp = ನೂತನ AstAssign{
            nodep->fileline(), fromp,
            ನೂತನ AstSFormatF{nodep->fileline(), format, ಅಸತ್ಯ, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        ತಳ್ಳು_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ಗಂಟು_DType* iterateEditMoveDTypep(Ast_ಗಂಟು* parentp, Ast_ಗಂಟು_DType* dtnodep) {
        UASSERT_OBJ(dtnodep, parentp, "Caller should check for nullptr before computing dtype");
        // Iterate into a data type to resolve that type.
        // The data type may either:
        // 1. Be a child (typically getChildDTypep() returns it)
        //    DTypes at parse time get added as these to some node types
        //    such as AstVars.
        //    This function will move it to global scope (that is #2
        //    will now apply).
        // 2. Be under the Netlist and pointed to by an Ast member variable
        //    (typically refDTypep() or dtypep() returns it)
        //    so removing/changing a variable won't lose the dtype

        // Case #1 above applies?
        ಧ್ರುವ ತಾರ್ಕಿಕ child1 = (parentp->getChildDTypep() == dtnodep);
        ಧ್ರುವ ತಾರ್ಕಿಕ child2 = (parentp->getChild2DTypep() == dtnodep);
        ಯದಿ (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, nullptr), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->getChildDTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlinkFrBack();
            v3Global.rootp()->typeTablep()->addTypesp(dtnodep);
        }
        ಯದಿ (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ಪ್ರತ್ಯಾಗಮ dtnodep;
    }

    AstConst* dimension_ಮೌಲ್ಯ(File_ಸಾಲು* fileline, Ast_ಗಂಟು_DType* nodep, VAttrType attrType,
                             ಪೂರ್ಣಾಙ್ಕ dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ಗಂಟು_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                declRange = adtypep->declRange();
                ಯದಿ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ಅನುವರ್ತ;
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ adtypep
                       = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ಭಙ್ಗ;  // Sub elements don't look like arrays and can't iterate into
            } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, BasicDType)) {
                ಯದಿ (adtypep->isRanged()) declRange = adtypep->declRange();
                ಭಙ್ಗ;
            }
            ಭಙ್ಗ;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        ಪೂರ್ಣಾಙ್ಕ val = 0;
        ವಿಕಲ್ಪ (attrType) {
        ಪಕ್ಷ VAttrType::DIM_BITS: {
            ಪೂರ್ಣಾಙ್ಕ bits = 1;
            ಯಾವತ್ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ adtypep = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ಅನುವರ್ತ;
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ adtypep
                           = VN_CAST(dtypep, ಗಂಟು_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    ಭಙ್ಗ;
                } ಅನ್ಯಥಾ ಯದಿ (ಧ್ರುವ AstBasicDType* ಧ್ರುವ adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    ಭಙ್ಗ;
                }
                ಭಙ್ಗ;
            }
            ಯದಿ (dim == 0) {
                val = 0;
            } ಅನ್ಯಥಾ ಯದಿ (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } ಅನ್ಯಥಾ {
                val = bits;
            }
            ಭಙ್ಗ;  // LCOV_EXCL_LINE
        }
        ಪಕ್ಷ VAttrType::DIM_ಮೇಲಿನ: val = !declRange.ranged() ? 0 : declRange.hi(); ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::DIM_ಎಡ: val = !declRange.ranged() ? 0 : declRange.ಎಡ(); ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::DIM_ಕೆಳಗಿನ: val = !declRange.ranged() ? 0 : declRange.lo(); ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::DIM_ಬಲ: val = !declRange.ranged() ? 0 : declRange.ಬಲ(); ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ಭಙ್ಗ;
        ಪಕ್ಷ VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); ಭಙ್ಗ;
        ಸ್ವಭಾವ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ಭಙ್ಗ;
        }
        ಯದಿ (!valp) valp = ನೂತನ AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        ಪ್ರತ್ಯಾಗಮ valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ಗಂಟು_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ಯದಿ (cachedpr) ಪ್ರತ್ಯಾಗಮ cachedpr->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

        Ast_ಗಂಟು_Array_DType* ಧ್ರುವ vardtypep = ನೂತನ AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), ನೂತನ AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{nodep->fileline(), vardtypep};
        ಪ್ರತಿ (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_ಮೌಲ್ಯ(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_ವೃಕ್ಷ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(ಸತ್ಯ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(ಸತ್ಯ);
        ತಳ್ಳು_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ಪ್ರತ್ಯಾಗಮ cachedpr->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }
    ಸ್ಥಿರ uint64_t enum_Max_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು* errNodep, ಧ್ರುವ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ಧ್ರುವ AstConst* ಧ್ರುವ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ಯದಿ (!vconstp->ಅಂಕೆ().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        ಯದಿ (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ಪ್ರತ್ಯಾಗಮ 64;
        }
        ಪ್ರತ್ಯಾಗಮ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ಧ್ರುವ nodep, VAttrType attrType, ತಾರ್ಕಿಕ assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        ಯದಿ (cachedpr) ಪ್ರತ್ಯಾಗಮ cachedpr->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        Ast_ಗಂಟು_DType* basep;
        ಯದಿ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } ಅನ್ಯಥಾ {
            basep = nodep->dtypep();
        }
        Ast_ಗಂಟು_DType* vardtypep;
        ಯದಿ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = ನೂತನ AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ಅನ್ಯಥಾ {
            vardtypep = ನೂತನ AstUnpackArrayDType{flp, basep, ನೂತನ AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        ಯದಿ (attrType == VAttrType::ENUM_NAME) {
            defaultp = ನೂತನ AstConst{flp, AstConst::String{}, ""};
        } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = ನೂತನ AstConst{flp, V3_ಸಂಖ್ಯೆ{nodep, nodep->width(), 0}};
        } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_VALID) {
            defaultp = ನೂತನ AstConst{flp, AstConst::BitFalse{}};
        } ಅನ್ಯಥಾ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ*> ಮೌಲ್ಯಗಳು;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ಧ್ರುವ firstp = nodep->itemsp();
            ಧ್ರುವ AstEnumItem* prevp = firstp;  // Prev must start with last item
            ಯಾವತ್ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            ಪ್ರತಿ (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ಧ್ರುವ nextp = VN_AS(itemp->nextp(), EnumItem);
                ಧ್ರುವ AstConst* ಧ್ರುವ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ಯದಿ (!vconstp->ಅಂಕೆ().isAnyXZ()) {  // Can 2-state runtime decode
                    ಧ್ರುವ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    ಯದಿ (attrType == VAttrType::ENUM_NAME) {
                        ಮೌಲ್ಯಗಳು[i] = ನೂತನ AstConst{flp, AstConst::String{},
                                                 V3_ಸಂಖ್ಯೆ::displayedEnumName(itemp)};
                    } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_NEXT) {
                        ಮೌಲ್ಯಗಳು[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);  // A const
                    } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_PREV) {
                        ಮೌಲ್ಯಗಳು[i] = prevp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);  // A const
                    } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_VALID) {
                        ಮೌಲ್ಯಗಳು[i] = ನೂತನ AstConst{flp, AstConst::BitTrue{}};
                    } ಅನ್ಯಥಾ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valuep = nullptr;
        ಯದಿ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ಧ್ರುವ consp = ನೂತನ AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            ಪ್ರತಿ (ಧ್ರುವ auto& itr : ಮೌಲ್ಯಗಳು) {
                V3_ಸಂಖ್ಯೆ ಕೀಲಿ{nodep, nodep->width()};
                ಕೀಲಿ.setQuad(itr.first);
                consp->addItemsp(ನೂತನ AstConsAssocItem{flp, ನೂತನ AstConst{flp, ಕೀಲಿ}, itr.second});
            }
            valuep = consp;
        } ಅನ್ಯಥಾ {
            AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{flp, vardtypep, defaultp};
            ಪ್ರತಿ (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ಯದಿ (ಮೌಲ್ಯಗಳು[i]) consp->valuep(i, ಮೌಲ್ಯಗಳು[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(ಸತ್ಯ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(ಸತ್ಯ);
        ತಳ್ಳು_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ಪ್ರತ್ಯಾಗಮ cachedpr->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }

    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumSelect(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ಯದಿ (attrType == VAttrType::ENUM_NAME) {
            ಯದಿ (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->ಅಂಕೆ().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
                ಪ್ರತ್ಯಾಗಮ newp;
            }
        }
        ಯದಿ (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_ಅಭಿವ್ಯಕ್ತಿ_Stmt* ಧ್ರುವ capturep = newEnumCapture(nodep, adtypep);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            ಪ್ರತ್ಯಾಗಮ capturep;
        }
        ಯದಿ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* indexp = ನೂತನ AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            ಪ್ರತಿ (ಧ್ರುವ AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ಧ್ರುವ AstConst* ಧ್ರುವ constp = VN_AS(itemp->valuep(), Const);
                ಯದಿ (constp->ಅಂಕೆ().isAnyXZ()) ಅನುವರ್ತ;
                indexp = ನೂತನ AstCond{nodep->fileline(),
                                     ನೂತನ AstEq{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                               itemp->valuep()->clone_ವೃಕ್ಷ(ಅಸತ್ಯ)},
                                     ನೂತನ AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ಧ್ರುವ tabRefp = enumVarRefp(adtypep, attrType, ಅಸತ್ಯ, ordinal);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ newp = ನೂತನ AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
            ಪ್ರತ್ಯಾಗಮ newp;
        }
        ಧ್ರುವ uint64_t msbdim = enum_Max_ಮೌಲ್ಯ(nodep, adtypep);
        ಧ್ರುವ ತಾರ್ಕಿಕ assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newp;
        ಯದಿ (assoc) {
            AstVarRef* ಧ್ರುವ tabRefp = enumVarRefp(adtypep, attrType, ಸತ್ಯ, 0);
            newp = ನೂತನ AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ಧ್ರುವ tabRefp
                = enumVarRefp(adtypep, attrType, ಅಸತ್ಯ, (1ULL << selwidth) - 1);
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? ನೂತನ AstGt{nodep->fileline(), nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ),
                                  ನೂತನ AstConst{nodep->fileline(),
                                               V3_ಸಂಖ್ಯೆ{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = ನೂತನ AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                ನೂತನ AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ಯದಿ (boundp) {
                newp = ನೂತನ AstCond{nodep->fileline(), boundp,
                                   ನೂತನ AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ಯದಿ (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } ಅನ್ಯಥಾ ಯದಿ (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } ಅನ್ಯಥಾ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ಪ್ರತ್ಯಾಗಮ newp;
    }
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* enumTestValid(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* valp, AstEnumDType* enumDtp) {
        ಧ್ರುವ uint64_t maxval = enum_Max_ಮೌಲ್ಯ(valp, enumDtp);
        ಧ್ರುವ ತಾರ್ಕಿಕ assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* testp = nullptr;
        File_ಸಾಲು* ಧ್ರುವ fl_novalue = ನೂತನ File_ಸಾಲು{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, ಸತ್ಯ);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, ಸತ್ಯ);
        ಯದಿ (assoc) {
            AstVarRef* ಧ್ರುವ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ಸತ್ಯ, 0);
            testp = ನೂತನ AstAssocSel{fl_novalue, tabRefp, valp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ)};
        } ಅನ್ಯಥಾ {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ selwidth = V3_ಸಂಖ್ಯೆ::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ಧ್ರುವ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ಅಸತ್ಯ, (1ULL << selwidth) - 1);
            File_ಸಾಲು* ಧ್ರುವ fl_nowidth = ನೂತನ File_ಸಾಲು{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, ಸತ್ಯ);
            testp = ನೂತನ AstCond{
                fl_novalue,
                ನೂತನ AstGt{fl_nowidth, valp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ),
                          ನೂತನ AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                ನೂತನ AstConst{fl_novalue, AstConst::BitFalse{}},
                ನೂತನ AstArraySel{fl_novalue, tabRefp,
                                ನೂತನ AstSel{fl_novalue, valp->clone_ವೃಕ್ಷ_Pure(ಅಸತ್ಯ), 0, selwidth}}};
        }
        ಪ್ರತ್ಯಾಗಮ testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, ಧ್ರುವ VNumRange& range) {
        PatVecMap patmap;
        ಪೂರ್ಣಾಙ್ಕ element = range.ಎಡ();
        ಪ್ರತಿ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ಯದಿ (patp->keyp()) {
                ಯದಿ (ಧ್ರುವ AstConst* ಧ್ರುವ constp = patternConstKeyp(patp)) {
                    ಯದಿ (constp->ಅಂಕೆ().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ಅನ್ಯಥಾ {
                        element = constp->toSInt();
                    }
                }
            }
            ಧ್ರುವ ತಾರ್ಕಿಕ newEntry = patmap.emplace(element, patp).second;
            ಯದಿ (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ಎಡ_To_ಬಲ_Inc();
        }
        ಪ್ರತ್ಯಾಗಮ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    ಶೂನ್ಯ patConcatConvertRecurse(AstPattern* patternp, Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* nodep) {
        ಯದಿ (AstConcat* ಧ್ರುವ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ಅನ್ಯಥಾ {
            AstPatMember* ಧ್ರುವ newp
                = ನೂತನ AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(ಸತ್ಯ);
            patternp->addItemsp(newp);
        }
    }

    ಶೂನ್ಯ makeOpenArrayShell(Ast_ಗಂಟು_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ಗಂಟು_FTask* ಧ್ರುವ oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ಗಂಟು_FTask* ಧ್ರುವ newTaskp = oldTaskp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        newTaskp->dpiOpenChild(ಸತ್ಯ);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ಧ್ರುವ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ಪ್ರತಿ (ಧ್ರುವ auto& tconnect : tconnects) {
            AstVar* ಧ್ರುವ portp = tconnect.first;
            ಧ್ರುವ AstArg* ಧ್ರುವ argp = tconnect.second;
            ಧ್ರುವ Ast_ಗಂಟು* ಧ್ರುವ pinp = argp->exprp();
            ಯದಿ (!pinp) ಅನುವರ್ತ;  // Argument error we'll find later
            ಯದಿ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ಗಂಟು* nodep) {
                    ಯದಿ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    ತಾರ್ಕಿಕ markHasOpenArray(Ast_ಗಂಟು_FTask* nodep) {
        ತಾರ್ಕಿಕ hasOpen = ಅಸತ್ಯ;
        ಪ್ರತಿ (Ast_ಗಂಟು* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (AstVar* ಧ್ರುವ portp = VN_CAST(stmtp, Var)) {
                ಯದಿ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(ಸತ್ಯ);
                    hasOpen = ಸತ್ಯ;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ hasOpen;
    }
    ತಾರ್ಕಿಕ hasOpenArrayDTypeRecurse(Ast_ಗಂಟು_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        ಯದಿ (VN_IS(nodep, UnsizedArrayDType)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (nodep->subDTypep()) ಪ್ರತ್ಯಾಗಮ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    Ast_ಗಂಟು_DType* dtypeNotIntAtomOrVecRecurse(Ast_ಗಂಟು_DType* nodep, ತಾರ್ಕಿಕ ranged = ಅಸತ್ಯ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        ಯದಿ (AstBasicDType* ಧ್ರುವ dtp = VN_CAST(nodep, BasicDType)) {
            ಯದಿ (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                ಪ್ರತ್ಯಾಗಮ dtp;  // Packed when already packed
            ಯದಿ (dtp->keyword().isIntNumeric()) ಪ್ರತ್ಯಾಗಮ nullptr;
            ಪ್ರತ್ಯಾಗಮ dtp;
        } ಅನ್ಯಥಾ ಯದಿ (AstPackArrayDType* ಧ್ರುವ dtp = VN_CAST(nodep, PackArrayDType)) {
            ಯದಿ (ranged) ಪ್ರತ್ಯಾಗಮ dtp;  // Packed when already packed
            ಪ್ರತ್ಯಾಗಮ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), ಸತ್ಯ);
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    Ast_ಗಂಟು_DType* dtypeNot4StateIntegralRecurse(Ast_ಗಂಟು_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ಯದಿ (AstBasicDType* ಧ್ರುವ dtp = VN_CAST(nodep, BasicDType)) {
            ಯದಿ (!dtp->keyword().isFourstate()) ಪ್ರತ್ಯಾಗಮ dtp;
            ಪ್ರತ್ಯಾಗಮ nullptr;
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Array_DType* ಧ್ರುವ dtp = VN_CAST(nodep, ಗಂಟು_Array_DType)) {
            ಪ್ರತ್ಯಾಗಮ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_UOr_Struct_DType* ಧ್ರುವ dtp = VN_CAST(nodep, ಗಂಟು_UOr_Struct_DType)) {
            ಪ್ರತಿ (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ಗಂಟು_DType* ಧ್ರುವ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ಯದಿ (badDtp) ಪ್ರತ್ಯಾಗಮ badDtp;
            }
            ಪ್ರತ್ಯಾಗಮ nullptr;
        }
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಶೂನ್ಯ checkForceReleaseLhs(Ast_ಗಂಟು* nodep, Ast_ಗಂಟು* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    ಶೂನ್ಯ assert_At_ಅಭಿವ್ಯಕ್ತಿ(Ast_ಗಂಟು* nodep) {
        ಯದಿ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    ಶೂನ್ಯ assertAtStatement(Ast_ಗಂಟು* nodep) {
        ಯದಿ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    ಶೂನ್ಯ checkConstantOrReplace(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ noFourState, ಧ್ರುವ string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ಧ್ರುವ constp = VN_CAST(nodep, Const);
        ಯದಿ (!constp || (noFourState && constp->ಅಂಕೆ().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(ನೂತನ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ತಳ್ಳು_Deletep(nodep), nodep, constp);
            ಪ್ರತ್ಯಾಗಮ;
        }
    }
    Ast_ಗಂಟು* ಗಂಟು_For_Unsized_Warning(Ast_ಗಂಟು* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ಯದಿ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            ಪ್ರತ್ಯಾಗಮ nodep->op1p();
        } ಅನ್ಯಥಾ ಯದಿ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            ಪ್ರತ್ಯಾಗಮ nodep->op2p();
        }
        ಪ್ರತ್ಯಾಗಮ nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_ಗಂಟು* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ಯದಿ (VL_LIKELY(!nodep)) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಯದಿ (auto* ಧ್ರುವ refp = VN_CAST(nodep, RefDType)) {
            ಯದಿ (refp->typedefp() == typedefp) ಪ್ರತ್ಯಾಗಮ refp;
        }
        ಯದಿ (auto* ಧ್ರುವ refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) ಪ್ರತ್ಯಾಗಮ refp;
        ಯದಿ (auto* ಧ್ರುವ refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) ಪ್ರತ್ಯಾಗಮ refp;
        ಯದಿ (auto* ಧ್ರುವ refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) ಪ್ರತ್ಯಾಗಮ refp;
        ಯದಿ (auto* ಧ್ರುವ refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) ಪ್ರತ್ಯಾಗಮ refp;
        ಪ್ರತ್ಯಾಗಮ nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ಸ್ಥಿರ ತಾರ್ಕಿಕ isIfaceArrayDType(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        ಯದಿ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಧ್ರುವ AstIfaceRefDType* ಧ್ರುವ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ಪ್ರತ್ಯಾಗಮ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ಸ್ಥಿರ std::string unpackedArraySize(ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        ಯಾವತ್ (ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ಯದಿ (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ಪ್ರತ್ಯಾಗಮ size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ಸ್ಥಿರ Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* newIfaceArrayElement(Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* fromp, ಪೂರ್ಣಾಙ್ಕ bit) {
        AstConsArray* ಧ್ರುವ consp = VN_CAST(fromp, ConsArray);
        ಯದಿ (!consp || !isIfaceArrayDType(consp->dtypep())) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ಯದಿ (bit < 0 || bit >= arrp->elementsConst()) ಪ್ರತ್ಯಾಗಮ nullptr;
        ಪ್ರತ್ಯಾಗಮ consp->valuep(bit)->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* new_Iface_Array_ಮೌಲ್ಯ(ಧ್ರುವ Ast_ಗಂಟು_Var_Ref* refp, Ast_ಗಂಟು_DType* dtypep,
                                    ಧ್ರುವ std::string& name, ಧ್ರುವ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_ಸಾಲು* ಧ್ರುವ flp = refp->fileline();
        AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ಯದಿ (!arrp) {
            AstVar* ಧ್ರುವ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ಗಂಟು_Var_Ref* newp;
            ಯದಿ (ಧ್ರುವ AstVarXRef* ಧ್ರುವ xrefp = VN_CAST(refp, VarXRef)) {
                newp = ನೂತನ AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ಅನ್ಯಥಾ {
                newp = ನೂತನ AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            ಪ್ರತ್ಯಾಗಮ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ಧ್ರುವ consp = ನೂತನ AstConsArray{flp, arrp};
        ಧ್ರುವ VNumRange range = arrp->declRange();
        ಪ್ರತಿ (ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i : range.seq_ಎಡ_To_ಬಲ()) {
            ಧ್ರುವ std::string s = name + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_ಮೌಲ್ಯ(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ಪ್ರತ್ಯಾಗಮ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    ಶೂನ್ಯ replaceSplitIfaceArrayRef(Ast_ಗಂಟು_Var_Ref* nodep) {
        AstVar* ಧ್ರುವ varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ಯದಿ (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_ಮೌಲ್ಯ(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    ಶೂನ್ಯ expandIfaceArrayPinDimensions(AstPin* nodep, ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep,
                                       Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* conp, ಧ್ರುವ std::string& suffix,
                                       AstVar*& elemVarpr) {
        ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ಯದಿ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ಧ್ರುವ newp = nodep->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ಯದಿ (conp) newp->exprp(conp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            nodep->addNextHere(newp);
            ಪ್ರತ್ಯಾಗಮ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ಧ್ರುವ VNumRange range = arrp->declRange();
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ n = range.elements() - 1; n >= 0; --n) {
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ i = range.ಎಡ() + n * range.ಎಡ_To_ಬಲ_Inc();
            ಧ್ರುವ std::string s = suffix + "__BRA__" + Ast_ಗಂಟು::encode_ಸಂಖ್ಯೆ(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ elemConnp = [&]() -> Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* {
                ಯದಿ (!conp) ಪ್ರತ್ಯಾಗಮ nullptr;
                ಧ್ರುವ AstUnpackArrayDType* ಧ್ರುವ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ಪ್ರತ್ಯಾಗಮ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    ಶೂನ್ಯ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ಧ್ರುವ portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        ಪ್ರತಿ (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ exprp = VN_AS(nodep->exprp(), ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ);
        ಯದಿ (exprp) ತಳ್ಳು_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ಧ್ರುವ ತಾರ್ಕಿಕ connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ತಳ್ಳು_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ಗಂಟು* userIterateSubtreeReturnEdits(Ast_ಗಂಟು* nodep, WidthVP* vup) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ nullptr;
        Ast_ಗಂಟು* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ಪ್ರತ್ಯಾಗಮ ret;
    }
    ಶೂನ್ಯ userIterate(Ast_ಗಂಟು* nodep, WidthVP* vup) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    ಶೂನ್ಯ userIterateAndNext(Ast_ಗಂಟು* nodep, WidthVP* vup) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (nodep->didWidth()) ಪ್ರತ್ಯಾಗಮ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    ಶೂನ್ಯ userIterateChildren(Ast_ಗಂಟು* nodep, WidthVP* vup) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    ಶೂನ್ಯ userIterateChildrenBackwardsConst(Ast_ಗಂಟು* nodep, WidthVP* vup) {
        ಯದಿ (!nodep) ಪ್ರತ್ಯಾಗಮ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    WidthVisitor(ತಾರ್ಕಿಕ paramsOnly,  // [in] TRUE if we are considering parameters only.
                 ತಾರ್ಕಿಕ doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ಗಂಟು* mainAcceptEdit(Ast_ಗಂಟು* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        ಪ್ರತಿ (AstVar* ಧ್ರುವ varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(ತಳ್ಳು_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        ಪ್ರತ್ಯಾಗಮ nodep;
    }
    ಸ್ಥಿರ ತಾರ್ಕಿಕ lowerAsFixedAggregate(ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ dtypep) {
        ಪ್ರತ್ಯಾಗಮ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    ಶೂನ್ಯ streamAssignLower(Ast_ಗಂಟು_Assign* nodep) {
        Ast_ಗಂಟು_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ಯದಿ (Ast_ಗಂಟು_Stream* ಧ್ರುವ streamp = VN_CAST(nodep->rhsp(), ಗಂಟು_Stream)) {
            Ast_ಗಂಟು_DType* ಧ್ರುವ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lwidth = lhsDTypeSkippedRefp->widthStream();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            ಯದಿ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ಯದಿ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            ಯದಿ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    ನೂತನ AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ಯದಿ (Ast_ಗಂಟು_Stream* ಧ್ರುವ streamp = VN_CAST(nodep->lhsp(), ಗಂಟು_Stream)) {
            ಧ್ರುವ Ast_ಗಂಟು_DType* ಧ್ರುವ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ಗಂಟು_DType* ಧ್ರುವ lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lwidth = lhsStreamDTypep->widthStream();
            ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ rwidth = rhsDTypep->widthStream();
            ಯದಿ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ಯದಿ (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ dstp = streamp->lhsp()->unlinkFrBack();
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* srcp = nodep->rhsp()->unlinkFrBack();
                ಯದಿ (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } ಅನ್ಯಥಾ {
                    ಯದಿ (srcp->width() > lwidth) {
                        srcp = ನೂತನ AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ತಳ್ಳು_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ನೂತನ AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } ಅನ್ಯಥಾ ಯದಿ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ಗಂಟು_ಅಭಿವ್ಯಕ್ತಿ* ಧ್ರುವ rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(ನೂತನ AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = ಸ್ವಭಾವ;
};

//######################################################################
// Width class functions

ಶೂನ್ಯ V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ಧ್ರುವ WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ಧ್ರುವ VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{ಅಸತ್ಯ, ಅಸತ್ಯ};
        (ಶೂನ್ಯ)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (ಶೂನ್ಯ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ವೃಕ್ಷ("width", 0, dump_ವೃಕ್ಷ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ಗಂಟು* V3Width::widthParamsEdit(Ast_ಗಂಟು* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{ಸತ್ಯ, ಅಸತ್ಯ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ಪ್ರತ್ಯಾಗಮ nodep;
}

ಶೂನ್ಯ V3Width::streamAssignLowerEdit(Ast_ಗಂಟು_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{ಅಸತ್ಯ, ಅಸತ್ಯ};
    visitor.streamAssignLower(nodep);
}

//! Single node parameter propagation for generate blocks.
//! Smaller step... Only do a single node for parameter propagation
//! If we are inside a generated "if", "case" or "for", we don't want to
//! trigger warnings when we deal with the width. It is possible that
//! these are spurious, existing within sub-expressions that will not
//! actually be generated. Since such occurrences, must be constant, in
//! order to be something a generate block can depend on, we can wait until
//! later to do the width check.
//! @return  Pointer to the edited node.
Ast_ಗಂಟು* V3Width::widthGenerateParamsEdit(
    Ast_ಗಂಟು* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{ಸತ್ಯ, ಸತ್ಯ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ಪ್ರತ್ಯಾಗಮ nodep;
}

Ast_ಗಂಟು* V3Width::selectNonConstantRecurse(Ast_ಗಂಟು* nodep, ತಾರ್ಕಿಕ inSel) {
    // If node has a non-constant select, return that select
    Ast_ಗಂಟು* resultp = nullptr;
    ಯದಿ (Ast_ಗಂಟು_Sel* ಧ್ರುವ anodep = VN_CAST(nodep, ಗಂಟು_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), ಸತ್ಯ);
    } ಅನ್ಯಥಾ ಯದಿ (AstSel* ಧ್ರುವ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), ಸತ್ಯ);
    } ಅನ್ಯಥಾ ಯದಿ (Ast_ಗಂಟು_Var_Ref* ಧ್ರುವ anodep = VN_CAST(nodep, ಗಂಟು_Var_Ref)) {
        ಯದಿ (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ಪ್ರತ್ಯಾಗಮ anodep;
    } ಅನ್ಯಥಾ {
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ resultp;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ resultp;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        ಯದಿ (resultp) ಪ್ರತ್ಯಾಗಮ resultp;
        ಯದಿ (Ast_ಗಂಟು* ಧ್ರುವ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    ಪ್ರತ್ಯಾಗಮ resultp;
}
