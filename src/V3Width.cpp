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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Width.h"

#അന്തര്ഭാവ "V3Assert.h"
#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Begin.h"
#അന്തര്ഭാവ "V3Const.h"
#അന്തര്ഭാവ "V3ConstPool.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3Global.h"
#അന്തര്ഭാവ "V3LinkLValue.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3Number.h"
#അന്തര്ഭാവ "V3Randomize.h"
#അന്തര്ഭാവ "V3String.h"
#അന്തര്ഭാവ "V3Task.h"
#അന്തര്ഭാവ "V3UniqueNames.h"
#അന്തര്ഭാവ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#define VERILATOR_V3WIDTH_CPP_
#അന്തര്ഭാവ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

വർഗ്ഗം ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<സ്ഥിരം Ast_കണ്ണി*, AstClass*> m_cache;

    AstClass* findCached(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        സ്ഥിരം auto it = m_cache.find(nodep);
        എങ്കിൽ (it != m_cache.end()) തിരികെ it->second;

        AstClass* classp = nullptr;
        എങ്കിൽ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClass* സ്ഥിരം parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassPackage* സ്ഥിരം packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } അല്ലെങ്കിൽ {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        തിരികെ classp;
    }

പൊതു:
    AstClass* find(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) തിരികെ classp;
        എങ്കിൽ (AstClassPackage* സ്ഥിരം packagep = VN_CAST(nodep, ClassPackage)) {
            തിരികെ packagep->classp();
        }
        തിരികെ findCached(nodep);
    }
};

//######################################################################

എണ്ണൽ Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& operator<<(std::ostream& str, സ്ഥിരം Stage& rhs) {
    തിരികെ str << ("-PFB"[static_cast<പൂർണ്ണസംഖ്യ>(rhs)]);
}

എണ്ണൽ Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ASSIGN  // Assignment-like where sign comes from RHS only
};
std::ostream& operator<<(std::ostream& str, സ്ഥിരം Determ& rhs) {
    സ്ഥിതം constexpr സ്ഥിരം അക്ഷരം* സ്ഥിരം s_det[] = {"SELF", "CNTX", "ASSN"};
    തിരികെ str << s_det[rhs];
}

എണ്ണൽ StreamUse : uint8_t {
    STREAM_USE_NONE,  // Stream concatenation is not allowed in this context
    STREAM_USE_ASSIGN,  // Assignment-like context permits stream concatenation
    STREAM_USE_CAST,  // Explicit cast context permits stream concatenation
};

#define v3widthWarn(lhs, rhs, സന്ദേശം) \
    v3warnCode(((lhs) < (rhs)   ? V3ErrorCode::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3ErrorCode::WIDTHEXPAND \
                                : V3ErrorCode::WIDTH), \
               സന്ദേശം)

//######################################################################
// Width state, as a visitor of each AstNode

വർഗ്ഗം WidthVP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_കണ്ണി_DType* സ്ഥിരം m_dtypep;  // Parent's data type to resolve to
    സ്ഥിരം Stage m_stage;  // If true, report errors
    സ്ഥിരം StreamUse m_streamUse;  // Current expression may be a stream concat

പൊതു:
    WidthVP(Ast_കണ്ണി_DType* dtypep, Stage stage, StreamUse streamUse = STREAM_USE_NONE)
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
        എങ്കിൽ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    WidthVP* p() { തിരികെ ഇത്; }
    തർക്കം selfDtm() സ്ഥിരം { തിരികെ m_dtypep == nullptr; }
    StreamUse streamUse() സ്ഥിരം { തിരികെ m_streamUse; }
    Ast_കണ്ണി_DType* dtypep() സ്ഥിരം {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        തിരികെ m_dtypep;
    }
    Ast_കണ്ണി_DType* dtypeNullp() സ്ഥിരം { തിരികെ m_dtypep; }
    Ast_കണ്ണി_DType* dtypeNullSkipRefp() സ്ഥിരം {
        Ast_കണ്ണി_DType* dtp = dtypeNullp();
        എങ്കിൽ (dtp) dtp = dtp->skipRefp();
        തിരികെ dtp;
    }
    Ast_കണ്ണി_DType* dtypeOverridep(Ast_കണ്ണി_DType* സ്ഥിരം defaultp) സ്ഥിരം {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        എങ്കിൽ (!m_dtypep) തിരികെ defaultp;
        എങ്കിൽ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            തിരികെ defaultp->findBitOrLogicDType(m_dtypep->width(), m_dtypep->widthMin(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        തിരികെ m_dtypep;
    }
    പൂർണ്ണസംഖ്യ width() സ്ഥിരം {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        തിരികെ m_dtypep->width();
    }
    പൂർണ്ണസംഖ്യ widthMin() സ്ഥിരം {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        തിരികെ m_dtypep->widthMin();
    }
    തർക്കം prelim() സ്ഥിരം { തിരികെ m_stage & PRELIM; }
    തർക്കം final() സ്ഥിരം { തിരികെ m_stage & FINAL; }
    ശൂന്യം dump(std::ostream& str) സ്ഥിരം {
        എങ്കിൽ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } അല്ലെങ്കിൽ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvtToHex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& operator<<(std::ostream& str, സ്ഥിരം WidthVP* vup) {
    എങ്കിൽ (vup) vup->dump(str);
    തിരികെ str;
}

//######################################################################

വർഗ്ഗം WidthClearVisitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    ശൂന്യം clearWidthRecurse(Ast_കണ്ണി* nodep) {
        വേണ്ടി (; nodep; nodep = nodep->nextp()) {
            nodep->didWidth(അസത്യം);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) clearWidthRecurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) clearWidthRecurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) clearWidthRecurse(refp);
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) clearWidthRecurse(refp);
        }
    }

പൊതു:
    // CONSTRUCTORS
    explicit WidthClearVisitor(AstNetlist* nodep) { clearWidthRecurse(nodep); }
    മായാ ~WidthClearVisitor() = സ്ഥിരസ്ഥിതി;
};

//######################################################################

#define accept in_Width_Visitor_use_Ast_കണ്ണി_iterate_instead_of_Ast_കണ്ണി_accept

//######################################################################

വർഗ്ഗം WidthVisitor final : പൊതു VNVisitor {
    // TYPES
    using PatVecMap = std::map<പൂർണ്ണസംഖ്യ, AstPatMember*>;
    using DTypeMap = std::map<സ്ഥിരം std::string, AstPatMember*>;

    // STATE
    V3UniqueNames m_enumTempNames;  // Captured enum expressions
    V3UniqueNames m_insideTempNames;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<സ്ഥിരം AstModport*, std::unordered_set<string>>
        m_modportNames;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    WidthVP* m_vup = nullptr;  // Current node state
    തർക്കം m_underFork = അസത്യം;  // Visiting under a fork
    തർക്കം m_underSExpr = അസത്യം;  // Visiting under a sequence expression
    തർക്കം m_underPackedArray = അസത്യം;  // Visiting under a AstPackArrayDType
    തർക്കം m_underMemberSel = അസത്യം;  // Viting under a MemberSel
    Ast_കണ്ണി* m_seqUnsupp = nullptr;  // Property has unsupported node
    തർക്കം m_hasSExpr = അസത്യം;  // Property has a sequence expression
    സ്ഥിരം AstCell* m_cellp = nullptr;  // Current cell for arrayed instantiations
    std::vector<AstVar*> m_splitIfaceArrayVarps;  // Interface arrays split by V3Param
    സ്ഥിരം AstEnumItem* m_enumItemp = nullptr;  // Current enum item
    Ast_കണ്ണി_FTask* m_ftaskp = nullptr;  // Current function/task
    Ast_കണ്ണി_Module* m_modep = nullptr;  // Current module
    സ്ഥിരം AstConstraint* m_constraintp = nullptr;  // Current constraint
    Ast_കണ്ണി_Procedure* m_procedurep = nullptr;  // Current final/always
    സ്ഥിരം AstWith* m_withp = nullptr;  // Current 'with' statement
    സ്ഥിരം AstFunc* m_funcp = nullptr;  // Current function
    സ്ഥിരം AstAttrOf* m_attrp = nullptr;  // Current attribute
    സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* m_randomizeFromp = nullptr;  // Current randomize method call fromp
    സ്ഥിരം തർക്കം m_paramsOnly;  // Computing parameter value; limit operation
    സ്ഥിരം തർക്കം m_doGenerate;  // Do errors later inside generate statement
    തർക്കം m_streamConcat = അസത്യം;  // True if visiting arguments of stream concatenation
    തർക്കം m_inParamOverride = അസത്യം;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<സ്ഥിരം Ast_കണ്ണി_DType*, VAttrType>, AstVarRef*> m_tableMap;
    // Queues with given index type
    std::map<സ്ഥിരം Ast_കണ്ണി_DType*, AstQueueDType*> m_queueDTypeIndexed;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_set<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_set<സ്ഥിരം AstVar*> m_curModVars;  // Variables declared in current module

    സ്ഥിതം constexpr പൂർണ്ണസംഖ്യ ENUM_LOOKUP_BITS = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    എണ്ണൽ ExtendRule : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    എണ്ണൽ വർഗ്ഗം ComparisonType : uint8_t {
        NORMAL_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        NORMAL_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    സ്ഥിതം StreamUse streamUseForDeterm(Determ determ) {
        തിരികെ determ == ASSIGN ? STREAM_USE_ASSIGN : STREAM_USE_NONE;
    }

    StreamUse currentStreamUse() സ്ഥിരം { തിരികെ m_vup ? m_vup->streamUse() : STREAM_USE_NONE; }

    സ്ഥിതം ശൂന്യം packIfUnpacked(Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nodep) {
        എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ elements_അക്കം = unpackDTypep->arrayUnpackedElements();
            സ്ഥിരം പൂർണ്ണസംഖ്യ unpackMinBits = elements_അക്കം * unpackDTypep->subDTypep()->widthMin();
            സ്ഥിരം പൂർണ്ണസംഖ്യ unpackBits = elements_അക്കം * unpackDTypep->subDTypep()->width();
            VNRelinker relinker;
            nodep->unlinkFrBack(&relinker);
            relinker.relink(പുതിയ AstCvtArrayToPacked{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpackBits, unpackMinBits, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    സ്ഥിതം Ast_കണ്ണി_DType* fromDTypep(Ast_കണ്ണി* fromp) {
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtypep = VN_CAST(fromp, കണ്ണി_DType))
            തിരികെ dtypep->skipRefOrNullp();
        തിരികെ fromp ? fromp->dtypep() : nullptr;
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
    ശൂന്യം visit(AstLogNot* nodep) override { visit_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    ശൂന്യം visit(AstLogAnd* nodep) override { visit_log_and_or(nodep); }
    ശൂന്യം visit(AstLogOr* nodep) override { visit_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    ശൂന്യം visit(AstSAnd* nodep) override { visit_log_and_or(nodep); }
    ശൂന്യം visit(AstSIntersect* nodep) override { visit_log_and_or(nodep); }
    ശൂന്യം visit(AstSOr* nodep) override { visit_log_and_or(nodep); }
    ശൂന്യം visit(AstSWithin* nodep) override { visit_log_and_or(nodep); }
    ശൂന്യം visit(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }
    ശൂന്യം visit(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        visit_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    ശൂന്യം visit(AstRedAnd* nodep) override { visit_red_and_or(nodep); }
    ശൂന്യം visit(AstRedOr* nodep) override { visit_red_and_or(nodep); }
    ശൂന്യം visit(AstRedXor* nodep) override { visit_red_and_or(nodep); }
    ശൂന്യം visit(AstOneHot* nodep) override { visit_red_and_or(nodep); }
    ശൂന്യം visit(AstOneHot0* nodep) override { visit_red_and_or(nodep); }
    ശൂന്യം visit(AstIsUnknown* nodep) override {
        visit_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    ശൂന്യം visit(AstEq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ശൂന്യം visit(AstNeq* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_EQ); }
    ശൂന്യം visit(AstGt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstGte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstLt* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstLte* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstGtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstGteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstLtS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstLteS* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::NORMAL_GL); }
    ശൂന്യം visit(AstEqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    ശൂന്യം visit(AstNeqCase* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    ശൂന്യം visit(AstEqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    ശൂന്യം visit(AstNeqWild* nodep) override { visit_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    ശൂന്യം visit(AstEqD* nodep) override { visit_cmp_real(nodep); }
    ശൂന്യം visit(AstNeqD* nodep) override { visit_cmp_real(nodep); }
    ശൂന്യം visit(AstLtD* nodep) override { visit_cmp_real(nodep); }
    ശൂന്യം visit(AstLteD* nodep) override { visit_cmp_real(nodep); }
    ശൂന്യം visit(AstGtD* nodep) override { visit_cmp_real(nodep); }
    ശൂന്യം visit(AstGteD* nodep) override { visit_cmp_real(nodep); }
    // ...    String compares
    ശൂന്യം visit(AstEqN* nodep) override { visit_cmp_string(nodep); }
    ശൂന്യം visit(AstNeqN* nodep) override { visit_cmp_string(nodep); }
    ശൂന്യം visit(AstLtN* nodep) override { visit_cmp_string(nodep); }
    ശൂന്യം visit(AstLteN* nodep) override { visit_cmp_string(nodep); }
    ശൂന്യം visit(AstGtN* nodep) override { visit_cmp_string(nodep); }
    ശൂന്യം visit(AstGteN* nodep) override { visit_cmp_string(nodep); }
    // ...    Data type compares
    ശൂന്യം visit(AstEqT* nodep) override { visit_cmp_type(nodep); }
    ശൂന്യം visit(AstNeqT* nodep) override { visit_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    ശൂന്യം visit(AstAnd* nodep) override { visit_boolexpr_and_or(nodep); }
    ശൂന്യം visit(AstOr* nodep) override { visit_boolexpr_and_or(nodep); }
    ശൂന്യം visit(AstXor* nodep) override { visit_boolexpr_and_or(nodep); }
    ശൂന്യം visit(AstBufIf1* nodep) override {
        visit_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    ശൂന്യം visit(AstAdd* nodep) override { visit_add_sub_replace(nodep, സത്യം, അസത്യം); }
    ശൂന്യം visit(AstSub* nodep) override { visit_add_sub_replace(nodep, സത്യം, അസത്യം); }
    ശൂന്യം visit(AstDiv* nodep) override { visit_add_sub_replace(nodep, സത്യം, സത്യം); }
    ശൂന്യം visit(AstMul* nodep) override { visit_add_sub_replace(nodep, സത്യം, അസത്യം); }
    // These can't promote to real
    ശൂന്യം visit(AstModDiv* nodep) override { visit_add_sub_replace(nodep, അസത്യം, സത്യം); }
    ശൂന്യം visit(AstModDivS* nodep) override { visit_add_sub_replace(nodep, അസത്യം, സത്യം); }
    ശൂന്യം visit(AstMulS* nodep) override { visit_add_sub_replace(nodep, അസത്യം, അസത്യം); }
    ശൂന്യം visit(AstDivS* nodep) override { visit_add_sub_replace(nodep, അസത്യം, സത്യം); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    ശൂന്യം visit(AstNegate* nodep) override { visit_negate_not(nodep, സത്യം); }
    // Unary never real
    ശൂന്യം visit(AstNot* nodep) override { visit_negate_not(nodep, അസത്യം); }

    // Real: inputs and output real
    ശൂന്യം visit(AstAddD* nodep) override { visit_real_add_sub(nodep); }
    ശൂന്യം visit(AstSubD* nodep) override { visit_real_add_sub(nodep); }
    ശൂന്യം visit(AstDivD* nodep) override { visit_real_add_sub(nodep); }
    ശൂന്യം visit(AstMulD* nodep) override { visit_real_add_sub(nodep); }
    ശൂന്യം visit(AstPowD* nodep) override { visit_real_add_sub(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_System_Biop_D* nodep) override { visit_real_add_sub(nodep); }
    // Real: Output real
    ശൂന്യം visit(AstNegateD* nodep) override { visit_real_neg_ceil(nodep); }
    ശൂന്യം visit(Ast_കണ്ണി_System_Uniop_D* nodep) override { visit_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    ശൂന്യം visit(AstSigned* nodep) override { visit_signed_unsigned(nodep, VSigning::SIGNED); }
    ശൂന്യം visit(AstUnsigned* nodep) override { visit_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    ശൂന്യം visit(AstShiftL* nodep) override { visit_shift(nodep); }
    ശൂന്യം visit(AstShiftR* nodep) override { visit_shift(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    ശൂന്യം visit(AstShiftRS* nodep) override { visit_shift(nodep); }

    //========
    // Widths: Output real, input integer signed
    ശൂന്യം visit(AstBitsToRealD* nodep) override { visit_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    ശൂന്യം visit(AstRToIS* nodep) override { visit_Os32_Lr(nodep); }
    ശൂന്യം visit(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->widthSized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    ശൂന്യം visit(AstRealToBits* nodep) override { visit_Ou64_Lr(nodep); }

    // Output integer, input string
    ശൂന്യം visit(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        എങ്കിൽ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ശൂന്യം visit(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    ശൂന്യം visit(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ശൂന്യം visit(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitSized(8, VSigning::UNSIGNED);
        }
    }
    ശൂന്യം visit(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ശൂന്യം visit(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetInt();
        }
    }
    ശൂന്യം visit(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            എങ്കിൽ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtypeSetDouble();
            } അല്ലെങ്കിൽ {
                nodep->dtypeSetInteger();
            }
        }
    }
    ശൂന്യം visit(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    ശൂന്യം visit(AstTime* nodep) override { nodep->dtypeSetTime2State(); }
    ശൂന്യം visit(AstTimeD* nodep) override { nodep->dtypeSetDouble(); }
    ശൂന്യം visit(AstTimePrecision* nodep) override { nodep->dtypeSetInteger2State(); }
    ശൂന്യം visit(AstGetInitialRandomSeed* nodep) override { nodep->dtypeSetInt(); }
    ശൂന്യം visit(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            പുതിയ AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().powerOfTen()});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstScopeName* nodep) override {
        nodep->dtypeSetUInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    സ്ഥിതം തർക്കം isOverridePattern(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (സ്ഥിരം AstPattern* സ്ഥിരം patternp = VN_CAST(nodep, Pattern)) {
            തിരികെ !patternp->childDTypep();
        }
        സ്ഥിരം AstCond* സ്ഥിരം condp = VN_CAST(nodep, Cond);
        തിരികെ condp && isOverridePattern(condp->thenp()) && isOverridePattern(condp->elsep());
    }
    ശൂന്യം visit(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_inParamOverride) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            സ്ഥിരം പൂർണ്ണസംഖ്യ errors = V3Error::error_എണ്ണം();
            {
                VL_RESTORER(m_inParamOverride);
                m_inParamOverride = അസത്യം;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->condp(), Const);
            സ്ഥിരം തർക്കം known = constp && !constp->അക്കം().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            എങ്കിൽ (!known && V3Error::error_എണ്ണം() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം keepp
                = (known && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlinkFrBack();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            userIterate(keepp, WidthVP{SELF, BOTH}.p());
            തിരികെ;
        }
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
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
            Ast_കണ്ണി_DType* സ്ഥിരം thenDTypep = nodep->thenp()->dtypep();
            Ast_കണ്ണി_DType* സ്ഥിരം elseDTypep = nodep->elsep()->dtypep();
            Ast_കണ്ണി_DType* സ്ഥിരം thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_കണ്ണി_DType* സ്ഥിരം elseDTypeSkippedp = elseDTypep->skipRefp();
            എങ്കിൽ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->thenp()->is_Class_Handle_മൂല്യം()
                       || nodep->elsep()->is_Class_Handle_മൂല്യം()) {
                Ast_കണ്ണി_DType* commonClassTypep = nullptr;
                എങ്കിൽ (nodep->thenp()->is_Class_Handle_മൂല്യം() && nodep->elsep()->is_Class_Handle_മൂല്യം()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_കണ്ണി::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                എങ്കിൽ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } അല്ലെങ്കിൽ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                എങ്കിൽ (Ast_കണ്ണി::computeCastable(elseDTypep, thenDTypep, nullptr).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } അല്ലെങ്കിൽ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    എങ്കിൽ (!Ast_കണ്ണി::computeCastable(thenDTypep, elseDTypep, nullptr)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtypeSetDouble();
            } അല്ലെങ്കിൽ എങ്കിൽ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtypeSetString();
            } അല്ലെങ്കിൽ {
                സ്ഥിരം തർക്കം isAnyResultFourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                തർക്കം handled = അസത്യം;
                സ്ഥിരം തർക്കം isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_കണ്ണി_DType* സ്ഥിരം thenEnumDTypep = thenDTypep->skipRefToEnump();
                എങ്കിൽ (VN_IS(thenDTypeSkippedp, കണ്ണി_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_വൃക്ഷം(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = സത്യം;
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_വൃക്ഷം(elseDTypep->skipRefToEnump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    എങ്കിൽ (isAnyResultFourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = സത്യം;
                    } അല്ലെങ്കിൽ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(പുതിയ File_വരി{nodep->fileline()});
                        nodep->fileline()->warnOff(V3ErrorCode::ENUMVALUE, സത്യം);
                    }
                }
                എങ്കിൽ (!handled) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(thenDTypep->width(), elseDTypep->width());
                    സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = std::max(thenDTypep->widthMin(), elseDTypep->widthMin());
                    സ്ഥിരം VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    സ്ഥിരം തർക്കം isFourstate = isAnyResultFourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    എങ്കിൽ ((width == elseDTypeSkippedp->width()
                         && widthMin == elseDTypeSkippedp->widthMin())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } അല്ലെങ്കിൽ എങ്കിൽ ((width == thenDTypeSkippedp->width()
                                && widthMin == thenDTypeSkippedp->widthMin())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } അല്ലെങ്കിൽ {
                        // None of the children dtypes match generate one
                        nodep->dtypeSetBitOrLogicUnsized(width, widthMin, numeric, isFourstate);
                    }
                }
            }
        }
        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    ശൂന്യം visit(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, WidthVP{SELF, BOTH}.p());
        // Conversions
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            userIterateAndNext(nodep->lhsp(), WidthVP{vdtypep, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            addConsItem(newp, nodep->lhsp()->unlinkFrBack());
            addConsItem(newp, nodep->rhsp()->unlinkFrBack());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        എങ്കിൽ (VN_IS(vdtypep, UnpackArrayDType)) {
            AstPattern* സ്ഥിരം newp = പുതിയ AstPattern{nodep->fileline(), nullptr};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            തിരികെ;
        }

        // Concat handling
        എങ്കിൽ (m_vup->prelim()) {
            എങ്കിൽ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            എങ്കിൽ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtypeSetString();
            } അല്ലെങ്കിൽ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                എങ്കിൽ (m_streamConcat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width() + nodep->rhsp()->width(),
                    nodep->lhsp()->widthMin() + nodep->rhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            എങ്കിൽ (സ്ഥിരം AstReplicate* സ്ഥിരം repp = VN_CAST(nodep->lhsp(), Replicate)) {
                എങ്കിൽ (repp->width() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlinkFrBack());
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
            എങ്കിൽ (സ്ഥിരം AstReplicate* സ്ഥിരം repp = VN_CAST(nodep->rhsp(), Replicate)) {
                എങ്കിൽ (repp->width() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
        }
        എങ്കിൽ (m_vup->final()) {
            എങ്കിൽ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_കണ്ണി_വ്യഞ്ജകം* lhsp = nodep->lhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* rhsp = nodep->rhsp()->unlinkFrBack();
                എങ്കിൽ (!lhsp->isString()) lhsp = പുതിയ AstCvtPackString{lhsp->fileline(), lhsp};
                എങ്കിൽ (!rhsp->isString()) rhsp = പുതിയ AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
            എങ്കിൽ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                കണ്ണി_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ശൂന്യം visit(AstConcatN* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetString();
        }
        എങ്കിൽ (m_vup->final()) {
            nodep->didWidth(സത്യം);
            എങ്കിൽ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                കണ്ണി_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ശൂന്യം visit(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    ശൂന്യം visit(AstDelay* nodep) override {
        എങ്കിൽ (nodep->isCycleDelay() && m_underSExpr) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            എങ്കിൽ (nodep->rhsp() && !nodep->isUnbounded()) {
                userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            എങ്കിൽ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                തിരികെ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::checkSvaCycleDelay(nodep, nodep);
            തിരികെ;
        }
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            തിരികെ;
        }
        എങ്കിൽ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->fileline()->timingOn()) {
            എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterateAndNextNull(nodep->stmtsp());
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } അല്ലെങ്കിൽ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        എങ്കിൽ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBackWithNext());
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    ശൂന്യം visit(AstFork* nodep) override {
        എങ്കിൽ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        എങ്കിൽ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().isSetTrue()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* സ്ഥിരം newp = പുതിയ AstBegin{nodep->fileline(), nodep->name(), nullptr, അസത്യം};
            nodep->replaceWith(newp);
            എങ്കിൽ (nodep->declsp()) newp->addDeclsp(nodep->declsp()->unlinkFrBackWithNext());
            എങ്കിൽ (nodep->stmtsp()) newp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            എങ്കിൽ (nodep->forksp()) newp->addStmtsp(nodep->forksp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
            VL_RESTORER(m_underFork);
            m_underFork = സത്യം;
            iterateChildren(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    ശൂന്യം visitWaitOrDisableFork(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (nodep->fileline()->timingOn()) {
            എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (!v3Global.opt.timing().isSetTrue()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    ശൂന്യം visit(AstDisableFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ശൂന്യം visit(AstWaitFork* nodep) override { visitWaitOrDisableFork(nodep); }
    ശൂന്യം visit(AstToLowerN* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ശൂന്യം visit(AstToUpperN* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetString();
        }
    }
    ശൂന്യം visit(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->countp(), Const);
            എങ്കിൽ (constp) {
                എങ്കിൽ (constp->അക്കം().isFourState()
                    || (constp->dtypep()->isSigned() && constp->അക്കം().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } അല്ലെങ്കിൽ {
                    times = constp->toSInt();
                }
            }

            Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypeNullSkipRefp();
            എങ്കിൽ (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                എങ്കിൽ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                എങ്കിൽ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    userIterateAndNext(nodep->srcp(), WidthVP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlinkFrBack());
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ {  // int a[] = {lhs} -> same as '{lhs}
                    AstPattern* സ്ഥിരം newp = പുതിയ AstPattern{
                        nodep->fileline(),
                        പുതിയ AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlinkFrBack(),
                                         nullptr, nullptr}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    തിരികെ;
                }
            }
            എങ്കിൽ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            എങ്കിൽ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } അല്ലെങ്കിൽ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            എങ്കിൽ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_കണ്ണി* സ്ഥിരം newp
                    = പുതിയ AstReplicateN{nodep->fileline(), nodep->srcp()->unlinkFrBack(),
                                        nodep->countp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (!constp) nodep->v3error("Replication value isn't a constant.");
                എങ്കിൽ (times == 0
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
        എങ്കിൽ (m_vup->final()) {
            എങ്കിൽ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                കണ്ണി_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ശൂന്യം visit(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtypeSetString();
        }
        എങ്കിൽ (m_vup->final()) {
            എങ്കിൽ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                കണ്ണി_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Dist_Biop* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_Dist_Triop* nodep) override {
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    തർക്കം streamImplicitUseAllowed(സ്ഥിരം Ast_കണ്ണി_Stream* nodep) സ്ഥിരം {
        എങ്കിൽ (m_streamConcat) തിരികെ സത്യം;
        എങ്കിൽ (m_vup && m_vup->streamUse() != STREAM_USE_NONE) തിരികെ സത്യം;

        സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം backp = nodep->backp();
        എങ്കിൽ (!backp) തിരികെ സത്യം;  // Error elsewhere

        തിരികെ VN_IS(backp, കണ്ണി_Assign);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Stream* nodep) override {
        VL_RESTORER(m_streamConcat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            m_streamConcat = സത്യം;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_streamConcat = അസത്യം;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->rhsp(), Const)) {
                എങ്കിൽ (constp->toUInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } അല്ലെങ്കിൽ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDtypep = nodep->lhsp()->dtypep()->skipRefToEnump();
            എങ്കിൽ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, കണ്ണി_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, കണ്ണി_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtypeSetStream();
            } അല്ലെങ്കിൽ എങ്കിൽ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtypeSetStream();
            } അല്ലെങ്കിൽ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->lhsp()->width(), nodep->lhsp()->widthMin(), VSigning::UNSIGNED,
                    Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        എങ്കിൽ (m_vup->final()) {
            എങ്കിൽ (!streamImplicitUseAllowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warnMore() << "... Suggest use a cast");
            }
            എങ്കിൽ (!nodep->dtypep()->widthSized()) {
                // See also error in V3Number
                കണ്ണി_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    ശൂന്യം visit(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), സത്യം,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), സത്യം,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        എങ്കിൽ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->elementsConst();
            എങ്കിൽ (nodep->fromBracket() && nodep->ഇടത്_Const() > nodep->വലത്_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-width + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } അല്ലെങ്കിൽ എങ്കിൽ (width > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << width << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            സ്ഥിരം തർക്കം inDeadModule = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            സ്ഥിരം തർക്കം inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            സ്ഥിരം തർക്കം inTypeTable = !m_modep;
            സ്ഥിരം Ast_കണ്ണി* basep = nodep->backp();
            ആകുമ്പോൾ (VN_IS(basep, Range)) basep = basep->backp();
            എങ്കിൽ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !inDeadModule && !inParameterizedTemplate && !inTypeTable) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ഇടത്_Const() << ":" << nodep->വലത്_Const()
                                            << "]");
            }
        }
    }

    ശൂന്യം visit(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), അസത്യം);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            പൂർണ്ണസംഖ്യ width = nodep->widthConst();
            എങ്കിൽ (width <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtypeSetBit();
                തിരികെ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            സ്ഥിരം തർക്കം isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            സ്ഥിരം തർക്കം inParameterizedTemplate
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            എങ്കിൽ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                width = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtypeSetBitOrLogicSized(width, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->widthConst(width);
                തള്ളുക_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(പുതിയ AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            എങ്കിൽ (nodep->fromp()->width() < width && !inParameterizedTemplate) {
                // If it is not a lvalue, extend it
                എങ്കിൽ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << width << " bits from only "
                                                          << nodep->fromp()->width()
                                                          << " bit number");
                    Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = nodep->findBitOrLogicDType(
                        width, width, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    അസത്യം /*noerror*/);
                } അല്ലെങ്കിൽ {
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
            പൂർണ്ണസംഖ്യ frommsb = nodep->fromp()->width() - 1;
            പൂർണ്ണസംഖ്യ fromlsb = 0;
            സ്ഥിരം പൂർണ്ണസംഖ്യ elw = nodep->declElWidth();  // Must adjust to tell user bit ranges
            എങ്കിൽ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } അല്ലെങ്കിൽ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ selwidth = V3_സംഖ്യ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_കണ്ണി_DType* സ്ഥിരം selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, FINAL}.p());
            userIterateAndNext(nodep->lsbp(), WidthVP{SELF, FINAL}.p());
            എങ്കിൽ (widthBad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->width() != 32) {
                എങ്കിൽ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
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
            എങ്കിൽ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                എങ്കിൽ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } അല്ലെങ്കിൽ എങ്കിൽ (!inParameterizedTemplate) {
                    എങ്കിൽ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } അല്ലെങ്കിൽ {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                എങ്കിൽ (!isWriteSelect) {
                    // Extend it
                    സ്ഥിരം പൂർണ്ണസംഖ്യ extendTo = nodep->msbConst() + 1;
                    Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = nodep->findBitOrLogicDType(
                        extendTo, extendTo, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    widthCheckSized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    അസത്യം /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            എങ്കിൽ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                widthCheckSized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                അസത്യം /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    ശൂന്യം visit(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            എങ്കിൽ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->bitp(), Const)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ bit = constp->toSInt();
                    Ast_കണ്ണി_വ്യഞ്ജകം* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    എങ്കിൽ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
            }
            //
            പൂർണ്ണസംഖ്യ frommsb;
            പൂർണ്ണസംഖ്യ fromlsb;
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefp();
            എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                എങ്കിൽ (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                എങ്കിൽ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } അല്ലെങ്കിൽ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ selwidth = V3_സംഖ്യ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_കണ്ണി_DType* സ്ഥിരം selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            എങ്കിൽ (widthBad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->width() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->width(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->width()
                                       << (nodep->bitp()->width() != nodep->bitp()->widthMin()
                                               ? " or " + cvtToStr(nodep->bitp()->widthMin())
                                               : "")
                                       << " bits.");
                എങ്കിൽ (!nodep->fileline()->warnIsOff(V3ErrorCode::WIDTH)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            എങ്കിൽ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                എങ്കിൽ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->toSInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->toSInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    എങ്കിൽ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->toSInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                widthCheckSized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                അസത്യം /*NOWARN*/);
            }
        }
    }

    ശൂന്യം visit(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefp();
            സ്ഥിരം AstAssocArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, AssocArrayDType);
            എങ്കിൽ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->താക്കോൽ_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ശൂന്യം visit(AstAlias* nodep) override {
        എങ്കിൽ (!nodep->didWidthAndSet()) {
            userIterateAndNext(nodep->itemsp(), WidthVP{SELF, BOTH}.p());
        }

        സ്ഥിരം auto check_If_വ്യഞ്ജകം_Ok = [ഇത്](സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp) {
            എങ്കിൽ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                തിരികെ അസത്യം;
            }

            സ്ഥിരം AstVarRef* സ്ഥിരം varRefp = VN_CAST(exprp, VarRef);
            എങ്കിൽ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                തിരികെ അസത്യം;
            }
            AstVar* സ്ഥിരം varp = varRefp->varp();
            എങ്കിൽ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                തിരികെ അസത്യം;
            }
            എങ്കിൽ (m_aliasedVars.find(varp) != m_aliasedVars.end()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                തിരികെ അസത്യം;
            } അല്ലെങ്കിൽ {
                m_aliasedVars.തിരുകുക(varp);
            }
            തിരികെ സത്യം;
        };

        check_If_വ്യഞ്ജകം_Ok(nodep->itemsp());
        Ast_കണ്ണി_DType* സ്ഥിരം firstItemDtypep = nodep->itemsp()->dtypep();
        വേണ്ടി (Ast_കണ്ണി* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_വ്യഞ്ജകം_Ok(VN_AS(itemp, കണ്ണി_വ്യഞ്ജകം));
            എങ്കിൽ (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    ശൂന്യം visit(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefp();
            സ്ഥിരം AstWildcardArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            എങ്കിൽ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            എങ്കിൽ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ശൂന്യം visit(AstSliceSel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefp();
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            എങ്കിൽ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_കണ്ണി_DType* സ്ഥിരം newDtp
                = പുതിയ AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          പുതിയ AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->addTypesp(newDtp);
            nodep->dtypeFrom(newDtp);

            എങ്കിൽ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                സ്ഥിരം പൂർണ്ണസംഖ്യ subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                സ്ഥിരം VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                എങ്കിൽ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(പുതിയ AstConsArray{
                        nodep->fileline(), newDtp,
                        പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ എങ്കിൽ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            എങ്കിൽ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* സ്ഥിരം fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* സ്ഥിരം newp = പുതിയ AstConsArray{nodep->fileline(), newDtp};
                സ്ഥിരം VNumRange range = nodep->declRange();
                വേണ്ടി (പൂർണ്ണസംഖ്യ n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_വൃക്ഷം(അസത്യം));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }

    ശൂന്യം visit(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->thsp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി* സ്ഥിരം selp = V3Width::widthSelNoIterEdit(nodep);
        എങ്കിൽ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            തിരികെ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    ശൂന്യം visit(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->leftp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->rightp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി* സ്ഥിരം selp = V3Width::widthSelNoIterEdit(nodep);
        എങ്കിൽ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            തിരികെ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    ശൂന്യം visit(AstSelPlus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി* സ്ഥിരം selp = V3Width::widthSelNoIterEdit(nodep);
        എങ്കിൽ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            തിരികെ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    ശൂന്യം visit(AstSelMinus* nodep) override {
        userIterateAndNext(nodep->fromp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->bitp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->widthp(), WidthVP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        userIterateAndNext(nodep->attrp(), WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി* സ്ഥിരം selp = V3Width::widthSelNoIterEdit(nodep);
        എങ്കിൽ (selp != nodep) {
            nodep = nullptr;
            userIterate(selp, m_vup);
            തിരികെ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    ശൂന്യം visit(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            എങ്കിൽ (VN_IS(nodep->dtypep()->skipRefToEnump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->അക്കം().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->അക്കം().isString()) {
                nodep->dtypeSetString();
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->അക്കം().isDouble()) {
                nodep->dtypeChgWidth(nodep->അക്കം().width(), nodep->അക്കം().sized()
                                                               ? nodep->അക്കം().width()
                                                               : nodep->അക്കം().widthToFit());
            } അല്ലെങ്കിൽ {
                nodep->dtypeSetBitOrLogicUnsized(
                    nodep->അക്കം().width(),
                    nodep->അക്കം().sized() ? nodep->അക്കം().width() : nodep->അക്കം().widthToFit(),
                    nodep->dtypep()->numeric(), nodep->അക്കം().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    ശൂന്യം visit(AstEmptyQueue* nodep) override {
        nodep->dtypeSetEmptyQueue();
        എങ്കിൽ (!VN_IS(nodep->backp(), Assign) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    ശൂന്യം visit(AstFell* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstFalling* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstFuture* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    ശൂന്യം visit(AstPast* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            nodep->dtypeFrom(nodep->exprp());
            എങ്കിൽ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ സത്യം);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->ticksp(), Const);
                എങ്കിൽ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_വൃക്ഷം();
                } അല്ലെങ്കിൽ എങ്കിൽ (constp->toSInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlinkFrBack()->delete_വൃക്ഷം();
                } അല്ലെങ്കിൽ എങ്കിൽ (V3AssertCommon::check_Sva_മൂല്യം_Limit(nodep, constp, "$past tick value")) {
                    എങ്കിൽ (constp->toSInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->toSInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), nullptr);
        }
    }
    ശൂന്യം visit(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            സ്ഥിരം AstConst* സ്ഥിരം minConstp = VN_CAST(nodep->countp(), Const);
            എങ്കിൽ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } അല്ലെങ്കിൽ എങ്കിൽ (minConstp->toSInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->toSInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            എങ്കിൽ (nodep->maxCountp()) {
                userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                സ്ഥിരം AstConst* സ്ഥിരം maxConstp = VN_CAST(nodep->maxCountp(), Const);
                എങ്കിൽ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } അല്ലെങ്കിൽ എങ്കിൽ (minConstp && maxConstp->toSInt() < minConstp->toSInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } അല്ലെങ്കിൽ എങ്കിൽ (maxConstp->toSInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->propp(), WidthVP{SELF, BOTH}.p());
            Ast_കണ്ണി_വ്യഞ്ജകം* propp = nodep->propp();
            എങ്കിൽ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                userIterateAndNext(nodep->loBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            എങ്കിൽ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                userIterateAndNext(nodep->hiBoundp(), WidthVP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            സ്ഥിരം തർക്കം loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            സ്ഥിരം തർക്കം hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            എങ്കിൽ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_കണ്ണി* സ്ഥിരം boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                തിരികെ;
            }
            എങ്കിൽ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtypeSetBit();
                തിരികെ;
            }
            സ്ഥിരം AstConst* സ്ഥിരം loConstp = VN_CAST(nodep->loBoundp(), Const);
            സ്ഥിരം AstConst* സ്ഥിരം hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            എങ്കിൽ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            എങ്കിൽ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            എങ്കിൽ (loConstp && loConstp->toSInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } അല്ലെങ്കിൽ എങ്കിൽ (loConstp) {
                V3AssertCommon::check_Sva_മൂല്യം_Limit(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            എങ്കിൽ (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_മൂല്യം_Limit(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            എങ്കിൽ (!hiUnbounded && loConstp && hiConstp && hiConstp->toSInt() < loConstp->toSInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            തർക്കം hasPropertyOp = propp->isMultiCycleSva();
            എങ്കിൽ (!hasPropertyOp) {
                propp->foreach([&](സ്ഥിരം Ast_കണ്ണി* np) {
                    എങ്കിൽ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = സത്യം;
                    }
                });
            }
            എങ്കിൽ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fixWidthReduce(propp), propp);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstRising* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstRose* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }

    ശൂന്യം visit(AstSampled* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    ശൂന്യം visit(AstSetuphold* nodep) override {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        AstAssignW* newp = nullptr;
        എങ്കിൽ (nodep->delrefp()) {
            newp = convertSetupholdToAssign(flp, nodep->refevp(), nodep->delrefp());
        }
        എങ്കിൽ (nodep->deldatap()) {
            എങ്കിൽ (!newp) {
                newp = convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap());
            } അല്ലെങ്കിൽ {
                newp->addNextHere(
                    convertSetupholdToAssign(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        എങ്കിൽ (!newp) {
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        nodep->replaceWith(പുതിയ AstAlways{newp});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    AstAssignW* convertSetupholdToAssign(File_വരി* സ്ഥിരം flp, Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം evp,
                                         Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം delp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = delp->clone_വൃക്ഷം_Pure(അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = evp->clone_വൃക്ഷം_Pure(അസത്യം);
        UASSERT_OBJ(VN_IS(lhsp, കണ്ണി_Var_Ref) || VN_IS(lhsp, കണ്ണി_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        എങ്കിൽ (Ast_കണ്ണി_Var_Ref* varRefp = VN_CAST(lhsp, കണ്ണി_Var_Ref)) {
            എങ്കിൽ (varRefp->varp()->direction() == VDirection::നിവേശം) {
                VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
                തിരികെ nullptr;
            }
            varRefp->access(VAccess::WRITE);
        }
        എങ്കിൽ (Ast_കണ്ണി_Pre_Sel* selp = VN_CAST(lhsp, കണ്ണി_Pre_Sel)) {
            എങ്കിൽ (Ast_കണ്ണി_Var_Ref* varRefp = VN_CAST(selp->fromp(), കണ്ണി_Var_Ref)) {
                എങ്കിൽ (varRefp->varp()->direction() == VDirection::നിവേശം) {
                    VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
                    തിരികെ nullptr;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        തിരികെ പുതിയ AstAssignW{flp, lhsp, rhsp};
    }

    ശൂന്യം visit(AstStable* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            userIterate(nodep->sentreep(), nullptr);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstSteady* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ സത്യം);
            nodep->dtypeSetBit();
        }
    }

    ശൂന്യം visit(Ast_Stmt_വ്യഞ്ജകം* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }

    ശൂന്യം visit(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }

    ശൂന്യം visit(AstUntil* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            എങ്കിൽ (nodep->lhsp()->width() != 1) fixWidthReduce(nodep->lhsp());
            എങ്കിൽ (nodep->rhsp()->width() != 1) fixWidthReduce(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstAbortOn* nodep) override { visitAbortProp(nodep); }

    ശൂന്യം visit(AstRand* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            എങ്കിൽ (nodep->urandom()) {
                nodep->dtypeSetUInt32();  // Says the spec
            } അല്ലെങ്കിൽ {
                nodep->dtypeSetInt();  // Says the spec
            }
            എങ്കിൽ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    ശൂന്യം visit(AstSEventually* nodep) override {
        എങ്കിൽ (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::Widthed_മൂല്യം{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstSGotoRep* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            എങ്കിൽ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Goto");
            } അല്ലെങ്കിൽ {
                width_Check_Rep_എണ്ണം(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstSNonConsRep* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            userIterateAndNext(nodep->countp(), WidthVP{SELF, BOTH}.p());
            എങ്കിൽ (nodep->maxCountp()) {
                widthCheckGotoRepRange(nodep, "Nonconsecutive");
            } അല്ലെങ്കിൽ {
                width_Check_Rep_എണ്ണം(nodep);
            }
            nodep->dtypeSetBit();
        }
    }
    template <typename T_Rep>
    ശൂന്യം width_Check_Rep_എണ്ണം(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->countp(), Const);
        എങ്കിൽ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } അല്ലെങ്കിൽ എങ്കിൽ (constp->dtypep()->isSigned() && constp->അക്കം().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } അല്ലെങ്കിൽ എങ്കിൽ (constp->അക്കം().mostSetBitP1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } അല്ലെങ്കിൽ എങ്കിൽ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    template <typename T_Rep>
    ശൂന്യം widthCheckGotoRepRange(T_Rep* nodep, സ്ഥിരം അക്ഷരം* kind) {
        userIterateAndNext(nodep->maxCountp(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        സ്ഥിരം AstConst* സ്ഥിരം minConstp = VN_CAST(nodep->countp(), Const);
        സ്ഥിരം AstConst* സ്ഥിരം maxConstp = VN_CAST(nodep->maxCountp(), Const);
        എങ്കിൽ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{kind}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            തിരികെ;
        }
        എങ്കിൽ (minConstp->toSInt() < 0) {
            nodep->v3error(std::string{kind}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            തിരികെ;
        }
        എങ്കിൽ (maxConstp->toSInt() < minConstp->toSInt()) {
            nodep->v3error(std::string{kind}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            തിരികെ;
        }
        എങ്കിൽ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + kind
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    ശൂന്യം visit(AstSThroughout* nodep) override {
        m_hasSExpr = സത്യം;
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = അസത്യം;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = സത്യം;
            iterate(nodep->rhsp());
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = സത്യം;
        m_hasSExpr = സത്യം;
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            എങ്കിൽ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = സത്യം;
        m_hasSExpr = സത്യം;
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            വേണ്ടി (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                userIterateAndNext(senp->sensp(), WidthVP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstURandomRange* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            nodep->dtypeSetUInt32();  // Says the spec
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = nodep->findUInt32DType();
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    ശൂന്യം visit(AstUnbounded* nodep) override {
        nodep->dtypeSetInt();  // Used in int context
        എങ്കിൽ (VN_IS(nodep->backp(), IsUnbounded)) തിരികെ;  // Ok, leave
        എങ്കിൽ (VN_IS(nodep->backp(), BracketArrayDType)) തിരികെ;  // Ok, leave
        എങ്കിൽ (VN_IS(nodep->backp(), InsideRange)) തിരികെ;  // Ok, leave
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം varp = VN_CAST(nodep->backp(), Var)) {
            എങ്കിൽ (varp->isParam()) തിരികെ;  // Ok, leave
        }
        Ast_കണ്ണി* backp = nodep->backp();
        എങ്കിൽ (VN_IS(backp, Sub)) backp = backp->backp();
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം selp = VN_CAST(backp, SelExtract)) {
            എങ്കിൽ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം selp = VN_CAST(backp, SelBit)) {
            എങ്കിൽ (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) തിരികെ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    ശൂന്യം visit(AstInferredDisable* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) nodep->dtypeSetBit();
    }
    ശൂന്യം visit(AstIsUnbounded* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        }
        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            എങ്കിൽ (nodep->width() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    ശൂന്യം visit(AstCLog2* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    template <typename T_Option>
    ശൂന്യം widthCoverWeight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = nodep->valuep();
        സ്ഥിരം std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
            എങ്കിൽ (!constp->അക്കം().isNegative()) തിരികെ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->toSInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } അല്ലെങ്കിൽ {
            തിരികെ;
        }
        // Continue with the default weight
        valuep->replaceWith(പുതിയ AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    template <typename T_Option>
    ശൂന്യം widthCoverInt(T_Option* nodep) {
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    ശൂന്യം visit(AstCgOptionAssign* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* സ്ഥിരം cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        എങ്കിൽ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            തിരികെ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        എങ്കിൽ (nodep->optType() == VCoverOptionType::AT_LEAST) {
            widthCoverInt(nodep);
            തിരികെ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        എങ്കിൽ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            എങ്കിൽ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->toSInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->toSInt() << endl);
            } അല്ലെങ്കിൽ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = nodep->valuep();
            എങ്കിൽ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(പുതിയ AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            }
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->valuep(), Const);
            എങ്കിൽ (!constp || !constp->അക്കം().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }
    ശൂന്യം visit(AstCoverOption* nodep) override {
        എങ്കിൽ (nodep->optType() == VCoverOptionType::WEIGHT) {
            widthCoverWeight(nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->optType() == VCoverOptionType::AT_LEAST
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            widthCoverInt(nodep);
            തിരികെ;
        }
        userIterateAndNext(nodep->valuep(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstCoverCross* nodep) override {
        userIterateAndNext(nodep->itemsp(), nullptr);
        എങ്കിൽ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
        userIterateAndNext(nodep->binsp(), nullptr);
    }
    ശൂന്യം visit(AstCoverCrossBin* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        എങ്കിൽ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    ശൂന്യം visit(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        userIterateAndNext(nodep->binsp(), WidthVP{nodep->exprp()->dtypep(), BOTH}.p());
        എങ്കിൽ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        userIterateAndNext(nodep->optionsp(), nullptr);
    }
    ശൂന്യം widthCovergroupRanges(Ast_കണ്ണി* rangesp, പൂർണ്ണസംഖ്യ fillWidth) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        സ്ഥിരം auto fill = [&](Ast_കണ്ണി* nodep) {
            എങ്കിൽ (!fillWidth) തിരികെ;
            Ast_കണ്ണി_വ്യഞ്ജകം* exprp = VN_AS(nodep, കണ്ണി_വ്യഞ്ജകം);
            fixAutoExtend(exprp /*ref*/, fillWidth);
        };
        വേണ്ടി (Ast_കണ്ണി *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            എങ്കിൽ (AstInsideRange* സ്ഥിരം rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, nullptr);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } അല്ലെങ്കിൽ {
                itemp = userIterateSubtreeReturnEdits(itemp, WidthVP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    ശൂന്യം visit(AstCoverBinsof* nodep) override {
        userIterateAndNext(nodep->pointp(), nullptr);
        widthCovergroupRanges(nodep->rangesp(), 0);
    }
    ശൂന്യം visit(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        widthCovergroupRanges(nodep->rangesp(), m_vup ? m_vup->dtypep()->width() : 0);
        എങ്കിൽ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        എങ്കിൽ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            userIterateAndNext(nodep->arraySizep(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        userIterateAndNext(nodep->transp(), m_vup);
    }
    ശൂന്യം visit(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        userIterateAndNext(nodep->itemp(), nullptr);
        എങ്കിൽ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        എങ്കിൽ (!VN_IS(nodep->subp(), CoverpointRef)) {
            widthCovergroupRanges(nodep->subp(), m_vup->dtypep()->width());
        }
        widthCoverFilter(nodep->filterp(), "Bin");
    }
    ശൂന്യം visit(AstCoverCrossWith* nodep) override {
        userIterateAndNext(nodep->selectp(), nullptr);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        userIterateAndNext(nodep->varsp(), nullptr);
        widthCoverFilter(nodep->filterp(), "Cross bin");
        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം countp = nodep->countp()) {
            എങ്കിൽ (VN_IS(countp, Unbounded)) {
                countp->dtypeSetInt();  // 'matches $'
            } അല്ലെങ്കിൽ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                userIterateAndNext(countp, WidthVP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    ശൂന്യം widthCoverFilter(Ast_കണ്ണി_വ്യഞ്ജകം* filterp, സ്ഥിരം അക്ഷരം* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, WidthVP{SELF, BOTH}.p()), കണ്ണി_വ്യഞ്ജകം);
        File_വരി* സ്ഥിരം fl = filterp->fileline();
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം resultp = filterp->dtypep()->skipRefp();
        എങ്കിൽ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlinkFrBack(&relinker);
            relinker.relink(
                പുതിയ AstNeqD{fl, filterp, പുതിയ AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } അല്ലെങ്കിൽ എങ്കിൽ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(പുതിയ AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(filterp), filterp);
        } അല്ലെങ്കിൽ {
            fixWidthReduce(filterp);
        }
    }
    ശൂന്യം visit(AstCoverTransSet* nodep) override { userIterateAndNext(nodep->itemsp(), m_vup); }
    ശൂന്യം visit(AstCoverTransItem* nodep) override {
        widthCovergroupRanges(nodep->valuesp(), m_vup ? m_vup->dtypep()->width() : 0);
    }
    ശൂന്യം visit(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            എങ്കിൽ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                തിരികെ;
            }
            checkCvtUS(nodep->lhsp(), അസത്യം);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            എങ്കിൽ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->അക്കം().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }

        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_കണ്ണി_DType* സ്ഥിരം lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->width(), expDTypep->widthMin(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_കണ്ണി* newp = nullptr;  // No change
            എങ്കിൽ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = പുതിയ AstPowSS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = പുതിയ AstPowSU{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = പുതിയ AstPowUS{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                    nodep->rhsp()->unlinkFrBack()};
            }
            എങ്കിൽ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }
    ശൂന്യം visit(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(Ast_എണ്ണം_Bits* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = V3_സംഖ്യ::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ശൂന്യം visit(Ast_എണ്ണം_Ones* nodep) override {
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            സ്ഥിരം പൂർണ്ണസംഖ്യ widthMin = V3_സംഖ്യ::log2b(nodep->lhsp()->width()) + 1;
            nodep->dtypeSetBitUnsized(32, widthMin, VSigning::SIGNED);
        }
    }
    ശൂന്യം visit(AstCvtPackString* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dt = nodep->lhsp()->dtypep()) {
            എങ്കിൽ (VN_IS(dt->skipRefToEnump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    ശൂന്യം visit(AstCvtPackedToArray* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ശൂന്യം visit(AstCvtArrayToArray* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ശൂന്യം visit(AstCvtArrayToPacked* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH, currentStreamUse()}.p());
        // Type set in constructor
    }
    ശൂന്യം visit(AstCvtUnpackToQueue* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // Opaque returns, so arbitrary
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Type set in constructor
    }
    ശൂന്യം visit(AstCvtUnpackToUnpack* nodep) override {
        // Type set when created, the value converted is of its own type
        എങ്കിൽ (m_vup->final()) {
            userIterateAndNext(nodep->fromp(), WidthVP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    ശൂന്യം visit(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        സ്ഥിരം ഇരട്ട timePrescale = constp->അക്കം().toDouble();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        സ്ഥിരം ഇരട്ട time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstEventControl* nodep) override {
        എങ്കിൽ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->fileline()->timingOn()) {
            എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                iterateChildren(nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warnMore()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } അല്ലെങ്കിൽ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        എങ്കിൽ (nodep->stmtsp()) nodep->addNextHere(nodep->stmtsp()->unlinkFrBack());
        VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    സ്ഥിതം string typenameExtends(സ്ഥിരം AstClass* classp) {
        // An interface class may extend several, so those are not named
        എങ്കിൽ (!classp || classp->isInterfaceClass()) തിരികെ "";
        വേണ്ടി (സ്ഥിരം AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            എങ്കിൽ (extendsp->isImplements()) തുടരുക;
            സ്ഥിരം AstClass* സ്ഥിരം basep = extendsp->classOrNullp();
            തിരികെ basep ? " extends class " + basep->dtypeName(സത്യം) : "";
        }
        തിരികെ "";
    }
    ശൂന്യം visit(AstAttrOf* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (nodep->dimp()) userIterateAndNext(nodep->dimp(), WidthVP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        തിരഞ്ഞെടുപ്പ് (nodep->attrType()) {
        അവസ്ഥ VAttrType::DIM_DIMENSIONS:
        അവസ്ഥ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            സ്ഥിരം std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(സത്യം);
            സ്ഥിരം പൂർണ്ണസംഖ്യ val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            നിർത്തുക;
        }
        അവസ്ഥ VAttrType::DIM_BITS_OR_സംഖ്യ: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            എങ്കിൽ (VN_IS(nodep->fromp(), കണ്ണി_വ്യഞ്ജകം)) {
                nodep->replaceWith(nodep->fromp()->unlinkFrBack());
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            } അല്ലെങ്കിൽ {
                Ast_കണ്ണി* newp = പുതിയ AstAttrOf{nodep->fileline(), VAttrType::DIM_BITS,
                                              nodep->fromp()->unlinkFrBack()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                userIterateAndNext(newp, WidthVP{SELF, BOTH}.p());  // Convert AttrOf
            }
            തിരികെ;
        }
        അവസ്ഥ VAttrType::DIM_BITS:
        അവസ്ഥ VAttrType::DIM_മുകളിലെ:
        അവസ്ഥ VAttrType::DIM_INCREMENT:
        അവസ്ഥ VAttrType::DIM_ഇടത്:
        അവസ്ഥ VAttrType::DIM_താഴെയുള്ള:
        അവസ്ഥ VAttrType::DIM_വലത്:
        അവസ്ഥ VAttrType::DIM_SIZE: {
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            എങ്കിൽ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                സ്ഥിരം Ast_കണ്ണി_Module* സ്ഥിരം ownModp = v3Global.rootp()->containingModule(dtypep);
                എങ്കിൽ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtypeSetInt();
                    തിരികെ;
                }
            }

            എങ്കിൽ (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                തിരഞ്ഞെടുപ്പ് (nodep->attrType()) {
                അവസ്ഥ VAttrType::DIM_SIZE: {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                    Ast_കണ്ണി* സ്ഥിരം newp
                        = പുതിയ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    newp->dtypeSetInt();
                    newp->didWidth(സത്യം);
                    newp->protect(അസത്യം);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                    നിർത്തുക;
                }
                അവസ്ഥ VAttrType::DIM_ഇടത്:
                അവസ്ഥ VAttrType::DIM_താഴെയുള്ള: {
                    Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    നിർത്തുക;
                }
                അവസ്ഥ VAttrType::DIM_വലത്:
                അവസ്ഥ VAttrType::DIM_മുകളിലെ: {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sizep
                        = പുതിയ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_SIZE};
                    sizep->dtypeSetInt();
                    sizep->didWidth(സത്യം);
                    sizep->protect(അസത്യം);
                    Ast_കണ്ണി* സ്ഥിരം newp
                        = പുതിയ AstSub{nodep->fileline(), sizep,
                                     പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                    നിർത്തുക;
                }
                അവസ്ഥ VAttrType::DIM_INCREMENT: {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                        = പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    നിർത്തുക;
                }
                അവസ്ഥ VAttrType::DIM_BITS: {
                    എങ്കിൽ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } അല്ലെങ്കിൽ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    നിർത്തുക;
                }
                സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } അല്ലെങ്കിൽ {
                സ്ഥിരം std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(സത്യം);
                സ്ഥിരം uint32_t msbdim = dimpair.first + dimpair.second;
                എങ്കിൽ (!nodep->dimp() || msbdim < 1) {
                    എങ്കിൽ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), കണ്ണി_വ്യഞ്ജകം);
                        Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstShiftL{
                            nodep->fileline(), പുതിയ AstLenN{nodep->fileline(), fromp},
                            പുതിയ AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    } അല്ലെങ്കിൽ {
                        Ast_കണ്ണി_DType* സ്ഥിരം baseDTypep = dtypep->skipRefp();
                        UASSERT_OBJ(baseDTypep, nodep, "Unsized expression");
                        സ്ഥിരം പൂർണ്ണസംഖ്യ dim = 1;
                        AstConst* സ്ഥിരം newp = dimension_മൂല്യം(nodep->fileline(), baseDTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep->dimp(), Const)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ dim = VN_AS(nodep->dimp(), Const)->toSInt();
                    AstConst* സ്ഥിരം newp
                        = dimension_മൂല്യം(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                } അല്ലെങ്കിൽ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* സ്ഥിരം tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dimp = nodep->dimp()->unlinkFrBack();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                }
            }
            നിർത്തുക;
        }
        അവസ്ഥ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            നിർത്തുക;
        അവസ്ഥ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_കണ്ണി_DType* സ്ഥിരം protoDtp = nodep->fromp()->dtypep();
            Ast_കണ്ണി_DType* സ്ഥിരം declDtp = [&]() {
                എങ്കിൽ (m_ftaskp->fvarp()) തിരികെ m_ftaskp->fvarp()->dtypep();
                Ast_കണ്ണി_DType* സ്ഥിരം voidp = പുതിയ AstVoidDType{m_ftaskp->fileline()};
                തള്ളുക_Deletep(voidp);  // Note voidp used past here
                തിരികെ voidp;
            }();
            എങ്കിൽ (!similarDTypeRecurse(protoDtp, declDtp)) {
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
            നിർത്തുക;
        }
        അവസ്ഥ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->fromp()->dtypep();
            string result = dtypep->prettyDTypeName(സത്യം);
            എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                result += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << result << "' from " << dtypep);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::String{}, result};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            നിർത്തുക;
        }
        അവസ്ഥ VAttrType::TYPEID:
            എങ്കിൽ (Ast_കണ്ണി_DType* dtypep = VN_CAST(nodep->fromp(), കണ്ണി_DType)) {
                nodep->dtypep(dtypep);
            } അല്ലെങ്കിൽ {
                nodep->dtypep(VN_AS(nodep->fromp(), കണ്ണി_വ്യഞ്ജകം)->dtypep());
            }
            നിർത്തുക;
        അവസ്ഥ VAttrType::VAR_BASE:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            നിർത്തുക;
        സ്ഥിരസ്ഥിതി: {
            // Everything else resolved earlier
            nodep->dtypeSetBitUnsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            നിർത്തുക;
        }
        }
    }
    ശൂന്യം visit(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    ശൂന്യം visit(AstCellArrayRef* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        userIterateAndNext(nodep->selp(), WidthVP{SELF, PRELIM}.p());
        userIterateAndNext(nodep->selp(), WidthVP{SELF, FINAL}.p());
        nodep->dtypeSetVoid();  // placeholder; this node shouldnt survive beyond linking
        nodep->didWidth(സത്യം);
    }

    ശൂന്യം visit(AstCellRef* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;

        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cellExprp = VN_CAST(nodep->cellp(), കണ്ണി_വ്യഞ്ജകം)) {
            userIterateAndNext(cellExprp, WidthVP{SELF, PRELIM}.p());
            userIterateAndNext(cellExprp, WidthVP{SELF, FINAL}.p());
        }

        എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം)) {
            userIterateAndNext(exprp, WidthVP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            userIterateAndNext(exprp, WidthVP{SELF, FINAL}.p());
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->didWidth(സത്യം);
    }

    // DTYPES
    ശൂന്യം visit(Ast_കണ്ണി_Array_DType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed

        എങ്കിൽ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* സ്ഥിരം basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            എങ്കിൽ (basicp->implicit()) {
                UASSERT_OBJ(basicp->width() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->width() << " bits");
                AstBasicDType* സ്ഥിരം newp = പുതിയ AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->widthForce(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        എങ്കിൽ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = സത്യം;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->widthFromSub(nodep->subDTypep());
            എങ്കിൽ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(സത്യം);
        } അല്ലെങ്കിൽ {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം elemDTypep = nodep->subDTypep()->skipRefp();
            എങ്കിൽ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->subDTypep()->width() * nodep->rangep()->elementsConst();
            nodep->widthForce(width, width);
            എങ്കിൽ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                സ്ഥിരം Ast_കണ്ണി_DType* baseDTypep = nodep->subDTypep();
                ആകുമ്പോൾ (!VN_IS(baseDTypep, RefDType) && baseDTypep->subDTypep()) {
                    baseDTypep = baseDTypep->subDTypep();
                }
                സ്ഥിരം AstBasicDType* സ്ഥിരം basicp = nodep->basicp();
                എങ്കിൽ (!VN_IS(baseDTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(സത്യം));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstAssocArrayDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->താക്കോൽ_DTypep(iterateEditMoveDTypep(nodep, nodep->താക്കോൽ_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_കണ്ണി_DType* സ്ഥിരം childp = nodep->childDTypep();
        childp->unlinkFrBack();
        Ast_കണ്ണി* സ്ഥിരം elementsp = nodep->elementsp()->unlinkFrBack();
        Ast_കണ്ണി* newp;
        എങ്കിൽ (VN_IS(elementsp, Unbounded)) {
            newp = പുതിയ AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, nullptr};
            VL_DO_DANGLING(elementsp->delete_വൃക്ഷം(), elementsp);
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം keyp = VN_CAST(elementsp, കണ്ണി_DType)) {
            newp = പുതിയ AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } അല്ലെങ്കിൽ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_വരി* സ്ഥിരം elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3ErrorCode::WIDTHEXPAND, സത്യം);
            // Must be expression that is constant, but we'll determine that later
            newp = പുതിയ AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                പുതിയ AstRange{nodep->fileline(), പുതിയ AstConst(elementsp->fileline(), 0),
                             പുതിയ AstSub{elementsNewFl, VN_AS(elementsp, കണ്ണി_വ്യഞ്ജകം),
                                        പുതിയ AstConst(elementsNewFl, 1)},
                             സത്യം}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, nullptr), newp);
    }
    ശൂന്യം visit(AstConstraintRefDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    ശൂന്യം visit(AstDynArrayDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstQueueDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        userIterateAndNext(nodep->boundp(), WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlinkFrBack()->delete_വൃക്ഷം();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstVoidDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstUnsizedArrayDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        എങ്കിൽ (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstWildcardArrayDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstBasicDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        എങ്കിൽ (nodep->generic()) തിരികെ;  // Already perfect
        എങ്കിൽ (nodep->rangep()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->widthForce(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->isRanged()) {
            nodep->widthForce(nodep->nrange().elements(), nodep->nrange().elements());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->implicit()) {
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
    ശൂന്യം visit(AstConstDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(AstRefDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(സത്യം);
        എങ്കിൽ (nodep->typeofp()) {  // type(typeofp_expression)
            എങ്കിൽ (Ast_കണ്ണി_DType* typeofDtp = VN_CAST(nodep->typeofp(), കണ്ണി_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } അല്ലെങ്കിൽ {
                // Type comes from expression's type, e.g. "type(variable)"
                userIterateAndNext(nodep->typeofp(), WidthVP{SELF, BOTH}.p());
                Ast_കണ്ണി* സ്ഥിരം typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlinkFrBack()->delete_വൃക്ഷം(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, nullptr);
        എങ്കിൽ (nodep->subDTypep()) {
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
        എങ്കിൽ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            സ്ഥിരം തർക്കം inTemplateModule = !m_modep || m_modep->parameterizedTemplate();
            എങ്കിൽ (inTemplateModule) {
                nodep->doingWidth(അസത്യം);
                തിരികെ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->widthFromSub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doingWidth(അസത്യം);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    ശൂന്യം widthDTypeNameParams(സ്ഥിരം AstTypedef* nodep) {
        സ്ഥിരം VDTypeNameScopes* സ്ഥിരം scopesp = VDTypeNameScopes::currentp();
        എങ്കിൽ (!scopesp) തിരികെ;  // Computing parameters, as by V3Param, so naming as found
        Ast_കണ്ണി* scopep = nullptr;
        വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            എങ്കിൽ (സ്ഥിരം std::vector<Ast_കണ്ണി*>* സ്ഥിരം paramsp = scopesp->paramsp(scopep)) {
                വേണ്ടി (Ast_കണ്ണി* സ്ഥിരം paramp : *paramsp) {
                    എങ്കിൽ (!paramp->didWidth()) userIterate(paramp, nullptr);
                }
            }
        }
    }
    ശൂന്യം visit(AstTypedef* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        എങ്കിൽ (auto* സ്ഥിരം refp = checkRefToTypedefRecurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(nullptr);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
            തിരികെ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        സ്ഥിരം തർക്കം declares = nodep->childDTypep() != nullptr;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        എങ്കിൽ (declares) {
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->dtypep();
            എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം sdtypep = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
                widthDTypeNameParams(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } അല്ലെങ്കിൽ എങ്കിൽ (AstEnumDType* സ്ഥിരം edtypep = VN_CAST(dtypep, EnumDType)) {
                widthDTypeNameParams(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, nullptr);
    }
    ശൂന്യം visit(AstParamTypeDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, nullptr);
        nodep->widthFromSub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        എങ്കിൽ (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(nullptr); }
    }
    ശൂന്യം visit(AstRequireDType* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം dtp = VN_CAST(nodep->lhsp(), കണ്ണി_DType)) {
            nodep->replaceWith(dtp->unlinkFrBack());
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(പുതിയ AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി_DType* സ്ഥിരം toDtp = nodep->മുകൾ()->dtypep()->skipRefToEnump();
        Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        സ്ഥിരം auto castable = Ast_കണ്ണി::computeCastable(toDtp, fromDtp, nodep->fromp());
        Ast_കണ്ണി* newp;
        എങ്കിൽ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* സ്ഥിരം enumDtp = VN_AS(toDtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം testp = enumTestValid(nodep->fromp(), enumDtp);
            File_വരി* സ്ഥിരം fl_novalue = പുതിയ File_വരി{fl};
            fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, സത്യം);
            newp = പുതിയ AstCond{
                fl, testp,
                പുതിയ Ast_വ്യഞ്ജകം_Stmt{fl,
                                പുതിയ AstAssign{fl_novalue, nodep->മുകൾ()->unlinkFrBack(),
                                              nodep->fromp()->unlinkFrBack()},
                                പുതിയ AstConst{fl, AstConst::Signed32{}, 1}},
                പുതിയ AstConst{fl, AstConst::Signed32{}, 0}};
        } അല്ലെങ്കിൽ എങ്കിൽ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(toDtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << toDtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warnMore() << "... Suggest static cast");
            newp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{
                fl,
                പുതിയ AstAssign{fl, nodep->മുകൾ()->unlinkFrBack(),
                              പുതിയ AstCast{fl, nodep->fromp()->unlinkFrBack(), toDtp}},
                പുതിയ AstConst{fl, AstConst::Signed32{}, 1}};
        } അല്ലെങ്കിൽ എങ്കിൽ (castable == VCastable::INCOMPATIBLE) {
            newp = പുതിയ AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << toDtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } അല്ലെങ്കിൽ {
            newp = പുതിയ AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << toDtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warnMore()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    ശൂന്യം visit(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlinkFrBack();
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstCastSize{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം refp = VN_CAST(nodep->dtp(), കണ്ണി_DType)) {
            refp->unlinkFrBack();
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstCast{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlinkFrBack());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstCast* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        എങ്കിൽ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            എങ്കിൽ (AstSigned* സ്ഥിരം fromp = VN_CAST(nodep->fromp(), Signed)) {
                എങ്കിൽ (VN_IS(fromp->lhsp(), കണ്ണി_Stream)) {
                    Ast_കണ്ണി* സ്ഥിരം lhsp = fromp->lhsp()->unlinkFrBack();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_വൃക്ഷം(), fromp);
                }
            }
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_കണ്ണി_DType* സ്ഥിരം toDtp = nodep->dtypep()->skipRefToEnump();
            Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
            സ്ഥിരം auto castable = Ast_കണ്ണി::computeCastable(toDtp, fromDtp, nodep->fromp());
            തർക്കം bad = അസത്യം;
            എങ്കിൽ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << toDtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } അല്ലെങ്കിൽ എങ്കിൽ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warnMore() << "... Suggest dynamic $cast");
                bad = സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << toDtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = സത്യം;
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
            എങ്കിൽ (bad) {
            } അല്ലെങ്കിൽ എങ്കിൽ (AstBasicDType* സ്ഥിരം basicp = toDtp->basicp()) {
                എങ്കിൽ (!basicp->isString() && fromDtp->isString()) {
                    newp = പുതിയ AstNToI{nodep->fileline(), nodep->fromp()->unlinkFrBack(), toDtp};
                } അല്ലെങ്കിൽ എങ്കിൽ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_കണ്ണി_DType* സ്ഥിരം origDTypep = nodep->dtypep();
                    എങ്കിൽ (!VN_IS(fromDtp, StreamDType)) {
                        സ്ഥിരം പൂർണ്ണസംഖ്യ width = toDtp->width();
                        castSized(nodep, nodep->fromp(), width);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } അല്ലെങ്കിൽ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 അസത്യം);
                }
                എങ്കിൽ (newp) {
                } അല്ലെങ്കിൽ എങ്കിൽ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    എങ്കിൽ (nodep->fromp()->isSigned()) {
                        newp = പുതിയ AstISToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    } അല്ലെങ്കിൽ {
                        newp = പുതിയ AstIToRD{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = പുതിയ AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                    newp->dtypep(basicp);
                } അല്ലെങ്കിൽ എങ്കിൽ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = പുതിയ AstSigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } അല്ലെങ്കിൽ എങ്കിൽ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = പുതിയ AstUnsigned{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
                } അല്ലെങ്കിൽ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toDtp, QueueDType)) {
                എങ്കിൽ (VN_IS(fromDtp, BasicDType)) {
                    newp = പുതിയ AstCvtPackedToArray{nodep->fileline(),
                                                   nodep->fromp()->unlinkFrBack(), toDtp};
                } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, StreamDType)) {
                    പൂർണ്ണസംഖ്യ srcElementBits = 1;
                    പൂർണ്ണസംഖ്യ dstElementBits = 1;
                    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = fromDtp->subDTypep()) {
                        srcElementBits = elemDtp->width();
                    }
                    സ്ഥിരം AstQueueDType* സ്ഥിരം dstQueueDtp = VN_AS(toDtp, QueueDType);
                    എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം elemDtp = dstQueueDtp->subDTypep()) {
                        dstElementBits = elemDtp->width();
                    }
                    newp = പുതിയ AstCvtArrayToArray{nodep->fileline(),
                                                  nodep->fromp()->unlinkFrBack(),
                                                  toDtp,
                                                  അസത്യം,
                                                  1,
                                                  dstElementBits,
                                                  srcElementBits};
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(toDtp, ClassRefDType)) {
                // Can just remove cast
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << toDtp->prettyDTypeNameQ());
            }
            എങ്കിൽ (!newp) newp = nodep->fromp()->unlinkFrBack();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        എങ്കിൽ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, അസത്യം, STREAM_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം underp = nodep->fromp()->unlinkFrBack();
            underp->dtypeFrom(nodep);
            underp->didWidth(സത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstCastWrap{nodep->fileline(), underp};
            newp->didWidth(സത്യം);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(AstCastSize* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            പൂർണ്ണസംഖ്യ width = nodep->rhsp()->toSInt();
            എങ്കിൽ (width < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << width);
                width = 1;
            }
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM, STREAM_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), width);  // lhsp may change
        }
        എങ്കിൽ (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_കണ്ണി* സ്ഥിരം underp = nodep->lhsp()->unlinkFrBack();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    ശൂന്യം visit(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->dtypep(), BOTH, STREAM_USE_CAST}.p());
    }
    ശൂന്യം castSized(Ast_കണ്ണി* nodep, Ast_കണ്ണി* underp, പൂർണ്ണസംഖ്യ width) {
        സ്ഥിരം AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        എങ്കിൽ (!underDtp) underDtp = underp->dtypep()->basicp();
        എങ്കിൽ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            സ്ഥിരം പൂർണ്ണസംഖ്യ calcWidth = std::max(width, underDtp->width());
            Ast_കണ്ണി_DType* സ്ഥിരം calcDtp = nodep->findBitOrLogicDType(
                calcWidth, calcWidth, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         അസത്യം, STREAM_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_കണ്ണി_DType* സ്ഥിരം outDtp = nodep->findBitOrLogicDType(
                width, width, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(outDtp);
            // We ignore warnings as that is sort of the point of a cast
            widthCheckSized(nodep, "Cast expr", VN_AS(underp, കണ്ണി_വ്യഞ്ജകം), outDtp, EXTEND_EXP,
                            അസത്യം);
            VL_DANGLING(underp);
        }
    }
    ശൂന്യം visit(AstConstraintBefore* nodep) override {
        userIterateAndNext(nodep->lhssp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->rhssp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(Ast_Constraint_വ്യഞ്ജകം* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstConstraintUnique* nodep) override {
        userIterateAndNext(nodep->rangesp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), WidthVP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    ശൂന്യം visit(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        എങ്കിൽ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            തള്ളുക_Deletep(nodep->valuep()->unlinkFrBack());
            nodep->valuep(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->didWidth(സത്യം);
            തിരികെ;
        }
        nodep->doingWidth(സത്യം);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        എങ്കിൽ (!m_paramsOnly && nodep->isIfaceArraySplit()) m_splitIfaceArrayVarps.തള്ളുക_back(nodep);
        എങ്കിൽ (nodep->attrsp()) {
            nodep->attrsp()->foreach([ഇത്, nodep](AstAttrOf* attrp) {
                എങ്കിൽ (attrp->attrType() == VAttrType::VAR_PORT_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    എങ്കിൽ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), കണ്ണി_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(തള്ളുക_Deletep(attrp->unlinkFrBack()), attrp);
                }
            });
        }
        എങ്കിൽ (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_കണ്ണി_DType* dtp = nodep->dtypep();
            Ast_കണ്ണി_DType* np = nullptr;
            ആകുമ്പോൾ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    എങ്കിൽ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } അല്ലെങ്കിൽ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* സ്ഥിരം newp
                        = പുതിയ AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    എങ്കിൽ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } അല്ലെങ്കിൽ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->addTypesp(newp);
                    np = newp;
                } അല്ലെങ്കിൽ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        എങ്കിൽ (AstWildcardArrayDType* സ്ഥിരം wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        എങ്കിൽ (VN_IS(nodep->dtypep()->skipRefToConstp(), ConstDType)) nodep->isConst(സത്യം);
        // Parameters if implicit untyped inherit from what they are assigned to
        സ്ഥിരം AstBasicDType* സ്ഥിരം bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        തർക്കം didchk = അസത്യം;
        സ്ഥിരം തർക്കം implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        എങ്കിൽ (implicitParam) {
            എങ്കിൽ (nodep->valuep()) {
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
                എങ്കിൽ (nodep->valuep()->isDouble()) {
                    nodep->dtypeSetDouble();
                    VL_DANGLING(bdtypep);
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->valuep()->isString()) {
                    nodep->dtypeSetString();
                    VL_DANGLING(bdtypep);
                } അല്ലെങ്കിൽ {
                    പൂർണ്ണസംഖ്യ width = 0;
                    Ast_കണ്ണി_DType* സ്ഥിരം മൂല്യം_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(മൂല്യം_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    സ്ഥിരം AstBasicDType* സ്ഥിരം മൂല്യം_Bdtypep = മൂല്യം_DTypep->basicp();
                    തർക്കം issigned = അസത്യം;
                    എങ്കിൽ (bdtypep->isNosign()) {
                        എങ്കിൽ (മൂല്യം_Bdtypep && മൂല്യം_Bdtypep->isSigned()) issigned = സത്യം;
                    } അല്ലെങ്കിൽ {
                        issigned = bdtypep->isSigned();
                    }
                    എങ്കിൽ (മൂല്യം_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        എങ്കിൽ (സ്ഥിരം auto* സ്ഥിരം constp = VN_CAST(nodep->valuep(), Const)) {
                            എങ്കിൽ (constp->അക്കം().isString()) {
                                width = constp->അക്കം().toString().length() * 8;
                            }
                        }
                        എങ്കിൽ (width < 8) width = 8;
                    } അല്ലെങ്കിൽ എങ്കിൽ (nodep->valuep()->dtypep()->widthSized()) {
                        width = nodep->valuep()->width();
                    } അല്ലെങ്കിൽ {
                        എങ്കിൽ (nodep->valuep()->width() > 32) {
                            nodep->valuep()->v3warn(
                                WIDTH,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        width = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    സ്ഥിരം തർക്കം is_മൂല്യം_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    എങ്കിൽ (width == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->widthMin(),
                            VSigning::fromBool(issigned), is_മൂല്യം_Fourstate));
                    } അല്ലെങ്കിൽ {
                        nodep->dtypeSetBitOrLogicUnsized(width, nodep->valuep()->widthMin(),
                                                         VSigning::fromBool(issigned),
                                                         is_മൂല്യം_Fourstate);
                    }
                    didchk = സത്യം;
                }
                iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } അല്ലെങ്കിൽ {
                // Or, if nothing assigned, they're integral
                nodep->dtypeSetInteger();
                VL_DANGLING(bdtypep);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtypeSetLogicSized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        എങ്കിൽ (nodep->isNet()) {
            Ast_കണ്ണി_DType* സ്ഥിരം badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            എങ്കിൽ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        എങ്കിൽ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        userIterateAndNext(nodep->delayp(), WidthVP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->didWidth(സത്യം);
        nodep->doingWidth(അസത്യം);
    }
    ശൂന്യം visit(Ast_കണ്ണി_Var_Ref* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        എങ്കിൽ (!nodep->varp()) {
            എങ്കിൽ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, അസത്യം,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        എങ്കിൽ (!nodep->varp()->didWidth()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), nullptr);
        }
        എങ്കിൽ (!m_paramsOnly && nodep->varp()->isIfaceArraySplit()) {
            replaceSplitIfaceArrayRef(nodep);
            തിരികെ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        എങ്കിൽ (VN_IS(nodep->backp(), കണ്ണി_Assign) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        എങ്കിൽ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(സത്യം);
        എങ്കിൽ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->access().isWriteOrRW() && nodep->varp()->is_നിവേശം()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), AssignForce) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            സ്ഥിരം തർക്കം hierRef = !m_curModVars.എണ്ണം(nodep->varp());
            സ്ഥിരം തർക്കം netPort
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::PORT;
            സ്ഥിരം തർക്കം contAssign = VN_IS(nodep->abovep(), AssignW);
            സ്ഥിരം തർക്കം pinConn = VN_IS(nodep->abovep(), Pin);
            എങ്കിൽ (!(hierRef && netPort && (contAssign || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        എങ്കിൽ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->didWidth(സത്യം);
    }

    ശൂന്യം visit(AstEnumDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_കണ്ണി_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_കണ്ണി_DType* സ്ഥിരം badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        എങ്കിൽ (badDtp) {
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
        V3_സംഖ്യ അക്കം(nodep, nodep->width(), 0);
        സ്ഥിരം V3_സംഖ്യ one{nodep, nodep->width(), 1};
        തർക്കം wrapAround = അസത്യം;
        std::map<സ്ഥിരം V3_സംഖ്യ, AstEnumItem*> inits;
        വേണ്ടി (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            എങ്കിൽ (itemp->valuep()) {
                എങ്കിൽ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_വൃക്ഷം("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                എങ്കിൽ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlinkFrBack()->delete_വൃക്ഷം();
                    തുടരുക;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = അസത്യം;  // Explicit value resets wrap-around tracking
            }
            എങ്കിൽ (!itemp->valuep()) {
                എങ്കിൽ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                എങ്കിൽ (അക്കം.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(പുതിയ AstConst{itemp->fileline(), അക്കം});
            }

            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(itemp->valuep(), Const);
            എങ്കിൽ (constp->അക്കം().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            അക്കം.opAssign(constp->അക്കം());
            // Look for duplicates
            സ്ഥിരം auto pair = inits.emplace(അക്കം, itemp);
            എങ്കിൽ (!pair.second) {  // IEEE says illegal
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            സ്ഥിരം തർക്കം prevNeg = constp->അക്കം().isNegative();
            അക്കം.opAdd(one, constp->അക്കം());
            എങ്കിൽ (basicp->isSigned()) {
                wrapAround = !prevNeg && അക്കം.isNegative();
            } അല്ലെങ്കിൽ {
                wrapAround = അക്കം.isEqZero();
            }
        }
    }
    ശൂന്യം visit(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        എങ്കിൽ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            userIterateAndNext(nodep->valuep(), WidthVP{CONTEXT_DET, PRELIM}.p());
            തർക്കം warnOn = സത്യം;
            Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nodep->valuep();
            എങ്കിൽ (സ്ഥിരം AstAdd* സ്ഥിരം anodep = VN_CAST(valuep, Add)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                എങ്കിൽ (anodep->fileline()->equal_First_വരി_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(valuep, Const)) {
                എങ്കിൽ (static_cast<പൂർണ്ണസംഖ്യ>(constp->അക്കം().mostSetBitP1()) > nodep->width()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = അസത്യം;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(പുതിയ AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlinkFrBack(),
                                      nodep->dtypep()});
        }
    }
    ശൂന്യം visit(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        എങ്കിൽ (!nodep->itemp()->didWidth()) {
            // We need to do the whole enum en masse
            Ast_കണ്ണി* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            വേണ്ടി (; enump; enump = enump->backp()) {
                എങ്കിൽ (VN_IS(enump, EnumDType)) നിർത്തുക;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Cons* nodep) override {}  // Widthed when created
    ശൂന്യം visit(Ast_കണ്ണി_Cons_Item* nodep) override {}  // Widthed when created
    ശൂന്യം visit(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        വേണ്ടി (Ast_കണ്ണി *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_കണ്ണി_DType* subDTypep = nullptr;
        nodep->dtypeSetBit();

        എങ്കിൽ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            subDTypep = nodep->findStringDType();
        } അല്ലെങ്കിൽ എങ്കിൽ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            subDTypep = nodep->findDoubleDType();
        } അല്ലെങ്കിൽ {
            // Take width as maximum across all items
            പൂർണ്ണസംഖ്യ width = nodep->exprp()->width();
            പൂർണ്ണസംഖ്യ mwidth = nodep->exprp()->widthMin();
            തർക്കം isFourstate = nodep->exprp()->dtypep()->isFourstate();
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                width, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        വേണ്ടി (Ast_കണ്ണി *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            എങ്കിൽ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{subDTypep, FINAL}.p());
            } അല്ലെങ്കിൽ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        എങ്കിൽ (m_constraintp) {
            തർക്കം canLower = സത്യം;
            വേണ്ടി (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                എങ്കിൽ (VN_IS(ditemp->rangep(), InsideRange)) തുടരുക;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                എങ്കിൽ (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->isPure()) {
                    canLower = അസത്യം;
                    നിർത്തുക;
                }
            }
            എങ്കിൽ (canLower) തിരികെ;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        വേണ്ടി (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* inewp
                = insideItem(nodep, nodep->exprp()->clone_വൃക്ഷം_Pure(അസത്യം), itemp->rangep());
            എങ്കിൽ (!inewp) തുടരുക;
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം cmpp
                = പുതിയ AstGt{itemp->fileline(), itemp->weightp()->unlinkFrBack(),
                            പുതിയ AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::UNSIGNED, സത്യം);
            cmpp->fileline()->modifyWarnOff(V3ErrorCode::CMPCONST, സത്യം);
            inewp = പുതിയ AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? പുതിയ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        എങ്കിൽ (!newp) newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    ശൂന്യം visit(AstInside* nodep) override {
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        വേണ്ടി (Ast_കണ്ണി *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, WidthVP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_കണ്ണി_DType* expDTypep = nullptr;

        എങ്കിൽ (dtype && dtype->isString()) {
            nodep->dtypeSetString();
            expDTypep = nodep->findStringDType();
        } അല്ലെങ്കിൽ എങ്കിൽ (dtype && dtype->isDouble()) {
            nodep->dtypeSetDouble();
            expDTypep = nodep->findDoubleDType();
        } അല്ലെങ്കിൽ {
            // Take width as maximum across all items
            പൂർണ്ണസംഖ്യ width = nodep->exprp()->width();
            പൂർണ്ണസംഖ്യ mwidth = nodep->exprp()->widthMin();
            തർക്കം isFourstate = nodep->exprp()->dtypep()->isFourstate();
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                width = std::max(width, itemp->width());
                mwidth = std::max(mwidth, itemp->widthMin());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtypeSetBit();
            സ്ഥിരം VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(width, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        വേണ്ടി (Ast_കണ്ണി *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            എങ്കിൽ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, WidthVP{expDTypep, FINAL}.p());
            } അല്ലെങ്കിൽ എങ്കിൽ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* exprp;
        Ast_വ്യഞ്ജകം_Stmt* വ്യഞ്ജകം_Stmtp = nullptr;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        എങ്കിൽ (!m_constraintp && !nodep->exprp()->isPure()) {
            File_വരി* സ്ഥിരം fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_കണ്ണി_DType* സ്ഥിരം വ്യഞ്ജകം_Dtp = nodep->exprp()->dtypep();
            സ്ഥിരം പൂർണ്ണസംഖ്യ w = വ്യഞ്ജകം_Dtp->width();
            Ast_കണ്ണി_DType* സ്ഥിരം tempDTypep
                = വ്യഞ്ജകം_Dtp->widthSized() ? വ്യഞ്ജകം_Dtp
                                        : nodep->findBitOrLogicDType(w, w, വ്യഞ്ജകം_Dtp->numeric(),
                                                                     വ്യഞ്ജകം_Dtp->isFourstate());
            AstVar* സ്ഥിരം varp
                = പുതിയ AstVar{fl, VVarType::XTEMP, m_insideTempNames.get(nodep), tempDTypep};
            exprp = പുതിയ AstVarRef{fl, varp, VAccess::READ};
            വ്യഞ്ജകം_Stmtp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{fl,
                                        പുതിയ AstAssign{fl, പുതിയ AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlinkFrBack()},
                                        exprp};
            എങ്കിൽ (m_ftaskp) {
                varp->funcLocal(സത്യം);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } അല്ലെങ്കിൽ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(വ്യഞ്ജകം_Stmtp);
        } അല്ലെങ്കിൽ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), കണ്ണി_വ്യഞ്ജകം);  // Will be unlinking
            Ast_കണ്ണി_വ്യഞ്ജകം* inewp;
            എങ്കിൽ (വ്യഞ്ജകം_Stmtp) {
                inewp = insideItem(nodep, വ്യഞ്ജകം_Stmtp, itemp);
                വ്യഞ്ജകം_Stmtp = nullptr;
            } അല്ലെങ്കിൽ {
                inewp = insideItem(nodep, exprp->clone_വൃക്ഷം_Pure(അസത്യം), itemp);
            }
            എങ്കിൽ (!inewp) തുടരുക;
            newp = newp ? പുതിയ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        എങ്കിൽ (വ്യഞ്ജകം_Stmtp) VL_DO_DANGLING(വ്യഞ്ജകം_Stmtp->delete_വൃക്ഷം(), വ്യഞ്ജകം_Stmtp);
        എങ്കിൽ (!newp) newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* insideItem(Ast_കണ്ണി* nodep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_വ്യഞ്ജകം* itemp) {
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം itemDtp = itemp->dtypep()->skipRefp();
        എങ്കിൽ (AstInsideRange* സ്ഥിരം irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            തിരികെ irangep->newAndFromInside(exprp, irangep->lhsp()->unlinkFrBack(),
                                             irangep->rhsp()->unlinkFrBack());
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം inewp = പുതിയ AstCMethodHard{nodep->fileline(), itemp->unlinkFrBack(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtypeSetBit();
            inewp->didWidth(സത്യം);
            തിരികെ inewp;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_വൃക്ഷം(), exprp);
            തിരികെ nullptr;
        }
        തിരികെ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlinkFrBack());
    }
    ശൂന്യം visit(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        എങ്കിൽ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (m_vup->prelim()) {
                userIterateAndNext(nodep->lhsp(), m_vup);
                userIterateAndNext(nodep->rhsp(), m_vup);
            }
            എങ്കിൽ (m_vup->final()) {
                Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                സ്ഥിരം പൂർണ്ണസംഖ്യ expWidth = expDTypep->width();
                സ്ഥിരം തർക്കം waiveLhs = expWidth == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->widthMin() >= nodep->lhsp()->width();
                സ്ഥിരം തർക്കം waiveRhs = expWidth == 32
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

    ശൂന്യം visit(AstIfaceRefDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        എങ്കിൽ (nodep->isVirtual()) nodep->ifaceViaCellp()->setHasVirtualRef();
        UINFO(4, "dtWidthed " << nodep);
    }
    ശൂന്യം visit(Ast_കണ്ണി_UOr_Struct_DType* nodep) override {
        എങ്കിൽ (nodep->doingWidth()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtypeSetBit();
            nodep->doingWidth(അസത്യം);
            തിരികെ;
        }
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doingWidth(സത്യം);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        എങ്കിൽ (സ്ഥിരം AstUnionDType* സ്ഥിരം unionp = VN_CAST(nodep, UnionDType)) {
            എങ്കിൽ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, nullptr);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(അസത്യം);
        // Error checks
        വേണ്ടി (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_കണ്ണി_DType* സ്ഥിരം dtp = itemp->subDTypep()->skipRefp();
            എങ്കിൽ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            എങ്കിൽ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                തള്ളുക_Deletep(itemp->valuep()->unlinkFrBack());
            }
        }
        സ്ഥിരം തർക്കം isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        സ്ഥിരം തർക്കം inTemplateModule = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        എങ്കിൽ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            പൂർണ്ണസംഖ്യ lsb = 0;
            പൂർണ്ണസംഖ്യ width = 0;
            തർക്കം first = സത്യം;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            വേണ്ടി (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            വേണ്ടി (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                എങ്കിൽ (itemp->isFourstate()) nodep->isFourstate(സത്യം);
                itemp->lsb(lsb);
                എങ്കിൽ (VN_IS(nodep, UnionDType)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ itemWidth = itemp->width();
                    // Skip union size check for template modules with unresolved parameters
                    എങ്കിൽ (!first && isHardPackedUnion && itemWidth != width && !inTemplateModule) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    width = std::max(width, itemWidth);
                } അല്ലെങ്കിൽ {
                    lsb += itemp->width();
                    width += itemp->width();
                }
                first = അസത്യം;
            }
            nodep->widthForce(width, width);  // Signing stays as-is, as parsed from declaration
        } അല്ലെങ്കിൽ {
            nodep->widthForce(1, 1);
        }
        nodep->doingWidth(അസത്യം);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    ശൂന്യം visit(AstThisRef* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        എങ്കിൽ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    ശൂന്യം visit(AstClassRefDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    ശൂന്യം visit(AstClassOrPackageRef* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        userIterateChildren(nodep, nullptr);
    }
    ശൂന്യം visit(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warnMore() << "... Suggest use a typedef");
        nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    ശൂന്യം visit(AstClassExtends* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        എങ്കിൽ (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    ശൂന്യം visit(AstMemberDType* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->widthFromSub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        എങ്കിൽ (nodep->valuep()) {
            userIterateAndNext(nodep->valuep(), WidthVP{nodep->dtypep(), PRELIM}.p());
            iterateCheckAssign(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    ശൂന്യം visit(AstStructSel* nodep) override {
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = സത്യം;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        UINFO(9, "     from dt " << fromDtp);
        സ്ഥിരം AstMemberSel* സ്ഥിരം fromSel = VN_CAST(nodep->fromp(), MemberSel);
        എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : nullptr) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            എങ്കിൽ (AstVar* സ്ഥിരം varp = memberSelClocking(nodep, clockingp)) {
                എങ്കിൽ (!varp->didWidth()) userIterate(varp, nullptr);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlinkFrBack());
                VL_DO_DANGLING(fromp->delete_വൃക്ഷം(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                എങ്കിൽ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::linkLValueSet(nodep, nodep->access());
                }
                എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->didWidth(സത്യം);  // Must not visit again: will confuse scopes
                തിരികെ;
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം adtypep = VN_CAST(fromDtp, കണ്ണി_UOr_Struct_DType)) {
            എങ്കിൽ (memberSelStruct(nodep, adtypep)) തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassRefDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            എങ്കിൽ (memberSelClass(nodep, adtypep)) തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            എങ്കിൽ (Ast_കണ്ണി* foundp = memberSelIface(nodep, adtypep)) {
                എങ്കിൽ (AstClocking* സ്ഥിരം clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(foundp, Var)) {
                    എങ്കിൽ (!varp->didWidth()) userIterate(varp, nullptr);
                    AstIface* സ്ഥിരം ifacep = adtypep->ifaceViaCellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep->fromp(), VarRef);
                    എങ്കിൽ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* സ്ഥിരം newp = പുതിയ AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->didWidth(സത്യം);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                        തിരികെ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(സത്യം);
                    തിരികെ;
                }
                എങ്കിൽ (AstModport* സ്ഥിരം modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* സ്ഥിരം newDtypep = പുതിയ AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = nodep->fromp()->unlinkFrBack();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
                എങ്കിൽ (AstCell* സ്ഥിരം cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    എങ്കിൽ (VN_IS(cellp->modp(), Iface)) {
                        സ്ഥിരം string viftopName = cellp->name() + "__Viftop";
                        Ast_കണ്ണി_Module* സ്ഥിരം parentIfacep = adtypep->ifaceViaCellp();
                        AstVar* viftopVarp = nullptr;
                        വേണ്ടി (Ast_കണ്ണി* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            എങ്കിൽ (AstVar* സ്ഥിരം vp = VN_CAST(itemp, Var)) {
                                എങ്കിൽ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    നിർത്തുക;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        എങ്കിൽ (!viftopVarp->didWidth()) userIterate(viftopVarp, nullptr);
                        Ast_കണ്ണി_DType* subDtypep = viftopVarp->dtypep();
                        എങ്കിൽ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* സ്ഥിരം newDtypep = പുതിയ AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(സത്യം);
                                v3Global.rootp()->typeTablep()->addTypesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->didWidth(സത്യം);
                        തിരികെ;
                    }
                }
                എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp = VN_CAST(foundp, കണ്ണി_FTask)) {
                    AstMethodCall* സ്ഥിരം newp = പുതിയ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    തിരികെ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstMethodCall{nodep->fileline(),
                                                    nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            തിരികെ;
        } അല്ലെങ്കിൽ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }
    തർക്കം memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        എങ്കിൽ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            AstMethodCall* സ്ഥിരം newp = പുതിയ AstMethodCall{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            visit(newp);
            തിരികെ സത്യം;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* സ്ഥിരം first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        വേണ്ടി (AstClass* classp = first_classp; classp;) {
            എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം foundp = m_memberMap.findMember(classp, nodep->name())) {
                എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(foundp, Var)) {
                    എങ്കിൽ (!varp->didWidth()) userIterate(varp, nullptr);
                    എങ്കിൽ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* സ്ഥിരം varRefp
                            = പുതിയ AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                        തിരികെ സത്യം;
                    }
                    എങ്കിൽ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->didWidth(സത്യം);
                    എങ്കിൽ (nodep->fromp()->same_വൃക്ഷം(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::linkLValueSet(nodep);
                    തിരികെ സത്യം;
                }
                എങ്കിൽ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(പുതിയ AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), constrp});
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ സത്യം;
                }
                എങ്കിൽ (AstEnumItemRef* സ്ഥിരം adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_വൃക്ഷം(അസത്യം));
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ സത്യം;
                }
                എങ്കിൽ (Ast_കണ്ണി_FTask* ftaskp = VN_CAST(foundp, കണ്ണി_FTask)) {
                    AstMethodCall* newp = പുതിയ AstMethodCall{
                        nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ സത്യം;
                }
                എങ്കിൽ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    തിരികെ സത്യം;
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
        വേണ്ടി (AstClass* classp = first_classp; classp;) {
            വേണ്ടി (Ast_കണ്ണി* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.തള്ളുക_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        തിരികെ അസത്യം;  // Caller handles error
    }
    തർക്കം modportAllowsItem(Ast_കണ്ണി* nodep, സ്ഥിരം AstIfaceRefDType* adtypep, സ്ഥിരം Ast_കണ്ണി* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        സ്ഥിരം AstModport* സ്ഥിരം modportp = adtypep->modportp();
        എങ്കിൽ (!modportp) തിരികെ സത്യം;
        എങ്കിൽ (സ്ഥിരം AstVar* സ്ഥിരം varp = VN_CAST(itemp, Var)) {
            എങ്കിൽ (varp->isParam()) തിരികെ സത്യം;
        } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(itemp, കണ്ണി_FTask) && !VN_IS(itemp, Clocking)) {
            തിരികെ സത്യം;
        }
        auto pair = m_modportNames.emplace(modportp, std::unordered_set<string>{});
        എങ്കിൽ (pair.second) {
            വേണ്ടി (സ്ഥിരം Ast_കണ്ണി* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.തിരുകുക(mpItemp->name());
            }
        }
        എങ്കിൽ (pair.first->second.എണ്ണം(itemp->name())) തിരികെ സത്യം;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->ifaceViaCellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        തിരികെ അസത്യം;
    }
    Ast_കണ്ണി* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_കണ്ണി_Module* സ്ഥിരം ifacep = adtypep->ifaceViaCellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        വേണ്ടി (Ast_കണ്ണി* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (itemp->name() == nodep->name()) {
                എങ്കിൽ (!modportAllowsItem(nodep, adtypep, itemp)) {
                    തിരികെ nullptr;  // Caller handles error
                }
                തിരികെ itemp;
            }
            എങ്കിൽ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.തള്ളുക_Candidate(itemp->prettyName());
            }
        }
        സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        തിരികെ nullptr;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        വേണ്ടി (Ast_കണ്ണി* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            എങ്കിൽ (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                എങ്കിൽ (citemp->varp()->name() == nodep->name()) തിരികെ citemp->varp();
                speller.തള്ളുക_Candidate(citemp->varp()->prettyName());
            }
        }
        സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        തിരികെ nullptr;  // Caller handles error
    }
    തർക്കം memberSelStruct(AstMemberSel* nodep, Ast_കണ്ണി_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        എങ്കിൽ (AstMemberDType* സ്ഥിരം memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            എങ്കിൽ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } അല്ലെങ്കിൽ എങ്കിൽ (adtypep->packed()) {
                AstSel* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                                memberp->lsb(), memberp->width()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(സത്യം);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } അല്ലെങ്കിൽ {
                AstStructSel* സ്ഥിരം newp = പുതിയ AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlinkFrBack(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skipRefToEnump());
                newp->didWidth(സത്യം);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            തിരികെ സത്യം;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        തിരികെ അസത്യം;
    }

    ശൂന്യം visit(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    ശൂന്യം visit(AstMethodCall* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), WidthVP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_കണ്ണി_DType* സ്ഥിരം fromDtp = nodep->fromp()->dtypep()->skipRefToEnump();
        AstBasicDType* സ്ഥിരം basicp = fromDtp ? fromDtp->basicp() : nullptr;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (nodep->name() == "rand_mode") {
            methodCallRandMode(nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "constraint_mode") {
            methodCallConstraint(nodep, nullptr);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstEnumDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, EnumDType)) {
            methodCallEnum(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstAssocArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            methodCallAssoc(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstWildcardArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            methodCallWildcard(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstDynArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            methodCallDyn(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstQueueDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, QueueDType)) {
            methodCallQueue(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstClassRefDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            methodCallClass(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstIfaceRefDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            methodCallIfaceRef(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            methodCallUnpack(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (AstConstraintRefDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            methodCallConstraint(nodep, adtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isEvent()) {
            methodCallEvent(nodep, basicp);
        } അല്ലെങ്കിൽ എങ്കിൽ (basicp && basicp->isString()) {
            methodCallString(nodep, basicp);
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtypeSetVoid();
        }
    }
    AstWith* methodWithClause(Ast_കണ്ണി_FTask_Ref* nodep, തർക്കം required, തർക്കം arbReturn,
                              Ast_കണ്ണി_DType* returnDtp, Ast_കണ്ണി_DType* indexDtp,
                              Ast_കണ്ണി_DType* മൂല്യം_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        എങ്കിൽ (AstWith* സ്ഥിരം withp = nodep->withp()) {
            withp->indexArgRefp()->dtypep(indexDtp);
            withp->മൂല്യം_Arg_Refp()->dtypep(മൂല്യം_Dtp);
            userIterate(withp, WidthVP{returnDtp, BOTH}.p());
            withp->unlinkFrBack();
            തിരികെ withp;
        }
        എങ്കിൽ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        തിരികെ nullptr;
    }
    ശൂന്യം methodOkArguments(Ast_കണ്ണി_FTask_Ref* nodep, പൂർണ്ണസംഖ്യ minArg, പൂർണ്ണസംഖ്യ maxArg) {
        പൂർണ്ണസംഖ്യ narg = 0;
        എങ്കിൽ (AstWith* സ്ഥിരം withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(തള്ളുക_Deletep(withp->unlinkFrBack()), withp);
        }
        വേണ്ടി (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        സ്ഥിരം തർക്കം ok = (narg >= minArg) && (narg <= maxArg);
        എങ്കിൽ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvtToStr(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvtToStr(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            വേണ്ടി (; narg < minArg; ++narg) {
                nodep->addArgsp(
                    പുതിയ AstArg{nodep->fileline(), "",
                               പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            വേണ്ടി (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ആകുമ്പോൾ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlinkFrBack();
                VL_DO_DANGLING(argp->delete_വൃക്ഷം(), argp);
            }
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* methodArg(AstMethodCall* nodep, പൂർണ്ണസംഖ്യ arg) {
        AstArg* argp = nodep->argsp();
        വേണ്ടി (പൂർണ്ണസംഖ്യ narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        തിരികെ argp->exprp();
    }

    ശൂന്യം methodCallEnum(AstMethodCall* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        എങ്കിൽ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = nullptr;
            methodOkArguments(nodep, 0, 0);
            എങ്കിൽ (nodep->name() == "num") {
                പൂർണ്ണസംഖ്യ ഇനങ്ങൾ = 0;
                വേണ്ടി (Ast_കണ്ണി* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++ഇനങ്ങൾ;
                newp = പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{}, ഇനങ്ങൾ);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "first") {
                സ്ഥിരം AstEnumItem* itemp = adtypep->itemsp();
                എങ്കിൽ (!itemp) {
                    newp = പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } അല്ലെങ്കിൽ {
                    newp = VN_AS(itemp->valuep()->clone_വൃക്ഷം(അസത്യം), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "last") {
                സ്ഥിരം AstEnumItem* itemp = adtypep->itemsp();
                ആകുമ്പോൾ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                എങ്കിൽ (!itemp) {
                    newp = പുതിയ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } അല്ലെങ്കിൽ {
                    newp = VN_AS(itemp->valuep()->clone_വൃക്ഷം(അസത്യം), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            എങ്കിൽ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                methodOkArguments(nodep, 0, 0);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_NEXT;
                methodOkArguments(nodep, 0, 1);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                methodOkArguments(nodep, 0, 1);
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Bad case");
            }

            എങ്കിൽ (nodep->name() != "name" && nodep->argsp()) {
                Ast_കണ്ണി_വ്യഞ്ജകം* stepp = methodArg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = methodArg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHEXPAND, സത്യം);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                എങ്കിൽ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(stepp), stepp);
                    stepp = newp;
                }
                സ്ഥിരം uint32_t stepWidth = VN_AS(stepp, Const)->toUInt();
                എങ്കിൽ (stepWidth == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    AstMethodCall* സ്ഥിരം newp = പുതിയ AstMethodCall{
                        nodep->fileline(),
                        പുതിയ AstMethodCall{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                    തിരികെ;
                } അല്ലെങ്കിൽ എങ്കിൽ (stepWidth != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    തള്ളുക_Deletep(nodep->argsp()->unlinkFrBack());
                    AstMethodCall* സ്ഥിരം clonep = nodep->clone_വൃക്ഷം(അസത്യം);
                    VN_AS(stepp, Const)->അക്കം().setLong(1);
                    AstConst* സ്ഥിരം constp = പുതിയ AstConst(nodep->fileline(), stepWidth - 1);
                    AstArg* സ്ഥിരം argp = പുതിയ AstArg{nodep->fileline(), "", constp};
                    AstMethodCall* സ്ഥിരം newp
                        = പുതിയ AstMethodCall{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                    തിരികെ;
                }
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                = enumSelect(nodep->fromp()->unlinkFrBack(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    ശൂന്യം methodCallWildcard(AstMethodCall* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        എങ്കിൽ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Wildcard_Index_വ്യഞ്ജകം(nodep, adtypep);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            എങ്കിൽ (!nodep->argsp()) {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } അല്ലെങ്കിൽ {
                iterateCheckSelf(nodep, "argument", methodArg(nodep, 0), SELF, BOTH);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Wildcard_Index_വ്യഞ്ജകം(nodep, adtypep);
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, സത്യം, അസത്യം, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "map") {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, സത്യം, അസത്യം, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        എങ്കിൽ (newp) {
            newp->protect(അസത്യം);
            newp->didWidth(സത്യം);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    ശൂന്യം methodCallAssoc(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        എങ്കിൽ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        എങ്കിൽ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_SIZE};  // So don't need num()
            newp->dtypeSetInt();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->താക്കോൽ_DTypep(), BOTH);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Assoc_Index_വ്യഞ്ജകം(nodep, adtypep);
            methodCallLValueRecurse(nodep, index_exprp, VAccess::READWRITE);
            newp
                = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                     VCMethod::arrayMethod(nodep->name()),  // first/last/next/prev
                                     index_exprp->unlinkFrBack()};
            newp->dtypeSetInt();
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->താക്കോൽ_DTypep(), BOTH);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Assoc_Index_വ്യഞ്ജകം(nodep, adtypep);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ASSOC_EXISTS, index_exprp->unlinkFrBack()};
            newp->dtypeSetBitUnsized(32, 1, VSigning::SIGNED);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            എങ്കിൽ (!nodep->argsp()) {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtypeSetVoid();
            } അല്ലെങ്കിൽ {
                iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->താക്കോൽ_DTypep(),
                                  BOTH);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Assoc_Index_വ്യഞ്ജകം(nodep, adtypep);
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::ASSOC_ERASE, index_exprp->unlinkFrBack()};
                newp->dtypeSetVoid();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* സ്ഥിരം withp = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->subDTypep(),
                                                    adtypep->താക്കോൽ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* സ്ഥിരം withp = methodWithClause(
                nodep, അസത്യം, സത്യം, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            എങ്കിൽ (nodep->name() == "unique_index") {
                newp->dtypep(queueDTypeIndexedBy(adtypep->താക്കോൽ_DTypep()));
            } അല്ലെങ്കിൽ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* സ്ഥിരം withp = methodWithClause(nodep, സത്യം, അസത്യം, nodep->findBitDType(),
                                                    adtypep->താക്കോൽ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* സ്ഥിരം withp = methodWithClause(nodep, സത്യം, അസത്യം, nodep->findBitDType(),
                                                    adtypep->താക്കോൽ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->താക്കോൽ_DTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "map") {
            AstWith* സ്ഥിരം withp = methodWithClause(nodep, സത്യം, അസത്യം, adtypep->subDTypep(),
                                                    adtypep->താക്കോൽ_DTypep(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        എങ്കിൽ (newp) {
            newp->protect(അസത്യം);
            newp->didWidth(സത്യം);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* method_Call_Assoc_Index_വ്യഞ്ജകം(AstMethodCall* nodep, AstAssocArrayDType* adtypep) {
        Ast_കണ്ണി* സ്ഥിരം index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->താക്കോൽ_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        തിരികെ nodep->argsp()->exprp();
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* method_Call_Wildcard_Index_വ്യഞ്ജകം(AstMethodCall* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_കണ്ണി* സ്ഥിരം index_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", index_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(index_exprp);  // May have been edited
        തിരികെ nodep->argsp()->exprp();
    }
    ശൂന്യം methodCallLValueRecurse(AstMethodCall* nodep, Ast_കണ്ണി* childp, സ്ഥിരം VAccess& access) {
        എങ്കിൽ (AstCMethodHard* സ്ഥിരം ichildp = VN_CAST(childp, CMethodHard)) {
            തർക്കം methAt = അസത്യം;
            തർക്കം methAtBack = അസത്യം;
            തർക്കം methAtWrite = അസത്യം;
            തിരഞ്ഞെടുപ്പ് (ichildp->method()) {
            അവസ്ഥ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = സത്യം;
                നിർത്തുക;
            അവസ്ഥ VCMethod::ARRAY_AT_BACK:  // FALLTHRU
                methAtBack = സത്യം;
                നിർത്തുക;
            അവസ്ഥ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            അവസ്ഥ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            അവസ്ഥ VCMethod::DYN_AT_WRITE_APPEND_BACK:  //
                methAtWrite = സത്യം;
                നിർത്തുക;
            സ്ഥിരസ്ഥിതി: നിർത്തുക;
            }
            എങ്കിൽ (methAt || methAtBack || methAtWrite) {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                എങ്കിൽ (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    എങ്കിൽ (methAt) {
                        ichildp->method(VCMethod::ARRAY_AT_WRITE);
                    } അല്ലെങ്കിൽ എങ്കിൽ (methAtBack) {
                        ichildp->method(VCMethod::DYN_AT_WRITE_APPEND_BACK);
                    }
                }
                methodCallLValueRecurse(nodep, ichildp->fromp(), access);
                തിരികെ;
            }
        }
        എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(childp, കണ്ണി_Var_Ref)) {
            varrefp->access(access);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstMemberSel* സ്ഥിരം ichildp = VN_CAST(childp, MemberSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം ichildp = VN_CAST(childp, StructSel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Sel* സ്ഥിരം ichildp = VN_CAST(childp, കണ്ണി_Sel)) {
            methodCallLValueRecurse(nodep, ichildp->fromp(), access);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } അല്ലെങ്കിൽ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* methodCallArray(AstMethodCall* nodep, Ast_കണ്ണി_DType* adtypep) {
        AstCMethodHard* newp = nullptr;

        എങ്കിൽ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = nullptr;
            എങ്കിൽ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = methodWithClause(nodep, അസത്യം, സത്യം, nullptr, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* സ്ഥിരം withp = methodWithClause(
                nodep, അസത്യം, സത്യം, nullptr, nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            എങ്കിൽ (nodep->name() == "unique_index") {
                newp->dtypep(newp->findQueueIndexDType());
            } അല്ലെങ്കിൽ {
                newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            }
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, സത്യം, അസത്യം, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, സത്യം, അസത്യം, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->findQueueIndexDType());
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, സത്യം, അസത്യം, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queueDTypeIndexedBy(withp ? withp->dtypep() : adtypep->subDTypep()));
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        }
        തിരികെ newp;
    }
    ശൂന്യം methodCallDyn(AstMethodCall* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        എങ്കിൽ (nodep->name() == "at") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atWrite") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "delete") {  // function void delete()
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_CLEAR};
            newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ ((newp = methodCallArray(nodep, adtypep))) {
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        എങ്കിൽ (newp) {
            newp->protect(അസത്യം);
            newp->didWidth(സത്യം);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    ശൂന്യം methodCallQueue(AstMethodCall* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = nullptr;
        എങ്കിൽ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            methodOkArguments(nodep, 0, 0);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::DYN_SIZE};
            newp->dtypeSetInt();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "delete") {  // function void delete([input integer index])
            methodOkArguments(nodep, 0, 1);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            എങ്കിൽ (!nodep->argsp()) {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_CLEAR};
                newp->dtypeSetVoid();
            } അല്ലെങ്കിൽ {
                iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Queue_Index_വ്യഞ്ജകം(nodep);
                എങ്കിൽ (index_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_എടുക്കുക_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtypeSetVoid();
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                              VCMethod::DYN_ERASE, index_exprp->unlinkFrBack()};
                    newp->dtypeSetVoid();
                }
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "insert") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം index_exprp = method_Call_Queue_Index_വ്യഞ്ജകം(nodep);
            AstArg* സ്ഥിരം argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            എങ്കിൽ (index_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_തള്ളുക_FRONT, argp->exprp()->unlinkFrBack()};
                newp->dtypeSetVoid();
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                          VCMethod::DYN_തിരുകുക, index_exprp->unlinkFrBack()};
                newp->addPinsp(argp->exprp()->unlinkFrBack());
                newp->dtypeSetVoid();
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            methodOkArguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", methodArg(nodep, 0), adtypep->subDTypep(), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* സ്ഥിരം argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod(nodep->name()),
                                      argp->exprp()->unlinkFrBack()};
            newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                      VCMethod::arrayMethod("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ ((newp = methodCallArray(nodep, adtypep))) {
        } അല്ലെങ്കിൽ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        എങ്കിൽ (newp) {
            newp->protect(അസത്യം);
            newp->didWidth(സത്യം);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* method_Call_Queue_Index_വ്യഞ്ജകം(AstMethodCall* nodep) {
        Ast_കണ്ണി* സ്ഥിരം index_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", index_exprp, BOTH);
        VL_DANGLING(index_exprp);  // May have been edited
        തിരികെ nodep->argsp()->exprp();
    }
    ശൂന്യം methodCallWarnTiming(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം nodep, സ്ഥിരം std::string& className) {
        എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } അല്ലെങ്കിൽ എങ്കിൽ (!v3Global.opt.timing().isSetTrue()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    ശൂന്യം methodCallIfaceRef(AstMethodCall* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* സ്ഥിരം ifacep = adtypep->ifaceViaCellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), കണ്ണി_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            എങ്കിൽ (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtypeSetVoid();
                തിരികെ;
            }
            എങ്കിൽ (adtypep->isVirtual()) {
                വേണ്ടി (Ast_കണ്ണി* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    എങ്കിൽ (AstVar* സ്ഥിരം mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, nullptr);
            എങ്കിൽ (ftaskp->isStatic()) {
                AstArg* സ്ഥിരം argsp = nodep->argsp();
                എങ്കിൽ (argsp) argsp->unlinkFrBackWithNext();
                Ast_കണ്ണി_FTask_Ref* newp = nullptr;
                എങ്കിൽ (VN_IS(ftaskp, Task)) {
                    newp = പുതിയ AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } അല്ലെങ്കിൽ {
                    newp = പുതിയ AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            } അല്ലെങ്കിൽ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                എങ്കിൽ (VN_IS(ftaskp, Task)) nodep->dtypeSetVoid();
                processFTaskRefArgs(nodep);
            }
            തിരികെ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    ശൂന്യം handleRandomizeArgs(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം nodep, AstClass* സ്ഥിരം classp) {
        തർക്കം hasNonNullArgs = അസത്യം;
        AstConst* nullp = nullptr;
        വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = nullptr;
            Ast_കണ്ണി_വ്യഞ്ജകം* exprp = argp->exprp();
            എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(exprp, Const)) {
                എങ്കിൽ (constp->അക്കം().isNull()) {
                    nullp = constp;
                    തുടരുക;
                }
            }
            hasNonNullArgs = സത്യം;
            AstVar* fromVarp = nullptr;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            എങ്കിൽ (AstMethodCall* methodCallp = VN_CAST(nodep, MethodCall)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* fromp = methodCallp->fromp();
                ആകുമ്പോൾ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* സ്ഥിരം varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            എങ്കിൽ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
                തുടരുക;
            }
            ആകുമ്പോൾ (exprp) {
                എങ്കിൽ (AstMemberSel* സ്ഥിരം memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (AstVarRef* സ്ഥിരം varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } അല്ലെങ്കിൽ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
                    }
                    exprp = nullptr;
                }
                // All variables in the dot hierarchy must be randomizable
                എങ്കിൽ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            എങ്കിൽ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } അല്ലെങ്കിൽ എങ്കിൽ (classp->existsMember([&](സ്ഥിരം AstClass*, സ്ഥിരം AstVar* memberVarp) {
                           തിരികെ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } അല്ലെങ്കിൽ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlinkFrBack()->delete_വൃക്ഷം(), argp);
            }
        }
        എങ്കിൽ (nullp && hasNonNullArgs) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    ശൂന്യം methodCallClass(AstMethodCall* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* സ്ഥിരം first_classp = adtypep->classp();
        AstWith* withp = nullptr;
        എങ്കിൽ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            വേണ്ടി (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                എങ്കിൽ (argp->exprp()) userIterate(argp->exprp(), WidthVP{SELF, BOTH}.p());
            }
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handleRandomizeArgs(nodep, first_classp);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "srandom") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        വേണ്ടി (AstClass* classp = first_classp; classp;) {
            എങ്കിൽ (nodep->fileline()->timingOn()) {
                എങ്കിൽ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* സ്ഥിരം packagep = getItemPackage(classp);
                    // Check if it's std
                    എങ്കിൽ (packagep && packagep->name() == "std") {
                        എങ്കിൽ (classp->name() == "process") {
                            methodCallWarnTiming(nodep, "process");
                        } അല്ലെങ്കിൽ എങ്കിൽ (classp->name() == "semaphore" && nodep->name() == "get") {
                            methodCallWarnTiming(nodep, "semaphore");
                        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            methodCallWarnTiming(nodep, "mailbox");
                        }
                    }
                }
            }
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), കണ്ണി_FTask)) {
                userIterate(ftaskp, nullptr);
                എങ്കിൽ (ftaskp->isStatic()) {
                    AstArg* സ്ഥിരം argsp = nodep->argsp();
                    എങ്കിൽ (argsp) argsp->unlinkFrBackWithNext();
                    Ast_കണ്ണി_FTask_Ref* newp = nullptr;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    എങ്കിൽ (!m_vup) {  // Called as task
                        newp = പുതിയ AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } അല്ലെങ്കിൽ {
                        newp = പുതിയ AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                } അല്ലെങ്കിൽ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    എങ്കിൽ (VN_IS(ftaskp, Task)) {
                        എങ്കിൽ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtypeSetVoid();
                    }
                    എങ്കിൽ (withp) nodep->withp(withp);
                    processFTaskRefArgs(nodep);
                }
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                first_classp->baseMostClassp()->needRNG(സത്യം);
                v3Global.useRandomizeMethods(സത്യം);
                AstCMethodHard* സ്ഥിരം newp
                    = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_GET_RANDSTATE, nullptr};
                newp->usePtr(സത്യം);
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->argsp()->exprp();
                first_classp->baseMostClassp()->needRNG(സത്യം);
                v3Global.useRandomizeMethods(സത്യം);
                AstCMethodHard* സ്ഥിരം newp
                    = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::RNG_SET_RANDSTATE, exprp->unlinkFrBack()};
                newp->usePtr(സത്യം);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(സത്യം);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
        }
        {
            VSpellCheck speller;
            വേണ്ടി (AstClass* classp = first_classp; classp;) {
                വേണ്ടി (Ast_കണ്ണി* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    എങ്കിൽ (VN_IS(itemp, കണ്ണി_FTask)) speller.തള്ളുക_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr;
            }
            സ്ഥിരം string suggest = speller.best_Candidate_സന്ദേശം(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.empty() ? "" : nodep->fileline()->warnMore() + suggest));
        }
        nodep->dtypeSetInt();  // Guess on error
    }
    ശൂന്യം methodCallConstraint(AstMethodCall* nodep, AstConstraintRefDType*) {
        എങ്കിൽ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            methodOkArguments(nodep, 0, 1);
            എങ്കിൽ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } അല്ലെങ്കിൽ {
                nodep->dtypeSetVoid();
            }
            v3Global.useRandomizeMethods(സത്യം);
        } അല്ലെങ്കിൽ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം methodCallRandMode(AstMethodCall* nodep) {
        methodOkArguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        എങ്കിൽ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", methodArg(nodep, 0), BOTH);
            nodep->dtypeSetVoid();
        } അല്ലെങ്കിൽ {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(സത്യം);
    }
    ശൂന്യം methodCallUnpack(AstMethodCall* nodep, AstUnpackArrayDType* adtypep) {
        എണ്ണൽ : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_SUM,
            ARRAY_PRODUCT
        } methodId;

        methodId = UNKNOWN;
        എങ്കിൽ (nodep->name() == "or") {
            methodId = ARRAY_OR;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "and") {
            methodId = ARRAY_AND;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "xor") {
            methodId = ARRAY_XOR;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "sum") {
            methodId = ARRAY_SUM;
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "product") {
            methodId = ARRAY_PRODUCT;
        }

        എങ്കിൽ (methodId) {
            AstWith* സ്ഥിരം withp
                = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            methodOkArguments(nodep, 0, 0);
            എങ്കിൽ (withp) {
                methodCallLValueRecurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* സ്ഥിരം newp
                    = പുതിയ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlinkFrBack(),
                                         VCMethod::arrayMethod("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                എങ്കിൽ (!nodep->firstAbovep()) newp->dtypeSetVoid();
                newp->protect(അസത്യം);
                newp->didWidth(സത്യം);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_വരി* സ്ഥിരം fl = nodep->fileline();
                Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
                വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം arrayRef = nodep->fromp()->clone_വൃക്ഷം_Pure(അസത്യം);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം selector = പുതിയ AstArraySel{fl, arrayRef, i};
                    എങ്കിൽ (!newp) {
                        newp = selector;
                    } അല്ലെങ്കിൽ {
                        തിരഞ്ഞെടുപ്പ് (methodId) {
                        അവസ്ഥ ARRAY_OR: newp = പുതിയ AstOr{fl, newp, selector}; നിർത്തുക;
                        അവസ്ഥ ARRAY_AND: newp = പുതിയ AstAnd{fl, newp, selector}; നിർത്തുക;
                        അവസ്ഥ ARRAY_XOR: newp = പുതിയ AstXor{fl, newp, selector}; നിർത്തുക;
                        അവസ്ഥ ARRAY_SUM: newp = പുതിയ AstAdd{fl, newp, selector}; നിർത്തുക;
                        അവസ്ഥ ARRAY_PRODUCT: newp = പുതിയ AstMul{fl, newp, selector}; നിർത്തുക;
                        സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (AstCMethodHard* newp = methodCallArray(nodep, adtypep)) {
            newp->protect(അസത്യം);
            newp->didWidth(സത്യം);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    ശൂന്യം methodCallEvent(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on event
        എങ്കിൽ (nodep->name() == "triggered") {
            methodOkArguments(nodep, 0, 0);
            AstCMethodHard* സ്ഥിരം callp = പുതിയ AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlinkFrBack(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtypeSetBit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    ശൂന്യം methodCallString(AstMethodCall* nodep, AstBasicDType*) {
        // Method call on string
        എങ്കിൽ (nodep->name() == "len") {
            // Constant value
            methodOkArguments(nodep, 0, 0);
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstLenN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "itoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "hextoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "octtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "bintoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "realtoa") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", methodArg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "tolower") {
            methodOkArguments(nodep, 0, 0);
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstToLowerN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "toupper") {
            methodOkArguments(nodep, 0, 0);
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstToUpperN{nodep->fileline(), nodep->fromp()->unlinkFrBack()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "compare" || nodep->name() == "icompare") {
            സ്ഥിരം തർക്കം ignoreCase = nodep->name()[0] == 'i';
            methodOkArguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* സ്ഥിരം argp = nodep->argsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhs = nodep->fromp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhs = argp->exprp()->unlinkFrBack();
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "putc") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* സ്ഥിരം arg0p = nodep->argsp();
            AstArg* സ്ഥിരം arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_കണ്ണി_Var_Ref* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thsp = arg1p->exprp()->unlinkFrBack();
            AstVarRef* സ്ഥിരം varrefp
                = പുതിയ AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAssign{
                nodep->fileline(), fromp, പുതിയ AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            തള്ളുക_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "getc") {
            methodOkArguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", methodArg(nodep, 0), BOTH);
            AstArg* സ്ഥിരം arg0p = nodep->argsp();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->fromp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "substr") {
            methodOkArguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", methodArg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", methodArg(nodep, 1), BOTH);
            AstArg* സ്ഥിരം arg0p = nodep->argsp();
            AstArg* സ്ഥിരം arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->fromp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = arg0p->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം thsp = arg1p->exprp()->unlinkFrBack();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            എങ്കിൽ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } അല്ലെങ്കിൽ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            methodOkArguments(nodep, 0, 0);
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstAtoN{nodep->fileline(), nodep->fromp()->unlinkFrBack(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queueDTypeIndexedBy(Ast_കണ്ണി_DType* indexDTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        എങ്കിൽ (AstQueueDType* സ്ഥിരം queuep = m_queueDTypeIndexed[indexDTypep]) {
            തിരികെ queuep;
        } അല്ലെങ്കിൽ {
            AstQueueDType* സ്ഥിരം newp
                = പുതിയ AstQueueDType{indexDTypep->fileline(), indexDTypep, nullptr};
            v3Global.rootp()->typeTablep()->addTypesp(newp);
            m_queueDTypeIndexed[indexDTypep] = newp;
            തിരികെ newp;
        }
    }

    ശൂന്യം visit(AstNew* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        AstClass* classp = nullptr;
        തർക്കം assign = അസത്യം;
        എങ്കിൽ (VN_IS(nodep->backp(), Assign)) {  // assignment case
            assign = സത്യം;
            Ast_കണ്ണി* warnp = nullptr;
            AstClassRefDType* refp = nullptr;

            എങ്കിൽ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                എങ്കിൽ (AstClass* സ്ഥിരം clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* സ്ഥിരം adtypep
                        = പുതിയ AstClassRefDType{nodep->fileline(), clsp, nullptr};
                    v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                    refp = adtypep;
                }
            } അല്ലെങ്കിൽ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : nullptr;
            }
            എങ്കിൽ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, കണ്ണി_DType)
                           ? ("data type "s + VN_AS(warnp, കണ്ണി_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                തിരികെ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } അല്ലെങ്കിൽ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } അല്ലെങ്കിൽ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        processFTaskRefArgs(nodep);
        എങ്കിൽ (!assign) nodep->dtypeFrom(nodep->taskp());
    }
    ശൂന്യം visit(AstNewCopy* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        AstClassRefDType* സ്ഥിരം refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        എങ്കിൽ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            തിരികെ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    ശൂന്യം visit(AstNewDynamic* nodep) override {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        AstDynArrayDType* സ്ഥിരം adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        എങ്കിൽ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            തിരികെ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        എങ്കിൽ (!VN_IS(nodep->backp(), കണ്ണി_Assign)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            തിരികെ;
        }
        nodep->dtypep(adtypep);
        എങ്കിൽ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        എങ്കിൽ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* സ്ഥിരം consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            എങ്കിൽ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlinkFrBack());
                nodep->replaceWith(consp->unlinkFrBack());
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }

    ശൂന്യം visit(Ast_Tagged_വ്യഞ്ജകം* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ശൂന്യം visit(AstTaggedPattern* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }
    ശൂന്യം visit(AstPatternVar* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtypeSetBit();
    }
    ശൂന്യം visit(AstPatternStar* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtypeSetBit();
    }
    ശൂന്യം visit(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtypeSetBit();
        userIterateChildren(nodep, m_vup);
    }

    ശൂന്യം visit(AstPattern* nodep) override {
        എങ്കിൽ (m_inParamOverride && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_inParamOverride);
            വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_inParamOverride = അസത്യം;
                userIterateAndNext(patp->repp(), WidthVP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                എങ്കിൽ (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), കണ്ണി_DType)) {
                    userIterateAndNext(patp->keyp(), WidthVP{SELF, BOTH}.p());
                }
                വേണ്ടി (Ast_കണ്ണി *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_inParamOverride = VN_IS(valuep, Pattern);
                    userIterate(valuep, WidthVP{SELF, BOTH}.p());
                }
            }
            തിരികെ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_inParamOverride);
        m_inParamOverride = അസത്യം;
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        UINFO(9, "PATTERN " << nodep);
        എങ്കിൽ (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        എങ്കിൽ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_കണ്ണി_DType* dtypep = nodep->dtypep();
        എങ്കിൽ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            എങ്കിൽ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                തിരികെ;
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            എങ്കിൽ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                തിരികെ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ times = visitPatMemberRep(patp);
                വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i < times; i++) {
                    AstPatMember* സ്ഥിരം newp = patp->clone_വൃക്ഷം(അസത്യം);
                    patp->addNextHere(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                എങ്കിൽ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlinkFrBack(&relinkHandle);
                    ആകുമ്പോൾ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം movep = VN_AS(patp->lhssp()->nextp(), കണ്ണി_വ്യഞ്ജകം)) {
                        movep->unlinkFrBack();  // Not unlinkFrBackWithNext, just one
                        Ast_കണ്ണി* newkeyp = nullptr;
                        എങ്കിൽ (patp->keyp()) newkeyp = patp->keyp()->clone_വൃക്ഷം(സത്യം);
                        AstPatMember* സ്ഥിരം newp
                            = പുതിയ AstPatMember{patp->fileline(), movep, newkeyp, nullptr};
                        patp->addNext(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = nullptr;
            വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* സ്ഥിരം nextp = VN_AS(patp->nextp(), PatMember);
                എങ്കിൽ (patp->isDefault()) {
                    എങ്കിൽ (defaultp) {
                        VL_DO_DANGLING(തള്ളുക_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlinkFrBack();
                }
                patp = nextp;
            }
            ആകുമ്പോൾ (സ്ഥിരം AstConstDType* സ്ഥിരം vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, WidthVP{SELF, BOTH}.p());

            എങ്കിൽ (auto* സ്ഥിരം vdtypep = VN_CAST(dtypep, StructDType)) {
                patternStruct(nodep, vdtypep, defaultp);
            } അല്ലെങ്കിൽ എങ്കിൽ (auto* സ്ഥിരം vdtypep = VN_CAST(dtypep, കണ്ണി_Array_DType)) {
                patternArray(nodep, vdtypep, defaultp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* സ്ഥിരം consp = പുതിയ AstConsAssoc{nodep->fileline()};
                patternKeyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* സ്ഥിരം consp = പുതിയ AstConsWildcard{nodep->fileline()};
                patternKeyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                patternDynArrayOrQueue(nodep, dtypep, defaultp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                patternBasic(nodep, dtypep, defaultp);
            } അല്ലെങ്കിൽ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            എങ്കിൽ (defaultp) VL_DO_DANGLING(തള്ളുക_Deletep(defaultp), defaultp);
        }
    }
    ശൂന്യം patternStruct(AstPattern* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        using PatMap = std::map<സ്ഥിരം AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            സ്ഥിരം AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ആകുമ്പോൾ (patp) {
                ചെയ്യുക {
                    എങ്കിൽ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        എങ്കിൽ (സ്ഥിരം AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            എങ്കിൽ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                നിർത്തുക;
                            } അല്ലെങ്കിൽ {
                                സ്ഥിരം std::pair<PatMap::iterator, തർക്കം> ret
                                    = patmap.emplace(memp, patp);
                                എങ്കിൽ (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_DType* nodedtypep = VN_CAST(patp->keyp(), കണ്ണി_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, WidthVP{SELF, BOTH}.p());
                            സ്ഥിരം string dtype = nodedtypep->dtypep()->prettyDTypeName(സത്യം);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } അല്ലെങ്കിൽ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } അല്ലെങ്കിൽ എങ്കിൽ (memp) {
                        // constant expr
                        സ്ഥിരം std::pair<PatMap::iterator, തർക്കം> ret = patmap.emplace(memp, patp);
                        എങ്കിൽ (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } അല്ലെങ്കിൽ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ആകുമ്പോൾ (അസത്യം);

                // Next
                എങ്കിൽ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        എങ്കിൽ (vdtypep->packed()) {
            വേണ്ടി (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                സ്ഥിരം auto it = patmap.find(memp);
                എങ്കിൽ (it == patmap.end()) {  // default or default_type assignment
                    എങ്കിൽ (AstStructDType* സ്ഥിരം memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalueConcat_patternStruct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } അല്ലെങ്കിൽ {
                        AstPatMember* സ്ഥിരം patp
                            = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                        newp = മൂല്യം_Concat_pattern_Struct(patp, newp, memp, nodep);
                        എങ്കിൽ (patp) VL_DO_DANGLING(തള്ളുക_Deletep(patp), patp);
                    }
                } അല്ലെങ്കിൽ {  // member assignment
                    newp = മൂല്യം_Concat_pattern_Struct(it->second, newp, memp, nodep);
                }
            }
        } അല്ലെങ്കിൽ {  // Unpacked
            AstConsStruct* സ്ഥിരം consp = പുതിയ AstConsStruct{nodep->fileline(), vdtypep};
            വേണ്ടി (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                സ്ഥിരം auto it = patmap.find(memp);
                AstPatMember* patp = nullptr;
                എങ്കിൽ (it == patmap.end()) {  // Default or default_type assignment
                    patp = defaultPatp_patternStruct(nodep, memp, defaultp, dtypemap);
                    തള്ളുക_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    തള്ളുക_Deletep(patp);  // patp used below
                } അല്ലെങ്കിൽ {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = pattern_Member_മൂല്യം_Iterate(patp);
                consp->addItemsp(പുതിയ AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        എങ്കിൽ (newp) {
            nodep->replaceWith(newp);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* nestedvalueConcat_patternStruct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_കണ്ണി_വ്യഞ്ജകം* newp,
                                                 AstPattern* nodep, സ്ഥിരം DTypeMap& dtypemap) {
        വേണ്ടി (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            എങ്കിൽ (AstStructDType* സ്ഥിരം memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalueConcat_patternStruct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } അല്ലെങ്കിൽ {
                AstPatMember* സ്ഥിരം patp
                    = defaultPatp_patternStruct(nodep, memp_nested, defaultp, dtypemap);
                newp = മൂല്യം_Concat_pattern_Struct(patp, newp, memp_nested, nodep);
                എങ്കിൽ (patp) VL_DO_DANGLING(തള്ളുക_Deletep(patp), patp);
            }
        }
        തിരികെ newp;
    }

    AstPatMember* defaultPatp_patternStruct(AstPattern* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, സ്ഥിരം DTypeMap& dtypemap) {
        സ്ഥിരം string memp_DType = memp->virtRefDTypep()->prettyDTypeName(സത്യം);
        സ്ഥിരം auto it = dtypemap.find(memp_DType);
        എങ്കിൽ (it != dtypemap.end()) {
            // default_value for data_type
            തിരികെ it->second->clone_വൃക്ഷം(അസത്യം);
        }
        എങ്കിൽ (defaultp) {
            // default_value for any unmatched member yet
            തിരികെ defaultp->clone_വൃക്ഷം(അസത്യം);
        }
        എങ്കിൽ (memp->valuep()) {
            തിരികെ പുതിയ AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_വൃക്ഷം(അസത്യം), കണ്ണി_വ്യഞ്ജകം),
                                    പുതിയ AstText{nodep->fileline(), memp->name()}, nullptr};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        തിരികെ nullptr;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* മൂല്യം_Concat_pattern_Struct(AstPatMember* patp, Ast_കണ്ണി_വ്യഞ്ജകം* newp,
                                           AstMemberDType* memp, AstPattern* nodep) {
        എങ്കിൽ (patp) {
            patp->dtypep(memp);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = pattern_Member_മൂല്യം_Iterate(patp);
            എങ്കിൽ (!newp) {
                newp = valuep;
            } അല്ലെങ്കിൽ {
                AstConcat* സ്ഥിരം concatp = പുതിയ AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, സത്യം);
                valuep->fileline()->modifyWarnOff(V3ErrorCode::WIDTHCONCAT, സത്യം);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        തിരികെ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_കണ്ണി_DType* elemDTypep,
                                       സ്ഥിരം DTypeMap& dtypemap = DTypeMap{}) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = defaultp->lhssp()->clone_വൃക്ഷം(അസത്യം);
        Ast_കണ്ണി_DType* സ്ഥിരം elemDTypeSkipRefp = elemDTypep->skipRefp();
        സ്ഥിരം AstStructDType* സ്ഥിരം structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        സ്ഥിരം തർക്കം unpackedStruct = structp && !structp->packed();

        എങ്കിൽ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            തിരികെ defaultp->clone_വൃക്ഷം(അസത്യം);
        }
        എങ്കിൽ (VN_IS(valuep, Pattern)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            തിരികെ defaultp->clone_വൃക്ഷം(അസത്യം);
        }
        എങ്കിൽ (!valuep->dtypep()) userIterate(valuep, WidthVP{SELF, BOTH}.p());
        തർക്കം wholeElement = unpackedStruct;
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം മൂല്യം_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            wholeElement = unpackedStruct ? !മൂല്യം_DTypep->skipRefp()->isIntegralOrPacked()
                                          : മൂല്യം_DTypep->similarDType(elemDTypeSkipRefp);
        }
        എങ്കിൽ (wholeElement) {
            VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            തിരികെ defaultp->clone_വൃക്ഷം(അസത്യം);
        }

        AstPatMember* സ്ഥിരം nestedDefaultp
            = പുതിയ AstPatMember{defaultp->fileline(), valuep, nullptr, nullptr};
        nestedDefaultp->isDefault(സത്യം);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        വേണ്ടി (സ്ഥിരം auto& entry : dtypemap) nestedDefaultp->addNext(entry.second->clone_വൃക്ഷം(അസത്യം));
        AstPattern* സ്ഥിരം recursivePatternp = പുതിയ AstPattern{defaultp->fileline(), nestedDefaultp};
        തിരികെ പുതിയ AstPatMember{defaultp->fileline(), recursivePatternp, nullptr, nullptr};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_കണ്ണി_വ്യഞ്ജകം* patternDefaultValuep(AstPatMember* defaultp, Ast_കണ്ണി_DType* elemDTypep) {
        എങ്കിൽ (!defaultp) തിരികെ nullptr;
        AstPatMember* സ്ഥിരം elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = pattern_Member_മൂല്യം_Iterate(elemDefaultp);
        VL_DO_DANGLING(തള്ളുക_Deletep(elemDefaultp), elemDefaultp);
        തിരികെ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    ശൂന്യം patternConcatArray(AstPattern* nodep, AstUnpackArrayDType* arrayDtp) {
        സ്ഥിരം VNumRange range = arrayDtp->declRange();
        സ്ഥിരം പൂർണ്ണസംഖ്യ elements = range.elements();
        AstConsArray* സ്ഥിരം newp = പുതിയ AstConsArray{nodep->fileline(), arrayDtp};
        പൂർണ്ണസംഖ്യ entn = 0;  // Number of elements given so far, from the left
        തർക്കം error = അസത്യം;  // Reported an error, so the value is not complete
        വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = pattern_Member_മൂല്യം_Iterate(patp);
            സ്ഥിരം uint64_t index = range.ascending() ? entn : elements - 1 - entn;
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            എങ്കിൽ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_മൂല്യങ്ങൾ(index, valuep);
                തുടരുക;
            }
            സ്ഥിരം VCastable castable = Ast_കണ്ണി::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            എങ്കിൽ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = സത്യം;
            }
            newp->valuep(index, valuep);
            ++entn;
        }
        എങ്കിൽ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = സത്യം;
        }
        എങ്കിൽ (error) {
            VL_DO_DANGLING(തള്ളുക_Deletep(newp), newp);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } അല്ലെങ്കിൽ {
            nodep->replaceWith(newp);
        }
    }
    ശൂന്യം patternArray(AstPattern* nodep, Ast_കണ്ണി_Array_DType* arrayDtp, AstPatMember* defaultp) {
        എങ്കിൽ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            patternConcatArray(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            തിരികെ;
        }
        സ്ഥിരം VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.ഇടത്() << " to " << range.വലത്());
        Ast_കണ്ണി* newp = nullptr;
        തർക്കം missed = അസത്യം;  // Some element missed, so not a complete value
        വേണ്ടി (പൂർണ്ണസംഖ്യ entn = 0, ent = range.ഇടത്(); entn < range.elements();
             ++entn, ent += range.ഇടത്_To_വലത്_Inc()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;
            സ്ഥിരം auto it = patmap.find(ent);
            എങ്കിൽ (it == patmap.end()) {
                എങ്കിൽ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, arrayDtp->subDTypep());
                } അല്ലെങ്കിൽ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = സത്യം;
                }
            } അല്ലെങ്കിൽ {
                AstPatMember* സ്ഥിരം patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = pattern_Member_മൂല്യം_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                എങ്കിൽ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warnMore()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
                    valuep = പുതിയ AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            എങ്കിൽ (valuep) {
                എങ്കിൽ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    എങ്കിൽ (!newp) newp = പുതിയ AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } അല്ലെങ്കിൽ {  // Packed. Convert to concat for now.
                    എങ്കിൽ (!newp) {
                        newp = valuep;
                    } അല്ലെങ്കിൽ {
                        AstConcat* സ്ഥിരം concatp
                            = പുതിയ AstConcat{valuep->fileline(), VN_AS(newp, കണ്ണി_വ്യഞ്ജകം), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        എങ്കിൽ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        എങ്കിൽ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(തള്ളുക_Deletep(newp), newp);
            newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        എങ്കിൽ (newp) {
            nodep->replaceWith(newp);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* patternConstKeyp(AstPatMember* patp) {
        എങ്കിൽ (VN_IS(patp->keyp(), കണ്ണി_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            തിരികെ nullptr;
        }
        എങ്കിൽ (patp->താക്കോൽ_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->താക്കോൽ_Exprp());  // May change
        AstConst* സ്ഥിരം constp = VN_CAST(patp->താക്കോൽ_Exprp(), Const);
        എങ്കിൽ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        തിരികെ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* patternScalarKeyp(AstPatMember* patp, Ast_കണ്ണി_DType* താക്കോൽ_DTypep) {
        എങ്കിൽ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            തിരികെ nullptr;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        എങ്കിൽ (താക്കോൽ_DTypep->skipRefp()->isIntegralOrPacked() && patp->താക്കോൽ_Exprp()) {
            userIterate(patp->താക്കോൽ_Exprp(), WidthVP{താക്കോൽ_DTypep, PRELIM}.p());
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(patp->താക്കോൽ_Exprp()->width(), താക്കോൽ_DTypep->width());
            Ast_കണ്ണി_DType* സ്ഥിരം contextDTypep
                = താക്കോൽ_DTypep->findBitOrLogicDType(width, width, താക്കോൽ_DTypep->numeric(), സത്യം);
            iterateCheck(patp, "key", patp->താക്കോൽ_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, അസത്യം);
        }
        AstConst* സ്ഥിരം constp = patternConstKeyp(patp);
        എങ്കിൽ (!constp) തിരികെ nullptr;
        എങ്കിൽ (constp->അക്കം().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            തിരികെ nullptr;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, താക്കോൽ_DTypep, EXTEND_EXP, അസത്യം);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം convp = patp->താക്കോൽ_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        തിരികെ VN_AS(keyp->unlinkFrBack(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_കണ്ണി_വ്യഞ്ജകം* patternKeyp(AstConsAssoc*, AstPatMember* patp, Ast_കണ്ണി_DType* arrayDtp) {
        Ast_കണ്ണി_DType* സ്ഥിരം താക്കോൽ_DTypep = VN_AS(arrayDtp, AssocArrayDType)->താക്കോൽ_DTypep();
        എങ്കിൽ (!താക്കോൽ_DTypep->skipRefp()->isAggregateType() || !patp->താക്കോൽ_Exprp()) {
            തിരികെ patternScalarKeyp(patp, താക്കോൽ_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->താക്കോൽ_Exprp(), താക്കോൽ_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->താക്കോൽ_Exprp());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം foldedp = patp->താക്കോൽ_Exprp();
        എങ്കിൽ (!foldedp->is_Constant_വൃക്ഷം()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            തിരികെ nullptr;
        }
        എങ്കിൽ (foldedp->exists([](സ്ഥിരം AstConst* constp) { തിരികെ constp->അക്കം().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            തിരികെ nullptr;
        }
        തിരികെ foldedp->unlinkFrBack();
    }
    AstConst* patternKeyp(AstConsWildcard*, AstPatMember* patp, Ast_കണ്ണി_DType* arrayDtp) {
        തിരികെ patternScalarKeyp(patp, arrayDtp->findStringDType());
    }
    AstConst* patternKeyp(AstConsDynArray*, AstPatMember* patp, Ast_കണ്ണി_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        തിരികെ patternScalarKeyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    template <typename T_Item, typename T_Cons>
    ശൂന്യം sort_Cons_ഇനങ്ങൾ(T_Cons* nodep) {
        std::vector<T_Item*> itemps;
        ആകുമ്പോൾ (T_Item* സ്ഥിരം itemp = nodep->itemsp()) {
            itemp->unlinkFrBack();
            itemps.തള്ളുക_back(itemp);
        }
        സ്ഥിരം auto താക്കോൽ_Of = [](സ്ഥിരം T_Item* itemp) -> സ്ഥിരം V3_സംഖ്യ& {
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം keyp = itemp->keyp();
            തിരികെ VN_AS(keyp, Const)->അക്കം();
        };
        std::stable_sort(itemps.begin(), itemps.end(), [&](സ്ഥിരം T_Item* ap, സ്ഥിരം T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            തിരികെ താക്കോൽ_Of(ap).isLtXZ(താക്കോൽ_Of(bp));
        });
        സ്ഥിരം T_Item* firstp = nullptr;  // First item with the current key
        വേണ്ടി (T_Item* സ്ഥിരം itemp : itemps) {
            എങ്കിൽ (firstp && താക്കോൽ_Of(firstp).isCaseEq(താക്കോൽ_Of(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } അല്ലെങ്കിൽ {
                firstp = itemp;
            }
            nodep->addItemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    template <typename T_Item, typename T_Cons>
    ശൂന്യം patternKeyed(AstPattern* nodep, T_Cons* consp, Ast_കണ്ണി_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(patternDefaultValuep(defaultp, arrayDtp->subDTypep()));
        വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            സ്ഥിരം auto keyp = patternKeyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = pattern_Member_മൂല്യം_Iterate(patp);
            എങ്കിൽ (keyp) {
                consp->addItemsp(പുതിയ T_Item{patp->fileline(), keyp, valuep});
            } അല്ലെങ്കിൽ {
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        സ്ഥിരം AstAssocArrayDType* സ്ഥിരം assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        എങ്കിൽ (!assocDtp || !assocDtp->താക്കോൽ_DTypep()->skipRefp()->isAggregateType()) {
            sort_Cons_ഇനങ്ങൾ<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* newConsDynArrayOrQueue(File_വരി* fileline, സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        എങ്കിൽ (VN_IS(dtypep, DynArrayDType)) തിരികെ പുതിയ AstConsDynArray{fileline};
        എങ്കിൽ (VN_IS(dtypep, QueueDType)) തിരികെ പുതിയ AstConsQueue{fileline};
        തിരികെ nullptr;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    template <typename T_Item, typename T_Cons>
    ശൂന്യം addConsItem(T_Cons* consp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        Ast_കണ്ണി_DType* സ്ഥിരം arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        സ്ഥിരം തർക്കം isElement
            = Ast_കണ്ണി::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), nullptr)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        സ്ഥിരം AstQueueDType* സ്ഥിരം subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        എങ്കിൽ (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* സ്ഥിരം subp = Ast_കണ്ണി::cast<T_Cons>(valuep);
            എങ്കിൽ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                എങ്കിൽ (subp->itemsp()) consp->addItemsp(subp->itemsp()->unlinkFrBackWithNext());
                VL_DO_DANGLING(തള്ളുക_Deletep(subp), subp);
                തിരികെ;
            }
            എങ്കിൽ (AstConsQueue* സ്ഥിരം queuep = VN_CAST(valuep, ConsQueue)) {
                add_Cons_ഇനങ്ങൾ(consp, queuep);
                തിരികെ;
            }
            എങ്കിൽ (AstConsDynArray* സ്ഥിരം dynp = VN_CAST(valuep, ConsDynArray)) {
                add_Cons_ഇനങ്ങൾ(consp, dynp);
                തിരികെ;
            }
        }
        T_Item* സ്ഥിരം itemp = പുതിയ T_Item{valuep->fileline(), valuep};
        consp->addItemsp(itemp);
        // Width the value now, as constructors are widthed when created
        എങ്കിൽ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } അല്ലെങ്കിൽ {
            userIterateAndNext(valuep, WidthVP{arrayDtp, FINAL}.p());
        }
    }
    ശൂന്യം addConsItem(Ast_കണ്ണി_വ്യഞ്ജകം* consp, Ast_കണ്ണി_വ്യഞ്ജകം* valuep) {
        എങ്കിൽ (AstConsDynArray* സ്ഥിരം dynp = VN_CAST(consp, ConsDynArray)) {
            addConsItem<AstConsDynArrayItem>(dynp, valuep);
        } അല്ലെങ്കിൽ {
            addConsItem<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    template <typename T_Sub>
    ശൂന്യം add_Cons_ഇനങ്ങൾ(Ast_കണ്ണി_വ്യഞ്ജകം* consp, T_Sub* subp) {
        ആകുമ്പോൾ (auto* സ്ഥിരം itemp = subp->itemsp()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = itemp->valuep()->unlinkFrBack();
            VL_DO_DANGLING(തള്ളുക_Deletep(itemp->unlinkFrBack()), itemp);
            addConsItem(consp, valuep);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(subp), subp);
    }
    ശൂന്യം patternDynArrayOrQueue(AstPattern* nodep, Ast_കണ്ണി_DType* arrayp, AstPatMember* defaultp) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        സ്ഥിരം AstPatMember* സ്ഥിരം firstp = VN_AS(nodep->itemsp(), PatMember);
        സ്ഥിരം തർക്കം keyed = defaultp || (firstp && firstp->keyp());
        എങ്കിൽ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            patternKeyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            തിരികെ;
        }
        സ്ഥിരം അക്ഷരം* സ്ഥിരം kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        എങ്കിൽ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            എങ്കിൽ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            addConsItem(newp, pattern_Member_മൂല്യം_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    ശൂന്യം patternBasic(AstPattern* nodep, Ast_കണ്ണി_DType* vdtypep, AstPatMember* defaultp) {
        സ്ഥിരം AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        സ്ഥിരം VNumRange range = bdtypep->declRange();
        PatVecMap patmap = patVectorMap(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = nullptr;
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ ent : range.seqHiToLo()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;
            സ്ഥിരം auto it = patmap.find(ent);
            എങ്കിൽ (it == patmap.end()) {
                എങ്കിൽ (defaultp) {
                    valuep = patternDefaultValuep(defaultp, nodep->findBitDType());
                } അല്ലെങ്കിൽ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } അല്ലെങ്കിൽ {
                AstPatMember* സ്ഥിരം patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = pattern_Member_മൂല്യം_Iterate(patp);
            }
            എങ്കിൽ (valuep) {  // Packed. Convert to concat for now.
                എങ്കിൽ (!newp) {
                    newp = valuep;
                } അല്ലെങ്കിൽ {
                    AstConcat* സ്ഥിരം concatp = പുതിയ AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        എങ്കിൽ (!patmap.empty()) nodep->v3error("Assignment pattern with too many elements");
        എങ്കിൽ (newp) {
            nodep->replaceWith(newp);
        } അല്ലെങ്കിൽ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* pattern_Member_മൂല്യം_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, WidthVP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = patp->lhssp()->unlinkFrBack();
        എങ്കിൽ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            എങ്കിൽ (AstConst* സ്ഥിരം newp = V3WidthCommit::newIfConstCommitSize(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        തിരികെ valuep;
    }

    സ്ഥിതം ശൂന്യം checkEventAssignment(സ്ഥിരം Ast_കണ്ണി_Assign* സ്ഥിരം asgnp) {
        string unsupEvtAsgn;
        എങ്കിൽ (!usesDynamicScheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        എങ്കിൽ (VN_IS(asgnp->rhsp(), CReset)) തിരികെ;
        എങ്കിൽ (asgnp->rhsp()->dtypep()->isEvent() && !usesDynamicScheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.empty() ? "from" : " and from");
        }
        എങ്കിൽ (!unsupEvtAsgn.empty()) {
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

    സ്ഥിതം തർക്കം usesDynamicScheduler(Ast_കണ്ണി* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ആകുമ്പോൾ (സത്യം) {
            AstVarRef* സ്ഥിരം vrefp = VN_CAST(nodep, VarRef);
            എങ്കിൽ (vrefp) തിരികെ usesDynamicScheduler(vrefp);
            എങ്കിൽ (VN_IS(nodep, MemberSel)) {
                തിരികെ സത്യം;
            } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Sel* selp = VN_CAST(nodep, കണ്ണി_Sel)) {
                nodep = selp->fromp();
            } അല്ലെങ്കിൽ {
                തിരികെ അസത്യം;
            }
        }
    }

    സ്ഥിതം തർക്കം usesDynamicScheduler(AstVarRef* vrefp) {
        തിരികെ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    ശൂന്യം visit(AstPatMember* nodep) override {
        Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, WidthVP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ASSIGN, FINAL, vdtypep, EXTEND_LHS);
    }
    പൂർണ്ണസംഖ്യ visitPatMemberRep(AstPatMember* nodep) {
        uint32_t times = 1;
        എങ്കിൽ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep->repp(), Const);
            എങ്കിൽ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } അല്ലെങ്കിൽ {
                times = constp->toUInt();
            }
            എങ്കിൽ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlinkFrBackWithNext()
                ->delete_വൃക്ഷം();  // Done with replicate before cloning
        }
        തിരികെ times;
    }

    ശൂന്യം visit(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_കണ്ണി* propp = nodep->propp();
            ആകുമ്പോൾ (AstLogNot* സ്ഥിരം notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* സ്ഥിരം untilp = VN_CAST(propp, Until);
            എങ്കിൽ (untilp && VN_IS(nodep->backp(), കണ്ണി_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().isSetFalse() || !v3Global.opt.timing().isSetTrue())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(പുതിയ AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(തള്ളുക_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            userIterateAndNext(nodep->sensesp(), nullptr);
            എങ്കിൽ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtypeSetBit();
        }
    }

    തർക്കം firstNewStatementOkRecurse(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (AstVar* സ്ഥിരം varp = VN_CAST(nodep, Var)) {
            എങ്കിൽ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) തിരികെ സത്യം;
        }
        എങ്കിൽ (AstAssign* സ്ഥിരം aitemp = VN_CAST(nodep, Assign)) {
            എങ്കിൽ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) തിരികെ സത്യം;
        }
        എങ്കിൽ (AstInitialStaticStmt* സ്ഥിരം aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            വേണ്ടി (Ast_കണ്ണി* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (!firstNewStatementOkRecurse(stmtp)) തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        }
        എങ്കിൽ (AstInitialAutomaticStmt* സ്ഥിരം aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            വേണ്ടി (Ast_കണ്ണി* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                എങ്കിൽ (!firstNewStatementOkRecurse(stmtp)) തിരികെ അസത്യം;
            }
            തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }

    //--------------------
    // Top levels

    template <typename CaseItem>
    ശൂന്യം handleCaseType(Ast_കണ്ണി* casep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, CaseItem* itemsp) {
        AstAttrOf* സ്ഥിരം exprap = VN_CAST(exprp, AttrOf);
        എങ്കിൽ (!exprap) തിരികെ;
        എങ്കിൽ (exprap->attrType() != VAttrType::TYPEID) തിരികെ;

        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം വ്യഞ്ജകം_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << വ്യഞ്ജകം_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_വരി* സ്ഥിരം newfl = casep->fileline();
        newfl->warnOff(V3ErrorCode::CASEINCOMPLETE, സത്യം);  // Side effect of transform
        newfl->warnOff(V3ErrorCode::CASEOVERLAP, സത്യം);  // Side effect of transform
        casep->fileline(newfl);
        വേണ്ടി (CaseItem* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<CaseItem>(itemp->nextp())) {
            എങ്കിൽ (itemp->isDefault()) തുടരുക;
            തർക്കം hit = അസത്യം;
            വേണ്ടി (Ast_കണ്ണി* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                സ്ഥിരം AstAttrOf* സ്ഥിരം condAttrp = VN_CAST(condp, AttrOf);
                എങ്കിൽ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } അല്ലെങ്കിൽ {
                    Ast_കണ്ണി_DType* സ്ഥിരം condDtp = condAttrp->dtypep();
                    എങ്കിൽ (Ast_കണ്ണി::computeCastable(വ്യഞ്ജകം_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        hit = സത്യം;
                        നിർത്തുക;
                    }
                }
            }
            തള്ളുക_Deletep(itemp->condsp()->unlinkFrBackWithNext());
            // Item condition becomes constant 1 if hits else 0
            itemp->addCondsp(പുതിയ AstConst{newfl, AstConst::BitTrue{}, hit});
        }
        exprap->replaceWith(പുതിയ AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(തള്ളുക_Deletep(exprap), exprap);
    }

    template <typename CaseItem>
    ശൂന്യം handleCase(Ast_കണ്ണി* casep, Ast_കണ്ണി_വ്യഞ്ജകം* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_കണ്ണി_DType* subDTypep = exprp->dtypep();
        വേണ്ടി (CaseItem* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<CaseItem>(itemp->nextp())) {
            വേണ്ടി (Ast_കണ്ണി* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                എങ്കിൽ (condp->dtypep() == subDTypep) തുടരുക;

                എങ്കിൽ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } അല്ലെങ്കിൽ എങ്കിൽ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } അല്ലെങ്കിൽ {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(subDTypep->width(), condp->width());
                    സ്ഥിരം പൂർണ്ണസംഖ്യ mwidth = std::max(subDTypep->widthMin(), condp->widthMin());
                    സ്ഥിരം തർക്കം issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        width, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        വേണ്ടി (CaseItem* itemp = itemsp; itemp; itemp = Ast_കണ്ണി::as<CaseItem>(itemp->nextp())) {
            വേണ്ടി (Ast_കണ്ണി *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    ശൂന്യം visit(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        വേണ്ടി (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            വേണ്ടി (Ast_കണ്ണി *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
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
    ശൂന്യം visit(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->initsp(), nullptr);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        userIterateAndNext(nodep->incsp(), nullptr);
    }
    ശൂന്യം visit(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    ശൂന്യം visit(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        വേണ്ടി (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            വേണ്ടി (Ast_കണ്ണി *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, WidthVP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ശൂന്യം visit(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_കണ്ണി_DType* സ്ഥിരം itemDTypep = nodep->findUInt32DType();
        വേണ്ടി (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            userIterateAndNext(itemp->stmtsp(), nullptr);
            വേണ്ടി (Ast_കണ്ണി *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    ശൂന്യം visit(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        userIterateAndNext(nodep->stmtsp(), nullptr);
    }
    ശൂന്യം visit(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    ശൂന്യം visit(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    ശൂന്യം visit(Ast_കണ്ണി_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        userIterateAndNext(nodep->thensp(), nullptr);
        userIterateAndNext(nodep->elsesp(), nullptr);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    ശൂന്യം visit(Ast_വ്യഞ്ജകം_Stmt* nodep) override {
        userIterateAndNext(nodep->stmtsp(), nullptr);
        // expected result is same as parent's expected result
        userIterateAndNext(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Foreach* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        nodep->didWidth(സത്യം);
        സ്ഥിരം AstForeachHeader* സ്ഥിരം headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        userIterateAndNext(headerp->fromp(), WidthVP{SELF, BOTH}.p());
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_കണ്ണി_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        വേണ്ടി (Ast_കണ്ണി *argsp = headerp->elementsp(), *next_argsp; argsp; argsp = next_argsp) {
            next_argsp = argsp->nextp();
            സ്ഥിരം തർക്കം empty = VN_IS(argsp, Empty);
            AstVar* സ്ഥിരം varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || empty, argsp, "Missing foreach loop variable");
            എങ്കിൽ (varp) varp->usedLoopIdx(സത്യം);
            എങ്കിൽ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            എങ്കിൽ (VN_IS(fromDtp, കണ്ണി_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } അല്ലെങ്കിൽ എങ്കിൽ (AstBasicDType* സ്ഥിരം adtypep = VN_CAST(fromDtp, BasicDType)) {
                എങ്കിൽ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                    തിരികെ;
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* സ്ഥിരം adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->താക്കോൽ_DTypep());
            } അല്ലെങ്കിൽ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
                തിരികെ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_കണ്ണി* സ്ഥിരം bodyp = nodep->bodyp();
        userIterateAndNext(bodyp, nullptr);
        എങ്കിൽ (AstForeach* സ്ഥിരം loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convertToWhile(loopp), loopp, nodep);
            തിരികെ;
        }
    }

    ശൂന്യം visit(Ast_കണ്ണി_Assign* nodep) override {
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
            എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                എങ്കിൽ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                    തിരികെ;
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
            Ast_കണ്ണി_DType* സ്ഥിരം lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            എങ്കിൽ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDtp = lhsDTypep->skipRefp();
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                സ്ഥിരം തർക്കം lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                സ്ഥിരം തർക്കം rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                എങ്കിൽ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    എങ്കിൽ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_കണ്ണി_Var_Ref, Ast_കണ്ണി_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), കണ്ണി_Var_Ref), VN_CAST(nodep->rhsp(), കണ്ണി_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                എങ്കിൽ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), കണ്ണി_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterateCheckAssign(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        എങ്കിൽ (VN_IS(nodep, AssignForce)) checkForceReleaseLhs(nodep, nodep->lhsp());
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം controlp = nodep->timingControlp()) {
            എങ്കിൽ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_വൃക്ഷം(), controlp);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->fileline()->timingOn() && v3Global.opt.timing().isSetTrue()) {
                iterateNull(controlp);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (nodep->fileline()->timingOn()) {
                    എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } അല്ലെങ്കിൽ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlinkFrBackWithNext()->delete_വൃക്ഷം(), controlp);
            }
        }
        എങ്കിൽ (VN_IS(nodep->rhsp(), EmptyQueue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            എങ്കിൽ (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
                തിരികെ;
            }
            AstMethodCall* സ്ഥിരം newp
                = പുതിയ AstMethodCall{nodep->fileline(), nodep->lhsp()->unlinkFrBack(), "delete"};
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            // Need to now convert it
            visit(newp);
            തിരികെ;
        }
        എങ്കിൽ (സ്ഥിരം AstNewDynamic* സ്ഥിരം dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            എങ്കിൽ (!dynp->rhsp()) {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlinkFrBack()};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlinkFrBack(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlinkFrBack()};
                newp->addPinsp(dynp->rhsp()->unlinkFrBack());
            }
            newp->didWidth(സത്യം);
            newp->protect(അസത്യം);
            newp->dtypeSetVoid();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        എങ്കിൽ (AstCMethodHard* സ്ഥിരം slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod assignMethod;
            എങ്കിൽ (slicep->method() == VCMethod::DYN_SLICE) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN;
            } അല്ലെങ്കിൽ എങ്കിൽ (slicep->method() == VCMethod::DYN_SLICE_BACK_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_BACK_BACK;
            } അല്ലെങ്കിൽ എങ്കിൽ (slicep->method() == VCMethod::DYN_SLICE_FRONT_BACK) {
                assignMethod = VCMethod::DYN_SLICE_ASSIGN_FRONT_BACK;
            } അല്ലെങ്കിൽ {
                assignMethod = VCMethod::_ENUM_MAX;  // not a slice
            }
            എങ്കിൽ (assignMethod != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = slicep->fromp()->unlinkFrBack();
                // Collect existing slice index pins (lsb, msb)
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lsbp = slicep->pinsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം msbp = slicep->pinsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                AstCMethodHard* സ്ഥിരം newp
                    = പുതിയ AstCMethodHard{nodep->fileline(), fromp, assignMethod};
                newp->addPinsp(lsbp);
                newp->addPinsp(msbp);
                newp->addPinsp(rhsp);
                newp->didWidth(സത്യം);
                newp->protect(അസത്യം);
                newp->dtypeSetVoid();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            }
        }

        എങ്കിൽ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.setAssignsEvents();
        }
    }

    ശൂന്യം visit(AstRSRule* nodep) override {
        എങ്കിൽ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        userIterateAndNext(nodep->prodlistsp(), nullptr);
        userIterateAndNext(nodep->weightStmtsp(), nullptr);
    }

    ശൂന്യം visit(AstRelease* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    ശൂന്യം visit(AstDeassign* nodep) override {
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->widthSized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    സ്ഥിതം തർക്കം isFormatNonNumericArg(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        തിരികെ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, കണ്ണി_UOr_Struct_DType)
                   && !VN_AS(dtypep, കണ്ണി_UOr_Struct_DType)->packed());
    }

    ശൂന്യം checkFormatNumericArg(Ast_കണ്ണി* argp, അക്ഷരം ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        എങ്കിൽ (std::tolower(ch) == 'd') തിരികെ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        എങ്കിൽ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warnMore() << "... Suggest use '%s'");
    }

    ശൂന്യം visit(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    ശൂന്യം visit(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->fmtp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstToStringN* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtypeSetString();
        userIterateAndNext(nodep->lhsp(), WidthVP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    ശൂന്യം visit(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        എങ്കിൽ (m_vup && !m_vup->prelim()) തിരികെ;  // Can be called as statement or function
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (!nodep->വ്യഞ്ജകം_Format_2()) {  // Non-constant format, can't check format string types
            visit_sformatf_format(nodep);
        }

        Ast_കണ്ണി_വ്യഞ്ജകം* oldExprsp = nodep->exprsp();
        എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), കണ്ണി_വ്യഞ്ജകം);
        }
        എങ്കിൽ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
        ആകുമ്പോൾ (Ast_കണ്ണി_വ്യഞ്ജകം* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), കണ്ണി_വ്യഞ്ജകം);
            എങ്കിൽ (oldExprsp) oldExprsp->unlinkFrBackWithNext();
            എങ്കിൽ (VN_IS(argp, SFormatArg)) {
                nodep->addExprsp(argp);
                തുടരുക;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_കണ്ണി_DType* സ്ഥിരം dtypep = argp->dtypep()->skipRefp();
            എങ്കിൽ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } അല്ലെങ്കിൽ എങ്കിൽ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } അല്ലെങ്കിൽ എങ്കിൽ (isFormatNonNumericArg(dtypep)) {
                എങ്കിൽ (AstClassRefDType* സ്ഥിരം classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    എങ്കിൽ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(സത്യം);
                    }
                } അല്ലെങ്കിൽ {
                    // Class handles inside containers print type names, not their members.
                    Ast_കണ്ണി_DType* കണ്ണി_Dtypep = dtypep;
                    ആകുമ്പോൾ (കണ്ണി_Dtypep) {
                        കണ്ണി_Dtypep = കണ്ണി_Dtypep->skipRefp();
                        എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം uOrStructDTypep
                            = VN_CAST(കണ്ണി_Dtypep, കണ്ണി_UOr_Struct_DType)) {
                            uOrStructDTypep->setEmitToString();
                            v3Global.hasPrintedObjects(സത്യം);
                        }
                        കണ്ണി_Dtypep = കണ്ണി_Dtypep->subDTypep();
                    }
                }
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstToStringN{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->വ്യഞ്ജകം_Format_2()) {
                എങ്കിൽ (AstEnumDType* സ്ഥിരം enumDtp = formatEnumDType(argp)) {
                    nodep->addExprsp(newFormatEnumArg(argp, enumDtp));
                    തുടരുക;
                }
            }
            എങ്കിൽ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            എങ്കിൽ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->addExprsp(argp);
            } അല്ലെങ്കിൽ {
                nodep->addExprsp(പുതിയ AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    ശൂന്യം visit(AstCReset* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    ശൂന്യം visit(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ശൂന്യം visit(AstConstraintRef* nodep) override { userIterateChildren(nodep, nullptr); }
    ശൂന്യം visit(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        എങ്കിൽ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        എങ്കിൽ (!m_paramsOnly) {
            nodep->foreach([ഇത്](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    പുതിയ AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            എങ്കിൽ (text.empty()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            തിരഞ്ഞെടുപ്പ് (nodep->displayType()) {
            അവസ്ഥ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); നിർത്തുക;
            അവസ്ഥ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); നിർത്തുക;
            സ്ഥിരസ്ഥിതി: UASSERT_OBJ(അസത്യം, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
        }
    }
    ശൂന്യം visit(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        എങ്കിൽ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    ശൂന്യം visit(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    ശൂന്യം visit(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    ശൂന്യം visit(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ശൂന്യം visit(AstFError* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            userIterateAndNext(nodep->strp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ശൂന്യം visit(AstFEof* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    ശൂന്യം visit(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        എങ്കിൽ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ശൂന്യം visit(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ശൂന്യം visit(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ശൂന്യം visit(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->offset(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    ശൂന്യം visit(AstFGetC* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ശൂന്യം visit(AstFGetS* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->strgp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ശൂന്യം visit(AstFUngetC* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    ശൂന്യം visit(AstFRead* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            എങ്കിൽ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            എങ്കിൽ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    ശൂന്യം visit(AstFScanF* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ശൂന്യം visit(AstSScanF* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->fromp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ശൂന്യം visit(Ast_അട്ടി_Trace_F* nodep) override { nodep->dtypeSetString(); }
    ശൂന്യം visit(AstSysIgnore* nodep) override {
        userIterateAndNext(nodep->exprsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstSystemF* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(Ast_കണ്ണി_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        userIterateAndNext(nodep->memp(), WidthVP{SELF, BOTH}.p());
        സ്ഥിരം Ast_കണ്ണി_DType* subp = nullptr;
        എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            എങ്കിൽ (!adtypep->താക്കോൽ_DTypep()->skipRefp()->basicp()
                || !adtypep->താക്കോൽ_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } അല്ലെങ്കിൽ {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        എങ്കിൽ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        userIterateAndNext(nodep->lsbp(), WidthVP{SELF, BOTH}.p());
        userIterateAndNext(nodep->msbp(), WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstTestPlusArgs* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    ശൂന്യം visit(Ast_മൂല്യം_Plus_Args* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            userIterateAndNext(nodep->outp(), WidthVP{SELF, BOTH}.p());
        }
    }
    ശൂന്യം visit(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        എങ്കിൽ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        എങ്കിൽ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        എങ്കിൽ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        എങ്കിൽ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    ശൂന്യം visit(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ശൂന്യം visit(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
    }
    ശൂന്യം visit(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
        userIterateAndNext(nodep->failsp(), nullptr);
    }
    ശൂന്യം visit(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        userIterateAndNext(nodep->passsp(), nullptr);
    }
    ശൂന്യം visit(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    തർക്കം pinInstArrayElement(AstPin* nodep, സ്ഥിരം AstRange* rangep, പൂർണ്ണസംഖ്യ idx) {
        Ast_കണ്ണി_DType* സ്ഥിരം modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_കണ്ണി_DType* സ്ഥിരം conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        എങ്കിൽ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) തിരികെ സത്യം;
        // Types don't match, but all dimensions done, still need the usual checks
        എങ്കിൽ (!rangep) തിരികെ അസത്യം;

        സ്ഥിരം AstRange* സ്ഥിരം subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        പൂർണ്ണസംഖ്യ subElems = 1;
        വേണ്ടി (സ്ഥിരം AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        സ്ഥിരം പൂർണ്ണസംഖ്യ elements = rangep->elementsConst();
        സ്ഥിരം പൂർണ്ണസംഖ്യ from_ഇടത് = idx / subElems;

        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(conDTypep, UnpackArrayDType);
        സ്ഥിരം uint32_t conDims = conDTypep->dimensions(അസത്യം).second;
        സ്ഥിരം uint32_t modDims = modDTypep->dimensions(അസത്യം).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        എങ്കിൽ (arrp && conDims != modDims) {
            എങ്കിൽ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                തിരികെ സത്യം;
            }
            // ArraySel index is counted from lo
            സ്ഥിരം പൂർണ്ണസംഖ്യ bit = arrp->declRange().ascending() ? from_ഇടത് : elements - 1 - from_ഇടത്;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_AS(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം)->unlinkFrBack();
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(തള്ളുക_Deletep(fromp), fromp);
            } അല്ലെങ്കിൽ {
                AstArraySel* സ്ഥിരം selp = പുതിയ AstArraySel{fromp->fileline(), fromp, bit};
                selp->didWidth(സത്യം);
                nodep->exprp(selp);
            }
            തിരികെ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        എങ്കിൽ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) തിരികെ അസത്യം;

        // Packed connection
        സ്ഥിരം പൂർണ്ണസംഖ്യ modwidth = modDTypep->width();
        സ്ഥിരം പൂർണ്ണസംഖ്യ conwidth = conDTypep->width();

        // Connection has the width of the port, connect it to every element
        എങ്കിൽ (conwidth == modwidth) തിരികെ അസത്യം;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        എങ്കിൽ (conwidth == modwidth * elements * subElems) {
            എങ്കിൽ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ഇടത്_Const() << ":"
                                                     << rangep->വലത്_Const() << "]");
            }
            സ്ഥിരം പൂർണ്ണസംഖ്യ selwidth = modwidth * subElems;
            // Connection is self-determined
            userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം fromp = VN_AS(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം)->unlinkFrBack();
            AstSel* സ്ഥിരം selp = പുതിയ AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ഇടത്), selwidth};
            selp->didWidth(സത്യം);
            nodep->exprp(selp);
            തിരികെ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        userIterateAndNext(nodep->exprp(), WidthVP{conDTypep, FINAL, STREAM_USE_ASSIGN}.p());
        തിരികെ സത്യം;
    }

    ശൂന്യം visit(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        എങ്കിൽ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* സ്ഥിരം concatp = VN_CAST(nodep->exprp(), Replicate);
            സ്ഥിരം AstConst* സ്ഥിരം countp = concatp ? VN_CAST(concatp->countp(), Const) : nullptr;
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            എങ്കിൽ (countp && countp->toUInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                AstPattern* സ്ഥിരം patternp = പുതിയ AstPattern{concatp->fileline(), nullptr};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(തള്ളുക_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_inParamOverride);
            m_inParamOverride = isOverridePattern(nodep->exprp());
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
        } അല്ലെങ്കിൽ എങ്കിൽ (!m_paramsOnly) {
            എങ്കിൽ (!nodep->modVarp()->didWidth()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), nullptr);
            }
            എങ്കിൽ (!nodep->exprp()) {  // No-connect
                എങ്കിൽ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                തിരികെ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            സ്ഥിരം VDirection pinDirection = nodep->modVarp()->direction();
            എങ്കിൽ (VN_IS(nodep->exprp(), കണ്ണി_Stream) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            userIterateAndNext(nodep->exprp(), WidthVP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_കണ്ണി_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_കണ്ണി_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* സ്ഥിരം modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                സ്ഥിരം AstIfaceRefDType* സ്ഥിരം conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                എങ്കിൽ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_കണ്ണി_DType* subDTypep = modDTypep;
            സ്ഥിരം പൂർണ്ണസംഖ്യ modwidth = modDTypep->width();
            പൂർണ്ണസംഖ്യ conwidth = conDTypep->width();
            എങ്കിൽ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                userIterateAndNext(nodep->exprp(),
                                   WidthVP{subDTypep, FINAL, STREAM_USE_ASSIGN}.p());
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    എങ്കിൽ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        എങ്കിൽ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
                        തിരികെ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->width();
                }
                എങ്കിൽ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->modVarp()->isTristate()) {
                    എങ്കിൽ (modwidth != conwidth) {
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
                } അല്ലെങ്കിൽ എങ്കിൽ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                എങ്കിൽ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം വ്യഞ്ജകം_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                എങ്കിൽ (വ്യഞ്ജകം_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpackedArraySize(conDTypep) != unpackedArraySize(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpackedArraySize(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpackedArraySize(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } അല്ലെങ്കിൽ എങ്കിൽ ((വ്യഞ്ജകം_Arrayp && !modArrayp) || (!വ്യഞ്ജകം_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (വ്യഞ്ജകം_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } അല്ലെങ്കിൽ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_കണ്ണി_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), കണ്ണി_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterateCheckAssign(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            എങ്കിൽ (nodep->modVarp()->isIfaceArraySplit()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    ശൂന്യം visit(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        എങ്കിൽ (!m_paramsOnly) {
            എങ്കിൽ (VN_IS(nodep->modp(), NotFoundModule)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_Path_Looked_സന്ദേശം(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            എങ്കിൽ (nodep->rangep()) userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->pinsp(), nullptr);
            // If element of an instance array, pins are now connected, no longer need the ranges
            എങ്കിൽ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                തള്ളുക_Deletep(nodep->rangep()->unlinkFrBackWithNext());
            }
        }
        userIterateAndNext(nodep->paramsp(), nullptr);
    }
    ശൂന്യം visit(AstGatePin* nodep) override {
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->rangep(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->exprp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം pinDTypep = nodep->exprp()->dtypep();
            സ്ഥിരം പൂർണ്ണസംഖ്യ അക്കം_Insts = nodep->rangep()->elementsConst();
            സ്ഥിരം പൂർണ്ണസംഖ്യ modwidth = അക്കം_Insts;
            സ്ഥിരം പൂർണ്ണസംഖ്യ pinwidth = pinDTypep->width();
            എങ്കിൽ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                userIterateAndNext(nodep->exprp(), WidthVP{subDTypep, FINAL}.p());
                Ast_കണ്ണി* സ്ഥിരം newp
                    = പുതിയ AstReplicate{nodep->fileline(), nodep->exprp()->unlinkFrBack(),
                                       static_cast<uint32_t>(അക്കം_Insts)};
                nodep->replaceWith(newp);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
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
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        എങ്കിൽ (nodep->doingWidth()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(സത്യം);
            nodep->didWidth(സത്യം);
            തിരികെ;
        }
        എങ്കിൽ (nodep->classMethod() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->classMethod() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        എങ്കിൽ (nodep->classMethod() && nodep->name() == "new") {
            എങ്കിൽ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            എങ്കിൽ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_കണ്ണി* firstp = nullptr;
            വേണ്ടി (Ast_കണ്ണി* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                എങ്കിൽ (Ast_Stmt_വ്യഞ്ജകം* സ്ഥിരം sep = VN_CAST(itemp, Stmt_വ്യഞ്ജകം)) {
                    എങ്കിൽ (AstNew* സ്ഥിരം newp = VN_CAST(sep->exprp(), New)) {
                        എങ്കിൽ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            നിർത്തുക;
                        }
                    }
                    തുടരുക;
                }
                എങ്കിൽ (firstNewStatementOkRecurse(itemp)) തുടരുക;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doingWidth(സത്യം);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        എങ്കിൽ (nodep->fvarp()) userIterate(nodep->fvarp(), nullptr);
        എങ്കിൽ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtypeSetVoid();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Property)) {
            nodep->dtypeSetBit();
        }
        WidthVP* vup = nullptr;
        എങ്കിൽ (VN_IS(nodep, Property)) vup = WidthVP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->didWidth(സത്യം);
        nodep->doingWidth(അസത്യം);
        എങ്കിൽ (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        എങ്കിൽ (nodep->fvarp() && !nodep->dpiImport() && !nodep->pureVirtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->existsAndNext([&](സ്ഥിരം AstVarRef* varrefp) {
                   തിരികെ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::vector<AstVar*> ports;
        std::vector<AstAttrOf*> protos;
        AstAttrOf* protop = nullptr;  // Base prototype
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_back(portp); }
            } അല്ലെങ്കിൽ എങ്കിൽ (AstAttrOf* സ്ഥിരം attrp = VN_CAST(stmtp, AttrOf)) {
                എങ്കിൽ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_back(attrp); }
                എങ്കിൽ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        എങ്കിൽ (protop) {
            വേണ്ടി (size_t i = 0; i < std::max(ports.size(), protos.size()); ++i) {
                എങ്കിൽ (i >= ports.size() || i >= protos.size()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    നിർത്തുക;
                } അല്ലെങ്കിൽ {
                    AstVar* സ്ഥിരം portp = ports[i];
                    AstAttrOf* സ്ഥിരം rprotop = protos[i];
                    Ast_കണ്ണി_DType* സ്ഥിരം declDtp = portp->dtypep();
                    Ast_കണ്ണി_DType* സ്ഥിരം protoDtp = rprotop->fromp()->dtypep();
                    എങ്കിൽ (portp->name() != rprotop->name()) {
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
                    } അല്ലെങ്കിൽ എങ്കിൽ (!similarDTypeRecurse(protoDtp, declDtp)) {
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
    ശൂന്യം visit(AstConstraint* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, nullptr);
        nodep->didWidth(സത്യം);
    }
    ശൂന്യം visit(AstProperty* nodep) override {
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        എങ്കിൽ (nodep->doingWidth()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(സത്യം);
            nodep->didWidth(സത്യം);
            തിരികെ;
        }
        nodep->doingWidth(സത്യം);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtypeSetBit();
        വേണ്ടി (Ast_കണ്ണി* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            എങ്കിൽ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, nullptr);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, nullptr);
            } അല്ലെങ്കിൽ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->didWidth(സത്യം);
        nodep->doingWidth(അസത്യം);
    }
    ശൂന്യം visit(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    ശൂന്യം visit(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        nodep->dtypeSetBit();
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, nullptr);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(stmtp, കണ്ണി_വ്യഞ്ജകം)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } അല്ലെങ്കിൽ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->didWidth(സത്യം);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_കണ്ണി* pkgItemp) {
        ആകുമ്പോൾ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        തിരികെ VN_CAST(pkgItemp->backp(), Package);
    }
    ശൂന്യം visit(AstFuncRef* nodep) override {
        visit(static_cast<Ast_കണ്ണി_FTask_Ref*>(nodep));
        എങ്കിൽ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            എങ്കിൽ (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_വ്യഞ്ജകം))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtypeSetVoid();
        } അല്ലെങ്കിൽ {
            nodep->dtypeFrom(nodep->taskp());
        }
        എങ്കിൽ (nodep->fileline()->timingOn()) {
            Ast_കണ്ണി_Module* സ്ഥിരം classp = nodep->classOrPackagep();
            എങ്കിൽ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* സ്ഥിരം packagep = getItemPackage(classp);
                എങ്കിൽ (packagep && packagep->name() == "std") {
                    methodCallWarnTiming(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    സ്ഥിതം തർക്കം areSameSize(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        സ്ഥിരം std::vector<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        സ്ഥിരം std::vector<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        എങ്കിൽ (dims0.size() != dims1.size()) തിരികെ അസത്യം;
        വേണ്ടി (size_t i = 0; i < dims0.size(); ++i) {
            എങ്കിൽ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) തിരികെ അസത്യം;
        }
        തിരികെ സത്യം;
    }
    // Makes sure that port and pin have same size and same datatype
    ശൂന്യം checkUnpackedArrayArgs(AstVar* portp, Ast_കണ്ണി* pinp) {
        എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം portDtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            എങ്കിൽ (AstUnpackArrayDType* സ്ഥിരം pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                എങ്കിൽ (!areSameSize(portDtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << portDtypep->prettyDTypeNameQ() << ")");
                }
                സ്ഥിരം AstBasicDType* സ്ഥിരം portBasicp = portDtypep->basicp();
                സ്ഥിരം AstBasicDType* സ്ഥിരം pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                സ്ഥിരം തർക്കം sameElemDType
                    = portBasicp && pinBasicp
                          ? portBasicp->width() == pinBasicp->width()
                                && portBasicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(portDtypep->elemDTypep());
                എങ്കിൽ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, portDtypep, "Argument", "Parameter"));
                }
            } അല്ലെങ്കിൽ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    ശൂന്യം processFTaskRefArgs(Ast_കണ്ണി_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        userIterate(nodep->taskp(), nullptr);
        //
        // And do the arguments to the task/function too
        ചെയ്യുക {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            സ്ഥിരം V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            എങ്കിൽ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
                സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
                AstArg* സ്ഥിരം argp = tconnect.second;
                Ast_കണ്ണി_വ്യഞ്ജകം* pinp = argp->exprp();
                എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                എങ്കിൽ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlinkFrBackWithNext(&handle);  // Format + additional args, if any
                    Ast_കണ്ണി_വ്യഞ്ജകം* argsp = nullptr;
                    ആകുമ്പോൾ (AstArg* സ്ഥിരം nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->addNext(nextargp->exprp()->unlinkFrBackWithNext());
                        nextargp->unlinkFrBack()->delete_വൃക്ഷം();  // Remove the call's Arg wrapper
                    }
                    string format;
                    എങ്കിൽ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->അക്കം().toString();
                    } അല്ലെങ്കിൽ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(തള്ളുക_Deletep(argp), argp);
                    AstSFormatF* സ്ഥിരം newp
                        = പുതിയ AstSFormatF{nodep->fileline(), format, അസത്യം, argsp};
                    എങ്കിൽ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(പുതിയ AstScopeName{newp->fileline(), സത്യം});
                    }
                    handle.relink(പുതിയ AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    പോകുക reloop;  // so exit early; next loop will correct it
                }  //
                അല്ലെങ്കിൽ എങ്കിൽ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlinkFrBack(&handle);  // No next, that's the next pin
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                എങ്കിൽ (portp->isWritable()) {
                    V3LinkLValue::linkLValueSet(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ആകുമ്പോൾ (അസത്യം);
        // Stage 2
        {
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
                AstVar* സ്ഥിരം portp = tconnect.first;
                സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = argp->exprp();
                എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
                // Change data types based on above accept completion
                എങ്കിൽ (nodep->taskp()->dpiImport()) checkUnpackedArrayArgs(portp, pinp);
                എങ്കിൽ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
                സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
                സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം pinp = argp->exprp();
                എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, WidthVP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        എങ്കിൽ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
                സ്ഥിരം AstVar* സ്ഥിരം portp = tconnect.first;
                സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
                Ast_കണ്ണി_വ്യഞ്ജകം* pinp = argp->exprp();
                എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
                Ast_കണ്ണി_DType* സ്ഥിരം portDTypep = portp->dtypep()->skipRefToEnump();
                എങ്കിൽ (VN_IS(pinp, EmptyQueue)) {
                    Ast_കണ്ണി_വ്യഞ്ജകം* newp = newConsDynArrayOrQueue(pinp->fileline(), portDTypep);
                    എങ്കിൽ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = പുതിയ AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(portDTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(pinp), pinp);
                    pinp = newp;
                }
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം pinDTypep = pinp->dtypep()->skipRefToEnump();
                സ്ഥിരം AstIfaceRefDType* സ്ഥിരം portIfacep
                    = VN_CAST(portDTypep->elemDTypep(സത്യം), IfaceRefDType);
                സ്ഥിരം VCastable ifaceCastable
                    = portIfacep ? Ast_കണ്ണി::computeCastable(portDTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ സത്യം)
                                 : VCastable{VCastable::UNSUPPORTED};
                സ്ഥിരം തർക്കം matchingRefDTypes = portIfacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(portDTypep, pinDTypep);
                സ്ഥിരം തർക്കം pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->അക്കം().isNull();
                എങ്കിൽ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    എങ്കിൽ (portIfacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } അല്ലെങ്കിൽ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << portDTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } അല്ലെങ്കിൽ എങ്കിൽ (portIfacep && portIfacep->isVirtual() && portp->is_നിവേശം() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } അല്ലെങ്കിൽ എങ്കിൽ (portp->isWritable() && pinp->width() != portp->width()) {
                    pinp->v3widthWarn(portp->width(), pinp->width(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->width()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->width() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlinkFrBack(&relinkHandle);
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                എങ്കിൽ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(portDTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && portDTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << portDTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    തുടരുക;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                എങ്കിൽ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } അല്ലെങ്കിൽ എങ്കിൽ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    checkClassAssign(nodep, "Function Argument", pinp, portDTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, WidthVP{portDTypep, FINAL, STREAM_USE_ASSIGN}.p());
                } അല്ലെങ്കിൽ {
                    iterateCheckAssign(nodep, "Function Argument", pinp, FINAL, portDTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    ശൂന്യം handleStdRandomizeArgs(Ast_കണ്ണി_FTask_Ref* സ്ഥിരം nodep) {
        AstConst* nullp = nullptr;
        വേണ്ടി (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = argp->exprp();
            എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(exprp, Const)) {
                എങ്കിൽ (constp->അക്കം().isNull()) {
                    nullp = constp;
                    തുടരുക;
                }
            }
            എങ്കിൽ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                തുടരുക;
            }
            എങ്കിൽ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                തുടരുക;
            }
            എങ്കിൽ (സ്ഥിരം AstCMethodHard* സ്ഥിരം methodp = VN_CAST(exprp, CMethodHard)) {
                എങ്കിൽ (methodp->method() == VCMethod::ARRAY_AT
                    || methodp->method() == VCMethod::ARRAY_AT_WRITE) {
                    തുടരുക;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        എങ്കിൽ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    ശൂന്യം visit(Ast_കണ്ണി_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        എങ്കിൽ (nodep->didWidth()) തിരികെ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = nullptr;
        എങ്കിൽ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(സത്യം);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            തിരികെ;  // Handled in V3Randomize
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* സ്ഥിരം classp = VN_CAST(nodep->classOrPackagep(), Class);
            എങ്കിൽ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(സത്യം);
                Ast_കണ്ണി_DType* സ്ഥിരം adtypep = nodep->findBitDType();
                withp = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                വേണ്ടി (സ്ഥിരം AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleStdRandomizeArgs(nodep);  // Provided args should be in current scope
                processFTaskRefArgs(nodep);
                എങ്കിൽ (withp) nodep->withp(withp);
                nodep->didWidth(സത്യം);
                തിരികെ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            എങ്കിൽ (nodep->name() == "randomize") {
                AstClassRefDType* സ്ഥിരം adtypep
                    = പുതിയ AstClassRefDType{nodep->fileline(), classp, nullptr};
                v3Global.rootp()->typeTablep()->addTypesp(adtypep);
                withp = methodWithClause(nodep, അസത്യം, അസത്യം, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                വേണ്ടി (സ്ഥിരം AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    userIterateAndNext(argp->exprp(), WidthVP{SELF, BOTH}.p());
                }
                handleRandomizeArgs(nodep, classp);
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "get_randstate") {
                methodOkArguments(nodep, 0, 0);
                classp->baseMostClassp()->needRNG(സത്യം);
                v3Global.useRandomizeMethods(സത്യം);
                AstCExpr* സ്ഥിരം newp
                    = പുതിയ AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtypeSetString();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->name() == "set_randstate") {
                methodOkArguments(nodep, 1, 1);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = nodep->argsp()->exprp();
                classp->baseMostClassp()->needRNG(സത്യം);
                v3Global.useRandomizeMethods(സത്യം);
                AstCExpr* സ്ഥിരം newp
                    = പുതിയ AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->add(exprp->unlinkFrBack());
                newp->add(")");
                newp->dtypeSetVoid();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                UASSERT_OBJ(അസത്യം, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        എങ്കിൽ (nodep->didWidth()) {
            എങ്കിൽ (withp) nodep->withp(withp);
            തിരികെ;
        }
        എങ്കിൽ ((nodep->taskp()->classMethod() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            തർക്കം allow = അസത്യം;
            എങ്കിൽ (m_ftaskp && m_ftaskp->classMethod() && !m_ftaskp->isStatic()) {
                എങ്കിൽ (സ്ഥിരം AstFuncRef* സ്ഥിരം funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstTaskRef* സ്ഥിരം taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                എങ്കിൽ (!allow) {
                    സ്ഥിരം AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    എങ്കിൽ (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    സ്ഥിരം AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    എങ്കിൽ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            എങ്കിൽ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                സ്ഥിരം AstClass* സ്ഥിരം cgClassp = m_containingClassFinder.find(nodep);
                എങ്കിൽ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    സ്ഥിരം AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    എങ്കിൽ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            എങ്കിൽ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        എങ്കിൽ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(പുതിയ AstScopeName{nodep->fileline(), അസത്യം});
        }
        // And do the arguments to the task/function too
        processFTaskRefArgs(nodep);
        എങ്കിൽ (withp) nodep->withp(withp);
        nodep->didWidth(സത്യം);
        // See steps that follow in visit(AstFuncRef*)
    }
    ശൂന്യം visit(Ast_കണ്ണി_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, nullptr);
    }
    ശൂന്യം visit(AstSenItem* nodep) override {
        എങ്കിൽ (nodep->isComboStar()) തിരികെ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        എങ്കിൽ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            auto* സ്ഥിരം concatOrReplp = VN_CAST(nodep->sensp(), കണ്ണി_Biop);
            auto* സ്ഥിരം rhsp = concatOrReplp->rhsp()->unlinkFrBack();
            എങ്കിൽ (nodep->edgeType() == VEdgeType::ET_CHANGED) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                auto* സ്ഥിരം lhsp = concatOrReplp->lhsp()->unlinkFrBack();
                nodep->addNextHere(പുതിയ AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(പുതിയ AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
        } അല്ലെങ്കിൽ {
            userIterateChildren(nodep, WidthVP{SELF, BOTH}.p());
            എങ്കിൽ (nodep->edgeType().anEdge()) {
                Ast_കണ്ണി_DType* സ്ഥിരം sensDtp = nodep->sensp()->dtypep()->skipRefp();
                എങ്കിൽ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } അല്ലെങ്കിൽ എങ്കിൽ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    ശൂന്യം visit(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* സ്ഥിരം refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, WidthVP{SELF, PRELIM}.p());
    }
    ശൂന്യം visit(AstWait* nodep) override {
        എങ്കിൽ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlinkFrBack()->delete_വൃക്ഷം(), nodep);
            തിരികെ;
        }
        എങ്കിൽ (nodep->fileline()->timingOn()) {
            എങ്കിൽ (v3Global.opt.timing().isSetTrue()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം varrefp = VN_CAST(nodep->condp(), കണ്ണി_Var_Ref)) {
                    എങ്കിൽ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warnMore() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (v3Global.opt.timing().isSetFalse()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } അല്ലെങ്കിൽ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം stmtsp = nodep->stmtsp()) {
            stmtsp->unlinkFrBackWithNext();
            nodep->replaceWith(stmtsp);
        } അല്ലെങ്കിൽ {
            nodep->unlinkFrBack();
        }
        VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
    }
    ശൂന്യം visit(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->indexArgRefp(), nullptr);
        userIterateChildren(nodep->മൂല്യം_Arg_Refp(), nullptr);
        എങ്കിൽ (!nodep->exprp()) {
            nodep->dtypeSetVoid();
        } അല്ലെങ്കിൽ {
            എങ്കിൽ (!nodep->exprp()->hasDType()) {
                userIterateAndNext(nodep->exprp(), nullptr);
            } അല്ലെങ്കിൽ {
                എങ്കിൽ (vdtypep) {
                    userIterateAndNext(nodep->exprp(), WidthVP{nodep->dtypep(), PRELIM}.p());
                } അല്ലെങ്കിൽ {  // 'sort with' allows arbitrary type
                    userIterateAndNext(nodep->exprp(), WidthVP{SELF, PRELIM}.p());
                }
            }

            എങ്കിൽ (!nodep->exprp()->hasDType()) {
                nodep->dtypeSetVoid();
            } അല്ലെങ്കിൽ {
                nodep->dtypeFrom(nodep->exprp());
                iterateCheckAssign(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    ശൂന്യം visit(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        എങ്കിൽ (nodep->index()) {
            nodep->dtypeFrom(m_withp->indexArgRefp());
        } അല്ലെങ്കിൽ {
            nodep->dtypeFrom(m_withp->മൂല്യം_Arg_Refp());
        }
    }
    ശൂന്യം visit(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, nullptr);
    }

    //--------------------
    // Default
    ശൂന്യം visit(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) override {
        എങ്കിൽ (!nodep->didWidth()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, nullptr);
    }
    ശൂന്യം visit(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << endl);
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        എങ്കിൽ (Ast_കണ്ണി_DType* സ്ഥിരം vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } അല്ലെങ്കിൽ എങ്കിൽ (!nodep->dtypep()) {
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
        nodep->didWidth(സത്യം);
    }
    ശൂന്യം visitClass(AstClass* nodep) {
        എങ്കിൽ (nodep->didWidthAndSet()) തിരികെ;

        // If the class is std::process
        എങ്കിൽ (nodep->name() == "process") {
            AstPackage* സ്ഥിരം packagep = getItemPackage(nodep);
            എങ്കിൽ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                എങ്കിൽ (AstVar* സ്ഥിരം varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_കണ്ണി_DType* സ്ഥിരം dtypep = varp->getChildDTypep();
                    എങ്കിൽ (!varp->dtypep()) {
                        VL_DO_DANGLING(തള്ളുക_Deletep(dtypep->unlinkFrBack()), dtypep);
                    }
                    AstBasicDType* സ്ഥിരം newdtypep = പുതിയ AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->addTypesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                എങ്കിൽ (Ast_കണ്ണി_FTask* സ്ഥിരം ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), കണ്ണി_FTask)) {
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
    ശൂന്യം visit(Ast_കണ്ണി_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enumTempNames);
        VL_RESTORER_COPY(m_insideTempNames);
        എങ്കിൽ (AstClass* സ്ഥിരം classp = VN_CAST(nodep, Class)) {
            visitClass(classp);
        } അല്ലെങ്കിൽ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ഇത്](സ്ഥിരം AstVar* varp) { m_curModVars.തിരുകുക(varp); });
            userIterateChildren(nodep, nullptr);
        }
    }
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, nullptr);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    ശൂന്യം visit_Or_Lu64(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    ശൂന്യം visit(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            എങ്കിൽ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    പുതിയ AstISToRD{nodep->fileline(), nodep->lhsp()->unlinkFrBack()});
                VL_DO_DANGLING(nodep->delete_വൃക്ഷം(), nodep);
            }
        }
    }
    ശൂന്യം visit(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtypeSetDouble();
            // Self-determined operand (TODO check if numeric type)
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
        }
    }
    ശൂന്യം visit_Os32_Lr(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetInteger2State();
        }
    }
    ശൂന്യം visit_Ou64_Lr(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetUInt64();
        }
    }

    ശൂന്യം visit_log_not(AstLogNot* nodep) {
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
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            എങ്കിൽ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* സ്ഥിരം newp
                    = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            }
        }
    }
    ശൂന്യം visit_log_and_or(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBitOrLogic(
                Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    ശൂന്യം visitAbortProp(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit_red_and_or(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtypeSetBitOrLogic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    ശൂന്യം visit_red_unknown(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            nodep->dtypeSetBit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    തർക്കം isEquivalentDType(സ്ഥിരം Ast_കണ്ണി_DType* lhs, സ്ഥിരം Ast_കണ്ണി_DType* rhs) {
        // a) If two types match, they are equivalent.
        എങ്കിൽ (!lhs || !rhs) തിരികെ അസത്യം;
        lhs = lhs->skipRefToEnump();
        rhs = rhs->skipRefToEnump();
        എങ്കിൽ (lhs == rhs) തിരികെ സത്യം;
        // If both are basic types, check if they are the same type
        എങ്കിൽ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            സ്ഥിരം auto* lb = VN_CAST(lhs, BasicDType);
            സ്ഥിരം auto* rb = VN_CAST(rhs, BasicDType);
            എങ്കിൽ (lb->isString() != rb->isString()) തിരികെ അസത്യം;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        സ്ഥിരം തർക്കം lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        സ്ഥിരം തർക്കം rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        എങ്കിൽ (lhsIsUnpackArray || rhsIsUnpackArray) {
            എങ്കിൽ (lhsIsUnpackArray && rhsIsUnpackArray) {
                സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം lhsp = VN_CAST(lhs, UnpackArrayDType);
                സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം rhsp = VN_CAST(rhs, UnpackArrayDType);
                സ്ഥിരം പൂർണ്ണസംഖ്യ lsz = lhsp->elementsConst();
                സ്ഥിരം പൂർണ്ണസംഖ്യ rsz = rhsp->elementsConst();
                എങ്കിൽ (lsz >= 0 && rsz >= 0 && lsz != rsz) തിരികെ അസത്യം;
                തിരികെ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            തിരികെ അസത്യം;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        സ്ഥിരം തർക്കം lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        സ്ഥിരം തർക്കം rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        സ്ഥിരം തർക്കം lhsIsQueue = VN_IS(lhs, QueueDType);
        സ്ഥിരം തർക്കം rhsIsQueue = VN_IS(rhs, QueueDType);
        സ്ഥിരം തർക്കം lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        സ്ഥിരം തർക്കം rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        എങ്കിൽ (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം lhsp = VN_CAST(lhs, DynArrayDType)) {
                എങ്കിൽ (സ്ഥിരം AstDynArrayDType* സ്ഥിരം rhsp = VN_CAST(rhs, DynArrayDType)) {
                    തിരികെ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം lhsp = VN_CAST(lhs, QueueDType)) {
                എങ്കിൽ (സ്ഥിരം AstQueueDType* സ്ഥിരം rhsp = VN_CAST(rhs, QueueDType)) {
                    തിരികെ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* സ്ഥിരം lhsp = VN_CAST(lhs, AssocArrayDType)) {
                എങ്കിൽ (സ്ഥിരം AstAssocArrayDType* സ്ഥിരം rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    തിരികെ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->താക്കോൽ_DTypep(), rhsp->താക്കോൽ_DTypep());
                }
            }

            തിരികെ അസത്യം;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        എങ്കിൽ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            എങ്കിൽ (lhs->width() != rhs->width()) തിരികെ അസത്യം;
            എങ്കിൽ (lhs->isFourstate() != rhs->isFourstate()) തിരികെ അസത്യം;
            എങ്കിൽ (lhs->isSigned() != rhs->isSigned()) തിരികെ അസത്യം;
            തിരികെ സത്യം;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        എങ്കിൽ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) തിരികെ അസത്യം;
        തിരികെ !(VN_IS(lhs, കണ്ണി_UOr_Struct_DType) || VN_IS(rhs, കണ്ണി_UOr_Struct_DType));
    }

    സ്ഥിതം തർക്കം isAggregateType(സ്ഥിരം Ast_കണ്ണി* nodep) {
        എങ്കിൽ (!nodep) തിരികെ അസത്യം;
        സ്ഥിരം Ast_കണ്ണി_DType* dtypep = nodep->dtypep();
        എങ്കിൽ (!dtypep) തിരികെ അസത്യം;
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (!dtypep) തിരികെ അസത്യം;
        തിരികെ dtypep->isAggregateType();
    }

    ശൂന്യം visit_cmp_eq_gt(Ast_കണ്ണി_Biop* nodep, സ്ഥിരം ComparisonType cmpType) {
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
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            സ്ഥിരം തർക്കം lhsIsPat = VN_IS(nodep->lhsp(), Pattern);
            സ്ഥിരം തർക്കം rhsIsPat = VN_IS(nodep->rhsp(), Pattern);
            എങ്കിൽ (lhsIsPat && !rhsIsPat) {
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->lhsp(), WidthVP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } അല്ലെങ്കിൽ എങ്കിൽ (rhsIsPat && !lhsIsPat) {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } അല്ലെങ്കിൽ {
                userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
                userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            }

            സ്ഥിരം തർക്കം isAggrLhs = isAggregateType(nodep->lhsp());
            സ്ഥിരം തർക്കം isAggrRhs = isAggregateType(nodep->rhsp());

            എങ്കിൽ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::NORMAL_EQ || cmpType == ComparisonType::CASE)) {
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDType = nodep->lhsp()->dtypep();
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsDType = nodep->rhsp()->dtypep();

                എങ്കിൽ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_കണ്ണി* സ്ഥിരം newp
                        = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convertUnpackedOrder(nodep->rhsp(), nodep->lhsp()->dtypep());
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                എങ്കിൽ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_കണ്ണി* സ്ഥിരം newp
                        = പുതിയ AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                    തിരികെ;
                }
                എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } അല്ലെങ്കിൽ {
                സ്ഥിരം തർക്കം signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                സ്ഥിരം പൂർണ്ണസംഖ്യ ewidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                തർക്കം warnOn = സത്യം;
                എങ്കിൽ (!signedFl && width == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    എങ്കിൽ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->width() >= nodep->rhsp()->widthMin())) {
                        warnOn = അസത്യം;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    എങ്കിൽ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->widthMin() >= nodep->rhsp()->width())) {
                        warnOn = അസത്യം;
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
    ശൂന്യം visit_cmp_real(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit_cmp_string(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetBit();
        }
    }
    ശൂന്യം visit_cmp_type(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, BOTH}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{SELF, BOTH}.p());
            സ്ഥിരം AstAttrOf* സ്ഥിരം lhsap = VN_AS(nodep->lhsp(), AttrOf);
            സ്ഥിരം AstAttrOf* സ്ഥിരം rhsap = VN_AS(nodep->rhsp(), AttrOf);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDtp = lhsap->dtypep();
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            സ്ഥിരം തർക്കം invert = VN_IS(nodep, NeqT);
            സ്ഥിരം തർക്കം identical
                = Ast_കണ്ണി::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            സ്ഥിരം തർക്കം eq = invert ^ identical;
            Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        }
    }

    ശൂന്യം visit_negate_not(Ast_കണ്ണി_Uniop* nodep, തർക്കം real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            എങ്കിൽ (!real_ok) checkCvtUS(nodep->lhsp(), അസത്യം);
        }
        എങ്കിൽ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            എങ്കിൽ (Ast_കണ്ണി_Uniop* സ്ഥിരം newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtypeSetDouble();
                തിരികെ;
            }
        } അല്ലെങ്കിൽ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        എങ്കിൽ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            // Some warning suppressions
            തർക്കം lhsWarn = സത്യം;
            എങ്കിൽ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                എങ്കിൽ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = അസത്യം;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    ശൂന്യം visit_signed_unsigned(Ast_കണ്ണി_Uniop* nodep, VSigning rs_out) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), സത്യം);
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = nodep->lhsp()->width();
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = nodep->findBitOrLogicDType(
                width, width, rs_out, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ശൂന്യം visit_shift(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_shift_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        സ്ഥിരം Ast_കണ്ണി_Biop* സ്ഥിരം newp = iterate_shift_final(nodep);
        VL_DANGLING(nodep);
        (ശൂന്യം)newp;  // Ununused
    }
    ശൂന്യം iterate_shift_prelim(Ast_കണ്ണി_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {
            userIterateAndNext(nodep->lhsp(), WidthVP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), അസത്യം);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            എങ്കിൽ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtypeSetLogicUnsized(nodep->width(), nodep->widthMin(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_കണ്ണി_Biop* iterate_shift_final(Ast_കണ്ണി_Biop* nodep) {
        // Nodep maybe edited
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            എങ്കിൽ (VN_IS(nodep, ShiftRS)) {
                എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            തർക്കം warnOn = സത്യം;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            എങ്കിൽ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), കണ്ണി_Assign)) warnOn = അസത്യം;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* സ്ഥിരം shiftp = VN_CAST(nodep->rhsp(), Const);
            എങ്കിൽ (shiftp && !shiftp->അക്കം().isFourState() && shiftp->width() <= 32) {
                സ്ഥിരം int64_t shiftVal = shiftp->അക്കം().toSQuad();
                എങ്കിൽ (VN_IS(nodep, ShiftL)) {
                    എങ്കിൽ (shiftVal > 0 && nodep->width() == nodep->lhsp()->width() + shiftVal)
                        warnOn = അസത്യം;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            എങ്കിൽ (nodep->rhsp()->width() > 32) {
                എങ്കിൽ (shiftp && shiftp->അക്കം().mostSetBitP1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_സംഖ്യ അക്കം{shiftp, 32, shiftp->അക്കം()};
                    Ast_കണ്ണി* സ്ഥിരം shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(പുതിയ AstConst{shiftrhsp->fileline(), അക്കം});
                    VL_DO_DANGLING(shiftrhsp->delete_വൃക്ഷം(), shiftrhsp);
                }
            }
        }
        തിരികെ nodep;  // May edit
    }

    ശൂന്യം visit_boolexpr_and_or(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), അസത്യം);
            checkCvtUS(nodep->rhsp(), അസത്യം);
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
            സ്ഥിരം പൂർണ്ണസംഖ്യ mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
            സ്ഥിരം തർക്കം expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtypeSetBitOrLogicUnsized(
                width, mwidth, VSigning::fromBool(expSigned),
                Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        എങ്കിൽ (m_vup->final()) {
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ശൂന്യം visit_add_sub_replace(Ast_കണ്ണി_Biop* nodep, തർക്കം real_ok, സ്ഥിരം തർക്കം modOrDiv) {
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
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            userIterateAndNext(nodep->lhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            userIterateAndNext(nodep->rhsp(), WidthVP{CONTEXT_DET, PRELIM}.p());
            എങ്കിൽ (!real_ok) {
                checkCvtUS(nodep->lhsp(), അസത്യം);
                checkCvtUS(nodep->rhsp(), അസത്യം);
            }
            എങ്കിൽ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtypeSetDouble();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, Add)
                            ? (nodep->warnMore()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ;
            } അല്ലെങ്കിൽ {
                സ്ഥിരം പൂർണ്ണസംഖ്യ width = std::max(nodep->lhsp()->width(), nodep->rhsp()->width());
                സ്ഥിരം പൂർണ്ണസംഖ്യ mwidth = std::max(nodep->lhsp()->widthMin(), nodep->rhsp()->widthMin());
                സ്ഥിരം തർക്കം expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtypeSetBitOrLogicUnsized(
                    width, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        എങ്കിൽ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_കണ്ണി_DType* സ്ഥിരം subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            എങ്കിൽ (Ast_കണ്ണി_Biop* സ്ഥിരം newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            തർക്കം lhsWarn = സത്യം;
            തർക്കം rhsWarn = സത്യം;
            എങ്കിൽ (VN_IS(nodep, Add) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                എങ്കിൽ (subDTypep->widthMin() == (nodep->lhsp()->widthMin() + 1)) lhsWarn = അസത്യം;
                എങ്കിൽ (subDTypep->widthMin() == (nodep->rhsp()->widthMin() + 1)) rhsWarn = അസത്യം;
                എങ്കിൽ (VN_IS(nodep, Add) && nodep->lhsp()->width() == 1
                    && nodep->rhsp()->width() != 1)
                    lhsWarn = അസത്യം;  // do_increment + ...
                എങ്കിൽ (nodep->rhsp()->width() == 1 && nodep->lhsp()->width() != 1)
                    rhsWarn = അസത്യം;  // ... + do_increment
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                എങ്കിൽ (subDTypep->widthMin() >= (nodep->lhsp()->widthMin())) lhsWarn = അസത്യം;
                എങ്കിൽ (subDTypep->widthMin() >= (nodep->rhsp()->widthMin())) rhsWarn = അസത്യം;
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
    ശൂന്യം visit_real_add_sub(Ast_കണ്ണി_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }
    ശൂന്യം visit_real_neg_ceil(Ast_കണ്ണി_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_വ്യഞ്ജകം(nodep);
        എങ്കിൽ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtypeSetDouble();
        }
    }

    ശൂന്യം visit_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->വ്യഞ്ജകം_Format_2(), nodep, "Assumes constant format");
        തർക്കം inPct = അസത്യം;
        Ast_കണ്ണി_വ്യഞ്ജകം* argp = nodep->exprsp();
        string newFormat;
        വേണ്ടി (അക്ഷരം ch : nodep->text()) {
            എങ്കിൽ (!inPct && ch == '%') {
                inPct = സത്യം;
                newFormat += ch;
            } അല്ലെങ്കിൽ എങ്കിൽ (inPct && (std::isdigit(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } അല്ലെങ്കിൽ എങ്കിൽ (!inPct) {  // Normal text
                newFormat += ch;
            } അല്ലെങ്കിൽ {
                inPct = അസത്യം;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം nextp = argp ? VN_AS(argp->nextp(), കണ്ണി_വ്യഞ്ജകം) : nullptr;
                AstSFormatArg* സ്ഥിരം fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_കണ്ണി_വ്യഞ്ജകം* subargp = fargp ? fargp->exprp() : argp;
                സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : nullptr;
                ch = std::tolower(ch);
                തിരഞ്ഞെടുപ്പ് (ch) {
                അവസ്ഥ '?':  // Unspecified by user, guess
                    എങ്കിൽ (dtypep->isDouble()) {
                        ch = 'g';
                    } അല്ലെങ്കിൽ എങ്കിൽ (dtypep->isString()) {
                        ch = 's';
                    } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } അല്ലെങ്കിൽ {
                        ch = 'p';
                    }
                    argp = nextp;
                    നിർത്തുക;
                അവസ്ഥ '%': നിർത്തുക;  // %% - just output a %
                അവസ്ഥ 'm': നിർത്തുക;  // %m - auto insert "name"
                അവസ്ഥ 'l': നിർത്തുക;  // %m - auto insert "library"
                അവസ്ഥ 'd':
                    എങ്കിൽ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        എങ്കിൽ (subargp->isDouble()) spliceCvtS(subargp, സത്യം, 64);
                        argp = nextp;
                    }
                    നിർത്തുക;
                അവസ്ഥ 'b':  // FALLTHRU
                അവസ്ഥ 'o':  // FALLTHRU
                അവസ്ഥ 'x':
                    എങ്കിൽ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    നിർത്തുക;
                അവസ്ഥ 't':  // Decimal/float time
                    എങ്കിൽ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    നിർത്തുക;
                അവസ്ഥ 'e':  // FALLTHRU
                അവസ്ഥ 'f':  // FALLTHRU
                അവസ്ഥ 'g':
                    എങ്കിൽ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        എങ്കിൽ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    നിർത്തുക;
                അവസ്ഥ 'p':  // FALLTHRU
                അവസ്ഥ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    എങ്കിൽ (subargp && !fargp) {
                        എങ്കിൽ (AstEnumDType* സ്ഥിരം enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlinkFrBack(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    നിർത്തുക;
                സ്ഥിരസ്ഥിതി:  // Most operators, just move to next argument
                    argp = nextp;
                    നിർത്തുക;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    സ്ഥിതം AstEnumDType* formatEnumDType(Ast_കണ്ണി_വ്യഞ്ജകം* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skipRefToEnump(), EnumDType);
        എങ്കിൽ (!enumDtp) {
            എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skipRefToEnump(), EnumDType);
            }
        }
        തിരികെ enumDtp;
    }

    Ast_വ്യഞ്ജകം_Stmt* newEnumCapture(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, AstEnumDType* dtypep) {
        File_വരി* സ്ഥിരം flp = valuep->fileline();
        AstVar* സ്ഥിരം varp = പുതിയ AstVar{flp, VVarType::XTEMP, m_enumTempNames.get(valuep), dtypep};
        varp->noSubst(സത്യം);
        എങ്കിൽ (m_ftaskp) {
            varp->funcLocal(സത്യം);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } അല്ലെങ്കിൽ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->addStmtsp(varp);
        }
        iterate(varp);
        Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം exprp = പുതിയ Ast_വ്യഞ്ജകം_Stmt{
            flp, പുതിയ AstAssign{flp, പുതിയ AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            പുതിയ AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        തിരികെ exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_കണ്ണി_വ്യഞ്ജകം* valuep, AstEnumDType* dtypep) {
        എങ്കിൽ (valuep->isPure()) valuep = V3Const::constifyEdit(valuep);
        Ast_കണ്ണി_വ്യഞ്ജകം* keyp;
        എങ്കിൽ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_വൃക്ഷം(അസത്യം);
        } അല്ലെങ്കിൽ {
            Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_വൃക്ഷം_Pure(അസത്യം);
        }
        സ്ഥിരം VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* സ്ഥിരം argp = പുതിയ AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        തിരികെ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    തർക്കം widthBad(Ast_കണ്ണി* nodep, Ast_കണ്ണി_DType* expDTypep) {
        സ്ഥിരം പൂർണ്ണസംഖ്യ expWidth = expDTypep->width();
        പൂർണ്ണസംഖ്യ expWidthMin = expDTypep->widthMin();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        എങ്കിൽ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) തിരികെ അസത്യം;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->width() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(expWidth != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        എങ്കിൽ (expWidthMin == 0) expWidthMin = expWidth;
        എങ്കിൽ (nodep->dtypep()->width() == expWidth) തിരികെ അസത്യം;
        എങ്കിൽ (nodep->dtypep()->widthSized() && nodep->width() != expWidthMin) തിരികെ സത്യം;
        എങ്കിൽ (!nodep->dtypep()->widthSized() && nodep->widthMin() > expWidthMin) തിരികെ സത്യം;
        തിരികെ അസത്യം;
    }

    ശൂന്യം fixWidthExtend(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, Ast_കണ്ണി_DType* expDTypep, ExtendRule extendRule) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<പൂർണ്ണസംഖ്യ>(extendRule) << ") old: " << nodep);
        എങ്കിൽ (extendRule == EXTEND_OFF) തിരികെ;
        AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        സ്ഥിരം പൂർണ്ണസംഖ്യ expWidth = expDTypep->width();
        എങ്കിൽ (constp && !constp->അക്കം().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_സംഖ്യ അക്കം{nodep, expWidth, constp->അക്കം()};
            അക്കം.isSigned(അസത്യം);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), അക്കം};
            constp->replaceWith(newp);
            VL_DO_DANGLING(തള്ളുക_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } അല്ലെങ്കിൽ എങ്കിൽ (expWidth < nodep->width()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstSel{nodep->fileline(), nodep, 0, expWidth};
            newp->didWidth(സത്യം);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } അല്ലെങ്കിൽ {
            // Extend
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            തർക്കം doSigned = അസത്യം;
            തിരഞ്ഞെടുപ്പ് (extendRule) {
            അവസ്ഥ EXTEND_ZERO: doSigned = അസത്യം; നിർത്തുക;
            അവസ്ഥ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); നിർത്തുക;
            അവസ്ഥ EXTEND_LHS: doSigned = nodep->isSigned(); നിർത്തുക;
            സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("bad case");
            }
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp
                = (doSigned ? static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_കണ്ണി_വ്യഞ്ജകം*>(പുതിയ AstExtend{nodep->fileline(), nodep}));
            newp->didWidth(സത്യം);
            linker.relink(newp);
            nodep = newp;
        }
        എങ്കിൽ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = spliceCvtD(nodep);
            nodep = newp;
        }
        എങ്കിൽ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtypeChgWidthSigned(expWidth, expDTypep->widthMin(), expDTypep->numeric());
        } അല്ലെങ്കിൽ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    ശൂന്യം fixWidthReduce(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        സ്ഥിരം പൂർണ്ണസംഖ്യ expWidth = 1;
        സ്ഥിരം പൂർണ്ണസംഖ്യ expSigned = അസത്യം;
        എങ്കിൽ (nodep->width() == expWidth) തിരികെ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        എങ്കിൽ (constp) {
            V3_സംഖ്യ അക്കം(nodep, expWidth);
            അക്കം.opRedOr(constp->അക്കം());
            അക്കം.isSigned(expSigned);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), അക്കം};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_വൃക്ഷം(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } അല്ലെങ്കിൽ {
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            Ast_കണ്ണി_DType* സ്ഥിരം dtypeSkipp = nodep->dtypep()->skipRefp();
            എങ്കിൽ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = പുതിയ AstNeq{nodep->fileline(),
                                  പുതിയ AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } അല്ലെങ്കിൽ {
                // But, AstRedOr can be faster on wide
                newp = പുതിയ AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtypeChgWidthSigned(expWidth, expWidth, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    തർക്കം fixAutoExtend(Ast_കണ്ണി_വ്യഞ്ജകം*& nodepr, പൂർണ്ണസംഖ്യ expWidth) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        എങ്കിൽ (AstConst* സ്ഥിരം constp = VN_CAST(nodepr, Const)) {
            എങ്കിൽ (constp->അക്കം().autoExtend() && !constp->അക്കം().sized() && constp->width() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_സംഖ്യ അക്കം(constp, expWidth);
                അക്കം.opRepl(constp->അക്കം(), expWidth);  // {width{'1}}
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{constp->fileline(), അക്കം};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_വൃക്ഷം(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                തിരികെ സത്യം;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            അല്ലെങ്കിൽ എങ്കിൽ (!constp->അക്കം().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && expWidth > 32 && constp->അക്കം().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << expWidth
                                                  << " bits: " << constp->prettyName());
                V3_സംഖ്യ അക്കം(constp, expWidth);
                അക്കം.opExtendXZ(constp->അക്കം(), constp->width());
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{constp->fileline(), അക്കം};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_വൃക്ഷം(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                തിരികെ സത്യം;
            }
        }
        തിരികെ അസത്യം;  // No change
    }
    തർക്കം isBaseClassRecurse(സ്ഥിരം AstClass* സ്ഥിരം cls1p, സ്ഥിരം AstClass* സ്ഥിരം cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        എങ്കിൽ (cls1p == cls2p) തിരികെ സത്യം;
        വേണ്ടി (സ്ഥിരം AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            എങ്കിൽ (isBaseClassRecurse(cls1p, cextp->classp())) തിരികെ സത്യം;
        }
        തിരികെ അസത്യം;
    }
    // Lines to end a message on nodep with the data types of its operands
    സ്ഥിതം string warnDTypes(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം Ast_കണ്ണി_DType* lhsDtp,
                             സ്ഥിരം Ast_കണ്ണി_DType* rhsDtp, സ്ഥിരം string& lhsName = "Left-hand",
                             സ്ഥിരം string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        എങ്കിൽ (lhsState == rhsState) lhsState = rhsState = "";
        തിരികെ "\n" + nodep->warnMore() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warnMore()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    സ്ഥിതം തർക്കം unpackedArraySizesMatch(സ്ഥിരം Ast_കണ്ണി_DType* lhsDtp, സ്ഥിരം Ast_കണ്ണി_DType* rhsDtp) {
        ആകുമ്പോൾ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            എങ്കിൽ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                തിരികെ അസത്യം;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        തിരികെ സത്യം;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    template <typename T, typename N>
    ശൂന്യം checkUnpackedArrayAssignmentCompatible(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം T* സ്ഥിരം lhsRefp,
                                                സ്ഥിരം N* സ്ഥിരം rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::മൂല്യം || std::is_same<T, Ast_കണ്ണി_Var_Ref>::മൂല്യം)
                && (std::is_same<N, AstVar>::മൂല്യം || std::is_same<N, Ast_കണ്ണി_Var_Ref>::മൂല്യം),
            "Unsupported types provided.");
        എങ്കിൽ (!lhsRefp || !rhsRefp) തിരികെ;
        std::string lhsName;
        std::string rhsName;
        എങ്കിൽ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } അല്ലെങ്കിൽ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsDtp = lhsRefp->dtypep()->skipRefp();
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsDtp = rhsRefp->dtypep()->skipRefp();
        സ്ഥിരം തർക്കം isLhsAggregate = lhsDtp->isAggregateType();
        സ്ഥിരം തർക്കം isRhsAggregate = rhsDtp->isAggregateType();
        എങ്കിൽ (!isLhsAggregate && !isRhsAggregate) തിരികെ;
        എങ്കിൽ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            തിരികെ;
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, EmptyQueueDType)) {
            തിരികെ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(അസത്യം),
                                      rhsDim = rhsDtp->dimensions(അസത്യം);
        // Check if unpacked array dimensions are matching
        എങ്കിൽ (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            തിരികെ;
        }

        സ്ഥിരം Ast_കണ്ണി_DType* lhsDtpIterp = lhsDtp;
        സ്ഥിരം Ast_കണ്ണി_DType* rhsDtpIterp = rhsDtp;
        വേണ്ടി (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            എങ്കിൽ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                തിരികെ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        എങ്കിൽ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    ശൂന്യം checkClassAssign(സ്ഥിരം Ast_കണ്ണി* nodep, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* rhsp,
                          Ast_കണ്ണി_DType* സ്ഥിരം lhsDTypep, തർക്കം isOutputArg = അസത്യം) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം lhsRawDTypep = lhsDTypep->skipRefp();
        സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsRawDTypep = rhsp->dtypep()->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            എങ്കിൽ (സ്ഥിരം AstClassRefDType* സ്ഥിരം rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                സ്ഥിരം AstClass* സ്ഥിരം dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                സ്ഥിരം AstClass* സ്ഥിരം srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                എങ്കിൽ (isBaseClassRecurse(dstp, srcp)) തിരികെ;
            } അല്ലെങ്കിൽ എങ്കിൽ (rhsp->isNull()) {
                തിരികെ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        എങ്കിൽ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlinkFrBack(&relinker);
            relinker.relink(
                പുതിയ AstCvtUnpackToQueue{rhsp->fileline(), VN_AS(rhsp, കണ്ണി_വ്യഞ്ജകം), lhsDTypep});
        }
    }
    സ്ഥിതം തർക്കം similarDTypeRecurse(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം node1p,
                                    സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം node2p) {
        തിരികെ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    ശൂന്യം iterateCheckFileDesc(Ast_കണ്ണി* parentp, Ast_കണ്ണി* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckReal(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    ശൂന്യം iterateCheckSigned8(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    ശൂന്യം iterateCheckSigned32(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    ശൂന്യം iterateCheckUInt32(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    ശൂന്യം iterateCheckDelay(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        എങ്കിൽ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        എങ്കിൽ (stage & FINAL) {
            Ast_കണ്ണി_DType* expDTypep;
            എങ്കിൽ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } അല്ലെങ്കിൽ {
                File_വരി* സ്ഥിരം newFl = പുതിയ File_വരി{underp->fileline()};
                newFl->warnOff(V3ErrorCode::WIDTHEXPAND, സത്യം);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, അസത്യം);
        }
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckString(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    ശൂന്യം iterateCheckTyped(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp,
                           Ast_കണ്ണി_DType* expDTypep, Stage stage) {
        എങ്കിൽ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{expDTypep, PRELIM}.p());
        }
        എങ്കിൽ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckTypedSelfPrelim(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp,
                                     Ast_കണ്ണി_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        എങ്കിൽ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        എങ്കിൽ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckIntegralSelf(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, സത്യം);
    }
    ശൂന്യം iterateCheckSelf(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Determ determ,
                          Stage stage, തർക്കം integralOnly = അസത്യം) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        എങ്കിൽ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, കണ്ണി_വ്യഞ്ജകം) ? checkCvtUS(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം), integralOnly) : underp;
        Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckSizedSelf(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Determ determ,
                               Stage stage, തർക്കം deferUntyped = അസത്യം) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        എങ്കിൽ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, WidthVP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, കണ്ണി_വ്യഞ്ജകം) ? checkCvtUS(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം), അസത്യം) : underp;
        Ast_കണ്ണി_DType* സ്ഥിരം expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_കണ്ണി_DType* സ്ഥിരം checkDtp = expDTypep->skipRefToEnump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        എങ്കിൽ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (ശൂന്യം)underp;  // cppcheck
    }
    ശൂന്യം iterateCheckAssign(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* rhsp, Stage stage,
                            Ast_കണ്ണി_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        എങ്കിൽ (AstConst* constp = VN_CAST(rhsp, Const)) {
            എങ്കിൽ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                എങ്കിൽ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    എങ്കിൽ (basicp->width() == 8 && constp->അക്കം().isFromString()) {
                        AstConsArray* newp = പുതിയ AstConsArray{
                            constp->fileline(), lhsDTypep,
                            പുതിയ AstConst{constp->fileline(), AstConst::Widthed_മൂല്യം{}, 8, 0}};
                        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ aindex : arrayp->declRange().seqLoToHi()) {
                            പൂർണ്ണസംഖ്യ cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_സംഖ്യ selected{constp, 8};
                            selected.opSel(constp->അക്കം(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            എങ്കിൽ (!selected.isFourState()) {
                                എങ്കിൽ (സ്ഥിരം uint32_t c = selected.toUInt()) {
                                    newp->valuep(aindex,
                                                 പുതിയ AstConst{constp->fileline(),
                                                              AstConst::Widthed_മൂല്യം{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        checkClassAssign(parentp, side, rhsp, lhsDTypep);
        സ്ഥിരം തർക്കം lhsStream = (VN_IS(parentp, കണ്ണി_Assign)
                                && VN_IS(VN_AS(parentp, കണ്ണി_Assign)->lhsp(), കണ്ണി_Stream));
        rhsp = iterateCheck(parentp, side, rhsp, ASSIGN, FINAL, lhsDTypep,
                            lhsStream ? EXTEND_OFF : EXTEND_LHS, സത്യം, STREAM_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (ശൂന്യം)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_കണ്ണി_വ്യഞ്ജകം* convertUnpackedOrder(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, Ast_കണ്ണി_DType* dtypep) {
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (!arrayp || !arrayp->isReverseOf(exprp->dtypep())) തിരികെ exprp;
        VNRelinker relinker;
        exprp->unlinkFrBack(&relinker);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstCvtUnpackToUnpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        തിരികെ newp;
    }

    സ്ഥിതം തർക്കം isLogicalOperator(സ്ഥിരം Ast_കണ്ണി* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        തിരികെ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    ശൂന്യം iterateCheckBool(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Stage stage) {
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
        Ast_കണ്ണി_DType* സ്ഥിരം underVDTypep = underp ? underp->dtypep()->skipRefp() : nullptr;
        എങ്കിൽ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstNeqD{parentp->fileline(), VN_AS(underp, കണ്ണി_വ്യഞ്ജകം),
                              പുതിയ AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } അല്ലെങ്കിൽ എങ്കിൽ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlinkFrBack(&linker);
            Ast_കണ്ണി* സ്ഥിരം newp
                = പുതിയ AstNeqN{parentp->fileline(), VN_AS(underp, കണ്ണി_വ്യഞ്ജകം),
                              പുതിയ AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം)), underp);  // Changed
        } അല്ലെങ്കിൽ എങ്കിൽ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(പുതിയ AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(തള്ളുക_Deletep(underp), underp);
        } അല്ലെങ്കിൽ {
            സ്ഥിരം തർക്കം bad = widthBad(underp, parentp->findBitDType());
            എങ്കിൽ (bad) {
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
            എങ്കിൽ (Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം വ്യഞ്ജകം_Stmtp = VN_CAST(underp, വ്യഞ്ജകം_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fixWidthReduce(വ്യഞ്ജകം_Stmtp->resultp());
                വ്യഞ്ജകം_Stmtp->dtypeFrom(വ്യഞ്ജകം_Stmtp->resultp());
            } അല്ലെങ്കിൽ എങ്കിൽ (underp->width() != 1) {
                VL_DO_DANGLING(fixWidthReduce(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം)), underp);  // Changed
            }
        }
    }

    സ്ഥിതം ശൂന്യം checkEnum(Ast_കണ്ണി* nodep, Ast_കണ്ണി* parentp, സ്ഥിരം Ast_കണ്ണി_DType* targetDTypep,
                          സ്ഥിരം Ast_കണ്ണി_DType* sourceDTypep, സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* sourceExprp) {
        എങ്കിൽ (സ്ഥിരം AstEnumDType* സ്ഥിരം targetEnump
            = VN_CAST(targetDTypep->skipRefToEnump(), EnumDType)) {
            സ്ഥിരം VCastable castable
                = Ast_കണ്ണി::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            എങ്കിൽ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
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

    Ast_കണ്ണി* iterateCheck(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side, Ast_കണ്ണി* underp, Determ determ,
                          Stage stage, Ast_കണ്ണി_DType* expDTypep, ExtendRule extendRule,
                          തർക്കം warnOn = സത്യം, StreamUse streamUse = STREAM_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        സ്ഥിരം StreamUse childStreamUse
            = streamUse != STREAM_USE_NONE ? streamUse : streamUseForDeterm(determ);
        എങ്കിൽ (rhsAccess.isReadOnly()) {
            എങ്കിൽ (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_CAST(underp, കണ്ണി_വ്യഞ്ജകം)) {
                // A value only read is converted to a reversed expected type.
                underp = convertUnpackedOrder(exprp, expDTypep);
            }
        }
        എങ്കിൽ (VN_IS(underp, കണ്ണി_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } അല്ലെങ്കിൽ എങ്കിൽ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } അല്ലെങ്കിൽ എങ്കിൽ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   WidthVP{expDTypep, FINAL, childStreamUse}.p());
        } അല്ലെങ്കിൽ എങ്കിൽ (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_കണ്ണി* സ്ഥിരം oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } അല്ലെങ്കിൽ എങ്കിൽ (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_കണ്ണി* സ്ഥിരം oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം), സത്യം, expDTypep->width());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } അല്ലെങ്കിൽ എങ്കിൽ (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_കണ്ണി* സ്ഥിരം oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, കണ്ണി_വ്യഞ്ജകം));
            underp = userIterateSubtreeReturnEdits(oldp, WidthVP{SELF, FINAL, childStreamUse}.p());
        } അല്ലെങ്കിൽ {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം underDtp = underp->dtypep()->skipRefp();
            എങ്കിൽ (determ != ASSIGN && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_കണ്ണി* സ്ഥിരം newp
                    = പുതിയ AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(തള്ളുക_Deletep(underp), underp);
                underp = newp;
            }
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം expDtp = expDTypep->skipRefp();
            എങ്കിൽ (determ == ASSIGN && VN_IS(expDtp, കണ്ണി_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            എങ്കിൽ (determ == ASSIGN && !isIfaceArrayDType(expDtp)
                && !unpackedArraySizesMatch(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            സ്ഥിരം AstBasicDType* സ്ഥിരം expBasicp = expDTypep->basicp();
            സ്ഥിരം AstBasicDType* സ്ഥിരം underBasicp = underp->dtypep()->basicp();
            എങ്കിൽ (expBasicp && underBasicp) {
                എങ്കിൽ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3ErrorCode::ENUMVALUE)
                    && warnOn) {
                    എങ്കിൽ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, കണ്ണി_വ്യഞ്ജകം));
                    }
                    എങ്കിൽ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, nullptr);
                    }
                }
                Ast_കണ്ണി_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                എങ്കിൽ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{SELF, FINAL, childStreamUse}.p());
                } അല്ലെങ്കിൽ എങ്കിൽ (determ == ASSIGN) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    എങ്കിൽ (underp->isSigned() != subDTypep->isSigned()
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
                } അല്ലെങ്കിൽ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{subDTypep, FINAL, childStreamUse}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                widthCheckSized(parentp, side, VN_AS(underp, കണ്ണി_വ്യഞ്ജകം), expDTypep, extendRule,
                                warnOn);
            } അല്ലെങ്കിൽ എങ്കിൽ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                സ്ഥിരം AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                എങ്കിൽ (VN_IS(underp, Const) && VN_AS(underp, Const)->അക്കം().isNull()) {
                    // '= null' is ok
                } അല്ലെങ്കിൽ എങ്കിൽ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } അല്ലെങ്കിൽ എങ്കിൽ (expIfaceRefp->ifaceViaCellp() != underIfaceRefp->ifaceViaCellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->ifaceViaCellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->ifaceViaCellp()->prettyNameQ() << ").");
                } അല്ലെങ്കിൽ എങ്കിൽ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } അല്ലെങ്കിൽ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
                }
            } അല്ലെങ്കിൽ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, WidthVP{expDTypep, FINAL, childStreamUse}.p());
            }
        }
        തിരികെ underp;
    }

    ശൂന്യം
    widthCheckSized(Ast_കണ്ണി* parentp, സ്ഥിരം അക്ഷരം* side,
                    Ast_കണ്ണി_വ്യഞ്ജകം* underp,  // Node to be checked or have typecast added in front of
                    Ast_കണ്ണി_DType* expDTypep, ExtendRule extendRule, തർക്കം warnOn = സത്യം) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        സ്ഥിരം AstBasicDType* സ്ഥിരം expBasicp = expDTypep->basicp();
        സ്ഥിരം AstBasicDType* സ്ഥിരം underBasicp = underp->dtypep()->basicp();
        എങ്കിൽ (expDTypep == underp->dtypep()) {
            തിരികെ;  // Same type must match
        } അല്ലെങ്കിൽ എങ്കിൽ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#എങ്കിൽ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#endif
            തിരികെ;
        } അല്ലെങ്കിൽ {
            സ്ഥിരം പൂർണ്ണസംഖ്യ expWidth = expDTypep->width();
            പൂർണ്ണസംഖ്യ expWidthMin = expDTypep->widthMin();
            എങ്കിൽ (expWidthMin == 0) expWidthMin = expWidth;
            സ്ഥിരം തർക്കം bad = widthBad(underp, expDTypep);
            എങ്കിൽ ((bad || underp->width() != expWidth) && fixAutoExtend(underp /*ref*/, expWidth)) {
                underp = nullptr;  // Changes underp
                തിരികെ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            എങ്കിൽ (VN_IS(parentp->backp(), CastSize)) warnOn = അസത്യം;
            എങ്കിൽ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = അസത്യം;
            എങ്കിൽ (VN_IS(underp, Const) && VN_AS(underp, Const)->അക്കം().isFromString()
                && expWidth > underp->width()
                && (((expWidth - underp->width()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = അസത്യം;
            }
            എങ്കിൽ ((VN_IS(parentp, Add) && underp->width() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->width() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = അസത്യം;
            }
            എങ്കിൽ (bad && warnOn) {
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
            എങ്കിൽ (bad || underp->width() != expWidth) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                സ്ഥിരം Ast_കണ്ണി_Assign* assignp = VN_CAST(parentp, കണ്ണി_Assign);
                സ്ഥിരം AstPin* pinp = VN_CAST(parentp, Pin);
                എങ്കിൽ (assignp && VN_IS(assignp->lhsp(), കണ്ണി_Stream)) {
                } അല്ലെങ്കിൽ എങ്കിൽ (pinp && pinp->modVarp()->direction() != VDirection::നിവേശം) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } അല്ലെങ്കിൽ എങ്കിൽ (assignp && VN_IS(underp, കണ്ണി_Stream) && expWidth > underp->width()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    സ്ഥിരം പൂർണ്ണസംഖ്യ shift = expWidth - underp->width();
                    underp->unlinkFrBack(&linker);
                    AstExtend* സ്ഥിരം widenedp = പുതിയ AstExtend{underp->fileline(), underp};
                    widenedp->didWidth(സത്യം);
                    // Shift left so zeros fill on the right
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം shiftedp = പുതിയ AstShiftL{
                        underp->fileline(), widenedp,
                        പുതിയ AstConst{underp->fileline(), static_cast<uint32_t>(shift)}, expWidth};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } അല്ലെങ്കിൽ {
                    VL_DO_DANGLING(fixWidthExtend(underp, expDTypep, extendRule), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_കണ്ണി_വ്യഞ്ജകം* checkCvtUS(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, തർക്കം fatal) {
        എങ്കിൽ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            എങ്കിൽ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } അല്ലെങ്കിൽ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, അസത്യം, 32);
        }
        തിരികെ nodep;
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* spliceCvtD(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        എങ്കിൽ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_കണ്ണി_വ്യഞ്ജകം* newp;
            എങ്കിൽ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = പുതിയ AstISToRD{nodep->fileline(), nodep};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            തിരികെ newp;
        } അല്ലെങ്കിൽ {
            തിരികെ nodep;
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* spliceCvtS(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, തർക്കം warnOn, പൂർണ്ണസംഖ്യ width) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        എങ്കിൽ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                എങ്കിൽ (constp->isDouble() && V3_സംഖ്യ::epsilonIntegral(constp->അക്കം().toDouble())) {
                    warnOn = അസത്യം;
                }
            }
            എങ്കിൽ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtypeSetBitSized(width, VSigning::SIGNED);
            തിരികെ newp;
        } അല്ലെങ്കിൽ {
            തിരികെ nodep;
        }
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* spliceCvtString(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        എങ്കിൽ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlinkFrBack(&linker);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            തിരികെ newp;
        } അല്ലെങ്കിൽ {
            തിരികെ nodep;
        }
    }
    Ast_കണ്ണി_Biop* replaceWithUOrSVersion(Ast_കണ്ണി_Biop* nodep, തർക്കം signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        എങ്കിൽ (signedFlavorNeeded == nodep->signedFlavor()) തിരികെ nullptr;
        എങ്കിൽ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        തിരഞ്ഞെടുപ്പ് (nodep->type()) {
        അവസ്ഥ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::Add: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        അവസ്ഥ VNType::ShiftL: nodep->dtypeChgSigned(signedFlavorNeeded); തിരികെ nullptr;
        സ്ഥിരസ്ഥിതി: നിർത്തുക;
        }
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* newp = nullptr;
        തിരഞ്ഞെടുപ്പ് (nodep->type()) {
        അവസ്ഥ VNType::Gt: newp = പുതിയ AstGtS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::GtS: newp = പുതിയ AstGt{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Gte: newp = പുതിയ AstGteS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::GteS: newp = പുതിയ AstGte{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lt: newp = പുതിയ AstLtS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::LtS: newp = പുതിയ AstLt{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lte: newp = പുതിയ AstLteS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::LteS: newp = പുതിയ AstLte{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Div: newp = പുതിയ AstDivS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::DivS: newp = പുതിയ AstDiv{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::ModDiv: newp = പുതിയ AstModDivS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::ModDivS: newp = പുതിയ AstModDiv{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Mul: newp = പുതിയ AstMulS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::MulS: newp = പുതിയ AstMul{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::ShiftR: newp = പുതിയ AstShiftRS{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::ShiftRS: newp = പുതിയ AstShiftR{fl, lhsp, rhsp}; നിർത്തുക;
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            നിർത്തുക;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ newp;
    }
    Ast_കണ്ണി_Biop* replaceWithDVersion(Ast_കണ്ണി_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        എങ്കിൽ (nodep->doubleFlavor()) തിരികെ nullptr;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        തിരഞ്ഞെടുപ്പ് (nodep->type()) {
        അവസ്ഥ VNType::Add: newp = പുതിയ AstAddD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Sub: newp = പുതിയ AstSubD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Pow: newp = പുതിയ AstPowD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Eq:
        അവസ്ഥ VNType::EqCase: newp = പുതിയ AstEqD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Neq:
        അവസ്ഥ VNType::NeqCase: newp = പുതിയ AstNeqD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Gt:
        അവസ്ഥ VNType::GtS: newp = പുതിയ AstGtD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Gte:
        അവസ്ഥ VNType::GteS: newp = പുതിയ AstGteD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lt:
        അവസ്ഥ VNType::LtS: newp = പുതിയ AstLtD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lte:
        അവസ്ഥ VNType::LteS: newp = പുതിയ AstLteD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Div:
        അവസ്ഥ VNType::DivS: newp = പുതിയ AstDivD{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Mul:
        അവസ്ഥ VNType::MulS: newp = പുതിയ AstMulD{fl, lhsp, rhsp}; നിർത്തുക;
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            നിർത്തുക;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ newp;
    }
    Ast_കണ്ണി_Biop* replaceWithNVersion(Ast_കണ്ണി_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        എങ്കിൽ (nodep->stringFlavor()) തിരികെ nullptr;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
        Ast_കണ്ണി_Biop* newp = nullptr;
        // No width change on output;...                // All below have bool or double outputs
        തിരഞ്ഞെടുപ്പ് (nodep->type()) {
        അവസ്ഥ VNType::Eq:
        അവസ്ഥ VNType::EqCase:
        അവസ്ഥ VNType::EqWild: newp = പുതിയ AstEqN{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Neq:
        അവസ്ഥ VNType::NeqCase:
        അവസ്ഥ VNType::NeqWild: newp = പുതിയ AstNeqN{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Gt:
        അവസ്ഥ VNType::GtS: newp = പുതിയ AstGtN{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Gte:
        അവസ്ഥ VNType::GteS: newp = പുതിയ AstGteN{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lt:
        അവസ്ഥ VNType::LtS: newp = പുതിയ AstLtN{fl, lhsp, rhsp}; നിർത്തുക;
        അവസ്ഥ VNType::Lte:
        അവസ്ഥ VNType::LteS: newp = പുതിയ AstLteN{fl, lhsp, rhsp}; നിർത്തുക;
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            നിർത്തുക;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ newp;
    }
    Ast_കണ്ണി_Uniop* replaceWithDVersion(Ast_കണ്ണി_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        എങ്കിൽ (nodep->doubleFlavor()) തിരികെ nullptr;
        File_വരി* സ്ഥിരം fl = nodep->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_Uniop* newp = nullptr;
        തിരഞ്ഞെടുപ്പ് (nodep->type()) {
        അവസ്ഥ VNType::Negate: newp = പുതിയ AstNegateD{fl, lhsp}; നിർത്തുക;
        സ്ഥിരസ്ഥിതി:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            നിർത്തുക;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
        തിരികെ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    ശൂന്യം replaceWithSFormat(AstMethodCall* nodep, സ്ഥിരം string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* സ്ഥിരം argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_കണ്ണി_Var_Ref* സ്ഥിരം fromp = VN_AS(nodep->fromp()->unlinkFrBack(), VarRef);
        Ast_കണ്ണി* സ്ഥിരം newp = പുതിയ AstAssign{
            nodep->fileline(), fromp,
            പുതിയ AstSFormatF{nodep->fileline(), format, അസത്യം, argp->exprp()->unlinkFrBack()}};
        fromp->access(VAccess::WRITE);
        തള്ളുക_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_കണ്ണി_DType* iterateEditMoveDTypep(Ast_കണ്ണി* parentp, Ast_കണ്ണി_DType* dtnodep) {
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
        സ്ഥിരം തർക്കം child1 = (parentp->getChildDTypep() == dtnodep);
        സ്ഥിരം തർക്കം child2 = (parentp->getChild2DTypep() == dtnodep);
        എങ്കിൽ (child1 || child2) {
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
        എങ്കിൽ (!dtnodep->didWidth()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, nullptr);
            UASSERT_OBJ(dtnodep->didWidth(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        തിരികെ dtnodep;
    }

    AstConst* dimension_മൂല്യം(File_വരി* fileline, Ast_കണ്ണി_DType* nodep, VAttrType attrType,
                             പൂർണ്ണസംഖ്യ dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_കണ്ണി_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        വേണ്ടി (പൂർണ്ണസംഖ്യ i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(dtypep, കണ്ണി_Array_DType)) {
                declRange = adtypep->declRange();
                എങ്കിൽ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                തുടരുക;
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം adtypep
                       = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                നിർത്തുക;  // Sub elements don't look like arrays and can't iterate into
            } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം adtypep = VN_CAST(dtypep, BasicDType)) {
                എങ്കിൽ (adtypep->isRanged()) declRange = adtypep->declRange();
                നിർത്തുക;
            }
            നിർത്തുക;  // LCOV_EXCL_LINE
        }
        AstConst* valp = nullptr;  // If nullptr, construct from val
        പൂർണ്ണസംഖ്യ val = 0;
        തിരഞ്ഞെടുപ്പ് (attrType) {
        അവസ്ഥ VAttrType::DIM_BITS: {
            പൂർണ്ണസംഖ്യ bits = 1;
            ആകുമ്പോൾ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം adtypep = VN_CAST(dtypep, കണ്ണി_Array_DType)) {
                    bits *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    തുടരുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം adtypep
                           = VN_CAST(dtypep, കണ്ണി_UOr_Struct_DType)) {
                    bits *= adtypep->isStreamableFixedAggregate() ? adtypep->widthStream()
                                                                  : adtypep->width();
                    നിർത്തുക;
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstBasicDType* സ്ഥിരം adtypep = VN_CAST(dtypep, BasicDType)) {
                    bits *= adtypep->width();
                    നിർത്തുക;
                }
                നിർത്തുക;
            }
            എങ്കിൽ (dim == 0) {
                val = 0;
            } അല്ലെങ്കിൽ എങ്കിൽ (dim == 1 && !declRange.ranged()
                       && bits == 1) {  // $bits should be sane for non-arrays
                val = nodep->width();
            } അല്ലെങ്കിൽ {
                val = bits;
            }
            നിർത്തുക;  // LCOV_EXCL_LINE
        }
        അവസ്ഥ VAttrType::DIM_മുകളിലെ: val = !declRange.ranged() ? 0 : declRange.hi(); നിർത്തുക;
        അവസ്ഥ VAttrType::DIM_ഇടത്: val = !declRange.ranged() ? 0 : declRange.ഇടത്(); നിർത്തുക;
        അവസ്ഥ VAttrType::DIM_താഴെയുള്ള: val = !declRange.ranged() ? 0 : declRange.lo(); നിർത്തുക;
        അവസ്ഥ VAttrType::DIM_വലത്: val = !declRange.ranged() ? 0 : declRange.വലത്(); നിർത്തുക;
        അവസ്ഥ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            നിർത്തുക;
        അവസ്ഥ VAttrType::DIM_SIZE: val = !declRange.ranged() ? 0 : declRange.elements(); നിർത്തുക;
        സ്ഥിരസ്ഥിതി: nodep->v3fatalSrc("Missing DIM ATTR type case"); നിർത്തുക;
        }
        എങ്കിൽ (!valp) valp = പുതിയ AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvtToHex(dtypep) << "," << dim
                                << ")=" << valp);
        തിരികെ valp;
    }
    AstVarRef* dimensionVarRefp(Ast_കണ്ണി_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        എങ്കിൽ (cachedpr) തിരികെ cachedpr->clone_വൃക്ഷം(അസത്യം);

        Ast_കണ്ണി_Array_DType* സ്ഥിരം vardtypep = പുതിയ AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), പുതിയ AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        userIterate(vardtypep, WidthVP{SELF, BOTH}.p());
        AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{nodep->fileline(), vardtypep};
        വേണ്ടി (unsigned i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_മൂല്യം(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_വൃക്ഷം(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(സത്യം);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(സത്യം);
        തള്ളുക_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        തിരികെ cachedpr->clone_വൃക്ഷം(അസത്യം);
    }
    സ്ഥിതം uint64_t enum_Max_മൂല്യം(സ്ഥിരം Ast_കണ്ണി* errNodep, സ്ഥിരം AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            സ്ഥിരം AstConst* സ്ഥിരം vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            എങ്കിൽ (!vconstp->അക്കം().isAnyXZ() && vconstp->toUQuad() >= maxval)
                maxval = vconstp->toUQuad();
        }
        എങ്കിൽ (adtypep->itemsp()->width() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            തിരികെ 64;
        }
        തിരികെ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* സ്ഥിരം nodep, VAttrType attrType, തർക്കം assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_tableMap[std::make_pair(nodep, attrType)];
        എങ്കിൽ (cachedpr) തിരികെ cachedpr->clone_വൃക്ഷം(അസത്യം);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        Ast_കണ്ണി_DType* basep;
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } അല്ലെങ്കിൽ {
            basep = nodep->dtypep();
        }
        Ast_കണ്ണി_DType* vardtypep;
        എങ്കിൽ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = പുതിയ AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } അല്ലെങ്കിൽ {
            vardtypep = പുതിയ AstUnpackArrayDType{flp, basep, പുതിയ AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = nullptr;
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME) {
            defaultp = പുതിയ AstConst{flp, AstConst::String{}, ""};
        } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_NEXT || attrType == VAttrType::ENUM_PREV) {
            defaultp = പുതിയ AstConst{flp, V3_സംഖ്യ{nodep, nodep->width(), 0}};
        } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_VALID) {
            defaultp = പുതിയ AstConst{flp, AstConst::BitFalse{}};
        } അല്ലെങ്കിൽ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_കണ്ണി_വ്യഞ്ജകം*> മൂല്യങ്ങൾ;
        {
            uint64_t ordinal = 0;
            AstEnumItem* സ്ഥിരം firstp = nodep->itemsp();
            സ്ഥിരം AstEnumItem* prevp = firstp;  // Prev must start with last item
            ആകുമ്പോൾ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            വേണ്ടി (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* സ്ഥിരം nextp = VN_AS(itemp->nextp(), EnumItem);
                സ്ഥിരം AstConst* സ്ഥിരം vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                എങ്കിൽ (!vconstp->അക്കം().isAnyXZ()) {  // Can 2-state runtime decode
                    സ്ഥിരം uint64_t i = nodep->isWide() ? ++ordinal : vconstp->toUQuad();
                    എങ്കിൽ (attrType == VAttrType::ENUM_NAME) {
                        മൂല്യങ്ങൾ[i] = പുതിയ AstConst{flp, AstConst::String{},
                                                 V3_സംഖ്യ::displayedEnumName(itemp)};
                    } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_NEXT) {
                        മൂല്യങ്ങൾ[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_വൃക്ഷം(അസത്യം);  // A const
                    } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_PREV) {
                        മൂല്യങ്ങൾ[i] = prevp->valuep()->clone_വൃക്ഷം(അസത്യം);  // A const
                    } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_VALID) {
                        മൂല്യങ്ങൾ[i] = പുതിയ AstConst{flp, AstConst::BitTrue{}};
                    } അല്ലെങ്കിൽ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_കണ്ണി_വ്യഞ്ജകം* valuep = nullptr;
        എങ്കിൽ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* സ്ഥിരം consp = പുതിയ AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            വേണ്ടി (സ്ഥിരം auto& itr : മൂല്യങ്ങൾ) {
                V3_സംഖ്യ താക്കോൽ{nodep, nodep->width()};
                താക്കോൽ.setQuad(itr.first);
                consp->addItemsp(പുതിയ AstConsAssocItem{flp, പുതിയ AstConst{flp, താക്കോൽ}, itr.second});
            }
            valuep = consp;
        } അല്ലെങ്കിൽ {
            AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{flp, vardtypep, defaultp};
            വേണ്ടി (uint64_t i = 0; i < (msbdim + 1); ++i) {
                എങ്കിൽ (മൂല്യങ്ങൾ[i]) consp->valuep(i, മൂല്യങ്ങൾ[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(തള്ളുക_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->didWidth(സത്യം);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->didWidth(സത്യം);
        തള്ളുക_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        തിരികെ cachedpr->clone_വൃക്ഷം(അസത്യം);
    }

    Ast_കണ്ണി_വ്യഞ്ജകം* enumSelect(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME) {
            എങ്കിൽ (nodep->isPure()) nodep = V3Const::constifyEdit(nodep);
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->അക്കം().displayedEnumName(adtypep)};
                VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
                തിരികെ newp;
            }
        }
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME && !nodep->isPure() && (m_modep || m_ftaskp)) {
            Ast_വ്യഞ്ജകം_Stmt* സ്ഥിരം capturep = newEnumCapture(nodep, adtypep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം valuep = capturep->resultp()->unlinkFrBack();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtypeSetString();
            തിരികെ capturep;
        }
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_കണ്ണി_വ്യഞ്ജകം* indexp = പുതിയ AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            വേണ്ടി (സ്ഥിരം AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                സ്ഥിരം AstConst* സ്ഥിരം constp = VN_AS(itemp->valuep(), Const);
                എങ്കിൽ (constp->അക്കം().isAnyXZ()) തുടരുക;
                indexp = പുതിയ AstCond{nodep->fileline(),
                                     പുതിയ AstEq{nodep->fileline(), nodep->clone_വൃക്ഷം(അസത്യം),
                                               itemp->valuep()->clone_വൃക്ഷം(അസത്യം)},
                                     പുതിയ AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* സ്ഥിരം tabRefp = enumVarRefp(adtypep, attrType, അസത്യം, ordinal);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newp = പുതിയ AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtypeSetString();
            VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
            തിരികെ newp;
        }
        സ്ഥിരം uint64_t msbdim = enum_Max_മൂല്യം(nodep, adtypep);
        സ്ഥിരം തർക്കം assoc = msbdim > ENUM_LOOKUP_BITS;
        Ast_കണ്ണി_വ്യഞ്ജകം* newp;
        എങ്കിൽ (assoc) {
            AstVarRef* സ്ഥിരം tabRefp = enumVarRefp(adtypep, attrType, സത്യം, 0);
            newp = പുതിയ AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } അല്ലെങ്കിൽ {
            സ്ഥിരം പൂർണ്ണസംഖ്യ selwidth = V3_സംഖ്യ::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* സ്ഥിരം tabRefp
                = enumVarRefp(adtypep, attrType, അസത്യം, (1ULL << selwidth) - 1);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->width() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? പുതിയ AstGt{nodep->fileline(), nodep->clone_വൃക്ഷം(അസത്യം),
                                  പുതിയ AstConst{nodep->fileline(),
                                               V3_സംഖ്യ{nodep, adtypep->width(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : nullptr;
            newp = പുതിയ AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                പുതിയ AstSel{nodep->fileline(), nodep, 0, selwidth}};
            എങ്കിൽ (boundp) {
                newp = പുതിയ AstCond{nodep->fileline(), boundp,
                                   പുതിയ AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        എങ്കിൽ (attrType == VAttrType::ENUM_NAME) {
            newp->dtypeSetString();
        } അല്ലെങ്കിൽ എങ്കിൽ (attrType == VAttrType::ENUM_VALID) {
            newp->dtypeSetBit();
        } അല്ലെങ്കിൽ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        തിരികെ newp;
    }
    Ast_കണ്ണി_വ്യഞ്ജകം* enumTestValid(Ast_കണ്ണി_വ്യഞ്ജകം* valp, AstEnumDType* enumDtp) {
        സ്ഥിരം uint64_t maxval = enum_Max_മൂല്യം(valp, enumDtp);
        സ്ഥിരം തർക്കം assoc = maxval > ENUM_LOOKUP_BITS;
        Ast_കണ്ണി_വ്യഞ്ജകം* testp = nullptr;
        File_വരി* സ്ഥിരം fl_novalue = പുതിയ File_വരി{valp->fileline()};
        fl_novalue->warnOff(V3ErrorCode::ENUMVALUE, സത്യം);
        fl_novalue->warnOff(V3ErrorCode::CMPCONST, സത്യം);
        എങ്കിൽ (assoc) {
            AstVarRef* സ്ഥിരം tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, സത്യം, 0);
            testp = പുതിയ AstAssocSel{fl_novalue, tabRefp, valp->clone_വൃക്ഷം_Pure(അസത്യം)};
        } അല്ലെങ്കിൽ {
            സ്ഥിരം പൂർണ്ണസംഖ്യ selwidth = V3_സംഖ്യ::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* സ്ഥിരം tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, അസത്യം, (1ULL << selwidth) - 1);
            File_വരി* സ്ഥിരം fl_nowidth = പുതിയ File_വരി{fl_novalue};
            fl_nowidth->warnOff(V3ErrorCode::WIDTH, സത്യം);
            testp = പുതിയ AstCond{
                fl_novalue,
                പുതിയ AstGt{fl_nowidth, valp->clone_വൃക്ഷം_Pure(അസത്യം),
                          പുതിയ AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                പുതിയ AstConst{fl_novalue, AstConst::BitFalse{}},
                പുതിയ AstArraySel{fl_novalue, tabRefp,
                                പുതിയ AstSel{fl_novalue, valp->clone_വൃക്ഷം_Pure(അസത്യം), 0, selwidth}}};
        }
        തിരികെ testp;
    }

    PatVecMap patVectorMap(AstPattern* nodep, സ്ഥിരം VNumRange& range) {
        PatVecMap patmap;
        പൂർണ്ണസംഖ്യ element = range.ഇടത്();
        വേണ്ടി (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            എങ്കിൽ (patp->keyp()) {
                എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = patternConstKeyp(patp)) {
                    എങ്കിൽ (constp->അക്കം().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } അല്ലെങ്കിൽ {
                        element = constp->toSInt();
                    }
                }
            }
            സ്ഥിരം തർക്കം newEntry = patmap.emplace(element, patp).second;
            എങ്കിൽ (!newEntry) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ഇടത്_To_വലത്_Inc();
        }
        തിരികെ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    ശൂന്യം patConcatConvertRecurse(AstPattern* patternp, Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (AstConcat* സ്ഥിരം concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } അല്ലെങ്കിൽ {
            AstPatMember* സ്ഥിരം newp
                = പുതിയ AstPatMember{nodep->fileline(), nodep->unlinkFrBack(), nullptr, nullptr};
            newp->isConcat(സത്യം);
            patternp->addItemsp(newp);
        }
    }

    ശൂന്യം makeOpenArrayShell(Ast_കണ്ണി_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_കണ്ണി_FTask* സ്ഥിരം oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_കണ്ണി_FTask* സ്ഥിരം newTaskp = oldTaskp->clone_വൃക്ഷം(അസത്യം);
        newTaskp->dpiOpenChild(സത്യം);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvtToStr(oldTaskp->dpiOpenParent()));
        oldTaskp->addNextHere(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        സ്ഥിരം V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        വേണ്ടി (സ്ഥിരം auto& tconnect : tconnects) {
            AstVar* സ്ഥിരം portp = tconnect.first;
            സ്ഥിരം AstArg* സ്ഥിരം argp = tconnect.second;
            സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം pinp = argp->exprp();
            എങ്കിൽ (!pinp) തുടരുക;  // Argument error we'll find later
            എങ്കിൽ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_കണ്ണി_DType* സ്ഥിരം oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_കണ്ണി* nodep) {
                    എങ്കിൽ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    തർക്കം markHasOpenArray(Ast_കണ്ണി_FTask* nodep) {
        തർക്കം hasOpen = അസത്യം;
        വേണ്ടി (Ast_കണ്ണി* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            എങ്കിൽ (AstVar* സ്ഥിരം portp = VN_CAST(stmtp, Var)) {
                എങ്കിൽ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(സത്യം);
                    hasOpen = സത്യം;
                }
            }
        }
        തിരികെ hasOpen;
    }
    തർക്കം hasOpenArrayDTypeRecurse(Ast_കണ്ണി_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        എങ്കിൽ (VN_IS(nodep, UnsizedArrayDType)) തിരികെ സത്യം;
        എങ്കിൽ (nodep->subDTypep()) തിരികെ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        തിരികെ അസത്യം;
    }
    Ast_കണ്ണി_DType* dtypeNotIntAtomOrVecRecurse(Ast_കണ്ണി_DType* nodep, തർക്കം ranged = അസത്യം) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skipRefToEnump();
        എങ്കിൽ (AstBasicDType* സ്ഥിരം dtp = VN_CAST(nodep, BasicDType)) {
            എങ്കിൽ (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                തിരികെ dtp;  // Packed when already packed
            എങ്കിൽ (dtp->keyword().isIntNumeric()) തിരികെ nullptr;
            തിരികെ dtp;
        } അല്ലെങ്കിൽ എങ്കിൽ (AstPackArrayDType* സ്ഥിരം dtp = VN_CAST(nodep, PackArrayDType)) {
            എങ്കിൽ (ranged) തിരികെ dtp;  // Packed when already packed
            തിരികെ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), സത്യം);
        }
        തിരികെ nodep;
    }
    Ast_കണ്ണി_DType* dtypeNot4StateIntegralRecurse(Ast_കണ്ണി_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        എങ്കിൽ (AstBasicDType* സ്ഥിരം dtp = VN_CAST(nodep, BasicDType)) {
            എങ്കിൽ (!dtp->keyword().isFourstate()) തിരികെ dtp;
            തിരികെ nullptr;
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Array_DType* സ്ഥിരം dtp = VN_CAST(nodep, കണ്ണി_Array_DType)) {
            തിരികെ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_UOr_Struct_DType* സ്ഥിരം dtp = VN_CAST(nodep, കണ്ണി_UOr_Struct_DType)) {
            വേണ്ടി (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_കണ്ണി_DType* സ്ഥിരം badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                എങ്കിൽ (badDtp) തിരികെ badDtp;
            }
            തിരികെ nullptr;
        }
        തിരികെ nodep;
    }
    ശൂന്യം checkForceReleaseLhs(Ast_കണ്ണി* nodep, Ast_കണ്ണി* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം selNodep = V3Width::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    ശൂന്യം assert_At_വ്യഞ്ജകം(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    ശൂന്യം assertAtStatement(Ast_കണ്ണി* nodep) {
        എങ്കിൽ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    ശൂന്യം checkConstantOrReplace(Ast_കണ്ണി* nodep, തർക്കം noFourState, സ്ഥിരം string& message) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const);
        എങ്കിൽ (!constp || (noFourState && constp->അക്കം().isFourState())) {
            nodep->v3error(message);
            nodep->replaceWith(പുതിയ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(തള്ളുക_Deletep(nodep), nodep, constp);
            തിരികെ;
        }
    }
    Ast_കണ്ണി* കണ്ണി_For_Unsized_Warning(Ast_കണ്ണി* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        എങ്കിൽ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->widthSized()) {
            തിരികെ nodep->op1p();
        } അല്ലെങ്കിൽ എങ്കിൽ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->widthSized()) {
            തിരികെ nodep->op2p();
        }
        തിരികെ nodep;  // By default return this
    }
    AstRefDType* checkRefToTypedefRecurse(Ast_കണ്ണി* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        എങ്കിൽ (VL_LIKELY(!nodep)) തിരികെ nullptr;
        എങ്കിൽ (auto* സ്ഥിരം refp = VN_CAST(nodep, RefDType)) {
            എങ്കിൽ (refp->typedefp() == typedefp) തിരികെ refp;
        }
        എങ്കിൽ (auto* സ്ഥിരം refp = checkRefToTypedefRecurse(nodep->op1p(), typedefp)) തിരികെ refp;
        എങ്കിൽ (auto* സ്ഥിരം refp = checkRefToTypedefRecurse(nodep->op2p(), typedefp)) തിരികെ refp;
        എങ്കിൽ (auto* സ്ഥിരം refp = checkRefToTypedefRecurse(nodep->op3p(), typedefp)) തിരികെ refp;
        എങ്കിൽ (auto* സ്ഥിരം refp = checkRefToTypedefRecurse(nodep->op4p(), typedefp)) തിരികെ refp;
        തിരികെ nullptr;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    സ്ഥിതം തർക്കം isIfaceArrayDType(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        എങ്കിൽ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) തിരികെ അസത്യം;
        സ്ഥിരം AstIfaceRefDType* സ്ഥിരം irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        തിരികെ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    സ്ഥിതം std::string unpackedArraySize(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        std::string size;
        dtypep = dtypep->skipRefp();
        ആകുമ്പോൾ (സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            എങ്കിൽ (!size.empty()) size += "x";
            size += std::to_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        തിരികെ size;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newIfaceArrayElement(Ast_കണ്ണി_വ്യഞ്ജകം* fromp, പൂർണ്ണസംഖ്യ bit) {
        AstConsArray* സ്ഥിരം consp = VN_CAST(fromp, ConsArray);
        എങ്കിൽ (!consp || !isIfaceArrayDType(consp->dtypep())) തിരികെ nullptr;
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (bit < 0 || bit >= arrp->elementsConst()) തിരികെ nullptr;
        തിരികെ consp->valuep(bit)->clone_വൃക്ഷം(അസത്യം);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_കണ്ണി_വ്യഞ്ജകം* new_Iface_Array_മൂല്യം(സ്ഥിരം Ast_കണ്ണി_Var_Ref* refp, Ast_കണ്ണി_DType* dtypep,
                                    സ്ഥിരം std::string& name, സ്ഥിരം std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_വരി* സ്ഥിരം flp = refp->fileline();
        AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        എങ്കിൽ (!arrp) {
            AstVar* സ്ഥിരം varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_കണ്ണി_Var_Ref* newp;
            എങ്കിൽ (സ്ഥിരം AstVarXRef* സ്ഥിരം xrefp = VN_CAST(refp, VarXRef)) {
                newp = പുതിയ AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } അല്ലെങ്കിൽ {
                newp = പുതിയ AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, WidthVP{SELF, BOTH}.p());
            തിരികെ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* സ്ഥിരം consp = പുതിയ AstConsArray{flp, arrp};
        സ്ഥിരം VNumRange range = arrp->declRange();
        വേണ്ടി (സ്ഥിരം പൂർണ്ണസംഖ്യ i : range.seq_ഇടത്_To_വലത്()) {
            സ്ഥിരം std::string s = name + "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_മൂല്യം(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        തിരികെ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    ശൂന്യം replaceSplitIfaceArrayRef(Ast_കണ്ണി_Var_Ref* nodep) {
        AstVar* സ്ഥിരം varp = nodep->varp();
        UASSERT_OBJ(varp->isIfaceArraySplit(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        എങ്കിൽ (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_മൂല്യം(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    ശൂന്യം expandIfaceArrayPinDimensions(AstPin* nodep, സ്ഥിരം Ast_കണ്ണി_DType* dtypep,
                                       Ast_കണ്ണി_വ്യഞ്ജകം* conp, സ്ഥിരം std::string& suffix,
                                       AstVar*& elemVarpr) {
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        എങ്കിൽ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* സ്ഥിരം newp = nodep->clone_വൃക്ഷം(അസത്യം);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            എങ്കിൽ (conp) newp->exprp(conp->clone_വൃക്ഷം(അസത്യം));
            nodep->addNextHere(newp);
            തിരികെ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        സ്ഥിരം VNumRange range = arrp->declRange();
        വേണ്ടി (പൂർണ്ണസംഖ്യ n = range.elements() - 1; n >= 0; --n) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ i = range.ഇടത്() + n * range.ഇടത്_To_വലത്_Inc();
            സ്ഥിരം std::string s = suffix + "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം elemConnp = [&]() -> Ast_കണ്ണി_വ്യഞ്ജകം* {
                എങ്കിൽ (!conp) തിരികെ nullptr;
                സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                സ്ഥിരം പൂർണ്ണസംഖ്യ elements = conArrp->elementsConst();
                // Counted from lo
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                തിരികെ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    ശൂന്യം expandIfaceArrayPin(AstPin* nodep) {
        AstVar* സ്ഥിരം portp = nodep->modVarp();
        UASSERT_OBJ(portp->isIfaceArraySplit(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        വേണ്ടി (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = VN_AS(nodep->exprp(), കണ്ണി_വ്യഞ്ജകം);
        എങ്കിൽ (exprp) തള്ളുക_Deletep(exprp->unlinkFrBack());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        സ്ഥിരം തർക്കം connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpackedArraySize(exprp->dtypep()) == unpackedArraySize(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : nullptr, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(തള്ളുക_Deletep(nodep->unlinkFrBack()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_കണ്ണി* userIterateSubtreeReturnEdits(Ast_കണ്ണി* nodep, WidthVP* vup) {
        എങ്കിൽ (!nodep) തിരികെ nullptr;
        Ast_കണ്ണി* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        തിരികെ ret;
    }
    ശൂന്യം userIterate(Ast_കണ്ണി* nodep, WidthVP* vup) {
        എങ്കിൽ (!nodep) തിരികെ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    ശൂന്യം userIterateAndNext(Ast_കണ്ണി* nodep, WidthVP* vup) {
        എങ്കിൽ (!nodep) തിരികെ;
        എങ്കിൽ (nodep->didWidth()) തിരികെ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateAndNextNull(nodep);
    }
    ശൂന്യം userIterateChildren(Ast_കണ്ണി* nodep, WidthVP* vup) {
        എങ്കിൽ (!nodep) തിരികെ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    ശൂന്യം userIterateChildrenBackwardsConst(Ast_കണ്ണി* nodep, WidthVP* vup) {
        എങ്കിൽ (!nodep) തിരികെ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

പൊതു:
    // CONSTRUCTORS
    WidthVisitor(തർക്കം paramsOnly,  // [in] TRUE if we are considering parameters only.
                 തർക്കം doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enumTempNames{"__Venum"}
        , m_insideTempNames{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_കണ്ണി* mainAcceptEdit(Ast_കണ്ണി* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, WidthVP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        വേണ്ടി (AstVar* സ്ഥിരം varp : m_splitIfaceArrayVarps) {
            VL_DO_DANGLING(തള്ളുക_Deletep(varp->unlinkFrBack()), varp);
        }
        m_splitIfaceArrayVarps.clear();
        തിരികെ nodep;
    }
    സ്ഥിതം തർക്കം lowerAsFixedAggregate(സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം dtypep) {
        തിരികെ dtypep->isStreamableFixedAggregate() && dtypep->containsUnpackedStruct();
    }
    ശൂന്യം streamAssignLower(Ast_കണ്ണി_Assign* nodep) {
        Ast_കണ്ണി_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        എങ്കിൽ (Ast_കണ്ണി_Stream* സ്ഥിരം streamp = VN_CAST(nodep->rhsp(), കണ്ണി_Stream)) {
            Ast_കണ്ണി_DType* സ്ഥിരം lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lwidth = lhsDTypeSkippedRefp->widthStream();
            സ്ഥിരം പൂർണ്ണസംഖ്യ rwidth = streamp->lhsp()->dtypep()->skipRefp()->widthStream();
            എങ്കിൽ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            എങ്കിൽ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ queueElementSize = streamp->lhsp()->dtypep()->subDTypep()->width();
                UASSERT_OBJ(queueElementSize <= lwidth, nodep, "LHS < RHS");
            }
            എങ്കിൽ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || lowerAsFixedAggregate(lhsDTypeSkippedRefp)) {
                streamp->unlinkFrBack();
                nodep->rhsp(
                    പുതിയ AstCvtPackedToArray{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        എങ്കിൽ (Ast_കണ്ണി_Stream* സ്ഥിരം streamp = VN_CAST(nodep->lhsp(), കണ്ണി_Stream)) {
            സ്ഥിരം Ast_കണ്ണി_DType* സ്ഥിരം rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_കണ്ണി_DType* സ്ഥിരം lhsStreamDTypep = streamp->lhsp()->dtypep()->skipRefp();
            സ്ഥിരം പൂർണ്ണസംഖ്യ lwidth = lhsStreamDTypep->widthStream();
            സ്ഥിരം പൂർണ്ണസംഖ്യ rwidth = rhsDTypep->widthStream();
            എങ്കിൽ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            എങ്കിൽ (lowerAsFixedAggregate(lhsStreamDTypep)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം streamExprp = nodep->lhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം dstp = streamp->lhsp()->unlinkFrBack();
                Ast_കണ്ണി_വ്യഞ്ജകം* srcp = nodep->rhsp()->unlinkFrBack();
                എങ്കിൽ (VN_IS(streamp, StreamL)) {
                    streamp->lhsp(srcp);
                    streamp->dtypeSetBitOrLogicUnsized(
                        srcp->width(), srcp->widthMin(), VSigning::UNSIGNED,
                        Ast_കണ്ണി_വ്യഞ്ജകം::isFourstateOrNoDtype(streamExprp, srcp));
                    srcp = streamExprp;
                } അല്ലെങ്കിൽ {
                    എങ്കിൽ (srcp->width() > lwidth) {
                        srcp = പുതിയ AstSel{streamp->fileline(), srcp, srcp->width() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(തള്ളുക_Deletep(streamExprp), streamExprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(പുതിയ AstCvtPackedToArray{srcp->fileline(), srcp, lhsStreamDTypep});
                nodep->dtypeFrom(dstp);
            } അല്ലെങ്കിൽ എങ്കിൽ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp()->unlinkFrBack();
                nodep->rhsp(പുതിയ AstCvtArrayToPacked{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~WidthVisitor() override = സ്ഥിരസ്ഥിതി;
};

//######################################################################
// Width class functions

ശൂന്യം V3Width::width(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        സ്ഥിരം WidthClearVisitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        സ്ഥിരം VDTypeNameScopes scopes{nodep};
        WidthVisitor visitor{അസത്യം, അസത്യം};
        (ശൂന്യം)visitor.mainAcceptEdit(nodep);
        WidthRemoveVisitor rvisitor;
        (ശൂന്യം)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("width", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_കണ്ണി* V3Width::widthParamsEdit(Ast_കണ്ണി* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{സത്യം, അസത്യം};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    തിരികെ nodep;
}

ശൂന്യം V3Width::streamAssignLowerEdit(Ast_കണ്ണി_Assign* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    WidthVisitor visitor{അസത്യം, അസത്യം};
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
Ast_കണ്ണി* V3Width::widthGenerateParamsEdit(
    Ast_കണ്ണി* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    WidthVisitor visitor{സത്യം, സത്യം};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    തിരികെ nodep;
}

Ast_കണ്ണി* V3Width::selectNonConstantRecurse(Ast_കണ്ണി* nodep, തർക്കം inSel) {
    // If node has a non-constant select, return that select
    Ast_കണ്ണി* resultp = nullptr;
    എങ്കിൽ (Ast_കണ്ണി_Sel* സ്ഥിരം anodep = VN_CAST(nodep, കണ്ണി_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        എങ്കിൽ (resultp) തിരികെ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), സത്യം);
    } അല്ലെങ്കിൽ എങ്കിൽ (AstSel* സ്ഥിരം anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), inSel);
        എങ്കിൽ (resultp) തിരികെ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), സത്യം);
    } അല്ലെങ്കിൽ എങ്കിൽ (Ast_കണ്ണി_Var_Ref* സ്ഥിരം anodep = VN_CAST(nodep, കണ്ണി_Var_Ref)) {
        എങ്കിൽ (inSel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) തിരികെ anodep;
    } അല്ലെങ്കിൽ {
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, inSel);
        എങ്കിൽ (resultp) തിരികെ resultp;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, inSel);
        എങ്കിൽ (resultp) തിരികെ resultp;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, inSel);
        എങ്കിൽ (resultp) തിരികെ resultp;
        എങ്കിൽ (Ast_കണ്ണി* സ്ഥിരം refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, inSel);
    }
    തിരികെ resultp;
}
