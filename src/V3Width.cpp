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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Width.h"

#ضمن "V3Assert.h"
#ضمن "V3Ast.h"
#ضمن "V3Begin.h"
#ضمن "V3Const.h"
#ضمن "V3ConstPool.h"
#ضمن "V3Error.h"
#ضمن "V3Global.h"
#ضمن "V3LinkLValue.h"
#ضمن "V3MemberMap.h"
#ضمن "V3Number.h"
#ضمن "V3Randomize.h"
#ضمن "V3String.h"
#ضمن "V3Task.h"
#ضمن "V3UniqueNames.h"
#ضمن "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#تعریف_کړه VERILATOR_V3_پلنوالی_CPP
#ضمن "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ثابت Ast_غوټه*, AstClass*> m_cache;

    AstClass* پیدا_کړه_Cached(Ast_غوټه* nodep) {
        إذا (!nodep) أرجع تش_اشاره;
        ثابت خپلکاری it = m_cache.پیدا_کړه(nodep);
        إذا (it != m_cache.پای_ټکی()) أرجع it->دوهم;

        AstClass* classp = تش_اشاره;
        إذا (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = پیدا_کړه_Cached(nodep->backp());
        } وإلا إذا (AstClass* ثابت parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } وإلا إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } وإلا {
            classp = پیدا_کړه_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        أرجع classp;
    }

عام:
    AstClass* پیدا_کړه(Ast_غوټه* nodep) {
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) أرجع classp;
        إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep, ClassPackage)) {
            أرجع packagep->classp();
        }
        أرجع پیدا_کړه_Cached(nodep);
    }
};

//######################################################################

تعداد Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& چلوونکی<<(std::ostream& str, ثابت Stage& rhs) {
    أرجع str << ("-PFB"[static_cast<صحيح>(rhs)]);
}

تعداد Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ټاکل_3  // Assignment-like where sign comes from RHS only
};
std::ostream& چلوونکی<<(std::ostream& str, ثابت Determ& rhs) {
    ساكن constexpr ثابت حرف* ثابت s_det[] = {"SELF", "CNTX", "ASSN"};
    أرجع str << s_det[rhs];
}

تعداد بهیر_Use : uint8_t {
    بهیر_USE_NONE,  // Stream concatenation is not allowed in this context
    بهیر_USE_ټاکل,  // Assignment-like context permits stream concatenation
    بهیر_USE_CAST,  // Explicit cast context permits stream concatenation
};

#تعریف_کړه v3widthWarn(lhs, rhs, پیغام_2) \
    v3warn_کوډ(((lhs) < (rhs)   ? V3_Error_کوډ::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_کوډ::WIDTHEXPAND \
                                : V3_Error_کوډ::پلنوالی_2), \
               پیغام_2)

//######################################################################
// Width state, as a visitor of each AstNode

صنف پلنوالی_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_غوټه_DType* ثابت m_dtypep;  // Parent's data type to resolve to
    ثابت Stage m_stage;  // If true, report errors
    ثابت بهیر_Use m_بهیر_Use;  // Current expression may be a stream concat

عام:
    پلنوالی_VP(Ast_غوټه_DType* dtypep, Stage stage, بهیر_Use بهیر_Use_2 = بهیر_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_بهیر_Use{بهیر_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    پلنوالی_VP(Determ determ, Stage stage, بهیر_Use بهیر_Use_2 = بهیر_USE_NONE)
        : m_dtypep{تش_اشاره}
        , m_stage{stage}
        , m_بهیر_Use{بهیر_Use_2} {
        إذا (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    پلنوالی_VP* p() { أرجع هذا; }
    منطقي selfDtm() ثابت { أرجع m_dtypep == تش_اشاره; }
    بهیر_Use بهیر_Use_2() ثابت { أرجع m_بهیر_Use; }
    Ast_غوټه_DType* dtypep() ثابت {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        أرجع m_dtypep;
    }
    Ast_غوټه_DType* dtypeNullp() ثابت { أرجع m_dtypep; }
    Ast_غوټه_DType* dtypeNullSkipRefp() ثابت {
        Ast_غوټه_DType* dtp = dtypeNullp();
        إذا (dtp) dtp = dtp->skipRefp();
        أرجع dtp;
    }
    Ast_غوټه_DType* dtypeOverridep(Ast_غوټه_DType* ثابت defaultp) ثابت {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        إذا (!m_dtypep) أرجع defaultp;
        إذا (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            أرجع defaultp->پیدا_کړه_بټ_Or_Logic_DType(m_dtypep->پلنوالی(), m_dtypep->پلنوالی_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        أرجع m_dtypep;
    }
    صحيح پلنوالی() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->پلنوالی();
    }
    صحيح پلنوالی_Min() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->پلنوالی_Min();
    }
    منطقي prelim() ثابت { أرجع m_stage & PRELIM; }
    منطقي final() ثابت { أرجع m_stage & FINAL; }
    فراغ dump(std::ostream& str) ثابت {
        إذا (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } وإلا {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_منزل_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& چلوونکی<<(std::ostream& str, ثابت پلنوالی_VP* vup) {
    إذا (vup) vup->dump(str);
    أرجع str;
}

//######################################################################

صنف پلنوالی_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    فراغ clear_پلنوالی_Recurse(Ast_غوټه* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            nodep->did_پلنوالی(خطأ);
            إذا (Ast_غوټه* ثابت refp = nodep->op1p()) clear_پلنوالی_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op2p()) clear_پلنوالی_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op3p()) clear_پلنوالی_Recurse(refp);
            إذا (Ast_غوټه* ثابت refp = nodep->op4p()) clear_پلنوالی_Recurse(refp);
        }
    }

عام:
    // CONSTRUCTORS
    څرګند پلنوالی_Clear_Visitor(AstNetlist* nodep) { clear_پلنوالی_Recurse(nodep); }
    مجازي ~پلنوالی_Clear_Visitor() = افتراضي;
};

//######################################################################

#تعریف_کړه accept دننه_پلنوالی_Visitor_use_Ast_غوټه_iterate_instead_د_Ast_غوټه_accept

//######################################################################

صنف پلنوالی_Visitor final : عام VNVisitor {
    // TYPES
    کارول PatVecMap = std::map<صحيح, AstPatMember*>;
    کارول DTypeMap = std::map<ثابت std::string, AstPatMember*>;

    // STATE
    V3_Unique_نومونه m_enum_لنډمهاله_نومونه;  // Captured enum expressions
    V3_Unique_نومونه m_inside_لنډمهاله_نومونه;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ثابت AstModport*, std::unordered_سېټ<string>>
        m_modport_نومونه;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    پلنوالی_VP* m_vup = تش_اشاره;  // Current node state
    منطقي m_underFork = خطأ;  // Visiting under a fork
    منطقي m_underSExpr = خطأ;  // Visiting under a sequence expression
    منطقي m_underPackedArray = خطأ;  // Visiting under a AstPackArrayDType
    منطقي m_underMemberSel = خطأ;  // Viting under a MemberSel
    Ast_غوټه* m_seqUnsupp = تش_اشاره;  // Property has unsupported node
    منطقي m_hasSExpr = خطأ;  // Property has a sequence expression
    ثابت AstCell* m_cellp = تش_اشاره;  // Current cell for arrayed instantiations
    std::ویکټور<AstVar*> m_وېشل_Iface_Array_Varps;  // Interface arrays split by V3Param
    ثابت Ast_Enum_توکی* m_enumItemp = تش_اشاره;  // Current enum item
    Ast_غوټه_FTask* m_ftaskp = تش_اشاره;  // Current function/task
    Ast_غوټه_Module* m_modep = تش_اشاره;  // Current module
    ثابت AstConstraint* m_constraintp = تش_اشاره;  // Current constraint
    Ast_غوټه_Procedure* m_procedurep = تش_اشاره;  // Current final/always
    ثابت AstWith* m_withp = تش_اشاره;  // Current 'with' statement
    ثابت AstFunc* m_funcp = تش_اشاره;  // Current function
    ثابت Ast_Attr_د* m_attrp = تش_اشاره;  // Current attribute
    ثابت Ast_غوټه_بیان* m_randomizeFromp = تش_اشاره;  // Current randomize method call fromp
    ثابت منطقي m_paramsOnly;  // Computing parameter value; limit operation
    ثابت منطقي m_doGenerate;  // Do errors later inside generate statement
    منطقي m_بهیر_Concat = خطأ;  // True if visiting arguments of stream concatenation
    منطقي m_دننه_Param_Override = خطأ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ثابت Ast_غوټه_DType*, VAttrType>, AstVarRef*> m_جدول_Map;
    // Queues with given index type
    std::map<ثابت Ast_غوټه_DType*, Ast_کتار_DType*> m_کتار_DType_شاخصي;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_سېټ<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_سېټ<ثابت AstVar*> m_curModVars;  // Variables declared in current module

    ساكن constexpr صحيح ENUM_LOOKUP_بټونه = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    تعداد Extend_قاعده : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    تعداد صنف ComparisonType : uint8_t {
        نورمال_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        نورمال_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ساكن بهیر_Use بهیر_Use_For_Determ(Determ determ) {
        أرجع determ == ټاکل_3 ? بهیر_USE_ټاکل : بهیر_USE_NONE;
    }

    بهیر_Use current_بهیر_Use() ثابت { أرجع m_vup ? m_vup->بهیر_Use_2() : بهیر_USE_NONE; }

    ساكن فراغ packIfUnpacked(Ast_غوټه_بیان* ثابت nodep) {
        إذا (AstUnpackArrayDType* ثابت unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ثابت صحيح elements_عدد = unpackDTypep->arrayUnpackedElements();
            ثابت صحيح unpack_Min_بټونه = elements_عدد * unpackDTypep->subDTypep()->پلنوالی_Min();
            ثابت صحيح unpack_بټونه = elements_عدد * unpackDTypep->subDTypep()->پلنوالی();
            VNRelinker relinker;
            nodep->unlink_Fr_شا(&relinker);
            relinker.relink(جديد Ast_Cvt_Array_منزل_Packed{
                nodep->fileline(), nodep,
                nodep->پیدا_کړه_بټ_Or_Logic_DType(unpack_بټونه, unpack_Min_بټونه, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ساكن Ast_غوټه_DType* fromDTypep(Ast_غوټه* fromp) {
        إذا (Ast_غوټه_DType* ثابت dtypep = VN_CAST(fromp, غوټه_DType))
            أرجع dtypep->skipRefOrNullp();
        أرجع fromp ? fromp->dtypep() : تش_اشاره;
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
    فراغ لیدنه(AstLogNot* nodep) override { لیدنه_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    فراغ لیدنه(AstLogAnd* nodep) override { لیدنه_log_and_or(nodep); }
    فراغ لیدنه(AstLogOr* nodep) override { لیدنه_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    فراغ لیدنه(AstSAnd* nodep) override { لیدنه_log_and_or(nodep); }
    فراغ لیدنه(AstSIntersect* nodep) override { لیدنه_log_and_or(nodep); }
    فراغ لیدنه(AstSOr* nodep) override { لیدنه_log_and_or(nodep); }
    فراغ لیدنه(AstSWithin* nodep) override { لیدنه_log_and_or(nodep); }
    فراغ لیدنه(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        لیدنه_log_and_or(nodep);
    }
    فراغ لیدنه(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        لیدنه_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    فراغ لیدنه(AstRedAnd* nodep) override { لیدنه_red_and_or(nodep); }
    فراغ لیدنه(AstRedOr* nodep) override { لیدنه_red_and_or(nodep); }
    فراغ لیدنه(AstRedXor* nodep) override { لیدنه_red_and_or(nodep); }
    فراغ لیدنه(AstOneHot* nodep) override { لیدنه_red_and_or(nodep); }
    فراغ لیدنه(AstOneHot0* nodep) override { لیدنه_red_and_or(nodep); }
    فراغ لیدنه(AstIsUnknown* nodep) override {
        لیدنه_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    فراغ لیدنه(AstEq* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_EQ); }
    فراغ لیدنه(AstNeq* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_EQ); }
    فراغ لیدنه(AstGt* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstGte* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstLt* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstLte* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstGtS* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstGteS* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstLtS* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstLteS* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::نورمال_GL); }
    فراغ لیدنه(AstEqCase* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::CASE); }
    فراغ لیدنه(AstNeqCase* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    فراغ لیدنه(AstEqWild* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::WILD); }
    فراغ لیدنه(AstNeqWild* nodep) override { لیدنه_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    فراغ لیدنه(AstEqD* nodep) override { لیدنه_cmp_real(nodep); }
    فراغ لیدنه(AstNeqD* nodep) override { لیدنه_cmp_real(nodep); }
    فراغ لیدنه(AstLtD* nodep) override { لیدنه_cmp_real(nodep); }
    فراغ لیدنه(AstLteD* nodep) override { لیدنه_cmp_real(nodep); }
    فراغ لیدنه(AstGtD* nodep) override { لیدنه_cmp_real(nodep); }
    فراغ لیدنه(AstGteD* nodep) override { لیدنه_cmp_real(nodep); }
    // ...    String compares
    فراغ لیدنه(AstEqN* nodep) override { لیدنه_cmp_string(nodep); }
    فراغ لیدنه(AstNeqN* nodep) override { لیدنه_cmp_string(nodep); }
    فراغ لیدنه(AstLtN* nodep) override { لیدنه_cmp_string(nodep); }
    فراغ لیدنه(AstLteN* nodep) override { لیدنه_cmp_string(nodep); }
    فراغ لیدنه(AstGtN* nodep) override { لیدنه_cmp_string(nodep); }
    فراغ لیدنه(AstGteN* nodep) override { لیدنه_cmp_string(nodep); }
    // ...    Data type compares
    فراغ لیدنه(AstEqT* nodep) override { لیدنه_cmp_type(nodep); }
    فراغ لیدنه(AstNeqT* nodep) override { لیدنه_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    فراغ لیدنه(AstAnd* nodep) override { لیدنه_boolexpr_and_or(nodep); }
    فراغ لیدنه(AstOr* nodep) override { لیدنه_boolexpr_and_or(nodep); }
    فراغ لیدنه(AstXor* nodep) override { لیدنه_boolexpr_and_or(nodep); }
    فراغ لیدنه(AstBufIf1* nodep) override {
        لیدنه_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    فراغ لیدنه(Ast_جمع_کړه* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, ریښتیا, خطأ); }
    فراغ لیدنه(AstSub* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, ریښتیا, خطأ); }
    فراغ لیدنه(AstDiv* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, ریښتیا, ریښتیا); }
    فراغ لیدنه(AstMul* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, ریښتیا, خطأ); }
    // These can't promote to real
    فراغ لیدنه(AstModDiv* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, خطأ, ریښتیا); }
    فراغ لیدنه(AstModDivS* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, خطأ, ریښتیا); }
    فراغ لیدنه(AstMulS* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, خطأ, خطأ); }
    فراغ لیدنه(AstDivS* nodep) override { لیدنه_جمع_کړه_sub_replace(nodep, خطأ, ریښتیا); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    فراغ لیدنه(AstNegate* nodep) override { لیدنه_negate_not(nodep, ریښتیا); }
    // Unary never real
    فراغ لیدنه(AstNot* nodep) override { لیدنه_negate_not(nodep, خطأ); }

    // Real: inputs and output real
    فراغ لیدنه(Ast_جمع_کړه_D* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    فراغ لیدنه(AstSubD* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    فراغ لیدنه(AstDivD* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    فراغ لیدنه(AstMulD* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    فراغ لیدنه(AstPowD* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    فراغ لیدنه(Ast_غوټه_System_Biop_D* nodep) override { لیدنه_real_جمع_کړه_sub(nodep); }
    // Real: Output real
    فراغ لیدنه(AstNegateD* nodep) override { لیدنه_real_neg_ceil(nodep); }
    فراغ لیدنه(Ast_غوټه_System_Uniop_D* nodep) override { لیدنه_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    فراغ لیدنه(AstSigned* nodep) override { لیدنه_signed_unsigned(nodep, VSigning::SIGNED); }
    فراغ لیدنه(AstUnsigned* nodep) override { لیدنه_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    فراغ لیدنه(Ast_ښويدنه_L* nodep) override { لیدنه_ښويدنه_2(nodep); }
    فراغ لیدنه(Ast_ښويدنه_R* nodep) override { لیدنه_ښويدنه_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    فراغ لیدنه(Ast_ښويدنه_RS* nodep) override { لیدنه_ښويدنه_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    فراغ لیدنه(Ast_بټونه_منزل_Real_D* nodep) override { لیدنه_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    فراغ لیدنه(AstRToIS* nodep) override { لیدنه_Os32_Lr(nodep); }
    فراغ لیدنه(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->پلنوالی_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    فراغ لیدنه(Ast_Real_منزل_بټونه* nodep) override { لیدنه_Ou64_Lr(nodep); }

    // Output integer, input string
    فراغ لیدنه(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_Int();
        }
    }
    فراغ لیدنه(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_سېټ_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    فراغ لیدنه(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ_Sized(8, VSigning::UNSIGNED);
        }
    }
    فراغ لیدنه(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ_Sized(8, VSigning::UNSIGNED);
        }
    }
    فراغ لیدنه(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_سېټ_String();
        }
    }
    فراغ لیدنه(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_Int();
        }
    }
    فراغ لیدنه(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            إذا (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_سېټ_Double();
            } وإلا {
                nodep->dtype_سېټ_Integer();
            }
        }
    }
    فراغ لیدنه(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    فراغ لیدنه(Ast_وخت* nodep) override { nodep->dtype_سېټ_Time2_State(); }
    فراغ لیدنه(Ast_وخت_D* nodep) override { nodep->dtype_سېټ_Double(); }
    فراغ لیدنه(Ast_وخت_Precision* nodep) override { nodep->dtype_سېټ_Integer2_State(); }
    فراغ لیدنه(Ast_Get_Initial_Random_تخم* nodep) override { nodep->dtype_سېټ_Int(); }
    فراغ لیدنه(Ast_وخت_Unit* nodep) override {
        nodep->replaceWith(
            جديد AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_د_Ten()});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstScopeName* nodep) override {
        nodep->dtype_سېټ_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ساكن منطقي is_Override_نمونه(ثابت Ast_غوټه* nodep) {
        إذا (ثابت Ast_نمونه* ثابت patternp = VN_CAST(nodep, نمونه)) {
            أرجع !patternp->ماشوم_DTypep();
        }
        ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond);
        أرجع condp && is_Override_نمونه(condp->thenp()) && is_Override_نمونه(condp->elsep());
    }
    فراغ لیدنه(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_بیان(nodep);
        إذا (m_دننه_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ثابت صحيح errors = V3Error::error_شمېر();
            {
                VL_RESTORER(m_دننه_Param_Override);
                m_دننه_Param_Override = خطأ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const);
            ثابت منطقي معلوم = constp && !constp->عدد().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            إذا (!معلوم && V3Error::error_شمېر() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_غوټه_بیان* ثابت keepp
                = (معلوم && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_شا();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            userIterate(keepp, پلنوالی_VP{SELF, BOTH}.p());
            أرجع;
        }
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_بل(nodep->thenp(), پلنوالی_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_بل(nodep->elsep(), پلنوالی_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_غوټه_DType* ثابت thenDTypep = nodep->thenp()->dtypep();
            Ast_غوټه_DType* ثابت elseDTypep = nodep->elsep()->dtypep();
            Ast_غوټه_DType* ثابت thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_غوټه_DType* ثابت elseDTypeSkippedp = elseDTypep->skipRefp();
            إذا (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->پیدا_کړه_Null_DType());
            } وإلا إذا (nodep->thenp()->is_Class_Handle_ارزښت()
                       || nodep->elsep()->is_Class_Handle_ارزښت()) {
                Ast_غوټه_DType* commonClassTypep = تش_اشاره;
                إذا (nodep->thenp()->is_Class_Handle_ارزښت() && nodep->elsep()->is_Class_Handle_ارزښت()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_غوټه::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                إذا (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } وإلا {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } وإلا إذا (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                إذا (Ast_غوټه::computeCastable(elseDTypep, thenDTypep, تش_اشاره).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } وإلا {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    إذا (!Ast_غوټه::computeCastable(thenDTypep, elseDTypep, تش_اشاره)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } وإلا إذا (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_سېټ_Double();
            } وإلا إذا (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_سېټ_String();
            } وإلا {
                ثابت منطقي is_Any_پایله_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                منطقي handled = خطأ;
                ثابت منطقي isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_غوټه_DType* ثابت thenEnumDTypep = thenDTypep->skip_Ref_منزل_Enump();
                إذا (VN_IS(thenDTypeSkippedp, غوټه_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_ونه(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = ریښتیا;
                } وإلا إذا (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_ونه(elseDTypep->skip_Ref_منزل_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    إذا (is_Any_پایله_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = ریښتیا;
                    } وإلا {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(جديد File_کرښه{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_کوډ::ENUMVALUE, ریښتیا);
                    }
                }
                إذا (!handled) {
                    ثابت صحيح پلنوالی = std::max(thenDTypep->پلنوالی(), elseDTypep->پلنوالی());
                    ثابت صحيح پلنوالی_Min = std::max(thenDTypep->پلنوالی_Min(), elseDTypep->پلنوالی_Min());
                    ثابت VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ثابت منطقي isFourstate = is_Any_پایله_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    إذا ((پلنوالی == elseDTypeSkippedp->پلنوالی()
                         && پلنوالی_Min == elseDTypeSkippedp->پلنوالی_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } وإلا إذا ((پلنوالی == thenDTypeSkippedp->پلنوالی()
                                && پلنوالی_Min == thenDTypeSkippedp->پلنوالی_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } وإلا {
                        // None of the children dtypes match generate one
                        nodep->dtype_سېټ_بټ_Or_Logic_Unsized(پلنوالی, پلنوالی_Min, numeric, isFourstate);
                    }
                }
            }
        }
        إذا (m_vup->final()) {
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    فراغ لیدنه(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, پلنوالی_VP{SELF, BOTH}.p());
        // Conversions
        إذا (Ast_غوټه_بیان* ثابت newp = new_Cons_Dyn_Array_Or_کتار(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            جمع_کړه_Cons_توکی(newp, nodep->lhsp()->unlink_Fr_شا());
            جمع_کړه_Cons_توکی(newp, nodep->rhsp()->unlink_Fr_شا());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_نمونه* ثابت newp = جديد Ast_نمونه{nodep->fileline(), تش_اشاره};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        }

        // Concat handling
        إذا (m_vup->prelim()) {
            إذا (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, کتار_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            إذا (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_سېټ_String();
            } وإلا {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                إذا (m_بهیر_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                    nodep->lhsp()->پلنوالی() + nodep->rhsp()->پلنوالی(),
                    nodep->lhsp()->پلنوالی_Min() + nodep->rhsp()->پلنوالی_Min(), VSigning::UNSIGNED,
                    Ast_غوټه_بیان::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->lhsp(), Replicate)) {
                إذا (repp->پلنوالی() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_شا());
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->rhsp(), Replicate)) {
                إذا (repp->پلنوالی() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_شا());
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        إذا (m_vup->final()) {
            إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_غوټه_بیان* lhsp = nodep->lhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* rhsp = nodep->rhsp()->unlink_Fr_شا();
                إذا (!lhsp->isString()) lhsp = جديد AstCvtPackString{lhsp->fileline(), lhsp};
                إذا (!rhsp->isString()) rhsp = جديد AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_غوټه* ثابت newp = جديد AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (!nodep->dtypep()->پلنوالی_Sized()) {
                // See also error in V3Number
                غوټه_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ لیدنه(AstConcatN* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_String();
        }
        إذا (m_vup->final()) {
            nodep->did_پلنوالی(ریښتیا);
            إذا (!nodep->dtypep()->پلنوالی_Sized()) {
                // See also error in V3Number
                غوټه_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ لیدنه(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    فراغ لیدنه(AstDelay* nodep) override {
        إذا (nodep->is_دوره_Delay() && m_underSExpr) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            إذا (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            إذا (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                أرجع;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_دوره_Delay(nodep, nodep);
            أرجع;
        }
        إذا (Ast_غوټه_بیان* ثابت fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            أرجع;
        }
        إذا (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_سېټ_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_بل_Null(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_سېټ_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } وإلا {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->جمع_کړه_بل_دلته(nodep->stmtsp()->unlink_Fr_شا_With_بل());
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    فراغ لیدنه(AstFork* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        إذا (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_سېټ_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ثابت newp = جديد AstBegin{nodep->fileline(), nodep->name(), تش_اشاره, خطأ};
            nodep->replaceWith(newp);
            إذا (nodep->declsp()) newp->جمع_کړه_Declsp(nodep->declsp()->unlink_Fr_شا_With_بل());
            إذا (nodep->stmtsp()) newp->جمع_کړه_Stmtsp(nodep->stmtsp()->unlink_Fr_شا_With_بل());
            إذا (nodep->forksp()) newp->جمع_کړه_Stmtsp(nodep->forksp()->unlink_Fr_شا_With_بل());
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (v3Global.opt.timing().is_سېټ_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = ریښتیا;
            iterateChildren(nodep);
        } وإلا إذا (v3Global.opt.timing().is_سېټ_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
        } وإلا {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    فراغ لیدنه_Wait_Or_Disable_Fork(Ast_غوټه* nodep) {
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_سېټ_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                أرجع;
            } وإلا إذا (!v3Global.opt.timing().is_سېټ_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    فراغ لیدنه(AstDisableFork* nodep) override { لیدنه_Wait_Or_Disable_Fork(nodep); }
    فراغ لیدنه(AstWaitFork* nodep) override { لیدنه_Wait_Or_Disable_Fork(nodep); }
    فراغ لیدنه(Ast_منزل_ښکتنی_N* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_String();
        }
    }
    فراغ لیدنه(Ast_منزل_پاسنی_N* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_String();
        }
    }
    فراغ لیدنه(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
            إذا (constp) {
                إذا (constp->عدد().isFourState()
                    || (constp->dtypep()->isSigned() && constp->عدد().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } وإلا {
                    times = constp->منزل_SInt();
                }
            }

            Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
            إذا (VN_IS(vdtypep, کتار_DType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                إذا (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                إذا (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_بل(nodep->srcp(), پلنوالی_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_شا());
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                } وإلا {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_نمونه* ثابت newp = جديد Ast_نمونه{
                        nodep->fileline(),
                        جديد AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_شا(),
                                         تش_اشاره, تش_اشاره}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    أرجع;
                }
            }
            إذا (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            إذا (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } وإلا {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            إذا ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_غوټه* ثابت newp
                    = جديد AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_شا(),
                                        nodep->countp()->unlink_Fr_شا()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                إذا (!constp) nodep->v3error("Replication value isn't a constant.");
                إذا (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                    (nodep->srcp()->پلنوالی() * times), (nodep->srcp()->پلنوالی_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->پلنوالی_Sized()) {
                // See also error in V3Number
                غوټه_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ لیدنه(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_سېټ_String();
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->پلنوالی_Sized()) {
                // See also error in V3Number
                غوټه_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ لیدنه(Ast_غوټه_Dist_Biop* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    فراغ لیدنه(Ast_غوټه_Dist_Triop* nodep) override {
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    منطقي بهیر_Implicit_Use_Allowed(ثابت Ast_غوټه_بهیر* nodep) ثابت {
        إذا (m_بهیر_Concat) أرجع ریښتیا;
        إذا (m_vup && m_vup->بهیر_Use_2() != بهیر_USE_NONE) أرجع ریښتیا;

        ثابت Ast_غوټه* ثابت backp = nodep->backp();
        إذا (!backp) أرجع ریښتیا;  // Error elsewhere

        أرجع VN_IS(backp, غوټه_ټاکل);
    }
    فراغ لیدنه(Ast_غوټه_بهیر* nodep) override {
        VL_RESTORER(m_بهیر_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            m_بهیر_Concat = ریښتیا;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_بهیر_Concat = خطأ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
                إذا (constp->منزل_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } وإلا {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ثابت Ast_غوټه_DType* ثابت lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_منزل_Enump();
            إذا (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, کتار_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, غوټه_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, غوټه_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_سېټ_بهیر();
            } وإلا إذا (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_سېټ_بهیر();
            } وإلا {
                nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                    nodep->lhsp()->پلنوالی(), nodep->lhsp()->پلنوالی_Min(), VSigning::UNSIGNED,
                    Ast_غوټه_بیان::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            إذا (!بهیر_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_نور() << "... Suggest use a cast");
            }
            إذا (!nodep->dtypep()->پلنوالی_Sized()) {
                // See also error in V3Number
                غوټه_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    فراغ لیدنه(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), ریښتیا,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), ریښتیا,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        إذا (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ثابت صحيح پلنوالی = nodep->elementsConst();
            إذا (nodep->fromBracket() && nodep->کیڼ_Const() > nodep->ښی_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-پلنوالی + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } وإلا إذا (پلنوالی > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << پلنوالی << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ثابت منطقي دننه_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ثابت منطقي دننه_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ثابت منطقي دننه_Type_جدول = !m_modep;
            ثابت Ast_غوټه* basep = nodep->backp();
            طالما (VN_IS(basep, Range)) basep = basep->backp();
            إذا (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !دننه_Dead_Module && !دننه_Parameterized_Template && !دننه_Type_جدول) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->کیڼ_Const() << ":" << nodep->ښی_Const()
                                            << "]");
            }
        }
    }

    فراغ لیدنه(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        إذا (nodep->did_پلنوالی()) أرجع;
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بل(nodep->lsbp(), پلنوالی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), خطأ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            صحيح پلنوالی = nodep->پلنوالی_Const();
            إذا (پلنوالی <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_سېټ_بټ();
                أرجع;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ثابت منطقي isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ثابت منطقي دننه_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            إذا (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                پلنوالی = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_سېټ_بټ_Or_Logic_Sized(پلنوالی, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->پلنوالی_Const(پلنوالی);
                ټېل_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(جديد AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            إذا (nodep->fromp()->پلنوالی() < پلنوالی && !دننه_Parameterized_Template) {
                // If it is not a lvalue, extend it
                إذا (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << پلنوالی << " bits from only "
                                                          << nodep->fromp()->پلنوالی()
                                                          << " bit number");
                    Ast_غوټه_DType* ثابت subDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                        پلنوالی, پلنوالی, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    پلنوالی_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    خطأ /*noerror*/);
                } وإلا {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << پلنوالی << " bits to only "
                                                         << nodep->fromp()->پلنوالی()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            صحيح frommsb = nodep->fromp()->پلنوالی() - 1;
            صحيح fromlsb = 0;
            ثابت صحيح elw = nodep->decl_El_پلنوالی();  // Must adjust to tell user bit ranges
            إذا (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } وإلا {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ثابت صحيح selwidth = V3_شمېره::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_غوټه_DType* ثابت selwidthDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, FINAL}.p());
            user_Iterate_And_بل(nodep->lsbp(), پلنوالی_VP{SELF, FINAL}.p());
            إذا (پلنوالی_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->پلنوالی() != 32) {
                إذا (!nodep->fileline()->warnIsOff(V3_Error_کوډ::پلنوالی_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->پلنوالی() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->پلنوالی() / elw)
                            << (nodep->lsbp()->پلنوالی() != nodep->lsbp()->پلنوالی_Min()
                                    ? " or " + cvt_منزل_Str(nodep->lsbp()->پلنوالی_Min() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            إذا (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                إذا (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } وإلا إذا (!دننه_Parameterized_Template) {
                    إذا (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } وإلا {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                إذا (!isWriteSelect) {
                    // Extend it
                    ثابت صحيح extend_منزل = nodep->msbConst() + 1;
                    Ast_غوټه_DType* ثابت subDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                        extend_منزل, extend_منزل, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    پلنوالی_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    خطأ /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            إذا (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                پلنوالی_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    فراغ لیدنه(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            إذا (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const)) {
                    ثابت صحيح بټ_2 = constp->منزل_SInt();
                    Ast_غوټه_بیان* newp = newIfaceArrayElement(nodep->fromp(), بټ_2);
                    إذا (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ثابت AstUnpackArrayDType* ثابت arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << بټ_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            //
            صحيح frommsb;
            صحيح fromlsb;
            ثابت Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                إذا (fromlsb > frommsb) std::بدلون(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                إذا (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } وإلا {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ثابت صحيح selwidth = V3_شمېره::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_غوټه_DType* ثابت selwidthDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            إذا (پلنوالی_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->پلنوالی() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->پلنوالی(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->پلنوالی()
                                       << (nodep->bitp()->پلنوالی() != nodep->bitp()->پلنوالی_Min()
                                               ? " or " + cvt_منزل_Str(nodep->bitp()->پلنوالی_Min())
                                               : "")
                                       << " bits.");
                إذا (!nodep->fileline()->warnIsOff(V3_Error_کوډ::پلنوالی_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            إذا (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                إذا (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->منزل_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->منزل_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    إذا (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->منزل_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                پلنوالی_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
        }
    }

    فراغ لیدنه(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->کلی_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ لیدنه(AstAlias* nodep) override {
        إذا (!nodep->did_پلنوالی_And_سېټ()) {
            user_Iterate_And_بل(nodep->itemsp(), پلنوالی_VP{SELF, BOTH}.p());
        }

        ثابت خپلکاری check_If_بیان_Ok = [هذا](ثابت Ast_غوټه_بیان* ثابت exprp) {
            إذا (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                أرجع خطأ;
            }

            ثابت AstVarRef* ثابت varRefp = VN_CAST(exprp, VarRef);
            إذا (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                أرجع خطأ;
            }
            AstVar* ثابت varp = varRefp->varp();
            إذا (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                أرجع خطأ;
            }
            إذا (m_aliasedVars.پیدا_کړه(varp) != m_aliasedVars.پای_ټکی()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                أرجع خطأ;
            } وإلا {
                m_aliasedVars.ورننباسه(varp);
            }
            أرجع ریښتیا;
        };

        check_If_بیان_Ok(nodep->itemsp());
        Ast_غوټه_DType* ثابت لومړی_توکی_Dtypep = nodep->itemsp()->dtypep();
        لكل (Ast_غوټه* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_بیان_Ok(VN_AS(itemp, غوټه_بیان));
            إذا (!لومړی_توکی_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << لومړی_توکی_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    فراغ لیدنه(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ثابت AstBasicDType* ثابت basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            إذا (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->پیدا_کړه_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ لیدنه(Ast_ټوټه_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ثابت Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_غوټه_DType* ثابت newDtp
                = جديد AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          جديد AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newDtp);
            nodep->dtypeFrom(newDtp);

            إذا (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ثابت صحيح subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ثابت VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                إذا ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(جديد AstConsArray{
                        nodep->fileline(), newDtp,
                        جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}}});
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                } وإلا إذا ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            إذا (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ثابت fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ثابت newp = جديد AstConsArray{nodep->fileline(), newDtp};
                ثابت VNumRange range = nodep->declRange();
                لكل (صحيح n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_ونه(خطأ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }

    فراغ لیدنه(Ast_Sel_بټ* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->bitp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->thsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->attrp(), پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه* ثابت selp = V3_پلنوالی::پلنوالی_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    فراغ لیدنه(Ast_Sel_ایستل* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->leftp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->rightp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->attrp(), پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه* ثابت selp = V3_پلنوالی::پلنوالی_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = تش_اشاره;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    فراغ لیدنه(AstSelPlus* nodep) override {
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->bitp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->widthp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->attrp(), پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه* ثابت selp = V3_پلنوالی::پلنوالی_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = تش_اشاره;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    فراغ لیدنه(AstSelMinus* nodep) override {
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->bitp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->widthp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بل(nodep->attrp(), پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه* ثابت selp = V3_پلنوالی::پلنوالی_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = تش_اشاره;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    فراغ لیدنه(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        إذا (m_vup && m_vup->prelim()) {
            إذا (VN_IS(nodep->dtypep()->skip_Ref_منزل_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } وإلا إذا (nodep->عدد().isNull()) {
                nodep->dtypep(nodep->پیدا_کړه_Null_DType());
            } وإلا إذا (nodep->عدد().isString()) {
                nodep->dtype_سېټ_String();
            } وإلا إذا (nodep->عدد().isDouble()) {
                nodep->dtype_Chg_پلنوالی(nodep->عدد().پلنوالی(), nodep->عدد().sized()
                                                               ? nodep->عدد().پلنوالی()
                                                               : nodep->عدد().پلنوالی_منزل_Fit());
            } وإلا {
                nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                    nodep->عدد().پلنوالی(),
                    nodep->عدد().sized() ? nodep->عدد().پلنوالی() : nodep->عدد().پلنوالی_منزل_Fit(),
                    nodep->dtypep()->numeric(), nodep->عدد().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    فراغ لیدنه(Ast_تشه_کتار* nodep) override {
        nodep->dtype_سېټ_تشه_کتار();
        إذا (!VN_IS(nodep->backp(), ټاکل) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    فراغ لیدنه(AstFell* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            userIterate(nodep->sentreep(), تش_اشاره);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstFalling* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstFuture* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            userIterate(nodep->sentreep(), تش_اشاره);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    فراغ لیدنه(AstPast* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            nodep->dtypeFrom(nodep->exprp());
            إذا (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ ریښتیا);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ثابت AstConst* ثابت constp = VN_CAST(nodep->ticksp(), Const);
                إذا (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_شا()->delete_ونه();
                } وإلا إذا (constp->منزل_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_شا()->delete_ونه();
                } وإلا إذا (V3AssertCommon::check_Sva_ارزښت_حد(nodep, constp, "$past tick value")) {
                    إذا (constp->منزل_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->منزل_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), تش_اشاره);
        }
    }
    فراغ لیدنه(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->countp(), پلنوالی_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
            إذا (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (minConstp->منزل_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (!nodep->unbounded() && !nodep->maxCountp() && minConstp->منزل_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            إذا (nodep->maxCountp()) {
                user_Iterate_And_بل(nodep->maxCountp(), پلنوالی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
                إذا (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (minConstp && maxConstp->منزل_SInt() < minConstp->منزل_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (maxConstp->منزل_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->propp(), پلنوالی_VP{SELF, BOTH}.p());
            Ast_غوټه_بیان* propp = nodep->propp();
            إذا (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_بل(nodep->loBoundp(), پلنوالی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            إذا (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_بل(nodep->hiBoundp(), پلنوالی_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ثابت منطقي loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ثابت منطقي hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            إذا (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_غوټه* ثابت boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_سېټ_بټ();
                أرجع;
            }
            إذا (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_سېټ_بټ();
                أرجع;
            }
            ثابت AstConst* ثابت loConstp = VN_CAST(nodep->loBoundp(), Const);
            ثابت AstConst* ثابت hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            إذا (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            إذا (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            إذا (loConstp && loConstp->منزل_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } وإلا إذا (loConstp) {
                V3AssertCommon::check_Sva_ارزښت_حد(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_ارزښت_حد(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && loConstp && hiConstp && hiConstp->منزل_SInt() < loConstp->منزل_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            منطقي hasPropertyOp = propp->is_Multi_دوره_Sva();
            إذا (!hasPropertyOp) {
                propp->foreach([&](ثابت Ast_غوټه* np) {
                    إذا (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = ریښتیا;
                    }
                });
            }
            إذا (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_پلنوالی_Reduce(propp), propp);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstRising* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstRose* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            userIterate(nodep->sentreep(), تش_اشاره);
            nodep->dtype_سېټ_بټ();
        }
    }

    فراغ لیدنه(AstSampled* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    فراغ لیدنه(AstSetuphold* nodep) override {
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_ټاکل_W* newp = تش_اشاره;
        إذا (nodep->delrefp()) {
            newp = convert_Setuphold_منزل_ټاکل(flp, nodep->refevp(), nodep->delrefp());
        }
        إذا (nodep->deldatap()) {
            إذا (!newp) {
                newp = convert_Setuphold_منزل_ټاکل(flp, nodep->dataevp(), nodep->deldatap());
            } وإلا {
                newp->جمع_کړه_بل_دلته(
                    convert_Setuphold_منزل_ټاکل(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        إذا (!newp) {
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        nodep->replaceWith(جديد AstAlways{newp});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    Ast_ټاکل_W* convert_Setuphold_منزل_ټاکل(File_کرښه* ثابت flp, Ast_غوټه_بیان* ثابت evp,
                                         Ast_غوټه_بیان* ثابت delp) {
        Ast_غوټه_بیان* ثابت lhsp = delp->clone_ونه_خالص(خطأ);
        Ast_غوټه_بیان* ثابت rhsp = evp->clone_ونه_خالص(خطأ);
        UASSERT_OBJ(VN_IS(lhsp, غوټه_Var_Ref) || VN_IS(lhsp, غوټه_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        إذا (Ast_غوټه_Var_Ref* varRefp = VN_CAST(lhsp, غوټه_Var_Ref)) {
            إذا (varRefp->varp()->direction() == VDirection::ننوت) {
                VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ټېل_Deletep(rhsp), rhsp);
                أرجع تش_اشاره;
            }
            varRefp->access(VAccess::WRITE);
        }
        إذا (Ast_غوټه_Pre_Sel* selp = VN_CAST(lhsp, غوټه_Pre_Sel)) {
            إذا (Ast_غوټه_Var_Ref* varRefp = VN_CAST(selp->fromp(), غوټه_Var_Ref)) {
                إذا (varRefp->varp()->direction() == VDirection::ننوت) {
                    VL_DO_DANGLING(ټېل_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ټېل_Deletep(rhsp), rhsp);
                    أرجع تش_اشاره;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        أرجع جديد Ast_ټاکل_W{flp, lhsp, rhsp};
    }

    فراغ لیدنه(AstStable* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            userIterate(nodep->sentreep(), تش_اشاره);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstSteady* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ریښتیا);
            nodep->dtype_سېټ_بټ();
        }
    }

    فراغ لیدنه(Ast_Stmt_بیان* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{SELF, BOTH}.p());
    }

    فراغ لیدنه(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }

    فراغ لیدنه(AstUntil* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            إذا (nodep->lhsp()->پلنوالی() != 1) fix_پلنوالی_Reduce(nodep->lhsp());
            إذا (nodep->rhsp()->پلنوالی() != 1) fix_پلنوالی_Reduce(nodep->rhsp());
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstAbortOn* nodep) override { لیدنه_Abort_Prop(nodep); }

    فراغ لیدنه(AstRand* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->urandom()) {
                nodep->dtype_سېټ_UInt32();  // Says the spec
            } وإلا {
                nodep->dtype_سېټ_Int();  // Says the spec
            }
            إذا (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    فراغ لیدنه(AstSEventually* nodep) override {
        إذا (v3Global.opt.timing().is_سېټ_False() || !v3Global.opt.timing().is_سېټ_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Widthed_ارزښت{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            أرجع;
        }
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstSGotoRep* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_بل(nodep->countp(), پلنوالی_VP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                پلنوالی_Check_Goto_Rep_Range(nodep, "Goto");
            } وإلا {
                پلنوالی_Check_Rep_شمېر(nodep);
            }
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstSNonConsRep* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_بل(nodep->countp(), پلنوالی_VP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                پلنوالی_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } وإلا {
                پلنوالی_Check_Rep_شمېر(nodep);
            }
            nodep->dtype_سېټ_بټ();
        }
    }
    کينډۍ <ډول_نوم T_Rep>
    فراغ پلنوالی_Check_Rep_شمېر(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->dtypep()->isSigned() && constp->عدد().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->عدد().most_سېټ_بټ_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } وإلا إذا (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    کينډۍ <ډول_نوم T_Rep>
    فراغ پلنوالی_Check_Goto_Rep_Range(T_Rep* nodep, ثابت حرف* ډول) {
        user_Iterate_And_بل(nodep->maxCountp(), پلنوالی_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
        ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
        إذا (!minConstp || !maxConstp) {
            nodep->v3error(std::string{ډول}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->منزل_SInt() < 0) {
            nodep->v3error(std::string{ډول}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (maxConstp->منزل_SInt() < minConstp->منزل_SInt()) {
            nodep->v3error(std::string{ډول}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + ډول
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    فراغ لیدنه(AstSThroughout* nodep) override {
        m_hasSExpr = ریښتیا;
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = خطأ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = ریښتیا;
            iterate(nodep->rhsp());
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ریښتیا;
        m_hasSExpr = ریښتیا;
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ریښتیا;
        m_hasSExpr = ریښتیا;
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            لكل (Ast_Sen_توکی* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_توکی)) {
                user_Iterate_And_بل(senp->sensp(), پلنوالی_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstURandomRange* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            nodep->dtype_سېټ_UInt32();  // Says the spec
            Ast_غوټه_DType* ثابت expDTypep = nodep->پیدا_کړه_UInt32_DType();
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    فراغ لیدنه(AstUnbounded* nodep) override {
        nodep->dtype_سېټ_Int();  // Used in int context
        إذا (VN_IS(nodep->backp(), IsUnbounded)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), BracketArrayDType)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), InsideRange)) أرجع;  // Ok, leave
        إذا (ثابت خپلکاری* ثابت varp = VN_CAST(nodep->backp(), Var)) {
            إذا (varp->isParam()) أرجع;  // Ok, leave
        }
        Ast_غوټه* backp = nodep->backp();
        إذا (VN_IS(backp, Sub)) backp = backp->backp();
        إذا (ثابت خپلکاری* ثابت selp = VN_CAST(backp, Sel_ایستل)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), کتار_DType)) أرجع;
        }
        إذا (ثابت خپلکاری* ثابت selp = VN_CAST(backp, Sel_بټ)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), کتار_DType)) أرجع;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    فراغ لیدنه(AstInferredDisable* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) nodep->dtype_سېټ_بټ();
    }
    فراغ لیدنه(AstIsUnbounded* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        إذا (m_vup && m_vup->prelim()) {
            nodep->dtype_سېټ_بټ_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        }
        إذا (m_vup->final()) {
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            إذا (nodep->پلنوالی() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    فراغ لیدنه(AstCLog2* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    کينډۍ <ډول_نوم T_Option>
    فراغ پلنوالی_Cover_وزن(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_غوټه_بیان* ثابت valuep = nodep->valuep();
        ثابت std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            إذا (!constp->عدد().isNegative()) أرجع;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->منزل_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } وإلا إذا (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } وإلا {
            أرجع;
        }
        // Continue with the default weight
        valuep->replaceWith(جديد AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    کينډۍ <ډول_نوم T_Option>
    فراغ پلنوالی_Cover_Int(T_Option* nodep) {
        user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    فراغ لیدنه(Ast_Cg_Option_ټاکل* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ثابت cgClassp = m_containingClassFinder.پیدا_کړه(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        إذا (nodep->optType() == VCoverOptionType::وزن) {
            پلنوالی_Cover_وزن(nodep);
            أرجع;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        إذا (nodep->optType() == VCoverOptionType::AT_لږترلږه) {
            پلنوالی_Cover_Int(nodep);
            أرجع;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        إذا (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            إذا (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->منزل_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->منزل_SInt() << کرښه_پای);
            } وإلا {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } وإلا إذا (nodep->optType() == VCoverOptionType::یوځای_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_غوټه_بیان* ثابت valuep = nodep->valuep();
            إذا (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(جديد AstConst{valuep->fileline(), AstConst::بټ_False{}});
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            }
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::DISTRIBUTE_لومړی) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ثابت AstConst* ثابت constp = VN_CAST(nodep->valuep(), Const);
            إذا (!constp || !constp->عدد().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }
    فراغ لیدنه(AstCoverOption* nodep) override {
        إذا (nodep->optType() == VCoverOptionType::وزن) {
            پلنوالی_Cover_وزن(nodep);
            أرجع;
        }
        إذا (nodep->optType() == VCoverOptionType::AT_لږترلږه
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            پلنوالی_Cover_Int(nodep);
            أرجع;
        }
        user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstCoverCross* nodep) override {
        user_Iterate_And_بل(nodep->itemsp(), تش_اشاره);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_بل(nodep->optionsp(), تش_اشاره);
        user_Iterate_And_بل(nodep->binsp(), تش_اشاره);
    }
    فراغ لیدنه(AstCoverCrossBin* nodep) override {
        user_Iterate_And_بل(nodep->selectp(), تش_اشاره);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    فراغ لیدنه(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_بل(nodep->binsp(), پلنوالی_VP{nodep->exprp()->dtypep(), BOTH}.p());
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_بل(nodep->optionsp(), تش_اشاره);
    }
    فراغ پلنوالی_Covergroup_Ranges(Ast_غوټه* rangesp, صحيح fill_پلنوالی) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ثابت خپلکاری fill = [&](Ast_غوټه* nodep) {
            إذا (!fill_پلنوالی) أرجع;
            Ast_غوټه_بیان* exprp = VN_AS(nodep, غوټه_بیان);
            fixAutoExtend(exprp /*ref*/, fill_پلنوالی);
        };
        لكل (Ast_غوټه *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (AstInsideRange* ثابت rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, تش_اشاره);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } وإلا إذا (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } وإلا {
                itemp = userIterateSubtreeReturnEdits(itemp, پلنوالی_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    فراغ لیدنه(AstCoverBinsof* nodep) override {
        user_Iterate_And_بل(nodep->pointp(), تش_اشاره);
        پلنوالی_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    فراغ لیدنه(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        پلنوالی_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->پلنوالی() : 0);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        إذا (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_بل(nodep->arraySizep(), پلنوالی_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_بل(nodep->transp(), m_vup);
    }
    فراغ لیدنه(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_بل(nodep->itemp(), تش_اشاره);
        إذا (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        إذا (!VN_IS(nodep->subp(), CoverpointRef)) {
            پلنوالی_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->پلنوالی());
        }
        پلنوالی_Cover_Filter(nodep->filterp(), "Bin");
    }
    فراغ لیدنه(AstCoverCrossWith* nodep) override {
        user_Iterate_And_بل(nodep->selectp(), تش_اشاره);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_بل(nodep->varsp(), تش_اشاره);
        پلنوالی_Cover_Filter(nodep->filterp(), "Cross bin");
        إذا (Ast_غوټه_بیان* ثابت countp = nodep->countp()) {
            إذا (VN_IS(countp, Unbounded)) {
                countp->dtype_سېټ_Int();  // 'matches $'
            } وإلا {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_بل(countp, پلنوالی_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    فراغ پلنوالی_Cover_Filter(Ast_غوټه_بیان* filterp, ثابت حرف* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, پلنوالی_VP{SELF, BOTH}.p()), غوټه_بیان);
        File_کرښه* ثابت fl = filterp->fileline();
        ثابت Ast_غوټه_DType* ثابت resultp = filterp->dtypep()->skipRefp();
        إذا (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_شا(&relinker);
            relinker.relink(
                جديد AstNeqD{fl, filterp, جديد AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } وإلا إذا (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(جديد AstConst{fl, AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(filterp), filterp);
        } وإلا {
            fix_پلنوالی_Reduce(filterp);
        }
    }
    فراغ لیدنه(Ast_Cover_Trans_سېټ* nodep) override { user_Iterate_And_بل(nodep->itemsp(), m_vup); }
    فراغ لیدنه(Ast_Cover_Trans_توکی* nodep) override {
        پلنوالی_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->پلنوالی() : 0);
    }
    فراغ لیدنه(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                أرجع;
            }
            checkCvtUS(nodep->lhsp(), خطأ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            إذا (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->عدد().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_سېټ_Logic_Unsized(nodep->پلنوالی(), nodep->پلنوالی_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        إذا (m_vup->final()) {
            Ast_غوټه_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_غوټه_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->پیدا_کړه_بټ_Or_Logic_DType(expDTypep->پلنوالی(), expDTypep->پلنوالی_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_غوټه* newp = تش_اشاره;  // No change
            إذا (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                    nodep->rhsp()->unlink_Fr_شا()};
            } وإلا إذا (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                    nodep->rhsp()->unlink_Fr_شا()};
            } وإلا إذا (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                    nodep->rhsp()->unlink_Fr_شا()};
            }
            إذا (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }
    فراغ لیدنه(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_شمېر_بټونه* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح پلنوالی_Min = V3_شمېره::log2b(nodep->lhsp()->پلنوالی()) + 1;
            nodep->dtype_سېټ_بټ_Unsized(32, پلنوالی_Min, VSigning::SIGNED);
        }
    }
    فراغ لیدنه(Ast_شمېر_Ones* nodep) override {
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح پلنوالی_Min = V3_شمېره::log2b(nodep->lhsp()->پلنوالی()) + 1;
            nodep->dtype_سېټ_بټ_Unsized(32, پلنوالی_Min, VSigning::SIGNED);
        }
    }
    فراغ لیدنه(AstCvtPackString* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH, current_بهیر_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        إذا (Ast_غوټه_DType* ثابت dt = nodep->lhsp()->dtypep()) {
            إذا (VN_IS(dt->skip_Ref_منزل_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    فراغ لیدنه(Ast_Cvt_Packed_منزل_Array* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH, current_بهیر_Use()}.p());
        // Type set in constructor
    }
    فراغ لیدنه(Ast_Cvt_Array_منزل_Array* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH, current_بهیر_Use()}.p());
        // Type set in constructor
    }
    فراغ لیدنه(Ast_Cvt_Array_منزل_Packed* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH, current_بهیر_Use()}.p());
        // Type set in constructor
    }
    فراغ لیدنه(Ast_Cvt_Unpack_منزل_کتار* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    فراغ لیدنه(Ast_Cvt_Unpack_منزل_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        إذا (m_vup->final()) {
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    فراغ لیدنه(Ast_وخت_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ثابت مزدوج وخت_Prescale = constp->عدد().منزل_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ثابت مزدوج وخت_2 = وخت_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::RealDouble{}, وخت_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstEventControl* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_سېټ_True()) {
                iterateChildren(nodep);
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_سېټ_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_نور()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->جمع_کړه_بل_دلته(nodep->stmtsp()->unlink_Fr_شا());
        VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ساكن string typenameExtends(ثابت AstClass* classp) {
        // An interface class may extend several, so those are not named
        إذا (!classp || classp->isInterfaceClass()) أرجع "";
        لكل (ثابت AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            إذا (extendsp->isImplements()) استمر;
            ثابت AstClass* ثابت basep = extendsp->classOrNullp();
            أرجع basep ? " extends class " + basep->dtypeName(ریښتیا) : "";
        }
        أرجع "";
    }
    فراغ لیدنه(Ast_Attr_د* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
        إذا (nodep->dimp()) user_Iterate_And_بل(nodep->dimp(), پلنوالی_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        بدل (nodep->attrType()) {
        حالة VAttrType::DIM_DIMENSIONS:
        حالة VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_غوټه_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ثابت std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(ریښتیا);
            ثابت صحيح val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.دوهم
                                                                       : (dim.لومړی + dim.دوهم));
            nodep->replaceWith(جديد AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            اكسر;
        }
        حالة VAttrType::DIM_بټونه_OR_شمېره: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            إذا (VN_IS(nodep->fromp(), غوټه_بیان)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_شا());
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            } وإلا {
                Ast_غوټه* newp = جديد Ast_Attr_د{nodep->fileline(), VAttrType::DIM_بټونه,
                                              nodep->fromp()->unlink_Fr_شا()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                user_Iterate_And_بل(newp, پلنوالی_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            أرجع;
        }
        حالة VAttrType::DIM_بټونه:
        حالة VAttrType::DIM_لوړ:
        حالة VAttrType::DIM_INCREMENT:
        حالة VAttrType::DIM_کیڼ:
        حالة VAttrType::DIM_ټیټ:
        حالة VAttrType::DIM_ښی:
        حالة VAttrType::DIM_اندازه: {
            Ast_غوټه_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            إذا (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ثابت Ast_غوټه_Module* ثابت ownModp = v3Global.rootp()->containingModule(dtypep);
                إذا (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_سېټ_Int();
                    أرجع;
                }
            }

            إذا (VN_IS(dtypep, کتار_DType) || VN_IS(dtypep, DynArrayDType)) {
                بدل (nodep->attrType()) {
                حالة VAttrType::DIM_اندازه: {
                    Ast_غوټه_بیان* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), غوټه_بیان);
                    Ast_غوټه* ثابت newp
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_اندازه};
                    newp->dtype_سېټ_Int();
                    newp->did_پلنوالی(ریښتیا);
                    newp->protect(خطأ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_کیڼ:
                حالة VAttrType::DIM_ټیټ: {
                    Ast_غوټه* ثابت newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_ښی:
                حالة VAttrType::DIM_لوړ: {
                    Ast_غوټه_بیان* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), غوټه_بیان);
                    Ast_غوټه_بیان* ثابت sizep
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_اندازه};
                    sizep->dtype_سېټ_Int();
                    sizep->did_پلنوالی(ریښتیا);
                    sizep->protect(خطأ);
                    Ast_غوټه* ثابت newp
                        = جديد AstSub{nodep->fileline(), sizep,
                                     جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_INCREMENT: {
                    Ast_غوټه_بیان* ثابت newp
                        = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_بټونه: {
                    إذا (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } وإلا {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    اكسر;
                }
                افتراضي: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } وإلا {
                ثابت std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(ریښتیا);
                ثابت uint32_t msbdim = dimpair.لومړی + dimpair.دوهم;
                إذا (!nodep->dimp() || msbdim < 1) {
                    إذا (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_غوټه_بیان* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), غوټه_بیان);
                        Ast_غوټه* ثابت newp = جديد Ast_ښويدنه_L{
                            nodep->fileline(), جديد AstLenN{nodep->fileline(), fromp},
                            جديد AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    } وإلا {
                        Ast_غوټه_DType* ثابت بنسټ_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(بنسټ_DTypep, nodep, "Unsized expression");
                        ثابت صحيح dim = 1;
                        AstConst* ثابت newp = dimension_ارزښت(nodep->fileline(), بنسټ_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    }
                } وإلا إذا (VN_IS(nodep->dimp(), Const)) {
                    ثابت صحيح dim = VN_AS(nodep->dimp(), Const)->منزل_SInt();
                    AstConst* ثابت newp
                        = dimension_ارزښت(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                } وإلا {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ثابت tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_غوټه_بیان* ثابت dimp = nodep->dimp()->unlink_Fr_شا();
                    Ast_غوټه_بیان* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                }
            }
            اكسر;
        }
        حالة VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            اكسر;
        حالة VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_غوټه_DType* ثابت protoDtp = nodep->fromp()->dtypep();
            Ast_غوټه_DType* ثابت declDtp = [&]() {
                إذا (m_ftaskp->fvarp()) أرجع m_ftaskp->fvarp()->dtypep();
                Ast_غوټه_DType* ثابت voidp = جديد AstVoidDType{m_ftaskp->fileline()};
                ټېل_Deletep(voidp);  // Note voidp used past here
                أرجع voidp;
            }();
            إذا (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_نور()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_نور()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            اكسر;
        }
        حالة VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_غوټه_DType* ثابت dtypep = nodep->fromp()->dtypep();
            string پایله = dtypep->prettyDTypeName(ریښتیا);
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                پایله += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << پایله << "' from " << dtypep);
            Ast_غوټه* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{}, پایله};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            اكسر;
        }
        حالة VAttrType::TYPEID:
            إذا (Ast_غوټه_DType* dtypep = VN_CAST(nodep->fromp(), غوټه_DType)) {
                nodep->dtypep(dtypep);
            } وإلا {
                nodep->dtypep(VN_AS(nodep->fromp(), غوټه_بیان)->dtypep());
            }
            اكسر;
        حالة VAttrType::VAR_بنسټ:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            اكسر;
        افتراضي: {
            // Everything else resolved earlier
            nodep->dtype_سېټ_بټ_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            اكسر;
        }
        }
    }
    فراغ لیدنه(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_متن* nodep) override {
        // Only used in CStmts which don't care....
    }

    فراغ لیدنه(AstCellArrayRef* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        user_Iterate_And_بل(nodep->selp(), پلنوالی_VP{SELF, PRELIM}.p());
        user_Iterate_And_بل(nodep->selp(), پلنوالی_VP{SELF, FINAL}.p());
        nodep->dtype_سېټ_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_پلنوالی(ریښتیا);
    }

    فراغ لیدنه(AstCellRef* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;

        إذا (Ast_غوټه_بیان* ثابت cellExprp = VN_CAST(nodep->cellp(), غوټه_بیان)) {
            user_Iterate_And_بل(cellExprp, پلنوالی_VP{SELF, PRELIM}.p());
            user_Iterate_And_بل(cellExprp, پلنوالی_VP{SELF, FINAL}.p());
        }

        إذا (Ast_غوټه_بیان* ثابت exprp = VN_CAST(nodep->exprp(), غوټه_بیان)) {
            user_Iterate_And_بل(exprp, پلنوالی_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_بل(exprp, پلنوالی_VP{SELF, FINAL}.p());
        } وإلا {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_پلنوالی(ریښتیا);
    }

    // DTYPES
    فراغ لیدنه(Ast_غوټه_Array_DType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        إذا (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ثابت basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            إذا (basicp->implicit()) {
                UASSERT_OBJ(basicp->پلنوالی() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->پلنوالی() << " bits");
                AstBasicDType* ثابت newp = جديد AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->پلنوالی_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        إذا (VN_IS(nodep, PackArrayDType)) m_underPackedArray = ریښتیا;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_بل(nodep->rangep(), پلنوالی_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->پلنوالی_From_Sub(nodep->subDTypep());
            إذا (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(ریښتیا);
        } وإلا {
            ثابت Ast_غوټه_DType* ثابت elemDTypep = nodep->subDTypep()->skipRefp();
            إذا (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ثابت صحيح پلنوالی = nodep->subDTypep()->پلنوالی() * nodep->rangep()->elementsConst();
            nodep->پلنوالی_Force(پلنوالی, پلنوالی);
            إذا (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ثابت Ast_غوټه_DType* بنسټ_DTypep = nodep->subDTypep();
                طالما (!VN_IS(بنسټ_DTypep, RefDType) && بنسټ_DTypep->subDTypep()) {
                    بنسټ_DTypep = بنسټ_DTypep->subDTypep();
                }
                ثابت AstBasicDType* ثابت basicp = nodep->basicp();
                إذا (!VN_IS(بنسټ_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(ریښتیا));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstAssocArrayDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->کلی_DTypep(iterateEditMoveDTypep(nodep, nodep->کلی_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_غوټه_DType* ثابت childp = nodep->ماشوم_DTypep();
        childp->unlink_Fr_شا();
        Ast_غوټه* ثابت elementsp = nodep->elementsp()->unlink_Fr_شا();
        Ast_غوټه* newp;
        إذا (VN_IS(elementsp, Unbounded)) {
            newp = جديد Ast_کتار_DType{nodep->fileline(), VFlag_ماشوم_DType{}, childp, تش_اشاره};
            VL_DO_DANGLING(elementsp->delete_ونه(), elementsp);
        } وإلا إذا (Ast_غوټه_DType* ثابت keyp = VN_CAST(elementsp, غوټه_DType)) {
            newp = جديد AstAssocArrayDType{nodep->fileline(), VFlag_ماشوم_DType{}, childp, keyp};
        } وإلا {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_کرښه* ثابت elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_کوډ::WIDTHEXPAND, ریښتیا);
            // Must be expression that is constant, but we'll determine that later
            newp = جديد AstUnpackArrayDType{
                nodep->fileline(), VFlag_ماشوم_DType{}, childp,
                جديد AstRange{nodep->fileline(), جديد AstConst(elementsp->fileline(), 0),
                             جديد AstSub{elementsNewFl, VN_AS(elementsp, غوټه_بیان),
                                        جديد AstConst(elementsNewFl, 1)},
                             ریښتیا}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, تش_اشاره), newp);
    }
    فراغ لیدنه(AstConstraintRefDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    فراغ لیدنه(AstDynArrayDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(Ast_کتار_DType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_بل(nodep->boundp(), پلنوالی_VP{SELF, BOTH}.p());
        إذا (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_شا()->delete_ونه();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstVoidDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstUnsizedArrayDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->ماشوم_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstWildcardArrayDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstBasicDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->generic()) أرجع;  // Already perfect
        إذا (nodep->rangep()) {
            user_Iterate_And_بل(nodep->rangep(), پلنوالی_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->پلنوالی_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } وإلا إذا (nodep->isRanged()) {
            nodep->پلنوالی_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } وإلا إذا (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->پلنوالی_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstConstDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, تش_اشاره);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->پلنوالی_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(AstRefDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_پلنوالی(ریښتیا);
        إذا (nodep->typeofp()) {  // type(typeofp_expression)
            إذا (Ast_غوټه_DType* typeofDtp = VN_CAST(nodep->typeofp(), غوټه_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } وإلا {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_بل(nodep->typeofp(), پلنوالی_VP{SELF, BOTH}.p());
                Ast_غوټه* ثابت typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_شا()->delete_ونه(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, تش_اشاره);
        إذا (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), تش_اشاره);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), تش_اشاره);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        إذا (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ثابت منطقي دننه_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            إذا (دننه_Template_Module) {
                nodep->doing_پلنوالی(خطأ);
                أرجع;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->پلنوالی_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_پلنوالی(خطأ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    فراغ پلنوالی_DType_Name_Params(ثابت AstTypedef* nodep) {
        ثابت VDTypeNameScopes* ثابت scopesp = VDTypeNameScopes::currentp();
        إذا (!scopesp) أرجع;  // Computing parameters, as by V3Param, so naming as found
        Ast_غوټه* scopep = تش_اشاره;
        لكل (ثابت Ast_غوټه* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            إذا (ثابت std::ویکټور<Ast_غوټه*>* ثابت paramsp = scopesp->paramsp(scopep)) {
                لكل (Ast_غوټه* ثابت paramp : *paramsp) {
                    إذا (!paramp->did_پلنوالی()) userIterate(paramp, تش_اشاره);
                }
            }
        }
    }
    فراغ لیدنه(AstTypedef* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (خپلکاری* ثابت refp = check_Ref_منزل_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(تش_اشاره);
            VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
            أرجع;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ثابت منطقي declares = nodep->ماشوم_DTypep() != تش_اشاره;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (declares) {
            Ast_غوټه_DType* ثابت dtypep = nodep->dtypep();
            إذا (Ast_غوټه_UOr_Struct_DType* ثابت sdtypep = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
                پلنوالی_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } وإلا إذا (AstEnumDType* ثابت edtypep = VN_CAST(dtypep, EnumDType)) {
                پلنوالی_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, تش_اشاره);
    }
    فراغ لیدنه(AstParamTypeDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, تش_اشاره);
        nodep->پلنوالی_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        إذا (nodep->dtypep() && nodep->ماشوم_DTypep()) { nodep->ماشوم_DTypep(تش_اشاره); }
    }
    فراغ لیدنه(AstRequireDType* nodep) override {
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        إذا (Ast_غوټه_DType* ثابت dtp = VN_CAST(nodep->lhsp(), غوټه_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_شا());
        } وإلا {
            إذا (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(جديد AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه_DType* ثابت منزل_Dtp = nodep->سر()->dtypep()->skip_Ref_منزل_Enump();
        Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_منزل_Enump();
        File_کرښه* ثابت fl = nodep->fileline();
        ثابت خپلکاری castable = Ast_غوټه::computeCastable(منزل_Dtp, fromDtp, nodep->fromp());
        Ast_غوټه* newp;
        إذا (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            أرجع;
        } وإلا إذا (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ثابت enumDtp = VN_AS(منزل_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_غوټه_بیان* ثابت testp = enumTestValid(nodep->fromp(), enumDtp);
            File_کرښه* ثابت fl_novalue = جديد File_کرښه{fl};
            fl_novalue->warnOff(V3_Error_کوډ::ENUMVALUE, ریښتیا);
            newp = جديد AstCond{
                fl, testp,
                جديد Ast_بیان_Stmt{fl,
                                جديد Ast_ټاکل{fl_novalue, nodep->سر()->unlink_Fr_شا(),
                                              nodep->fromp()->unlink_Fr_شا()},
                                جديد AstConst{fl, AstConst::Signed32{}, 1}},
                جديد AstConst{fl, AstConst::Signed32{}, 0}};
        } وإلا إذا ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(منزل_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << منزل_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_نور() << "... Suggest static cast");
            newp = جديد Ast_بیان_Stmt{
                fl,
                جديد Ast_ټاکل{fl, nodep->سر()->unlink_Fr_شا(),
                              جديد AstCast{fl, nodep->fromp()->unlink_Fr_شا(), منزل_Dtp}},
                جديد AstConst{fl, AstConst::Signed32{}, 1}};
        } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << منزل_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } وإلا {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << منزل_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_نور()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    فراغ لیدنه(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        إذا (AstConst* ثابت constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_شا();
            Ast_غوټه* ثابت newp
                = جديد Ast_Cast_اندازه{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا إذا (Ast_غوټه_DType* ثابت refp = VN_CAST(nodep->dtp(), غوټه_DType)) {
            refp->unlink_Fr_شا();
            Ast_غوټه* ثابت newp = جديد AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                              VFlag_ماشوم_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_شا());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(AstCast* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            إذا (AstSigned* ثابت fromp = VN_CAST(nodep->fromp(), Signed)) {
                إذا (VN_IS(fromp->lhsp(), غوټه_بهیر)) {
                    Ast_غوټه* ثابت lhsp = fromp->lhsp()->unlink_Fr_شا();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_ونه(), fromp);
                }
            }
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, PRELIM, بهیر_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_غوټه_DType* ثابت منزل_Dtp = nodep->dtypep()->skip_Ref_منزل_Enump();
            Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_منزل_Enump();
            ثابت خپلکاری castable = Ast_غوټه::computeCastable(منزل_Dtp, fromDtp, nodep->fromp());
            منطقي bad = خطأ;
            إذا (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << منزل_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = ریښتیا;
            } وإلا إذا (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } وإلا إذا (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << منزل_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_نور() << "... Suggest dynamic $cast");
                bad = ریښتیا;
            } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << منزل_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = ریښتیا;
            } وإلا {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_غوټه_بیان* newp = تش_اشاره;
            إذا (bad) {
            } وإلا إذا (AstBasicDType* ثابت basicp = منزل_Dtp->basicp()) {
                إذا (!basicp->isString() && fromDtp->isString()) {
                    newp = جديد AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), منزل_Dtp};
                } وإلا إذا (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_غوټه_DType* ثابت origDTypep = nodep->dtypep();
                    إذا (!VN_IS(fromDtp, بهیر_DType)) {
                        ثابت صحيح پلنوالی = منزل_Dtp->پلنوالی();
                        castSized(nodep, nodep->fromp(), پلنوالی);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } وإلا {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 خطأ);
                }
                إذا (newp) {
                } وإلا إذا (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    إذا (nodep->fromp()->isSigned()) {
                        newp = جديد AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
                    } وإلا {
                        newp = جديد AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
                    }
                } وإلا إذا (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = جديد AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
                    newp->dtypep(basicp);
                } وإلا إذا (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = جديد AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
                } وإلا إذا (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = جديد AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
                } وإلا {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } وإلا إذا (VN_IS(منزل_Dtp, کتار_DType)) {
                إذا (VN_IS(fromDtp, BasicDType)) {
                    newp = جديد Ast_Cvt_Packed_منزل_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_شا(), منزل_Dtp};
                } وإلا إذا (VN_IS(fromDtp, کتار_DType) || VN_IS(fromDtp, بهیر_DType)) {
                    صحيح src_Element_بټونه = 1;
                    صحيح dst_Element_بټونه = 1;
                    إذا (Ast_غوټه_DType* ثابت elemDtp = fromDtp->subDTypep()) {
                        src_Element_بټونه = elemDtp->پلنوالی();
                    }
                    ثابت Ast_کتار_DType* ثابت dst_کتار_Dtp = VN_AS(منزل_Dtp, کتار_DType);
                    إذا (Ast_غوټه_DType* ثابت elemDtp = dst_کتار_Dtp->subDTypep()) {
                        dst_Element_بټونه = elemDtp->پلنوالی();
                    }
                    newp = جديد Ast_Cvt_Array_منزل_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_شا(),
                                                  منزل_Dtp,
                                                  خطأ,
                                                  1,
                                                  dst_Element_بټونه,
                                                  src_Element_بټونه};
                }
            } وإلا إذا (VN_IS(منزل_Dtp, ClassRefDType)) {
                // Can just remove cast
            } وإلا {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << منزل_Dtp->prettyDTypeNameQ());
            }
            إذا (!newp) newp = nodep->fromp()->unlink_Fr_شا();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        إذا (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, خطأ, بهیر_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_غوټه_بیان* ثابت underp = nodep->fromp()->unlink_Fr_شا();
            underp->dtypeFrom(nodep);
            underp->did_پلنوالی(ریښتیا);
            Ast_غوټه_بیان* ثابت newp = جديد AstCastWrap{nodep->fileline(), underp};
            newp->did_پلنوالی(ریښتیا);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(Ast_Cast_اندازه* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            صحيح پلنوالی = nodep->rhsp()->منزل_SInt();
            إذا (پلنوالی < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << پلنوالی);
                پلنوالی = 1;
            }
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM, بهیر_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), پلنوالی);  // lhsp may change
        }
        إذا (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_غوټه* ثابت underp = nodep->lhsp()->unlink_Fr_شا();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    فراغ لیدنه(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{nodep->dtypep(), BOTH, بهیر_USE_CAST}.p());
    }
    فراغ castSized(Ast_غوټه* nodep, Ast_غوټه* underp, صحيح پلنوالی) {
        ثابت AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        إذا (!underDtp) underDtp = underp->dtypep()->basicp();
        إذا (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->پیدا_کړه_بټ_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ثابت صحيح calc_پلنوالی = std::max(پلنوالی, underDtp->پلنوالی());
            Ast_غوټه_DType* ثابت calcDtp = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                calc_پلنوالی, calc_پلنوالی, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         خطأ, بهیر_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_غوټه_DType* ثابت بهر_Dtp = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                پلنوالی, پلنوالی, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(بهر_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            پلنوالی_Check_Sized(nodep, "Cast expr", VN_AS(underp, غوټه_بیان), بهر_Dtp, EXTEND_EXP,
                            خطأ);
            VL_DANGLING(underp);
        }
    }
    فراغ لیدنه(AstConstraintBefore* nodep) override {
        user_Iterate_And_بل(nodep->lhssp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->rhssp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_Constraint_بیان* nodep) override {
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstConstraintUnique* nodep) override {
        user_Iterate_And_بل(nodep->rangesp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_Dist_توکی* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), پلنوالی_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    فراغ لیدنه(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        إذا (nodep->did_پلنوالی()) أرجع;
        إذا (nodep->doing_پلنوالی()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ټېل_Deletep(nodep->valuep()->unlink_Fr_شا());
            nodep->valuep(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_پلنوالی(ریښتیا);
            أرجع;
        }
        nodep->doing_پلنوالی(ریښتیا);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        إذا (!m_paramsOnly && nodep->is_Iface_Array_وېشل()) m_وېشل_Iface_Array_Varps.ټېل_شا(nodep);
        إذا (nodep->attrsp()) {
            nodep->attrsp()->foreach([هذا, nodep](Ast_Attr_د* attrp) {
                إذا (attrp->attrType() == VAttrType::VAR_پورټ_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    إذا (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), غوټه_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ټېل_Deletep(attrp->unlink_Fr_شا()), attrp);
                }
            });
        }
        إذا (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_غوټه_DType* dtp = nodep->dtypep();
            Ast_غوټه_DType* np = تش_اشاره;
            طالما (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                إذا (ثابت AstDynArrayDType* ثابت unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    إذا (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } وإلا {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ثابت newp
                        = جديد AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    إذا (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } وإلا {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newp);
                    np = newp;
                } وإلا {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        إذا (AstWildcardArrayDType* ثابت wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        إذا (VN_IS(nodep->dtypep()->skip_Ref_منزل_Constp(), ConstDType)) nodep->isConst(ریښتیا);
        // Parameters if implicit untyped inherit from what they are assigned to
        ثابت AstBasicDType* ثابت bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        منطقي didchk = خطأ;
        ثابت منطقي implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        إذا (implicitParam) {
            إذا (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                إذا (nodep->valuep()->isDouble()) {
                    nodep->dtype_سېټ_Double();
                    VL_DANGLING(bdtypep);
                } وإلا إذا (nodep->valuep()->isString()) {
                    nodep->dtype_سېټ_String();
                    VL_DANGLING(bdtypep);
                } وإلا {
                    صحيح پلنوالی = 0;
                    Ast_غوټه_DType* ثابت ارزښت_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(ارزښت_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ثابت AstBasicDType* ثابت ارزښت_Bdtypep = ارزښت_DTypep->basicp();
                    منطقي issigned = خطأ;
                    إذا (bdtypep->isNosign()) {
                        إذا (ارزښت_Bdtypep && ارزښت_Bdtypep->isSigned()) issigned = ریښتیا;
                    } وإلا {
                        issigned = bdtypep->isSigned();
                    }
                    إذا (ارزښت_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        إذا (ثابت خپلکاری* ثابت constp = VN_CAST(nodep->valuep(), Const)) {
                            إذا (constp->عدد().isString()) {
                                پلنوالی = constp->عدد().منزل_String().اوږدتیا() * 8;
                            }
                        }
                        إذا (پلنوالی < 8) پلنوالی = 8;
                    } وإلا إذا (nodep->valuep()->dtypep()->پلنوالی_Sized()) {
                        پلنوالی = nodep->valuep()->پلنوالی();
                    } وإلا {
                        إذا (nodep->valuep()->پلنوالی() > 32) {
                            nodep->valuep()->v3warn(
                                پلنوالی_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        پلنوالی = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ثابت منطقي is_ارزښت_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    إذا (پلنوالی == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->پیدا_کړه_بټ_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->پلنوالی_Min(),
                            VSigning::fromBool(issigned), is_ارزښت_Fourstate));
                    } وإلا {
                        nodep->dtype_سېټ_بټ_Or_Logic_Unsized(پلنوالی, nodep->valuep()->پلنوالی_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_ارزښت_Fourstate);
                    }
                    didchk = ریښتیا;
                }
                iterate_Check_ټاکل(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } وإلا {
                // Or, if nothing assigned, they're integral
                nodep->dtype_سېټ_Integer();
                VL_DANGLING(bdtypep);
            }
        } وإلا إذا (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_سېټ_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        إذا (nodep->isNet()) {
            Ast_غوټه_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            إذا (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        إذا (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ټاکل(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_بل(nodep->delayp(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_پلنوالی(ریښتیا);
        nodep->doing_پلنوالی(خطأ);
    }
    فراغ لیدنه(Ast_غوټه_Var_Ref* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        إذا (!nodep->varp()) {
            إذا (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, خطأ,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                أرجع;
            } وإلا {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        إذا (!nodep->varp()->did_پلنوالی()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), تش_اشاره);
        }
        إذا (!m_paramsOnly && nodep->varp()->is_Iface_Array_وېشل()) {
            replace_وېشل_Iface_Array_Ref(nodep);
            أرجع;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        إذا (VN_IS(nodep->backp(), غوټه_ټاکل) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        إذا (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(ریښتیا);
        إذا (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } وإلا إذا (nodep->access().isWriteOrRW() && nodep->varp()->is_ننوت()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), ټاکل_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ثابت منطقي hierRef = !m_curModVars.شمېر(nodep->varp());
            ثابت منطقي net_پورټ
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::پورټ;
            ثابت منطقي cont_ټاکل = VN_IS(nodep->abovep(), ټاکل_W);
            ثابت منطقي pinConn = VN_IS(nodep->abovep(), Pin);
            إذا (!(hierRef && net_پورټ && (cont_ټاکل || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } وإلا إذا (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        إذا (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->did_پلنوالی(ریښتیا);
    }

    فراغ لیدنه(AstEnumDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_غوټه_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_غوټه_DType* ثابت badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        إذا (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->پیدا_کړه_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->پلنوالی_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_بل(nodep->itemsp(), پلنوالی_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_شمېره عدد(nodep, nodep->پلنوالی(), 0);
        ثابت V3_شمېره one{nodep, nodep->پلنوالی(), 1};
        منطقي wrapAround = خطأ;
        std::map<ثابت V3_شمېره, Ast_Enum_توکی*> inits;
        لكل (Ast_Enum_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
            إذا (itemp->valuep()) {
                إذا (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_ونه("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                إذا (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_شا()->delete_ونه();
                    استمر;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = خطأ;  // Explicit value resets wrap-around tracking
            }
            إذا (!itemp->valuep()) {
                إذا (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                إذا (عدد.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(جديد AstConst{itemp->fileline(), عدد});
            }

            ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
            إذا (constp->عدد().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            عدد.op_ټاکل(constp->عدد());
            // Look for duplicates
            ثابت خپلکاری pair = inits.emplace(عدد, itemp);
            إذا (!pair.دوهم) {  // IEEE says illegal
                ثابت Ast_غوټه* ثابت otherp = pair.لومړی->دوهم;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ثابت منطقي prevNeg = constp->عدد().isNegative();
            عدد.op_جمع_کړه(one, constp->عدد());
            إذا (basicp->isSigned()) {
                wrapAround = !prevNeg && عدد.isNegative();
            } وإلا {
                wrapAround = عدد.isEqZero();
            }
        }
    }
    فراغ لیدنه(Ast_Enum_توکی* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        إذا (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            منطقي warnOn = ریښتیا;
            Ast_غوټه_بیان* valuep = nodep->valuep();
            إذا (ثابت Ast_جمع_کړه* ثابت anodep = VN_CAST(valuep, جمع_کړه_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                إذا (anodep->fileline()->equal_لومړی_کرښه_ستون(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                إذا (static_cast<صحيح>(constp->عدد().most_سېټ_بټ_P1()) > nodep->پلنوالی()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = خطأ;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(جديد AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_شا(),
                                      nodep->dtypep()});
        }
    }
    فراغ لیدنه(Ast_Enum_توکی_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        إذا (!nodep->itemp()->did_پلنوالی()) {
            // We need to do the whole enum en masse
            Ast_غوټه* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            لكل (; enump; enump = enump->backp()) {
                إذا (VN_IS(enump, EnumDType)) اكسر;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    فراغ لیدنه(Ast_غوټه_Cons* nodep) override {}  // Widthed when created
    فراغ لیدنه(Ast_غوټه_Cons_توکی* nodep) override {}  // Widthed when created
    فراغ لیدنه(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_غوټه *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, پلنوالی_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_غوټه_DType* subDTypep = تش_اشاره;
        nodep->dtype_سېټ_بټ();

        إذا (dtype && dtype->isString()) {
            nodep->dtype_سېټ_String();
            subDTypep = nodep->پیدا_کړه_String_DType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtype_سېټ_Double();
            subDTypep = nodep->پیدا_کړه_Double_DType();
        } وإلا {
            // Take width as maximum across all items
            صحيح پلنوالی = nodep->exprp()->پلنوالی();
            صحيح mwidth = nodep->exprp()->پلنوالی_Min();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                پلنوالی = std::max(پلنوالی, itemp->پلنوالی());
                mwidth = std::max(mwidth, itemp->پلنوالی_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                پلنوالی, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        لكل (Ast_غوټه *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_توکی)->rangep();
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, پلنوالی_VP{subDTypep, FINAL}.p());
            } وإلا {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        إذا (m_constraintp) {
            منطقي can_ښکتنی = ریښتیا;
            لكل (Ast_Dist_توکی* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_توکی)) {
                إذا (VN_IS(ditemp->rangep(), InsideRange)) استمر;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ثابت Ast_غوټه_DType* ثابت توکی_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                إذا (!توکی_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_خالص()) {
                    can_ښکتنی = خطأ;
                    اكسر;
                }
            }
            إذا (can_ښکتنی) أرجع;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_غوټه_بیان* newp = تش_اشاره;
        لكل (Ast_Dist_توکی* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_توکی)) {
            Ast_غوټه_بیان* inewp
                = inside_توکی(nodep, nodep->exprp()->clone_ونه_خالص(خطأ), itemp->rangep());
            إذا (!inewp) استمر;
            Ast_غوټه_بیان* ثابت cmpp
                = جديد AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_شا(),
                            جديد AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_کوډ::UNSIGNED, ریښتیا);
            cmpp->fileline()->modifyWarnOff(V3_Error_کوډ::CMPCONST, ریښتیا);
            inewp = جديد AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    فراغ لیدنه(AstInside* nodep) override {
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_غوټه *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, پلنوالی_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_غوټه_DType* expDTypep = تش_اشاره;

        إذا (dtype && dtype->isString()) {
            nodep->dtype_سېټ_String();
            expDTypep = nodep->پیدا_کړه_String_DType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtype_سېټ_Double();
            expDTypep = nodep->پیدا_کړه_Double_DType();
        } وإلا {
            // Take width as maximum across all items
            صحيح پلنوالی = nodep->exprp()->پلنوالی();
            صحيح mwidth = nodep->exprp()->پلنوالی_Min();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_غوټه* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                پلنوالی = std::max(پلنوالی, itemp->پلنوالی());
                mwidth = std::max(mwidth, itemp->پلنوالی_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_سېټ_بټ();
            ثابت VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        لكل (Ast_غوټه *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, پلنوالی_VP{expDTypep, FINAL}.p());
            } وإلا إذا (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_غوټه_بیان* exprp;
        Ast_بیان_Stmt* بیان_Stmtp = تش_اشاره;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        إذا (!m_constraintp && !nodep->exprp()->is_خالص()) {
            File_کرښه* ثابت fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_غوټه_DType* ثابت بیان_Dtp = nodep->exprp()->dtypep();
            ثابت صحيح w = بیان_Dtp->پلنوالی();
            Ast_غوټه_DType* ثابت لنډمهاله_DTypep
                = بیان_Dtp->پلنوالی_Sized() ? بیان_Dtp
                                        : nodep->پیدا_کړه_بټ_Or_Logic_DType(w, w, بیان_Dtp->numeric(),
                                                                     بیان_Dtp->isFourstate());
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::XTEMP, m_inside_لنډمهاله_نومونه.get(nodep), لنډمهاله_DTypep};
            exprp = جديد AstVarRef{fl, varp, VAccess::READ};
            بیان_Stmtp = جديد Ast_بیان_Stmt{fl,
                                        جديد Ast_ټاکل{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_شا()},
                                        exprp};
            إذا (m_ftaskp) {
                varp->funcLocal(ریښتیا);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(بیان_Stmtp);
        } وإلا {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_غوټه_بیان* newp = تش_اشاره;
        لكل (Ast_غوټه_بیان *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), غوټه_بیان);  // Will be unlinking
            Ast_غوټه_بیان* inewp;
            إذا (بیان_Stmtp) {
                inewp = inside_توکی(nodep, بیان_Stmtp, itemp);
                بیان_Stmtp = تش_اشاره;
            } وإلا {
                inewp = inside_توکی(nodep, exprp->clone_ونه_خالص(خطأ), itemp);
            }
            إذا (!inewp) استمر;
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (بیان_Stmtp) VL_DO_DANGLING(بیان_Stmtp->delete_ونه(), بیان_Stmtp);
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    Ast_غوټه_بیان* inside_توکی(Ast_غوټه* nodep, Ast_غوټه_بیان* exprp, Ast_غوټه_بیان* itemp) {
        ثابت Ast_غوټه_DType* ثابت توکی_Dtp = itemp->dtypep()->skipRefp();
        إذا (AstInsideRange* ثابت irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            أرجع irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_شا(),
                                             irangep->rhsp()->unlink_Fr_شا());
        } وإلا إذا (VN_IS(توکی_Dtp, UnpackArrayDType) || VN_IS(توکی_Dtp, DynArrayDType)
                   || VN_IS(توکی_Dtp, کتار_DType)) {
            // Unsupported in parameters
            Ast_غوټه_بیان* ثابت inewp = جديد AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_شا(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, توکی_Dtp->subDTypep(), BOTH);
            inewp->dtype_سېټ_بټ();
            inewp->did_پلنوالی(ریښتیا);
            أرجع inewp;
        } وإلا إذا (VN_IS(توکی_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_ونه(), exprp);
            أرجع تش_اشاره;
        }
        أرجع AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_شا());
    }
    فراغ لیدنه(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        إذا (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } وإلا {
            إذا (m_vup->prelim()) {
                user_Iterate_And_بل(nodep->lhsp(), m_vup);
                user_Iterate_And_بل(nodep->rhsp(), m_vup);
            }
            إذا (m_vup->final()) {
                Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ثابت صحيح exp_پلنوالی = expDTypep->پلنوالی();
                ثابت منطقي waiveLhs = exp_پلنوالی == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->پلنوالی_Min() >= nodep->lhsp()->پلنوالی();
                ثابت منطقي waiveRhs = exp_پلنوالی == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_پلنوالی >= nodep->rhsp()->پلنوالی_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    فراغ لیدنه(AstIfaceRefDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        إذا (nodep->isVirtual()) nodep->iface_له_لارې_Cellp()->سېټ_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ لیدنه(Ast_غوټه_UOr_Struct_DType* nodep) override {
        إذا (nodep->doing_پلنوالی()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_سېټ_بټ();
            nodep->doing_پلنوالی(خطأ);
            أرجع;
        }
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_پلنوالی(ریښتیا);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        إذا (ثابت AstUnionDType* ثابت unionp = VN_CAST(nodep, UnionDType)) {
            إذا (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, تش_اشاره);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(خطأ);
        // Error checks
        لكل (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_غوټه_DType* ثابت dtp = itemp->subDTypep()->skipRefp();
            إذا (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            إذا ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ټېل_Deletep(itemp->valuep()->unlink_Fr_شا());
            }
        }
        ثابت منطقي isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ثابت منطقي دننه_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        إذا (VN_IS(nodep, UnionDType) || nodep->packed()) {
            صحيح lsb = 0;
            صحيح پلنوالی = 0;
            منطقي لومړی = ریښتیا;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            لكل (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            لكل (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                إذا (itemp->isFourstate()) nodep->isFourstate(ریښتیا);
                itemp->lsb(lsb);
                إذا (VN_IS(nodep, UnionDType)) {
                    ثابت صحيح توکی_پلنوالی = itemp->پلنوالی();
                    // Skip union size check for template modules with unresolved parameters
                    إذا (!لومړی && isHardPackedUnion && توکی_پلنوالی != پلنوالی && !دننه_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    پلنوالی = std::max(پلنوالی, توکی_پلنوالی);
                } وإلا {
                    lsb += itemp->پلنوالی();
                    پلنوالی += itemp->پلنوالی();
                }
                لومړی = خطأ;
            }
            nodep->پلنوالی_Force(پلنوالی, پلنوالی);  // Signing stays as-is, as parsed from declaration
        } وإلا {
            nodep->پلنوالی_Force(1, 1);
        }
        nodep->doing_پلنوالی(خطأ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    فراغ لیدنه(AstThisRef* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ماشوم_DTypep()));
        إذا (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    فراغ لیدنه(AstClassRefDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    فراغ لیدنه(AstClassOrPackageRef* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        userIterateChildren(nodep, تش_اشاره);
    }
    فراغ لیدنه(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_نور() << "... Suggest use a typedef");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    فراغ لیدنه(AstClassExtends* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        إذا (VN_IS(nodep->ماشوم_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ماشوم_DTypep()));
        }
    }
    فراغ لیدنه(AstMemberDType* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->پلنوالی_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        إذا (nodep->valuep()) {
            user_Iterate_And_بل(nodep->valuep(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ټاکل(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    فراغ لیدنه(AstStructSel* nodep) override {
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        إذا (nodep->did_پلنوالی()) أرجع;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = ریښتیا;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_منزل_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ثابت AstMemberSel* ثابت fromSel = VN_CAST(nodep->fromp(), MemberSel);
        إذا (AstClocking* ثابت clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->لومړی_Abovep(), Clocking)
                                               : تش_اشاره) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            إذا (AstVar* ثابت varp = memberSelClocking(nodep, clockingp)) {
                إذا (!varp->did_پلنوالی()) userIterate(varp, تش_اشاره);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_شا());
                VL_DO_DANGLING(fromp->delete_ونه(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                إذا (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_سېټ(nodep, nodep->access());
                }
                إذا (AstIfaceRefDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_پلنوالی(ریښتیا);  // Must not visit again: will confuse scopes
                أرجع;
            }
        } وإلا إذا (Ast_غوټه_UOr_Struct_DType* ثابت adtypep = VN_CAST(fromDtp, غوټه_UOr_Struct_DType)) {
            إذا (memberSelStruct(nodep, adtypep)) أرجع;
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            إذا (memberSelClass(nodep, adtypep)) أرجع;
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            إذا (Ast_غوټه* foundp = memberSelIface(nodep, adtypep)) {
                إذا (AstClocking* ثابت clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->did_پلنوالی()) userIterate(varp, تش_اشاره);
                    AstIface* ثابت ifacep = adtypep->iface_له_لارې_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ثابت AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
                    إذا (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ثابت newp = جديد AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_پلنوالی(ریښتیا);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                        أرجع;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_پلنوالی(ریښتیا);
                    أرجع;
                }
                إذا (AstModport* ثابت modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ثابت newDtypep = جديد AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newDtypep);
                    Ast_غوټه_بیان* ثابت fromp = nodep->fromp()->unlink_Fr_شا();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (AstCell* ثابت cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    إذا (VN_IS(cellp->modp(), Iface)) {
                        ثابت string viftopName = cellp->name() + "__Viftop";
                        Ast_غوټه_Module* ثابت پلار_Ifacep = adtypep->iface_له_لارې_Cellp();
                        AstVar* viftopVarp = تش_اشاره;
                        لكل (Ast_غوټه* itemp = پلار_Ifacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            إذا (AstVar* ثابت vp = VN_CAST(itemp, Var)) {
                                إذا (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    اكسر;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        إذا (!viftopVarp->did_پلنوالی()) userIterate(viftopVarp, تش_اشاره);
                        Ast_غوټه_DType* subDtypep = viftopVarp->dtypep();
                        إذا (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            إذا (AstIfaceRefDType* ثابت subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ثابت newDtypep = جديد AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(ریښتیا);
                                v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_پلنوالی(ریښتیا);
                        أرجع;
                    }
                }
                إذا (Ast_غوټه_FTask* ثابت ftaskp = VN_CAST(foundp, غوټه_FTask)) {
                    Ast_لاره_Call* ثابت newp = جديد Ast_لاره_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    أرجع;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } وإلا إذا (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, کتار_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_غوټه* ثابت newp = جديد Ast_لاره_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_شا(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        } وإلا {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }
    منطقي memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        إذا (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_لاره_Call* ثابت newp = جديد Ast_لاره_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            لیدنه(newp);
            أرجع ریښتیا;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت لومړی_classp = adtypep->classp();
        UASSERT_OBJ(لومړی_classp, nodep, "Unlinked");
        لكل (AstClass* classp = لومړی_classp; classp;) {
            إذا (Ast_غوټه* ثابت foundp = m_memberMap.پیدا_کړه_Member(classp, nodep->name())) {
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->did_پلنوالی()) userIterate(varp, تش_اشاره);
                    إذا (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ثابت varRefp
                            = جديد AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                        أرجع ریښتیا;
                    }
                    إذا (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_پلنوالی(ریښتیا);
                    إذا (nodep->fromp()->same_ونه(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_سېټ(nodep);
                    أرجع ریښتیا;
                }
                إذا (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(جديد AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), constrp});
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع ریښتیا;
                }
                إذا (Ast_Enum_توکی_Ref* ثابت adfoundp = VN_CAST(foundp, Enum_توکی_Ref)) {
                    nodep->replaceWith(adfoundp->clone_ونه(خطأ));
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع ریښتیا;
                }
                إذا (Ast_غوټه_FTask* ftaskp = VN_CAST(foundp, غوټه_FTask)) {
                    Ast_لاره_Call* newp = جديد Ast_لاره_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع ریښتیا;
                }
                إذا (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->پیدا_کړه_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    أرجع ریښتیا;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره;
        }

        VSpellCheck speller;
        لكل (AstClass* classp = لومړی_classp; classp;) {
            لكل (Ast_غوټه* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_توکی_Ref) || VN_IS(itemp, Var)) {
                    speller.ټېل_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره;
        }
        ثابت string suggest = speller.غوره_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << لومړی_classp->verilogKwd()
                      << " " << لومړی_classp->prettyNameQ() << "\n"
                      << (suggest.تشه() ? "" : nodep->fileline()->warn_نور() + suggest));
        أرجع خطأ;  // Caller handles error
    }
    منطقي modport_Allows_توکی(Ast_غوټه* nodep, ثابت AstIfaceRefDType* adtypep, ثابت Ast_غوټه* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ثابت AstModport* ثابت modportp = adtypep->modportp();
        إذا (!modportp) أرجع ریښتیا;
        إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) {
            إذا (varp->isParam()) أرجع ریښتیا;
        } وإلا إذا (!VN_IS(itemp, غوټه_FTask) && !VN_IS(itemp, Clocking)) {
            أرجع ریښتیا;
        }
        خپلکاری pair = m_modport_نومونه.emplace(modportp, std::unordered_سېټ<string>{});
        إذا (pair.دوهم) {
            لكل (ثابت Ast_غوټه* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.لومړی->دوهم.ورننباسه(mpItemp->name());
            }
        }
        إذا (pair.لومړی->دوهم.شمېر(itemp->name())) أرجع ریښتیا;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_له_لارې_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        أرجع خطأ;
    }
    Ast_غوټه* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_غوټه_Module* ثابت ifacep = adtypep->iface_له_لارې_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        لكل (Ast_غوټه* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp->name() == nodep->name()) {
                إذا (!modport_Allows_توکی(nodep, adtypep, itemp)) {
                    أرجع تش_اشاره;  // Caller handles error
                }
                أرجع itemp;
            }
            إذا (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ټېل_Candidate(itemp->prettyName());
            }
        }
        ثابت string suggest = speller.غوره_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.تشه() ? "" : nodep->fileline()->warn_نور() + suggest));
        أرجع تش_اشاره;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        لكل (Ast_غوټه* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_توکی* citemp = VN_CAST(itemp, Clocking_توکی)) {
                إذا (citemp->varp()->name() == nodep->name()) أرجع citemp->varp();
                speller.ټېل_Candidate(citemp->varp()->prettyName());
            }
        }
        ثابت string suggest = speller.غوره_Candidate_پیغام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.تشه() ? "" : nodep->fileline()->warn_نور() + suggest));
        أرجع تش_اشاره;  // Caller handles error
    }
    منطقي memberSelStruct(AstMemberSel* nodep, Ast_غوټه_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        إذا (AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.پیدا_کړه_Member(adtypep, nodep->name()), MemberDType)) {
            إذا (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } وإلا إذا (adtypep->packed()) {
                AstSel* ثابت newp = جديد AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                                memberp->lsb(), memberp->پلنوالی()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_منزل_Enump());
                newp->did_پلنوالی(ریښتیا);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } وإلا {
                AstStructSel* ثابت newp = جديد AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_منزل_Enump());
                newp->did_پلنوالی(ریښتیا);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            أرجع ریښتیا;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        أرجع خطأ;
    }

    فراغ لیدنه(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    فراغ لیدنه(Ast_لاره_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        إذا (nodep->did_پلنوالی()) أرجع;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_غوټه_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_منزل_Enump();
        AstBasicDType* ثابت basicp = fromDtp ? fromDtp->basicp() : تش_اشاره;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, پلنوالی_VP{SELF, BOTH}.p());
        إذا (nodep->name() == "rand_mode") {
            لاره_Call_Rand_ډېرپېښ(nodep);
        } وإلا إذا (nodep->name() == "constraint_mode") {
            لاره_Call_Constraint(nodep, تش_اشاره);
        } وإلا إذا (AstEnumDType* ثابت adtypep = VN_CAST(fromDtp, EnumDType)) {
            لاره_Call_Enum(nodep, adtypep);
        } وإلا إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            لاره_Call_Assoc(nodep, adtypep);
        } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            لاره_Call_Wildcard(nodep, adtypep);
        } وإلا إذا (AstDynArrayDType* ثابت adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            لاره_Call_Dyn(nodep, adtypep);
        } وإلا إذا (Ast_کتار_DType* ثابت adtypep = VN_CAST(fromDtp, کتار_DType)) {
            لاره_Call_کتار(nodep, adtypep);
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            لاره_Call_Class(nodep, adtypep);
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            لاره_Call_Iface_Ref(nodep, adtypep);
        } وإلا إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            لاره_Call_Unpack(nodep, adtypep);
        } وإلا إذا (AstConstraintRefDType* ثابت adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            لاره_Call_Constraint(nodep, adtypep);
        } وإلا إذا (basicp && basicp->isEvent()) {
            لاره_Call_Event(nodep, basicp);
        } وإلا إذا (basicp && basicp->isString()) {
            لاره_Call_String(nodep, basicp);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_سېټ_Void();
        }
    }
    AstWith* لاره_With_Clause(Ast_غوټه_FTask_Ref* nodep, منطقي required, منطقي arbReturn,
                              Ast_غوټه_DType* returnDtp, Ast_غوټه_DType* شاخص_Dtp,
                              Ast_غوټه_DType* ارزښت_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->شاخص_Arg_Refp()->dtypep(شاخص_Dtp);
            withp->ارزښت_Arg_Refp()->dtypep(ارزښت_Dtp);
            userIterate(withp, پلنوالی_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_شا();
            أرجع withp;
        }
        إذا (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        أرجع تش_اشاره;
    }
    فراغ لاره_Ok_Arguments(Ast_غوټه_FTask_Ref* nodep, صحيح minArg, صحيح maxArg) {
        صحيح narg = 0;
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ټېل_Deletep(withp->unlink_Fr_شا()), withp);
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ثابت منطقي ok = (narg >= minArg) && (narg <= maxArg);
        إذا (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_منزل_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_منزل_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            لكل (; narg < minArg; ++narg) {
                nodep->جمع_کړه_Argsp(
                    جديد AstArg{nodep->fileline(), "",
                               جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}}});
            }
            لكل (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                طالما (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_شا();
                VL_DO_DANGLING(argp->delete_ونه(), argp);
            }
        }
    }

    Ast_غوټه_بیان* لاره_Arg(Ast_لاره_Call* nodep, صحيح arg) {
        AstArg* argp = nodep->argsp();
        لكل (صحيح narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        أرجع argp->exprp();
    }

    فراغ لاره_Call_Enum(Ast_لاره_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        إذا (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = تش_اشاره;
            لاره_Ok_Arguments(nodep, 0, 0);
            إذا (nodep->name() == "num") {
                صحيح توکي = 0;
                لكل (Ast_غوټه* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++توکي;
                newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, توکي);
            } وإلا إذا (nodep->name() == "first") {
                ثابت Ast_Enum_توکی* itemp = adtypep->itemsp();
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_ونه(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } وإلا إذا (nodep->name() == "last") {
                ثابت Ast_Enum_توکی* itemp = adtypep->itemsp();
                طالما (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_توکی);
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_ونه(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            إذا (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                لاره_Ok_Arguments(nodep, 0, 0);
            } وإلا إذا (nodep->name() == "next") {
                attrType = VAttrType::ENUM_بل;
                لاره_Ok_Arguments(nodep, 0, 1);
            } وإلا إذا (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                لاره_Ok_Arguments(nodep, 0, 1);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }

            إذا (nodep->name() != "name" && nodep->argsp()) {
                Ast_غوټه_بیان* stepp = لاره_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = لاره_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_کوډ::WIDTHEXPAND, ریښتیا);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                إذا (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_غوټه_بیان* ثابت newp = جديد AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ثابت uint32_t ګام_پلنوالی = VN_AS(stepp, Const)->منزل_UInt();
                إذا (ګام_پلنوالی == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_لاره_Call* ثابت newp = جديد Ast_لاره_Call{
                        nodep->fileline(),
                        جديد Ast_لاره_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                    أرجع;
                } وإلا إذا (ګام_پلنوالی != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ټېل_Deletep(nodep->argsp()->unlink_Fr_شا());
                    Ast_لاره_Call* ثابت clonep = nodep->clone_ونه(خطأ);
                    VN_AS(stepp, Const)->عدد().سېټ_Long(1);
                    AstConst* ثابت constp = جديد AstConst(nodep->fileline(), ګام_پلنوالی - 1);
                    AstArg* ثابت argp = جديد AstArg{nodep->fileline(), "", constp};
                    Ast_لاره_Call* ثابت newp
                        = جديد Ast_لاره_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                    أرجع;
                }
            }
            Ast_غوټه_بیان* ثابت newp
                = enumSelect(nodep->fromp()->unlink_Fr_شا(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    فراغ لاره_Call_Wildcard(Ast_لاره_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = تش_اشاره;
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            لاره_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ASSOC_اندازه};  // So don't need num()
            newp->dtype_سېټ_Int();
        } وإلا إذا (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } وإلا إذا (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", لاره_Arg(nodep, 0), SELF, BOTH);
            Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_Wildcard_شاخص_بیان(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ASSOC_EXISTS, شاخص_exprp->unlink_Fr_شا()};
            newp->dtype_سېټ_بټ_Unsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            لاره_Ok_Arguments(nodep, 0, 1);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_سېټ_Void();
            } وإلا {
                iterateCheckSelf(nodep, "argument", لاره_Arg(nodep, 0), SELF, BOTH);
                Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_Wildcard_شاخص_بیان(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::ASSOC_ERASE, شاخص_exprp->unlink_Fr_شا()};
                newp->dtype_سېټ_Void();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   adtypep->پیدا_کړه_String_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, ریښتیا, خطأ, nodep->پیدا_کړه_بټ_DType(),
                                   adtypep->پیدا_کړه_String_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, ریښتیا, خطأ, adtypep->subDTypep(),
                                   adtypep->پیدا_کړه_String_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_پلنوالی(ریښتیا);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }
    فراغ لاره_Call_Assoc(Ast_لاره_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = تش_اشاره;
        إذا (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            لاره_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ASSOC_اندازه};  // So don't need num()
            newp->dtype_سېټ_Int();
        } وإلا إذا (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", لاره_Arg(nodep, 0), adtypep->کلی_DTypep(), BOTH);
            Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_Assoc_شاخص_بیان(nodep, adtypep);
            لاره_Call_LValue_Recurse(nodep, شاخص_exprp, VAccess::READWRITE);
            newp
                = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                     VCMethod::array_لاره(nodep->name()),  // first/last/next/prev
                                     شاخص_exprp->unlink_Fr_شا()};
            newp->dtype_سېټ_Int();
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", لاره_Arg(nodep, 0), adtypep->کلی_DTypep(), BOTH);
            Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_Assoc_شاخص_بیان(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ASSOC_EXISTS, شاخص_exprp->unlink_Fr_شا()};
            newp->dtype_سېټ_بټ_Unsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            لاره_Ok_Arguments(nodep, 0, 1);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_سېټ_Void();
            } وإلا {
                iterateCheckTyped(nodep, "argument", لاره_Arg(nodep, 0), adtypep->کلی_DTypep(),
                                  BOTH);
                Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_Assoc_شاخص_بیان(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::ASSOC_ERASE, شاخص_exprp->unlink_Fr_شا()};
                newp->dtype_سېټ_Void();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلی_DTypep(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = لاره_With_Clause(
                nodep, خطأ, ریښتیا, تش_اشاره, nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(کتار_DType_شاخصي_By(adtypep->کلی_DTypep()));
            } وإلا {
                newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            }
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp = لاره_With_Clause(nodep, ریښتیا, خطأ, nodep->پیدا_کړه_بټ_DType(),
                                                    adtypep->کلی_DTypep(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp = لاره_With_Clause(nodep, ریښتیا, خطأ, nodep->پیدا_کړه_بټ_DType(),
                                                    adtypep->کلی_DTypep(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(adtypep->کلی_DTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp = لاره_With_Clause(nodep, ریښتیا, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلی_DTypep(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_پلنوالی(ریښتیا);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }
    Ast_غوټه_بیان* لاره_Call_Assoc_شاخص_بیان(Ast_لاره_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_غوټه* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", شاخص_exprp, CONTEXT_DET, FINAL, adtypep->کلی_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    Ast_غوټه_بیان* لاره_Call_Wildcard_شاخص_بیان(Ast_لاره_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_غوټه* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", شاخص_exprp, CONTEXT_DET, FINAL, adtypep->پیدا_کړه_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ لاره_Call_LValue_Recurse(Ast_لاره_Call* nodep, Ast_غوټه* childp, ثابت VAccess& access) {
        إذا (AstCMethodHard* ثابت ichildp = VN_CAST(childp, CMethodHard)) {
            منطقي methAt = خطأ;
            منطقي meth_At_شا = خطأ;
            منطقي methAtWrite = خطأ;
            بدل (ichildp->لاره()) {
            حالة VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = ریښتیا;
                اكسر;
            حالة VCMethod::ARRAY_AT_شا:  // FALLTHRU
                meth_At_شا = ریښتیا;
                اكسر;
            حالة VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND_شا:  //
                methAtWrite = ریښتیا;
                اكسر;
            افتراضي: اكسر;
            }
            إذا (methAt || meth_At_شا || methAtWrite) {
                ثابت Ast_غوټه_DType* ثابت fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                إذا (VN_IS(fromDtypep, کتار_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    إذا (methAt) {
                        ichildp->لاره(VCMethod::ARRAY_AT_WRITE);
                    } وإلا إذا (meth_At_شا) {
                        ichildp->لاره(VCMethod::DYN_AT_WRITE_APPEND_شا);
                    }
                }
                لاره_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                أرجع;
            }
        }
        إذا (Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(childp, غوټه_Var_Ref)) {
            varrefp->access(access);
        } وإلا إذا (ثابت AstMemberSel* ثابت ichildp = VN_CAST(childp, MemberSel)) {
            لاره_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت AstStructSel* ثابت ichildp = VN_CAST(childp, StructSel)) {
            لاره_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت Ast_غوټه_Sel* ثابت ichildp = VN_CAST(childp, غوټه_Sel)) {
            لاره_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } وإلا {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* لاره_Call_Array(Ast_لاره_Call* nodep, Ast_غوټه_DType* adtypep) {
        AstCMethodHard* newp = تش_اشاره;

        إذا (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = تش_اشاره;
            إذا (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = لاره_With_Clause(nodep, خطأ, ریښتیا, تش_اشاره, nodep->پیدا_کړه_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = لاره_With_Clause(
                nodep, خطأ, ریښتیا, تش_اشاره, nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(newp->پیدا_کړه_کتار_شاخص_DType());
            } وإلا {
                newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            }
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, ریښتیا, خطأ, nodep->پیدا_کړه_بټ_DType(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, ریښتیا, خطأ, nodep->پیدا_کړه_بټ_DType(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->پیدا_کړه_کتار_شاخص_DType());
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, ریښتیا, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(کتار_DType_شاخصي_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        }
        أرجع newp;
    }
    فراغ لاره_Call_Dyn(Ast_لاره_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = تش_اشاره;
        إذا (nodep->name() == "at") {  // Created internally for []
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWrite") {  // Created internally for []
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "size") {
            لاره_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::DYN_اندازه};
            newp->dtype_سېټ_Int();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete()
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا ((newp = لاره_Call_Array(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_پلنوالی(ریښتیا);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }
    فراغ لاره_Call_کتار(Ast_لاره_Call* nodep, Ast_کتار_DType* adtypep) {
        AstCMethodHard* newp = تش_اشاره;
        إذا (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            لاره_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::DYN_اندازه};
            newp->dtype_سېټ_Int();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            لاره_Ok_Arguments(nodep, 0, 1);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_سېټ_Void();
            } وإلا {
                iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
                Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_کتار_شاخص_بیان(nodep);
                إذا (شاخص_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                              VCMethod::DYN_راوباسه_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_سېټ_Void();
                } وإلا {
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                              VCMethod::DYN_ERASE, شاخص_exprp->unlink_Fr_شا()};
                    newp->dtype_سېټ_Void();
                }
            }
        } وإلا إذا (nodep->name() == "insert") {
            لاره_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", لاره_Arg(nodep, 0), BOTH);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_غوټه_بیان* ثابت شاخص_exprp = لاره_Call_کتار_شاخص_بیان(nodep);
            AstArg* ثابت argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            إذا (شاخص_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::DYN_ټېل_FRONT, argp->exprp()->unlink_Fr_شا()};
                newp->dtype_سېټ_Void();
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                          VCMethod::DYN_ورننباسه, شاخص_exprp->unlink_Fr_شا()};
                newp->جمع_کړه_Pinsp(argp->exprp()->unlink_Fr_شا());
                newp->dtype_سېټ_Void();
            }
        } وإلا إذا (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            لاره_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "push_back" || nodep->name() == "push_front") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", لاره_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ثابت argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره(nodep->name()),
                                      argp->exprp()->unlink_Fr_شا()};
            newp->dtype_سېټ_Void();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                      VCMethod::array_لاره("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
        } وإلا إذا ((newp = لاره_Call_Array(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_پلنوالی(ریښتیا);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        }
    }
    Ast_غوټه_بیان* لاره_Call_کتار_شاخص_بیان(Ast_لاره_Call* nodep) {
        Ast_غوټه* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", شاخص_exprp, BOTH);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ لاره_Call_Warn_Timing(Ast_غوټه_FTask_Ref* ثابت nodep, ثابت std::string& className) {
        إذا (v3Global.opt.timing().is_سېټ_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } وإلا إذا (!v3Global.opt.timing().is_سېټ_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    فراغ لاره_Call_Iface_Ref(Ast_لاره_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ثابت ifacep = adtypep->iface_له_لارې_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        إذا (Ast_غوټه_FTask* ثابت ftaskp
            = VN_CAST(m_memberMap.پیدا_کړه_Member(ifacep, nodep->name()), غوټه_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            إذا (!modport_Allows_توکی(nodep, adtypep, ftaskp)) {
                nodep->dtype_سېټ_Void();
                أرجع;
            }
            إذا (adtypep->isVirtual()) {
                لكل (Ast_غوټه* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    إذا (AstVar* ثابت mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, تش_اشاره);
            إذا (ftaskp->isStatic()) {
                AstArg* ثابت argsp = nodep->argsp();
                إذا (argsp) argsp->unlink_Fr_شا_With_بل();
                Ast_غوټه_FTask_Ref* newp = تش_اشاره;
                إذا (VN_IS(ftaskp, Task)) {
                    newp = جديد AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } وإلا {
                    newp = جديد AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            } وإلا {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                إذا (VN_IS(ftaskp, Task)) nodep->dtype_سېټ_Void();
                process_FTask_Ref_دلیلونه(nodep);
            }
            أرجع;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    فراغ handle_Randomize_دلیلونه(Ast_غوټه_FTask_Ref* ثابت nodep, AstClass* ثابت classp) {
        منطقي has_Non_Null_دلیلونه = خطأ;
        AstConst* nullp = تش_اشاره;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = تش_اشاره;
            Ast_غوټه_بیان* exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->عدد().isNull()) {
                    nullp = constp;
                    استمر;
                }
            }
            has_Non_Null_دلیلونه = ریښتیا;
            AstVar* fromVarp = تش_اشاره;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            إذا (Ast_لاره_Call* لاره_Callp = VN_CAST(nodep, لاره_Call)) {
                Ast_غوټه_بیان* fromp = لاره_Callp->fromp();
                طالما (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ثابت varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            إذا (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
                استمر;
            }
            طالما (exprp) {
                إذا (AstMemberSel* ثابت memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } وإلا {
                    إذا (AstVarRef* ثابت varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } وإلا {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
                    }
                    exprp = تش_اشاره;
                }
                // All variables in the dot hierarchy must be randomizable
                إذا (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            إذا (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } وإلا إذا (classp->existsMember([&](ثابت AstClass*, ثابت AstVar* memberVarp) {
                           أرجع memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } وإلا {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_شا()->delete_ونه(), argp);
            }
        }
        إذا (nullp && has_Non_Null_دلیلونه) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    فراغ لاره_Call_Class(Ast_لاره_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت لومړی_classp = adtypep->classp();
        AstWith* withp = تش_اشاره;
        إذا (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کړه_Void_DType(),
                                     adtypep->پیدا_کړه_بټ_DType(), adtypep);
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                إذا (argp->exprp()) userIterate(argp->exprp(), پلنوالی_VP{SELF, BOTH}.p());
            }
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, لومړی_classp);
            handle_Randomize_دلیلونه(nodep, لومړی_classp);
        } وإلا إذا (nodep->name() == "srandom") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, لومړی_classp);
        }
        UASSERT_OBJ(لومړی_classp, nodep, "Unlinked");
        لكل (AstClass* classp = لومړی_classp; classp;) {
            إذا (nodep->fileline()->timingOn()) {
                إذا (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ثابت packagep = get_توکی_Package(classp);
                    // Check if it's std
                    إذا (packagep && packagep->name() == "std") {
                        إذا (classp->name() == "process") {
                            لاره_Call_Warn_Timing(nodep, "process");
                        } وإلا إذا (classp->name() == "semaphore" && nodep->name() == "get") {
                            لاره_Call_Warn_Timing(nodep, "semaphore");
                        } وإلا إذا (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            لاره_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            إذا (Ast_غوټه_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, nodep->name()), غوټه_FTask)) {
                userIterate(ftaskp, تش_اشاره);
                إذا (ftaskp->isStatic()) {
                    AstArg* ثابت argsp = nodep->argsp();
                    إذا (argsp) argsp->unlink_Fr_شا_With_بل();
                    Ast_غوټه_FTask_Ref* newp = تش_اشاره;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    إذا (!m_vup) {  // Called as task
                        newp = جديد AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } وإلا {
                        newp = جديد AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                } وإلا {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    إذا (VN_IS(ftaskp, Task)) {
                        إذا (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_بیان)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_سېټ_Void();
                    }
                    إذا (withp) nodep->withp(withp);
                    process_FTask_Ref_دلیلونه(nodep);
                }
                أرجع;
            } وإلا إذا (nodep->name() == "get_randstate") {
                لاره_Ok_Arguments(nodep, 0, 0);
                لومړی_classp->بنسټ_Most_Classp()->needRNG(ریښتیا);
                v3Global.useRandomizeMethods(ریښتیا);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                         VCMethod::RNG_GET_RANDSTATE, تش_اشاره};
                newp->usePtr(ریښتیا);
                newp->dtype_سېټ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                لاره_Ok_Arguments(nodep, 1, 1);
                Ast_غوټه_بیان* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_غوټه_بیان* ثابت exprp = nodep->argsp()->exprp();
                لومړی_classp->بنسټ_Most_Classp()->needRNG(ریښتیا);
                v3Global.useRandomizeMethods(ریښتیا);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                         VCMethod::RNG_سېټ_RANDSTATE, exprp->unlink_Fr_شا()};
                newp->usePtr(ریښتیا);
                newp->dtype_سېټ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(ریښتیا);
                nodep->dtypep(nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره;
        }
        {
            VSpellCheck speller;
            لكل (AstClass* classp = لومړی_classp; classp;) {
                لكل (Ast_غوټه* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    إذا (VN_IS(itemp, غوټه_FTask)) speller.ټېل_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : تش_اشاره;
            }
            ثابت string suggest = speller.غوره_Candidate_پیغام(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << لومړی_classp->verilogKwd() << " " << لومړی_classp->prettyNameQ()
                           << "\n"
                           << (suggest.تشه() ? "" : nodep->fileline()->warn_نور() + suggest));
        }
        nodep->dtype_سېټ_Int();  // Guess on error
    }
    فراغ لاره_Call_Constraint(Ast_لاره_Call* nodep, AstConstraintRefDType*) {
        إذا (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            لاره_Ok_Arguments(nodep, 0, 1);
            إذا (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::INT));
            } وإلا {
                nodep->dtype_سېټ_Void();
            }
            v3Global.useRandomizeMethods(ریښتیا);
        } وإلا {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لاره_Call_Rand_ډېرپېښ(Ast_لاره_Call* nodep) {
        لاره_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        إذا (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            nodep->dtype_سېټ_Void();
        } وإلا {
            nodep->dtypep(nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(ریښتیا);
    }
    فراغ لاره_Call_Unpack(Ast_لاره_Call* nodep, AstUnpackArrayDType* adtypep) {
        تعداد : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_مجموعه,
            ARRAY_حاصل_ضرب
        } لاره_Id;

        لاره_Id = UNKNOWN;
        إذا (nodep->name() == "or") {
            لاره_Id = ARRAY_OR;
        } وإلا إذا (nodep->name() == "and") {
            لاره_Id = ARRAY_AND;
        } وإلا إذا (nodep->name() == "xor") {
            لاره_Id = ARRAY_XOR;
        } وإلا إذا (nodep->name() == "sum") {
            لاره_Id = ARRAY_مجموعه;
        } وإلا إذا (nodep->name() == "product") {
            لاره_Id = ARRAY_حاصل_ضرب;
        }

        إذا (لاره_Id) {
            AstWith* ثابت withp
                = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کړه_UInt32_DType(), adtypep->subDTypep());
            لاره_Ok_Arguments(nodep, 0, 0);
            إذا (withp) {
                لاره_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(),
                                         VCMethod::array_لاره("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                إذا (!nodep->لومړی_Abovep()) newp->dtype_سېټ_Void();
                newp->protect(خطأ);
                newp->did_پلنوالی(ریښتیا);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
                أرجع;
            } وإلا {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_کرښه* ثابت fl = nodep->fileline();
                Ast_غوټه_بیان* newp = تش_اشاره;
                لكل (صحيح i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_غوټه_بیان* ثابت arrayRef = nodep->fromp()->clone_ونه_خالص(خطأ);
                    Ast_غوټه_بیان* ثابت selector = جديد AstArraySel{fl, arrayRef, i};
                    إذا (!newp) {
                        newp = selector;
                    } وإلا {
                        بدل (لاره_Id) {
                        حالة ARRAY_OR: newp = جديد AstOr{fl, newp, selector}; اكسر;
                        حالة ARRAY_AND: newp = جديد AstAnd{fl, newp, selector}; اكسر;
                        حالة ARRAY_XOR: newp = جديد AstXor{fl, newp, selector}; اكسر;
                        حالة ARRAY_مجموعه: newp = جديد Ast_جمع_کړه{fl, newp, selector}; اكسر;
                        حالة ARRAY_حاصل_ضرب: newp = جديد AstMul{fl, newp, selector}; اكسر;
                        افتراضي: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            }
        } وإلا إذا (AstCMethodHard* newp = لاره_Call_Array(nodep, adtypep)) {
            newp->protect(خطأ);
            newp->did_پلنوالی(ریښتیا);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    فراغ لاره_Call_Event(Ast_لاره_Call* nodep, AstBasicDType*) {
        // Method call on event
        إذا (nodep->name() == "triggered") {
            لاره_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_سېټ_بټ();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    فراغ لاره_Call_String(Ast_لاره_Call* nodep, AstBasicDType*) {
        // Method call on string
        إذا (nodep->name() == "len") {
            // Constant value
            لاره_Ok_Arguments(nodep, 0, 0);
            Ast_غوټه* ثابت newp = جديد AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "itoa") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } وإلا إذا (nodep->name() == "hextoa") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } وإلا إذا (nodep->name() == "octtoa") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } وإلا إذا (nodep->name() == "bintoa") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } وإلا إذا (nodep->name() == "realtoa") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } وإلا إذا (nodep->name() == "tolower") {
            لاره_Ok_Arguments(nodep, 0, 0);
            Ast_غوټه* ثابت newp
                = جديد Ast_منزل_ښکتنی_N{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (nodep->name() == "toupper") {
            لاره_Ok_Arguments(nodep, 0, 0);
            Ast_غوټه* ثابت newp
                = جديد Ast_منزل_پاسنی_N{nodep->fileline(), nodep->fromp()->unlink_Fr_شا()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (nodep->name() == "compare" || nodep->name() == "icompare") {
            ثابت منطقي ignoreCase = nodep->name()[0] == 'i';
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            AstArg* ثابت argp = nodep->argsp();
            Ast_غوټه_بیان* ثابت lhs = nodep->fromp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhs = argp->exprp()->unlink_Fr_شا();
            Ast_غوټه* ثابت newp = جديد AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (nodep->name() == "putc") {
            لاره_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", لاره_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", لاره_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_غوټه_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), VarRef);
            Ast_غوټه_بیان* ثابت rhsp = arg0p->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت thsp = arg1p->exprp()->unlink_Fr_شا();
            AstVarRef* ثابت varrefp
                = جديد AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_غوټه* ثابت newp = جديد Ast_ټاکل{
                nodep->fileline(), fromp, جديد AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ټېل_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } وإلا إذا (nodep->name() == "getc") {
            لاره_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", لاره_Arg(nodep, 0), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            Ast_غوټه_بیان* ثابت lhsp = nodep->fromp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = arg0p->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت newp = جديد AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (nodep->name() == "substr") {
            لاره_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", لاره_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", لاره_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_غوټه_بیان* ثابت lhsp = nodep->fromp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت rhsp = arg0p->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت thsp = arg1p->exprp()->unlink_Fr_شا();
            Ast_غوټه_بیان* ثابت newp = جديد AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا إذا (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            إذا (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } وإلا إذا (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } وإلا إذا (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } وإلا إذا (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } وإلا إذا (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } وإلا {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            لاره_Ok_Arguments(nodep, 0, 0);
            Ast_غوټه* ثابت newp
                = جديد AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_شا(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_کتار_DType* کتار_DType_شاخصي_By(Ast_غوټه_DType* شاخص_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        إذا (Ast_کتار_DType* ثابت queuep = m_کتار_DType_شاخصي[شاخص_DTypep]) {
            أرجع queuep;
        } وإلا {
            Ast_کتار_DType* ثابت newp
                = جديد Ast_کتار_DType{شاخص_DTypep->fileline(), شاخص_DTypep, تش_اشاره};
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newp);
            m_کتار_DType_شاخصي[شاخص_DTypep] = newp;
            أرجع newp;
        }
    }

    فراغ لیدنه(AstNew* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        AstClass* classp = تش_اشاره;
        منطقي ټاکل_2 = خطأ;
        إذا (VN_IS(nodep->backp(), ټاکل)) {  // assignment case
            ټاکل_2 = ریښتیا;
            Ast_غوټه* warnp = تش_اشاره;
            AstClassRefDType* refp = تش_اشاره;

            إذا (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                إذا (AstClass* ثابت clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ثابت adtypep
                        = جديد AstClassRefDType{nodep->fileline(), clsp, تش_اشاره};
                    v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(adtypep);
                    refp = adtypep;
                }
            } وإلا {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : تش_اشاره;
            }
            إذا (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, غوټه_DType)
                           ? ("data type "s + VN_AS(warnp, غوټه_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                أرجع;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            إذا (Ast_غوټه_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.پیدا_کړه_Member(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } وإلا {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } وإلا {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_دلیلونه(nodep);
        إذا (!ټاکل_2) nodep->dtypeFrom(nodep->taskp());
    }
    فراغ لیدنه(AstNewCopy* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        AstClassRefDType* ثابت refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        إذا (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            أرجع;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        إذا (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    فراغ لیدنه(AstNewDynamic* nodep) override {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        AstDynArrayDType* ثابت adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        إذا (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            أرجع;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        إذا (!VN_IS(nodep->backp(), غوټه_ټاکل)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            أرجع;
        }
        nodep->dtypep(adtypep);
        إذا (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        إذا (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ثابت consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            إذا (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlink_Fr_شا());
                nodep->replaceWith(consp->unlink_Fr_شا());
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }

    فراغ لیدنه(Ast_Tagged_بیان* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_سېټ_بټ();
        userIterateChildren(nodep, m_vup);
    }
    فراغ لیدنه(Ast_Tagged_نمونه* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_سېټ_بټ();
        userIterateChildren(nodep, m_vup);
    }
    فراغ لیدنه(Ast_نمونه_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_سېټ_بټ();
    }
    فراغ لیدنه(Ast_نمونه_ستوری* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_سېټ_بټ();
    }
    فراغ لیدنه(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_سېټ_بټ();
        userIterateChildren(nodep, m_vup);
    }

    فراغ لیدنه(Ast_نمونه* nodep) override {
        إذا (m_دننه_Param_Override && !nodep->ماشوم_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_دننه_Param_Override);
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_دننه_Param_Override = خطأ;
                user_Iterate_And_بل(patp->repp(), پلنوالی_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                إذا (!VN_IS(patp->keyp(), متن_2) && !VN_IS(patp->keyp(), غوټه_DType)) {
                    user_Iterate_And_بل(patp->keyp(), پلنوالی_VP{SELF, BOTH}.p());
                }
                لكل (Ast_غوټه *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_دننه_Param_Override = VN_IS(valuep, نمونه);
                    userIterate(valuep, پلنوالی_VP{SELF, BOTH}.p());
                }
            }
            أرجع;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_دننه_Param_Override);
        m_دننه_Param_Override = خطأ;
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        UINFO(9, "PATTERN " << nodep);
        إذا (nodep->ماشوم_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        إذا (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_غوټه_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            إذا (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                أرجع;
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            إذا (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                أرجع;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ثابت صحيح times = لیدنه_Pat_Member_Rep(patp);
                لكل (صحيح i = 1; i < times; i++) {
                    AstPatMember* ثابت newp = patp->clone_ونه(خطأ);
                    patp->جمع_کړه_بل_دلته(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                إذا (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_شا(&relinkHandle);
                    طالما (Ast_غوټه_بیان* ثابت movep = VN_AS(patp->lhssp()->nextp(), غوټه_بیان)) {
                        movep->unlink_Fr_شا();  // Not unlinkFrBackWithNext, just one
                        Ast_غوټه* newkeyp = تش_اشاره;
                        إذا (patp->keyp()) newkeyp = patp->keyp()->clone_ونه(ریښتیا);
                        AstPatMember* ثابت newp
                            = جديد AstPatMember{patp->fileline(), movep, newkeyp, تش_اشاره};
                        patp->جمع_کړه_بل(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = تش_اشاره;
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ثابت nextp = VN_AS(patp->nextp(), PatMember);
                إذا (patp->isDefault()) {
                    إذا (defaultp) {
                        VL_DO_DANGLING(ټېل_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_شا();
                }
                patp = nextp;
            }
            طالما (ثابت AstConstDType* ثابت vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, پلنوالی_VP{SELF, BOTH}.p());

            إذا (خپلکاری* ثابت vdtypep = VN_CAST(dtypep, StructDType)) {
                نمونه_Struct(nodep, vdtypep, defaultp);
            } وإلا إذا (خپلکاری* ثابت vdtypep = VN_CAST(dtypep, غوټه_Array_DType)) {
                نمونه_Array(nodep, vdtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ثابت consp = جديد AstConsAssoc{nodep->fileline()};
                نمونه_Keyed<Ast_Cons_Assoc_توکی>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ثابت consp = جديد AstConsWildcard{nodep->fileline()};
                نمونه_Keyed<Ast_Cons_Wildcard_توکی>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, کتار_DType)) {
                نمونه_Dyn_Array_Or_کتار(nodep, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                نمونه_Basic(nodep, dtypep, defaultp);
            } وإلا {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            إذا (defaultp) VL_DO_DANGLING(ټېل_Deletep(defaultp), defaultp);
        }
    }
    فراغ نمونه_Struct(Ast_نمونه* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        کارول PatMap = std::map<ثابت AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ثابت AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            طالما (patp) {
                افعل {
                    إذا (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        إذا (ثابت Ast_متن* textp = VN_CAST(patp->keyp(), متن_2)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.پیدا_کړه_Member(vdtypep, textp->متن()),
                                           MemberDType);
                            إذا (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->متن() << "' not found as member");
                                اكسر;
                            } وإلا {
                                ثابت std::pair<PatMap::iterator, منطقي> ret
                                    = patmap.emplace(memp, patp);
                                إذا (!ret.دوهم) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), متن_2)->متن());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } وإلا إذا (Ast_غوټه_DType* nodedtypep = VN_CAST(patp->keyp(), غوټه_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, پلنوالی_VP{SELF, BOTH}.p());
                            ثابت string dtype = nodedtypep->dtypep()->prettyDTypeName(ریښتیا);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } وإلا {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } وإلا إذا (memp) {
                        // constant expr
                        ثابت std::pair<PatMap::iterator, منطقي> ret = patmap.emplace(memp, patp);
                        إذا (!ret.دوهم) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), متن_2)->متن());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } وإلا {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } طالما (خطأ);

                //بل
                إذا (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_غوټه_بیان* newp = تش_اشاره;
        إذا (vdtypep->packed()) {
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خپلکاری it = patmap.پیدا_کړه(memp);
                إذا (it == patmap.پای_ټکی()) {  // default or default_type assignment
                    إذا (AstStructDType* ثابت memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_نمونه_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } وإلا {
                        AstPatMember* ثابت patp
                            = default_Patp_نمونه_Struct(nodep, memp, defaultp, dtypemap);
                        newp = ارزښت_Concat_نمونه_Struct(patp, newp, memp, nodep);
                        إذا (patp) VL_DO_DANGLING(ټېل_Deletep(patp), patp);
                    }
                } وإلا {  // member assignment
                    newp = ارزښت_Concat_نمونه_Struct(it->دوهم, newp, memp, nodep);
                }
            }
        } وإلا {  // Unpacked
            AstConsStruct* ثابت consp = جديد AstConsStruct{nodep->fileline(), vdtypep};
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خپلکاری it = patmap.پیدا_کړه(memp);
                AstPatMember* patp = تش_اشاره;
                إذا (it == patmap.پای_ټکی()) {  // Default or default_type assignment
                    patp = default_Patp_نمونه_Struct(nodep, memp, defaultp, dtypemap);
                    ټېل_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ټېل_Deletep(patp);  // patp used below
                } وإلا {
                    patp = it->دوهم;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_غوټه_بیان* ثابت valuep = نمونه_Member_ارزښت_Iterate(patp);
                consp->جمع_کړه_Itemsp(جديد Ast_Cons_Struct_توکی{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_غوټه_بیان* nestedvalue_Concat_نمونه_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_غوټه_بیان* newp,
                                                 Ast_نمونه* nodep, ثابت DTypeMap& dtypemap) {
        لكل (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            إذا (AstStructDType* ثابت memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_نمونه_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } وإلا {
                AstPatMember* ثابت patp
                    = default_Patp_نمونه_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = ارزښت_Concat_نمونه_Struct(patp, newp, memp_nested, nodep);
                إذا (patp) VL_DO_DANGLING(ټېل_Deletep(patp), patp);
            }
        }
        أرجع newp;
    }

    AstPatMember* default_Patp_نمونه_Struct(Ast_نمونه* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ثابت DTypeMap& dtypemap) {
        ثابت string memp_DType = memp->virtRefDTypep()->prettyDTypeName(ریښتیا);
        ثابت خپلکاری it = dtypemap.پیدا_کړه(memp_DType);
        إذا (it != dtypemap.پای_ټکی()) {
            // default_value for data_type
            أرجع it->دوهم->clone_ونه(خطأ);
        }
        إذا (defaultp) {
            // default_value for any unmatched member yet
            أرجع defaultp->clone_ونه(خطأ);
        }
        إذا (memp->valuep()) {
            أرجع جديد AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_ونه(خطأ), غوټه_بیان),
                                    جديد Ast_متن{nodep->fileline(), memp->name()}, تش_اشاره};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        أرجع تش_اشاره;
    }

    Ast_غوټه_بیان* ارزښت_Concat_نمونه_Struct(AstPatMember* patp, Ast_غوټه_بیان* newp,
                                           AstMemberDType* memp, Ast_نمونه* nodep) {
        إذا (patp) {
            patp->dtypep(memp);
            Ast_غوټه_بیان* ثابت valuep = نمونه_Member_ارزښت_Iterate(patp);
            إذا (!newp) {
                newp = valuep;
            } وإلا {
                AstConcat* ثابت concatp = جديد AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_کوډ::WIDTHCONCAT, ریښتیا);
                valuep->fileline()->modifyWarnOff(V3_Error_کوډ::WIDTHCONCAT, ریښتیا);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        أرجع newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_غوټه_DType* elemDTypep,
                                       ثابت DTypeMap& dtypemap = DTypeMap{}) {
        Ast_غوټه_بیان* ثابت valuep = defaultp->lhssp()->clone_ونه(خطأ);
        Ast_غوټه_DType* ثابت elemDTypeSkipRefp = elemDTypep->skipRefp();
        ثابت AstStructDType* ثابت structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ثابت منطقي unpackedStruct = structp && !structp->packed();

        إذا (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            أرجع defaultp->clone_ونه(خطأ);
        }
        إذا (VN_IS(valuep, نمونه)) {
            VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            أرجع defaultp->clone_ونه(خطأ);
        }
        إذا (!valuep->dtypep()) userIterate(valuep, پلنوالی_VP{SELF, BOTH}.p());
        منطقي ټولیز_Element = unpackedStruct;
        إذا (Ast_غوټه_DType* ثابت ارزښت_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            ټولیز_Element = unpackedStruct ? !ارزښت_DTypep->skipRefp()->isIntegralOrPacked()
                                          : ارزښت_DTypep->similarDType(elemDTypeSkipRefp);
        }
        إذا (ټولیز_Element) {
            VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            أرجع defaultp->clone_ونه(خطأ);
        }

        AstPatMember* ثابت nestedDefaultp
            = جديد AstPatMember{defaultp->fileline(), valuep, تش_اشاره, تش_اشاره};
        nestedDefaultp->isDefault(ریښتیا);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        لكل (ثابت خپلکاری& ثبت : dtypemap) nestedDefaultp->جمع_کړه_بل(ثبت.دوهم->clone_ونه(خطأ));
        Ast_نمونه* ثابت recursivePatternp = جديد Ast_نمونه{defaultp->fileline(), nestedDefaultp};
        أرجع جديد AstPatMember{defaultp->fileline(), recursivePatternp, تش_اشاره, تش_اشاره};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_غوټه_بیان* نمونه_Default_Valuep(AstPatMember* defaultp, Ast_غوټه_DType* elemDTypep) {
        إذا (!defaultp) أرجع تش_اشاره;
        AstPatMember* ثابت elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_غوټه_بیان* ثابت valuep = نمونه_Member_ارزښت_Iterate(elemDefaultp);
        VL_DO_DANGLING(ټېل_Deletep(elemDefaultp), elemDefaultp);
        أرجع valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    فراغ نمونه_Concat_Array(Ast_نمونه* nodep, AstUnpackArrayDType* arrayDtp) {
        ثابت VNumRange range = arrayDtp->declRange();
        ثابت صحيح elements = range.elements();
        AstConsArray* ثابت newp = جديد AstConsArray{nodep->fileline(), arrayDtp};
        صحيح entn = 0;  // Number of elements given so far, from the left
        منطقي error = خطأ;  // Reported an error, so the value is not complete
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_غوټه_بیان* ثابت valuep = نمونه_Member_ارزښت_Iterate(patp);
            ثابت uint64_t شاخص = range.ascending() ? entn : elements - 1 - entn;
            ثابت AstUnpackArrayDType* ثابت spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            إذا (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_ارزښتونه(شاخص, valuep);
                استمر;
            }
            ثابت VCastable castable = Ast_غوټه::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = ریښتیا;
            }
            newp->valuep(شاخص, valuep);
            ++entn;
        }
        إذا (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = ریښتیا;
        }
        إذا (error) {
            VL_DO_DANGLING(ټېل_Deletep(newp), newp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}});
        } وإلا {
            nodep->replaceWith(newp);
        }
    }
    فراغ نمونه_Array(Ast_نمونه* nodep, Ast_غوټه_Array_DType* arrayDtp, AstPatMember* defaultp) {
        إذا (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            نمونه_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            أرجع;
        }
        ثابت VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_ویکټور_Map(nodep, range);
        UINFO(9, "ent " << range.کیڼ() << " to " << range.ښی());
        Ast_غوټه* newp = تش_اشاره;
        منطقي missed = خطأ;  // Some element missed, so not a complete value
        لكل (صحيح entn = 0, ent = range.کیڼ(); entn < range.elements();
             ++entn, ent += range.کیڼ_منزل_ښی_Inc()) {
            Ast_غوټه_بیان* valuep = تش_اشاره;
            ثابت خپلکاری it = patmap.پیدا_کړه(ent);
            إذا (it == patmap.پای_ټکی()) {
                إذا (defaultp) {
                    valuep = نمونه_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = ریښتیا;
                }
            } وإلا {
                AstPatMember* ثابت patp = it->دوهم;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = نمونه_Member_ارزښت_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ثابت AstUnpackArrayDType* ثابت memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                إذا (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_نور()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
                    valuep = جديد AstConst{patp->fileline(), AstConst::بټ_False_Erroring{}};
                }
            }

            إذا (valuep) {
                إذا (VN_IS(arrayDtp, UnpackArrayDType)) {
                    إذا (!newp) newp = جديد AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } وإلا {  // Packed. Convert to concat for now.
                    إذا (!newp) {
                        newp = valuep;
                    } وإلا {
                        AstConcat* ثابت concatp
                            = جديد AstConcat{valuep->fileline(), VN_AS(newp, غوټه_بیان), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        إذا (!patmap.تشه()) nodep->v3error("Assignment pattern with too many elements");
        إذا (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ټېل_Deletep(newp), newp);
            newp = جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}};
        }
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* نمونه_Const_Keyp(AstPatMember* patp) {
        إذا (VN_IS(patp->keyp(), غوټه_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            أرجع تش_اشاره;
        }
        إذا (patp->کلی_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->کلی_Exprp());  // May change
        AstConst* ثابت constp = VN_CAST(patp->کلی_Exprp(), Const);
        إذا (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        أرجع constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* نمونه_Scalar_Keyp(AstPatMember* patp, Ast_غوټه_DType* کلی_DTypep) {
        إذا (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            أرجع تش_اشاره;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        إذا (کلی_DTypep->skipRefp()->isIntegralOrPacked() && patp->کلی_Exprp()) {
            userIterate(patp->کلی_Exprp(), پلنوالی_VP{کلی_DTypep, PRELIM}.p());
            ثابت صحيح پلنوالی = std::max(patp->کلی_Exprp()->پلنوالی(), کلی_DTypep->پلنوالی());
            Ast_غوټه_DType* ثابت contextDTypep
                = کلی_DTypep->پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, پلنوالی, کلی_DTypep->numeric(), ریښتیا);
            iterateCheck(patp, "key", patp->کلی_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, خطأ);
        }
        AstConst* ثابت constp = نمونه_Const_Keyp(patp);
        إذا (!constp) أرجع تش_اشاره;
        إذا (constp->عدد().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            أرجع تش_اشاره;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, کلی_DTypep, EXTEND_EXP, خطأ);
        Ast_غوټه_بیان* ثابت convp = patp->کلی_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_غوټه_بیان* ثابت keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        أرجع VN_AS(keyp->unlink_Fr_شا(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_غوټه_بیان* نمونه_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_غوټه_DType* arrayDtp) {
        Ast_غوټه_DType* ثابت کلی_DTypep = VN_AS(arrayDtp, AssocArrayDType)->کلی_DTypep();
        إذا (!کلی_DTypep->skipRefp()->isAggregateType() || !patp->کلی_Exprp()) {
            أرجع نمونه_Scalar_Keyp(patp, کلی_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->کلی_Exprp(), کلی_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->کلی_Exprp());
        Ast_غوټه_بیان* ثابت foldedp = patp->کلی_Exprp();
        إذا (!foldedp->is_Constant_ونه()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            أرجع تش_اشاره;
        }
        إذا (foldedp->exists([](ثابت AstConst* constp) { أرجع constp->عدد().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            أرجع تش_اشاره;
        }
        أرجع foldedp->unlink_Fr_شا();
    }
    AstConst* نمونه_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_غوټه_DType* arrayDtp) {
        أرجع نمونه_Scalar_Keyp(patp, arrayDtp->پیدا_کړه_String_DType());
    }
    AstConst* نمونه_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_غوټه_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        أرجع نمونه_Scalar_Keyp(patp, arrayDtp->پیدا_کړه_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    کينډۍ <ډول_نوم T_توکی, ډول_نوم T_Cons>
    فراغ ترتیبول_Cons_توکي(T_Cons* nodep) {
        std::ویکټور<T_توکی*> itemps;
        طالما (T_توکی* ثابت itemp = nodep->itemsp()) {
            itemp->unlink_Fr_شا();
            itemps.ټېل_شا(itemp);
        }
        ثابت خپلکاری کلی_د = [](ثابت T_توکی* itemp) -> ثابت V3_شمېره& {
            ثابت Ast_غوټه* ثابت keyp = itemp->keyp();
            أرجع VN_AS(keyp, Const)->عدد();
        };
        std::stable_ترتیبول(itemps.begin(), itemps.پای_ټکی(), [&](ثابت T_توکی* ap, ثابت T_توکی* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            أرجع کلی_د(ap).isLtXZ(کلی_د(bp));
        });
        ثابت T_توکی* firstp = تش_اشاره;  // First item with the current key
        لكل (T_توکی* ثابت itemp : itemps) {
            إذا (firstp && کلی_د(firstp).isCaseEq(کلی_د(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } وإلا {
                firstp = itemp;
            }
            nodep->جمع_کړه_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    کينډۍ <ډول_نوم T_توکی, ډول_نوم T_Cons>
    فراغ نمونه_Keyed(Ast_نمونه* nodep, T_Cons* consp, Ast_غوټه_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(نمونه_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ثابت خپلکاری keyp = نمونه_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_غوټه_بیان* ثابت valuep = نمونه_Member_ارزښت_Iterate(patp);
            إذا (keyp) {
                consp->جمع_کړه_Itemsp(جديد T_توکی{patp->fileline(), keyp, valuep});
            } وإلا {
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ثابت AstAssocArrayDType* ثابت assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        إذا (!assocDtp || !assocDtp->کلی_DTypep()->skipRefp()->isAggregateType()) {
            ترتیبول_Cons_توکي<T_توکی>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_غوټه_بیان* new_Cons_Dyn_Array_Or_کتار(File_کرښه* fileline, ثابت Ast_غوټه_DType* dtypep) {
        إذا (VN_IS(dtypep, DynArrayDType)) أرجع جديد AstConsDynArray{fileline};
        إذا (VN_IS(dtypep, کتار_DType)) أرجع جديد Ast_Cons_کتار{fileline};
        أرجع تش_اشاره;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    کينډۍ <ډول_نوم T_توکی, ډول_نوم T_Cons>
    فراغ جمع_کړه_Cons_توکی(T_Cons* consp, Ast_غوټه_بیان* valuep) {
        Ast_غوټه_DType* ثابت arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ثابت منطقي isElement
            = Ast_غوټه::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), تش_اشاره)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ثابت Ast_کتار_DType* ثابت sub_کتار_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), کتار_DType);
        إذا (!isElement && !(sub_کتار_Dtp && sub_کتار_Dtp->boundp())) {
            T_Cons* ثابت subp = Ast_غوټه::cast<T_Cons>(valuep);
            إذا (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                إذا (subp->itemsp()) consp->جمع_کړه_Itemsp(subp->itemsp()->unlink_Fr_شا_With_بل());
                VL_DO_DANGLING(ټېل_Deletep(subp), subp);
                أرجع;
            }
            إذا (Ast_Cons_کتار* ثابت queuep = VN_CAST(valuep, Cons_کتار)) {
                جمع_کړه_Cons_توکي(consp, queuep);
                أرجع;
            }
            إذا (AstConsDynArray* ثابت dynp = VN_CAST(valuep, ConsDynArray)) {
                جمع_کړه_Cons_توکي(consp, dynp);
                أرجع;
            }
        }
        T_توکی* ثابت itemp = جديد T_توکی{valuep->fileline(), valuep};
        consp->جمع_کړه_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        إذا (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } وإلا {
            user_Iterate_And_بل(valuep, پلنوالی_VP{arrayDtp, FINAL}.p());
        }
    }
    فراغ جمع_کړه_Cons_توکی(Ast_غوټه_بیان* consp, Ast_غوټه_بیان* valuep) {
        إذا (AstConsDynArray* ثابت dynp = VN_CAST(consp, ConsDynArray)) {
            جمع_کړه_Cons_توکی<Ast_Cons_Dyn_Array_توکی>(dynp, valuep);
        } وإلا {
            جمع_کړه_Cons_توکی<Ast_Cons_کتار_توکی>(VN_AS(consp, Cons_کتار), valuep);
        }
    }
    کينډۍ <ډول_نوم T_Sub>
    فراغ جمع_کړه_Cons_توکي(Ast_غوټه_بیان* consp, T_Sub* subp) {
        طالما (خپلکاری* ثابت itemp = subp->itemsp()) {
            Ast_غوټه_بیان* ثابت valuep = itemp->valuep()->unlink_Fr_شا();
            VL_DO_DANGLING(ټېل_Deletep(itemp->unlink_Fr_شا()), itemp);
            جمع_کړه_Cons_توکی(consp, valuep);
        }
        VL_DO_DANGLING(ټېل_Deletep(subp), subp);
    }
    فراغ نمونه_Dyn_Array_Or_کتار(Ast_نمونه* nodep, Ast_غوټه_DType* arrayp, AstPatMember* defaultp) {
        Ast_غوټه_بیان* ثابت newp = new_Cons_Dyn_Array_Or_کتار(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ثابت AstPatMember* ثابت firstp = VN_AS(nodep->itemsp(), PatMember);
        ثابت منطقي keyed = defaultp || (firstp && firstp->keyp());
        إذا (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            نمونه_Keyed<Ast_Cons_Dyn_Array_توکی>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            أرجع;
        }
        ثابت حرف* ثابت kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        إذا (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            إذا (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            جمع_کړه_Cons_توکی(newp, نمونه_Member_ارزښت_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    فراغ نمونه_Basic(Ast_نمونه* nodep, Ast_غوټه_DType* vdtypep, AstPatMember* defaultp) {
        ثابت AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ثابت VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_ویکټور_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_غوټه_بیان* newp = تش_اشاره;
        لكل (ثابت صحيح ent : range.seq_Hi_منزل_Lo()) {
            Ast_غوټه_بیان* valuep = تش_اشاره;
            ثابت خپلکاری it = patmap.پیدا_کړه(ent);
            إذا (it == patmap.پای_ټکی()) {
                إذا (defaultp) {
                    valuep = نمونه_Default_Valuep(defaultp, nodep->پیدا_کړه_بټ_DType());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } وإلا {
                AstPatMember* ثابت patp = it->دوهم;
                patmap.erase(it);
                patp->dtypep(nodep->پیدا_کړه_بټ_DType());
                valuep = نمونه_Member_ارزښت_Iterate(patp);
            }
            إذا (valuep) {  // Packed. Convert to concat for now.
                إذا (!newp) {
                    newp = valuep;
                } وإلا {
                    AstConcat* ثابت concatp = جديد AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        إذا (!patmap.تشه()) nodep->v3error("Assignment pattern with too many elements");
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_غوټه_بیان* نمونه_Member_ارزښت_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, پلنوالی_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_غوټه_بیان* valuep = patp->lhssp()->unlink_Fr_شا();
        إذا (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            إذا (AstConst* ثابت newp = V3_پلنوالی_Commit::new_If_Const_Commit_اندازه(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        أرجع valuep;
    }

    ساكن فراغ checkEventAssignment(ثابت Ast_غوټه_ټاکل* ثابت asgnp) {
        string unsupEvtAsgn;
        إذا (!کاروي_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        إذا (VN_IS(asgnp->rhsp(), CReset)) أرجع;
        إذا (asgnp->rhsp()->dtypep()->isEvent() && !کاروي_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.تشه() ? "from" : " and from");
        }
        إذا (!unsupEvtAsgn.تشه()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_نور()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_نور()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ساكن منطقي کاروي_Dynamic_Scheduler(Ast_غوټه* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        طالما (ریښتیا) {
            AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef);
            إذا (vrefp) أرجع کاروي_Dynamic_Scheduler(vrefp);
            إذا (VN_IS(nodep, MemberSel)) {
                أرجع ریښتیا;
            } وإلا إذا (Ast_غوټه_Sel* selp = VN_CAST(nodep, غوټه_Sel)) {
                nodep = selp->fromp();
            } وإلا {
                أرجع خطأ;
            }
        }
    }

    ساكن منطقي کاروي_Dynamic_Scheduler(AstVarRef* vrefp) {
        أرجع VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    فراغ لیدنه(AstPatMember* nodep) override {
        Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ټاکل_3, FINAL, vdtypep, EXTEND_LHS);
    }
    صحيح لیدنه_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        إذا (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ثابت AstConst* ثابت constp = VN_CAST(nodep->repp(), Const);
            إذا (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } وإلا {
                times = constp->منزل_UInt();
            }
            إذا (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_شا_With_بل()
                ->delete_ونه();  // Done with replicate before cloning
        }
        أرجع times;
    }

    فراغ لیدنه(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_غوټه* propp = nodep->propp();
            طالما (AstLogNot* ثابت notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ثابت untilp = VN_CAST(propp, Until);
            إذا (untilp && VN_IS(nodep->backp(), غوټه_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_سېټ_False() || !v3Global.opt.timing().is_سېټ_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(جديد AstConst{untilp->fileline(), AstConst::بټ_False{}});
                VL_DO_DANGLING(ټېل_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_بل(nodep->sensesp(), تش_اشاره);
            إذا (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_سېټ_بټ();
        }
    }

    منطقي لومړی_New_Statement_Ok_Recurse(Ast_غوټه* nodep) {
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            إذا (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) أرجع ریښتیا;
        }
        إذا (Ast_ټاکل* ثابت aitemp = VN_CAST(nodep, ټاکل)) {
            إذا (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) أرجع ریښتیا;
        }
        إذا (AstInitialStaticStmt* ثابت aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            لكل (Ast_غوټه* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!لومړی_New_Statement_Ok_Recurse(stmtp)) أرجع خطأ;
            }
            أرجع ریښتیا;
        }
        إذا (AstInitialAutomaticStmt* ثابت aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            لكل (Ast_غوټه* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!لومړی_New_Statement_Ok_Recurse(stmtp)) أرجع خطأ;
            }
            أرجع ریښتیا;
        }
        أرجع خطأ;
    }

    //--------------------
    // Top levels

    کينډۍ <ډول_نوم Case_توکی>
    فراغ handleCaseType(Ast_غوټه* casep, Ast_غوټه_بیان* exprp, Case_توکی* itemsp) {
        Ast_Attr_د* ثابت exprap = VN_CAST(exprp, Attr_د);
        إذا (!exprap) أرجع;
        إذا (exprap->attrType() != VAttrType::TYPEID) أرجع;

        ثابت Ast_غوټه_DType* ثابت بیان_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << بیان_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_کرښه* ثابت newfl = casep->fileline();
        newfl->warnOff(V3_Error_کوډ::CASEINCOMPLETE, ریښتیا);  // Side effect of transform
        newfl->warnOff(V3_Error_کوډ::CASEOVERLAP, ریښتیا);  // Side effect of transform
        casep->fileline(newfl);
        لكل (Case_توکی* itemp = itemsp; itemp; itemp = Ast_غوټه::as<Case_توکی>(itemp->nextp())) {
            إذا (itemp->isDefault()) استمر;
            منطقي موندنه = خطأ;
            لكل (Ast_غوټه* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ثابت Ast_Attr_د* ثابت condAttrp = VN_CAST(condp, Attr_د);
                إذا (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } وإلا {
                    Ast_غوټه_DType* ثابت condDtp = condAttrp->dtypep();
                    إذا (Ast_غوټه::computeCastable(بیان_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        موندنه = ریښتیا;
                        اكسر;
                    }
                }
            }
            ټېل_Deletep(itemp->condsp()->unlink_Fr_شا_With_بل());
            // Item condition becomes constant 1 if hits else 0
            itemp->جمع_کړه_Condsp(جديد AstConst{newfl, AstConst::بټ_True{}, موندنه});
        }
        exprap->replaceWith(جديد AstConst{newfl, AstConst::بټ_True{}});
        VL_DO_DANGLING(ټېل_Deletep(exprap), exprap);
    }

    کينډۍ <ډول_نوم Case_توکی>
    فراغ handleCase(Ast_غوټه* casep, Ast_غوټه_بیان* exprp, Case_توکی* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_غوټه_DType* subDTypep = exprp->dtypep();
        لكل (Case_توکی* itemp = itemsp; itemp; itemp = Ast_غوټه::as<Case_توکی>(itemp->nextp())) {
            لكل (Ast_غوټه* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                إذا (condp->dtypep() == subDTypep) استمر;

                إذا (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->پیدا_کړه_Double_DType();
                } وإلا إذا (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->پیدا_کړه_String_DType();
                } وإلا {
                    ثابت صحيح پلنوالی = std::max(subDTypep->پلنوالی(), condp->پلنوالی());
                    ثابت صحيح mwidth = std::max(subDTypep->پلنوالی_Min(), condp->پلنوالی_Min());
                    ثابت منطقي issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->پیدا_کړه_بټ_Or_Logic_DType(
                        پلنوالی, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        لكل (Case_توکی* itemp = itemsp; itemp; itemp = Ast_غوټه::as<Case_توکی>(itemp->nextp())) {
            لكل (Ast_غوټه *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    فراغ لیدنه(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_Gen_Case_توکی *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_توکی);
            لكل (Ast_غوټه *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, پلنوالی_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ لیدنه(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بل(nodep->initsp(), تش_اشاره);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_بل(nodep->incsp(), تش_اشاره);
    }
    فراغ لیدنه(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    فراغ لیدنه(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_Case_توکی *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_توکی);  // Prelim may cause the node to get replaced
            user_Iterate_And_بل(itemp->stmtsp(), تش_اشاره);
            لكل (Ast_غوټه *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, پلنوالی_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ لیدنه(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_غوټه_DType* ثابت توکی_DTypep = nodep->پیدا_کړه_UInt32_DType();
        لكل (Ast_Case_توکی *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_توکی);  // Prelim may cause the node to get replaced
            user_Iterate_And_بل(itemp->stmtsp(), تش_اشاره);
            لكل (Ast_غوټه *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, توکی_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    فراغ لیدنه(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_بل(nodep->stmtsp(), تش_اشاره);
    }
    فراغ لیدنه(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    فراغ لیدنه(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    فراغ لیدنه(Ast_غوټه_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_بل(nodep->thensp(), تش_اشاره);
        user_Iterate_And_بل(nodep->elsesp(), تش_اشاره);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    فراغ لیدنه(Ast_بیان_Stmt* nodep) override {
        user_Iterate_And_بل(nodep->stmtsp(), تش_اشاره);
        // expected result is same as parent's expected result
        user_Iterate_And_بل(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    فراغ لیدنه(Ast_غوټه_Foreach* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        nodep->did_پلنوالی(ریښتیا);
        ثابت Ast_Foreach_سرلیک* ثابت headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_بل(headerp->fromp(), پلنوالی_VP{SELF, BOTH}.p());
        Ast_غوټه_بیان* ثابت fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_غوټه_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        لكل (Ast_غوټه *argsp = headerp->elementsp(), *بل_argsp; argsp; argsp = بل_argsp) {
            بل_argsp = argsp->nextp();
            ثابت منطقي تشه = VN_IS(argsp, تشه_2);
            AstVar* ثابت varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || تشه, argsp, "Missing foreach loop variable");
            إذا (varp) varp->کارول_شوی_Loop_Idx(ریښتیا);
            إذا (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            إذا (VN_IS(fromDtp, غوټه_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, کتار_DType)) {
                // Nothing special here
            } وإلا إذا (AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                    أرجع;
                }
            } وإلا إذا (ثابت AstAssocArrayDType* ثابت adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->کلی_DTypep());
            } وإلا {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_غوټه* ثابت bodyp = nodep->bodyp();
        user_Iterate_And_بل(bodyp, تش_اشاره);
        إذا (AstForeach* ثابت loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_منزل_While(loopp), loopp, nodep);
            أرجع;
        }
    }

    فراغ لیدنه(Ast_غوټه_ټاکل* nodep) override {
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
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->پلنوالی_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            إذا (ثابت AstIfaceRefDType* ثابت irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                إذا (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                    أرجع;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            بهیر_ټاکل_ښکتنی(nodep);
            Ast_غوټه_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            إذا (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ثابت Ast_غوټه_DType* ثابت lhsDtp = lhsDTypep->skipRefp();
                ثابت Ast_غوټه_DType* ثابت rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ثابت منطقي lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ثابت منطقي rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                إذا (lhsIsUnpackArray && rhsIsUnpackArray) {
                    إذا (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_غوټه_Var_Ref, Ast_غوټه_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), غوټه_Var_Ref), VN_CAST(nodep->rhsp(), غوټه_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                إذا (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), غوټه_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_ټاکل(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        إذا (VN_IS(nodep, ټاکل_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        إذا (Ast_غوټه* ثابت controlp = nodep->timingControlp()) {
            إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_شا_With_بل()->delete_ونه(), controlp);
            } وإلا إذا (nodep->fileline()->timingOn() && v3Global.opt.timing().is_سېټ_True()) {
                iterateNull(controlp);
            } وإلا {
                إذا (nodep->fileline()->timingOn()) {
                    إذا (v3Global.opt.timing().is_سېټ_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } وإلا {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_شا_With_بل()->delete_ونه(), controlp);
            }
        }
        إذا (VN_IS(nodep->rhsp(), تشه_کتار)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ثابت Ast_غوټه_DType* ثابت lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            إذا (!VN_IS(lhsDtp, کتار_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
                أرجع;
            }
            Ast_لاره_Call* ثابت newp
                = جديد Ast_لاره_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(), "delete"};
            newp->dtype_سېټ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            // Need to now convert it
            لیدنه(newp);
            أرجع;
        }
        إذا (ثابت AstNewDynamic* ثابت dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            إذا (!dynp->rhsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_شا()};
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_شا()};
                newp->جمع_کړه_Pinsp(dynp->rhsp()->unlink_Fr_شا());
            }
            newp->did_پلنوالی(ریښتیا);
            newp->protect(خطأ);
            newp->dtype_سېټ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        إذا (AstCMethodHard* ثابت slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod ټاکل_لاره;
            إذا (slicep->لاره() == VCMethod::DYN_ټوټه) {
                ټاکل_لاره = VCMethod::DYN_ټوټه_ټاکل;
            } وإلا إذا (slicep->لاره() == VCMethod::DYN_ټوټه_شا_شا) {
                ټاکل_لاره = VCMethod::DYN_ټوټه_ټاکل_شا_شا;
            } وإلا إذا (slicep->لاره() == VCMethod::DYN_ټوټه_FRONT_شا) {
                ټاکل_لاره = VCMethod::DYN_ټوټه_ټاکل_FRONT_شا;
            } وإلا {
                ټاکل_لاره = VCMethod::_ENUM_MAX;  // not a slice
            }
            إذا (ټاکل_لاره != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_غوټه_بیان* ثابت fromp = slicep->fromp()->unlink_Fr_شا();
                // Collect existing slice index pins (lsb, msb)
                Ast_غوټه_بیان* ثابت lsbp = slicep->pinsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت msbp = slicep->pinsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), fromp, ټاکل_لاره};
                newp->جمع_کړه_Pinsp(lsbp);
                newp->جمع_کړه_Pinsp(msbp);
                newp->جمع_کړه_Pinsp(rhsp);
                newp->did_پلنوالی(ریښتیا);
                newp->protect(خطأ);
                newp->dtype_سېټ_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            }
        }

        إذا (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.سېټ_Assigns_Events();
        }
    }

    فراغ لیدنه(AstRSRule* nodep) override {
        إذا (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_بل(nodep->prodlistsp(), تش_اشاره);
        user_Iterate_And_بل(nodep->وزن_Stmtsp(), تش_اشاره);
    }

    فراغ لیدنه(AstRelease* nodep) override {
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->پلنوالی_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    فراغ لیدنه(AstDeassign* nodep) override {
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->پلنوالی_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ساكن منطقي isFormatNonNumericArg(ثابت Ast_غوټه_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        أرجع dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, کتار_DType)  //
               || (VN_IS(dtypep, غوټه_UOr_Struct_DType)
                   && !VN_AS(dtypep, غوټه_UOr_Struct_DType)->packed());
    }

    فراغ checkFormatNumericArg(Ast_غوټه* argp, حرف ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        إذا (std::tolower(ch) == 'd') أرجع;
        // In contrast, %x of a class reference, causes errors on several simulators.
        إذا (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_نور() << "... Suggest use '%s'");
    }

    فراغ لیدنه(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    فراغ لیدنه(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بل(nodep->fmtp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_منزل_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_سېټ_String();
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    فراغ لیدنه(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        إذا (m_vup && !m_vup->prelim()) أرجع;  // Can be called as statement or function
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;
        //
        UINFO(9, "  Display in " << nodep->متن());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        إذا (!nodep->بیان_Format_2()) {  // Non-constant format, can't check format string types
            لیدنه_sformatf_format(nodep);
        }

        Ast_غوټه_بیان* oldExprsp = nodep->exprsp();
        إذا (nodep->بیان_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), غوټه_بیان);
        }
        إذا (oldExprsp) oldExprsp->unlink_Fr_شا_With_بل();
        طالما (Ast_غوټه_بیان* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), غوټه_بیان);
            إذا (oldExprsp) oldExprsp->unlink_Fr_شا_With_بل();
            إذا (VN_IS(argp, SFormatArg)) {
                nodep->جمع_کړه_Exprsp(argp);
                استمر;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_غوټه_DType* ثابت dtypep = argp->dtypep()->skipRefp();
            إذا (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } وإلا إذا (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } وإلا إذا (isFormatNonNumericArg(dtypep)) {
                إذا (AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    إذا (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(ریښتیا);
                    }
                } وإلا {
                    // Class handles inside containers print type names, not their members.
                    Ast_غوټه_DType* غوټه_Dtypep = dtypep;
                    طالما (غوټه_Dtypep) {
                        غوټه_Dtypep = غوټه_Dtypep->skipRefp();
                        إذا (Ast_غوټه_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(غوټه_Dtypep, غوټه_UOr_Struct_DType)) {
                            uOrStructDTypep->سېټ_Emit_منزل_String();
                            v3Global.hasPrintedObjects(ریښتیا);
                        }
                        غوټه_Dtypep = غوټه_Dtypep->subDTypep();
                    }
                }
                Ast_غوټه_بیان* ثابت newp = جديد Ast_منزل_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } وإلا إذا (nodep->بیان_Format_2()) {
                إذا (AstEnumDType* ثابت enumDtp = formatEnumDType(argp)) {
                    nodep->جمع_کړه_Exprsp(newFormatEnumArg(argp, enumDtp));
                    استمر;
                }
            }
            إذا (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            إذا (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->جمع_کړه_Exprsp(argp);
            } وإلا {
                nodep->جمع_کړه_Exprsp(جديد AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->متن());
    }
    فراغ لیدنه(AstCReset* nodep) override {
        assert_At_بیان(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    فراغ لیدنه(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ لیدنه(AstConstraintRef* nodep) override { userIterateChildren(nodep, تش_اشاره); }
    فراغ لیدنه(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        إذا (!m_paramsOnly) {
            nodep->foreach([هذا](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    جديد AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string متن = VString::dequotePercent(nodep->fmtp()->متن());
            إذا (متن.تشه()) متن = "Elaboration system task message (IEEE 1800-2023 20.11)";
            بدل (nodep->displayType()) {
            حالة VDisplayType::DT_INFO: nodep->v3warn(USERINFO, متن); اكسر;
            حالة VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, متن); اكسر;
            حالة VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, متن); اكسر;
            حالة VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, متن); اكسر;
            افتراضي: UASSERT_OBJ(خطأ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
        }
    }
    فراغ لیدنه(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    فراغ لیدنه(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    فراغ لیدنه(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    فراغ لیدنه(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ لیدنه(AstFError* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_بل(nodep->strp(), پلنوالی_VP{SELF, BOTH}.p());
        }
    }
    فراغ لیدنه(AstFEof* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    فراغ لیدنه(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ لیدنه(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ لیدنه(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ لیدنه(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->دننه_واټن(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    فراغ لیدنه(AstFGetC* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ لیدنه(AstFGetS* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_بل(nodep->strgp(), پلنوالی_VP{SELF, BOTH}.p());
        }
    }
    فراغ لیدنه(AstFUngetC* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    فراغ لیدنه(AstFRead* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->memp(), پلنوالی_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            إذا (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            إذا (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    فراغ لیدنه(AstFScanF* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_بل(nodep->exprsp(), پلنوالی_VP{SELF, BOTH}.p());
        }
    }
    فراغ لیدنه(AstSScanF* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->fromp(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->exprsp(), پلنوالی_VP{SELF, BOTH}.p());
        }
    }
    فراغ لیدنه(Ast_ډېری_Trace_F* nodep) override { nodep->dtype_سېټ_String(); }
    فراغ لیدنه(AstSysIgnore* nodep) override {
        user_Iterate_And_بل(nodep->exprsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstSystemF* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_غوټه_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_بل(nodep->memp(), پلنوالی_VP{SELF, BOTH}.p());
        ثابت Ast_غوټه_DType* subp = تش_اشاره;
        إذا (ثابت AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            إذا (!adtypep->کلی_DTypep()->skipRefp()->basicp()
                || !adtypep->کلی_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } وإلا إذا (ثابت AstUnpackArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } وإلا إذا (ثابت AstDynArrayDType* ثابت adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } وإلا {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        إذا (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_بل(nodep->lsbp(), پلنوالی_VP{SELF, BOTH}.p());
        user_Iterate_And_بل(nodep->msbp(), پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(Ast_Test_Plus_دلیلونه* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    فراغ لیدنه(Ast_ارزښت_Plus_دلیلونه* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_بل(nodep->outp(), پلنوالی_VP{SELF, BOTH}.p());
        }
    }
    فراغ لیدنه(Ast_وخت_Format* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        إذا (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        إذا (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        إذا (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    فراغ لیدنه(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بل(nodep->passsp(), تش_اشاره);
        user_Iterate_And_بل(nodep->failsp(), تش_اشاره);
    }
    فراغ لیدنه(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
    }
    فراغ لیدنه(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بل(nodep->passsp(), تش_اشاره);
        user_Iterate_And_بل(nodep->failsp(), تش_اشاره);
    }
    فراغ لیدنه(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بل(nodep->passsp(), تش_اشاره);
    }
    فراغ لیدنه(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    منطقي pinInstArrayElement(AstPin* nodep, ثابت AstRange* rangep, صحيح idx) {
        Ast_غوټه_DType* ثابت modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_غوټه_DType* ثابت conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        إذا (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) أرجع ریښتیا;
        // Types don't match, but all dimensions done, still need the usual checks
        إذا (!rangep) أرجع خطأ;

        ثابت AstRange* ثابت subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        صحيح subElems = 1;
        لكل (ثابت AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ثابت صحيح elements = rangep->elementsConst();
        ثابت صحيح from_کیڼ = idx / subElems;

        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ثابت uint32_t conDims = conDTypep->dimensions(خطأ).دوهم;
        ثابت uint32_t modDims = modDTypep->dimensions(خطأ).دوهم;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        إذا (arrp && conDims != modDims) {
            إذا (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_بل(nodep->exprp(),
                                   پلنوالی_VP{conDTypep, FINAL, بهیر_USE_ټاکل}.p());
                أرجع ریښتیا;
            }
            // ArraySel index is counted from lo
            ثابت صحيح بټ_2 = arrp->declRange().ascending() ? from_کیڼ : elements - 1 - from_کیڼ;
            // Connection is self-determined
            user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{conDTypep, FINAL, بهیر_USE_ټاکل}.p());
            Ast_غوټه_بیان* ثابت fromp = VN_AS(nodep->exprp(), غوټه_بیان)->unlink_Fr_شا();
            إذا (Ast_غوټه_بیان* ثابت elemp = newIfaceArrayElement(fromp, بټ_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ټېل_Deletep(fromp), fromp);
            } وإلا {
                AstArraySel* ثابت selp = جديد AstArraySel{fromp->fileline(), fromp, بټ_2};
                selp->did_پلنوالی(ریښتیا);
                nodep->exprp(selp);
            }
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        إذا (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) أرجع خطأ;

        // Packed connection
        ثابت صحيح modwidth = modDTypep->پلنوالی();
        ثابت صحيح conwidth = conDTypep->پلنوالی();

        // Connection has the width of the port, connect it to every element
        إذا (conwidth == modwidth) أرجع خطأ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        إذا (conwidth == modwidth * elements * subElems) {
            إذا (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->کیڼ_Const() << ":"
                                                     << rangep->ښی_Const() << "]");
            }
            ثابت صحيح selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{conDTypep, FINAL, بهیر_USE_ټاکل}.p());
            Ast_غوټه_بیان* ثابت fromp = VN_AS(nodep->exprp(), غوټه_بیان)->unlink_Fr_شا();
            AstSel* ثابت selp = جديد AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_کیڼ), selwidth};
            selp->did_پلنوالی(ریښتیا);
            nodep->exprp(selp);
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{conDTypep, FINAL, بهیر_USE_ټاکل}.p());
        أرجع ریښتیا;
    }

    فراغ لیدنه(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        إذا (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ثابت concatp = VN_CAST(nodep->exprp(), Replicate);
            ثابت AstConst* ثابت countp = concatp ? VN_CAST(concatp->countp(), Const) : تش_اشاره;
            ثابت Ast_غوټه_DType* ثابت dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            إذا (countp && countp->منزل_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_نمونه* ثابت patternp = جديد Ast_نمونه{concatp->fileline(), تش_اشاره};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ټېل_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_دننه_Param_Override);
            m_دننه_Param_Override = is_Override_نمونه(nodep->exprp());
            userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
        } وإلا إذا (!m_paramsOnly) {
            إذا (!nodep->modVarp()->did_پلنوالی()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), تش_اشاره);
            }
            إذا (!nodep->exprp()) {  // No-connect
                إذا (nodep->modVarp()->is_Iface_Array_وېشل()) expandIfaceArrayPin(nodep);
                أرجع;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ثابت VDirection pinDirection = nodep->modVarp()->direction();
            إذا (VN_IS(nodep->exprp(), غوټه_بهیر) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_غوټه_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_غوټه_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            إذا (ثابت AstIfaceRefDType* ثابت modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* ثابت conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                إذا (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_غوټه_DType* subDTypep = modDTypep;
            ثابت صحيح modwidth = modDTypep->پلنوالی();
            صحيح conwidth = conDTypep->پلنوالی();
            إذا (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_بل(nodep->exprp(),
                                   پلنوالی_VP{subDTypep, FINAL, بهیر_USE_ټاکل}.p());
            } وإلا {
                إذا (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    إذا (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        إذا (nodep->modVarp()->is_Iface_Array_وېشل()) expandIfaceArrayPin(nodep);
                        أرجع;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->پلنوالی();
                }
                إذا (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } وإلا إذا (nodep->modVarp()->isTristate()) {
                    إذا (modwidth != conwidth) {
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
                } وإلا إذا (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                إذا ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ثابت Ast_غوټه_Array_DType* ثابت بیان_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ثابت Ast_غوټه_Array_DType* ثابت modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                إذا (بیان_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_اندازه(conDTypep) != unpacked_Array_اندازه(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_اندازه(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_اندازه(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا إذا ((بیان_Arrayp && !modArrayp) || (!بیان_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (بیان_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_غوټه_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), غوټه_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_ټاکل(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            إذا (nodep->modVarp()->is_Iface_Array_وېشل()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    فراغ لیدنه(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        إذا (!m_paramsOnly) {
            إذا (VN_IS(nodep->modp(), Not_وموندل_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_لار_Looked_پیغام(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            إذا (nodep->rangep()) user_Iterate_And_بل(nodep->rangep(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->pinsp(), تش_اشاره);
            // If element of an instance array, pins are now connected, no longer need the ranges
            إذا (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ټېل_Deletep(nodep->rangep()->unlink_Fr_شا_With_بل());
            }
        }
        user_Iterate_And_بل(nodep->paramsp(), تش_اشاره);
    }
    فراغ لیدنه(AstGatePin* nodep) override {
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->rangep(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ثابت Ast_غوټه_DType* ثابت pinDTypep = nodep->exprp()->dtypep();
            ثابت صحيح عدد_Insts = nodep->rangep()->elementsConst();
            ثابت صحيح modwidth = عدد_Insts;
            ثابت صحيح pinwidth = pinDTypep->پلنوالی();
            إذا (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_غوټه_DType* ثابت subDTypep = nodep->پیدا_کړه_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{subDTypep, FINAL}.p());
                Ast_غوټه* ثابت newp
                    = جديد AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_شا(),
                                       static_cast<uint32_t>(عدد_Insts)};
                nodep->replaceWith(newp);
            } وإلا {
                إذا (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_بل(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_شا());
            }
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }
    فراغ لیدنه(Ast_غوټه_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        إذا (nodep->did_پلنوالی()) أرجع;
        إذا (nodep->doing_پلنوالی()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(ریښتیا);
            nodep->did_پلنوالی(ریښتیا);
            أرجع;
        }
        إذا (nodep->class_لاره() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } وإلا إذا (nodep->class_لاره() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        إذا (nodep->class_لاره() && nodep->name() == "new") {
            إذا (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            إذا (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_غوټه* firstp = تش_اشاره;
            لكل (Ast_غوټه* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (Ast_Stmt_بیان* ثابت sep = VN_CAST(itemp, Stmt_بیان)) {
                    إذا (AstNew* ثابت newp = VN_CAST(sep->exprp(), New)) {
                        إذا (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            اكسر;
                        }
                    }
                    استمر;
                }
                إذا (لومړی_New_Statement_Ok_Recurse(itemp)) استمر;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_پلنوالی(ریښتیا);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        إذا (nodep->fvarp()) userIterate(nodep->fvarp(), تش_اشاره);
        إذا (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_سېټ_Void();
        } وإلا إذا (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } وإلا إذا (VN_IS(nodep, Property)) {
            nodep->dtype_سېټ_بټ();
        }
        پلنوالی_VP* vup = تش_اشاره;
        إذا (VN_IS(nodep, Property)) vup = پلنوالی_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_پلنوالی(ریښتیا);
        nodep->doing_پلنوالی(خطأ);
        إذا (nodep->dpiImport() && !nodep->dpi_Open_پلار() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_پلار_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        إذا (nodep->fvarp() && !nodep->dpiImport() && !nodep->خالص_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_بل([&](ثابت AstVarRef* varrefp) {
                   أرجع varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::ویکټور<AstVar*> ports;
        std::ویکټور<Ast_Attr_د*> protos;
        Ast_Attr_د* protop = تش_اشاره;  // Base prototype
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_شا(portp); }
            } وإلا إذا (Ast_Attr_د* ثابت attrp = VN_CAST(stmtp, Attr_د)) {
                إذا (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_شا(attrp); }
                إذا (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        إذا (protop) {
            لكل (اندازه_t i = 0; i < std::max(ports.اندازه(), protos.اندازه()); ++i) {
                إذا (i >= ports.اندازه() || i >= protos.اندازه()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    اكسر;
                } وإلا {
                    AstVar* ثابت portp = ports[i];
                    Ast_Attr_د* ثابت rprotop = protos[i];
                    Ast_غوټه_DType* ثابت declDtp = portp->dtypep();
                    Ast_غوټه_DType* ثابت protoDtp = rprotop->fromp()->dtypep();
                    إذا (portp->name() != rprotop->name()) {
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
                    } وإلا إذا (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_نور() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_نور() << "... Declaration data type: "
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
    فراغ لیدنه(AstConstraint* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, تش_اشاره);
        nodep->did_پلنوالی(ریښتیا);
    }
    فراغ لیدنه(AstProperty* nodep) override {
        إذا (nodep->did_پلنوالی()) أرجع;
        إذا (nodep->doing_پلنوالی()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(ریښتیا);
            nodep->did_پلنوالی(ریښتیا);
            أرجع;
        }
        nodep->doing_پلنوالی(ریښتیا);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_سېټ_بټ();
        لكل (Ast_غوټه* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            إذا (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, تش_اشاره);
            } وإلا إذا (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } وإلا إذا (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, تش_اشاره);
            } وإلا {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_پلنوالی(ریښتیا);
        nodep->doing_پلنوالی(خطأ);
    }
    فراغ لیدنه(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    فراغ لیدنه(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        إذا (nodep->did_پلنوالی()) أرجع;
        nodep->dtype_سېټ_بټ();
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Var)) {
                userIterate(stmtp, تش_اشاره);
            } وإلا إذا (VN_IS(stmtp, غوټه_بیان)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } وإلا {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_پلنوالی(ریښتیا);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_توکی_Package(Ast_غوټه* pkgItemp) {
        طالما (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        أرجع VN_CAST(pkgItemp->backp(), Package);
    }
    فراغ لیدنه(AstFuncRef* nodep) override {
        لیدنه(static_cast<Ast_غوټه_FTask_Ref*>(nodep));
        إذا (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            إذا (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_بیان))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_سېټ_Void();
        } وإلا {
            nodep->dtypeFrom(nodep->taskp());
        }
        إذا (nodep->fileline()->timingOn()) {
            Ast_غوټه_Module* ثابت classp = nodep->classOrPackagep();
            إذا (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ثابت packagep = get_توکی_Package(classp);
                إذا (packagep && packagep->name() == "std") {
                    لاره_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ساكن منطقي are_Same_اندازه(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ثابت std::ویکټور<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ثابت std::ویکټور<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        إذا (dims0.اندازه() != dims1.اندازه()) أرجع خطأ;
        لكل (اندازه_t i = 0; i < dims0.اندازه(); ++i) {
            إذا (dims0[i]->elementsConst() != dims1[i]->elementsConst()) أرجع خطأ;
        }
        أرجع ریښتیا;
    }
    // Makes sure that port and pin have same size and same datatype
    فراغ check_Unpacked_Array_دلیلونه(AstVar* portp, Ast_غوټه* pinp) {
        إذا (AstUnpackArrayDType* ثابت پورټ_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            إذا (AstUnpackArrayDType* ثابت pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                إذا (!are_Same_اندازه(پورټ_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << پورټ_Dtypep->prettyDTypeNameQ() << ")");
                }
                ثابت AstBasicDType* ثابت پورټ_Basicp = پورټ_Dtypep->basicp();
                ثابت AstBasicDType* ثابت pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ثابت منطقي sameElemDType
                    = پورټ_Basicp && pinBasicp
                          ? پورټ_Basicp->پلنوالی() == pinBasicp->پلنوالی()
                                && پورټ_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(پورټ_Dtypep->elemDTypep());
                إذا (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, پورټ_Dtypep, "Argument", "Parameter"));
                }
            } وإلا {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    فراغ process_FTask_Ref_دلیلونه(Ast_غوټه_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->did_پلنوالی()) أرجع;
        userIterate(nodep->taskp(), تش_اشاره);
        //
        // And do the arguments to the task/function too
        افعل {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ثابت V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            إذا (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            لكل (ثابت خپلکاری& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.لومړی;
                AstArg* ثابت argp = tconnect.دوهم;
                Ast_غوټه_بیان* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                إذا (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_شا_With_بل(&handle);  // Format + additional args, if any
                    Ast_غوټه_بیان* argsp = تش_اشاره;
                    طالما (AstArg* ثابت nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->جمع_کړه_بل(nextargp->exprp()->unlink_Fr_شا_With_بل());
                        nextargp->unlink_Fr_شا()->delete_ونه();  // Remove the call's Arg wrapper
                    }
                    string format;
                    إذا (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->عدد().منزل_String();
                    } وإلا {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ټېل_Deletep(argp), argp);
                    AstSFormatF* ثابت newp
                        = جديد AstSFormatF{nodep->fileline(), format, خطأ, argsp};
                    إذا (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(جديد AstScopeName{newp->fileline(), ریښتیا});
                    }
                    handle.relink(جديد AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    اذهب reloop;  // so exit early; next loop will correct it
                }  //
                وإلا إذا (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_شا(&handle);  // No next, that's the next pin
                    Ast_غوټه_بیان* ثابت newp = جديد AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                إذا (portp->isWritable()) {
                    V3LinkLValue::link_LValue_سېټ(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, پلنوالی_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } طالما (خطأ);
        // Stage 2
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خپلکاری& tconnect : tconnects) {
                AstVar* ثابت portp = tconnect.لومړی;
                ثابت AstArg* ثابت argp = tconnect.دوهم;
                Ast_غوټه_بیان* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Change data types based on above accept completion
                إذا (nodep->taskp()->dpiImport()) check_Unpacked_Array_دلیلونه(portp, pinp);
                إذا (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خپلکاری& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.لومړی;
                ثابت AstArg* ثابت argp = tconnect.دوهم;
                Ast_غوټه_بیان* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, پلنوالی_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        إذا (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خپلکاری& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.لومړی;
                ثابت AstArg* ثابت argp = tconnect.دوهم;
                Ast_غوټه_بیان* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                Ast_غوټه_DType* ثابت پورټ_DTypep = portp->dtypep()->skip_Ref_منزل_Enump();
                إذا (VN_IS(pinp, تشه_کتار)) {
                    Ast_غوټه_بیان* newp = new_Cons_Dyn_Array_Or_کتار(pinp->fileline(), پورټ_DTypep);
                    إذا (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = جديد AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(پورټ_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ثابت Ast_غوټه_DType* ثابت pinDTypep = pinp->dtypep()->skip_Ref_منزل_Enump();
                ثابت AstIfaceRefDType* ثابت پورټ_Ifacep
                    = VN_CAST(پورټ_DTypep->elemDTypep(ریښتیا), IfaceRefDType);
                ثابت VCastable ifaceCastable
                    = پورټ_Ifacep ? Ast_غوټه::computeCastable(پورټ_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ ریښتیا)
                                 : VCastable{VCastable::UNSUPPORTED};
                ثابت منطقي matchingRefDTypes = پورټ_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(پورټ_DTypep, pinDTypep);
                ثابت منطقي pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->عدد().isNull();
                إذا ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    إذا (پورټ_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } وإلا {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << پورټ_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } وإلا إذا (پورټ_Ifacep && پورټ_Ifacep->isVirtual() && portp->is_ننوت() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } وإلا إذا (portp->isWritable() && pinp->پلنوالی() != portp->پلنوالی()) {
                    pinp->v3widthWarn(portp->پلنوالی(), pinp->پلنوالی(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->پلنوالی()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->پلنوالی() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_شا(&relinkHandle);
                    Ast_غوټه_بیان* ثابت newp = جديد AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                إذا (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(پورټ_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && پورټ_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << پورټ_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    استمر;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                إذا (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } وإلا إذا (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_ټاکل(nodep, "Function Argument", pinp, پورټ_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, پلنوالی_VP{پورټ_DTypep, FINAL, بهیر_USE_ټاکل}.p());
                } وإلا {
                    iterate_Check_ټاکل(nodep, "Function Argument", pinp, FINAL, پورټ_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    فراغ handle_Std_Randomize_دلیلونه(Ast_غوټه_FTask_Ref* ثابت nodep) {
        AstConst* nullp = تش_اشاره;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_غوټه_بیان* ثابت exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->عدد().isNull()) {
                    nullp = constp;
                    استمر;
                }
            }
            إذا (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                استمر;
            }
            إذا (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                استمر;
            }
            إذا (ثابت AstCMethodHard* ثابت methodp = VN_CAST(exprp, CMethodHard)) {
                إذا (methodp->لاره() == VCMethod::ARRAY_AT
                    || methodp->لاره() == VCMethod::ARRAY_AT_WRITE) {
                    استمر;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        إذا (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    فراغ لیدنه(Ast_غوټه_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        إذا (nodep->did_پلنوالی()) أرجع;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = تش_اشاره;
        إذا (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(ریښتیا);
            nodep->dtypep(nodep->پیدا_کړه_Basic_DType(VBasicDTypeKwd::INT));
            أرجع;  // Handled in V3Randomize
        } وإلا إذا (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ثابت classp = VN_CAST(nodep->classOrPackagep(), Class);
            إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(ریښتیا);
                Ast_غوټه_DType* ثابت adtypep = nodep->پیدا_کړه_بټ_DType();
                withp = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کړه_Void_DType(),
                                         adtypep->پیدا_کړه_بټ_DType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_بل(argp->exprp(), پلنوالی_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_دلیلونه(nodep);  // Provided args should be in current scope
                process_FTask_Ref_دلیلونه(nodep);
                إذا (withp) nodep->withp(withp);
                nodep->did_پلنوالی(ریښتیا);
                أرجع;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            إذا (nodep->name() == "randomize") {
                AstClassRefDType* ثابت adtypep
                    = جديد AstClassRefDType{nodep->fileline(), classp, تش_اشاره};
                v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(adtypep);
                withp = لاره_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کړه_Void_DType(),
                                         adtypep->پیدا_کړه_بټ_DType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_بل(argp->exprp(), پلنوالی_VP{SELF, BOTH}.p());
                }
                handle_Randomize_دلیلونه(nodep, classp);
            } وإلا إذا (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } وإلا إذا (nodep->name() == "get_randstate") {
                لاره_Ok_Arguments(nodep, 0, 0);
                classp->بنسټ_Most_Classp()->needRNG(ریښتیا);
                v3Global.useRandomizeMethods(ریښتیا);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_سېټ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                لاره_Ok_Arguments(nodep, 1, 1);
                Ast_غوټه_بیان* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_غوټه_بیان* ثابت exprp = nodep->argsp()->exprp();
                classp->بنسټ_Most_Classp()->needRNG(ریښتیا);
                v3Global.useRandomizeMethods(ریښتیا);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->جمع_کړه(exprp->unlink_Fr_شا());
                newp->جمع_کړه(")");
                newp->dtype_سېټ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                UASSERT_OBJ(خطأ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->did_پلنوالی()) {
            إذا (withp) nodep->withp(withp);
            أرجع;
        }
        إذا ((nodep->taskp()->class_لاره() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            منطقي allow = خطأ;
            إذا (m_ftaskp && m_ftaskp->class_لاره() && !m_ftaskp->isStatic()) {
                إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                إذا (!allow) {
                    ثابت AstClass* callerClassp = m_containingClassFinder.پیدا_کړه(m_ftaskp);
                    إذا (!callerClassp) {
                        callerClassp = m_containingClassFinder.پیدا_کړه(m_ftaskp->classOrPackagep());
                    }
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.پیدا_کړه(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            إذا (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ثابت AstClass* ثابت cgClassp = m_containingClassFinder.پیدا_کړه(nodep);
                إذا (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.پیدا_کړه(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            إذا (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        إذا (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(جديد AstScopeName{nodep->fileline(), خطأ});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_دلیلونه(nodep);
        إذا (withp) nodep->withp(withp);
        nodep->did_پلنوالی(ریښتیا);
        // See steps that follow in visit(AstFuncRef*)
    }
    فراغ لیدنه(Ast_غوټه_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, تش_اشاره);
    }
    فراغ لیدنه(Ast_Sen_توکی* nodep) override {
        إذا (nodep->is_Combo_ستوری()) أرجع;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        إذا (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            خپلکاری* ثابت concatOrReplp = VN_CAST(nodep->sensp(), غوټه_Biop);
            خپلکاری* ثابت rhsp = concatOrReplp->rhsp()->unlink_Fr_شا();
            إذا (nodep->څنډه_Type() == VEdgeType::ET_بدل_شوی) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                خپلکاری* ثابت lhsp = concatOrReplp->lhsp()->unlink_Fr_شا();
                nodep->جمع_کړه_بل_دلته(جديد Ast_Sen_توکی{lhsp->fileline(), nodep->څنډه_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(جديد Ast_Sen_توکی{rhsp->fileline(), nodep->څنډه_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_ونه(), nodep);
        } وإلا {
            userIterateChildren(nodep, پلنوالی_VP{SELF, BOTH}.p());
            إذا (nodep->څنډه_Type().an_څنډه()) {
                Ast_غوټه_DType* ثابت sensDtp = nodep->sensp()->dtypep()->skipRefp();
                إذا (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } وإلا إذا (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    فراغ لیدنه(Ast_Clocking_توکی* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ثابت refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, پلنوالی_VP{SELF, PRELIM}.p());
    }
    فراغ لیدنه(AstWait* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_شا()->delete_ونه(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_سېټ_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                إذا (Ast_غوټه_Var_Ref* ثابت varrefp = VN_CAST(nodep->condp(), غوټه_Var_Ref)) {
                    إذا (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_نور() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_سېټ_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        إذا (Ast_غوټه* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_شا_With_بل();
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlink_Fr_شا();
        }
        VL_DO_DANGLING(nodep->delete_ونه(), nodep);
    }
    فراغ لیدنه(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->شاخص_Arg_Refp(), تش_اشاره);
        userIterateChildren(nodep->ارزښت_Arg_Refp(), تش_اشاره);
        إذا (!nodep->exprp()) {
            nodep->dtype_سېټ_Void();
        } وإلا {
            إذا (!nodep->exprp()->hasDType()) {
                user_Iterate_And_بل(nodep->exprp(), تش_اشاره);
            } وإلا {
                إذا (vdtypep) {
                    user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{nodep->dtypep(), PRELIM}.p());
                } وإلا {  // 'sort with' allows arbitrary type
                    user_Iterate_And_بل(nodep->exprp(), پلنوالی_VP{SELF, PRELIM}.p());
                }
            }

            إذا (!nodep->exprp()->hasDType()) {
                nodep->dtype_سېټ_Void();
            } وإلا {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_ټاکل(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    فراغ لیدنه(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        إذا (nodep->شاخص()) {
            nodep->dtypeFrom(m_withp->شاخص_Arg_Refp());
        } وإلا {
            nodep->dtypeFrom(m_withp->ارزښت_Arg_Refp());
        }
    }
    فراغ لیدنه(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, تش_اشاره);
    }

    //--------------------
    // Default
    فراغ لیدنه(Ast_غوټه_بیان* nodep) override {
        إذا (!nodep->did_پلنوالی()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, تش_اشاره);
    }
    فراغ لیدنه(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << کرښه_پای);
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        إذا (Ast_غوټه_DType* ثابت vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } وإلا إذا (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->پلنوالی() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->پلنوالی() << " bits -> " << nodep->پلنوالی()
                                    << " bits" << کرښه_پای);

        // Final processing
        user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, FINAL}.p());
        nodep->did_پلنوالی(ریښتیا);
    }
    فراغ لیدنه_Class(AstClass* nodep) {
        إذا (nodep->did_پلنوالی_And_سېټ()) أرجع;

        // If the class is std::process
        إذا (nodep->name() == "process") {
            AstPackage* ثابت packagep = get_توکی_Package(nodep);
            إذا (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                إذا (AstVar* ثابت varp
                    = VN_CAST(m_memberMap.پیدا_کړه_Member(nodep, "m_process"), Var)) {
                    Ast_غوټه_DType* ثابت dtypep = varp->get_ماشوم_DTypep();
                    إذا (!varp->dtypep()) {
                        VL_DO_DANGLING(ټېل_Deletep(dtypep->unlink_Fr_شا()), dtypep);
                    }
                    AstBasicDType* ثابت newdtypep = جديد AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                إذا (Ast_غوټه_FTask* ثابت ftaskp
                    = VN_CAST(m_memberMap.پیدا_کړه_Member(nodep, "self"), غوټه_FTask)) {
                    ftaskp->سېټ_Need_Process();
                    v3Global.سېټ_کاروي_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_بل(nodep->extendsp(), تش_اشاره);
        userIterateChildren(nodep, تش_اشاره);  // First size all members
    }
    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_لنډمهاله_نومونه);
        VL_RESTORER_COPY(m_inside_لنډمهاله_نومونه);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            لیدنه_Class(classp);
        } وإلا {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([هذا](ثابت AstVar* varp) { m_curModVars.ورننباسه(varp); });
            userIterateChildren(nodep, تش_اشاره);
        }
    }
    فراغ لیدنه(Ast_غوټه* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, تش_اشاره);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    فراغ لیدنه_Or_Lu64(Ast_غوټه_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سېټ_Double();
            Ast_غوټه_DType* ثابت subDTypep = nodep->پیدا_کړه_بټ_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    فراغ لیدنه(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سېټ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM}.p());
            إذا (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    جديد AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_شا()});
                VL_DO_DANGLING(nodep->delete_ونه(), nodep);
            }
        }
    }
    فراغ لیدنه(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_سېټ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM}.p());
        }
    }
    فراغ لیدنه_Os32_Lr(Ast_غوټه_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_Integer2_State();
        }
    }
    فراغ لیدنه_Ou64_Lr(Ast_غوټه_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_UInt64();
        }
    }

    فراغ لیدنه_log_not(AstLogNot* nodep) {
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
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_سېټ_بټ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            إذا (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ثابت newp
                    = جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            }
        }
    }
    فراغ لیدنه_log_and_or(Ast_غوټه_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ_Or_Logic(
                Ast_غوټه_بیان::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    فراغ لیدنه_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه_red_and_or(Ast_غوټه_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_سېټ_بټ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    فراغ لیدنه_red_unknown(Ast_غوټه_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            nodep->dtype_سېټ_بټ();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    منطقي isEquivalentDType(ثابت Ast_غوټه_DType* lhs, ثابت Ast_غوټه_DType* rhs) {
        // a) If two types match, they are equivalent.
        إذا (!lhs || !rhs) أرجع خطأ;
        lhs = lhs->skip_Ref_منزل_Enump();
        rhs = rhs->skip_Ref_منزل_Enump();
        إذا (lhs == rhs) أرجع ریښتیا;
        // If both are basic types, check if they are the same type
        إذا (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ثابت خپلکاری* lb = VN_CAST(lhs, BasicDType);
            ثابت خپلکاری* rb = VN_CAST(rhs, BasicDType);
            إذا (lb->isString() != rb->isString()) أرجع خطأ;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ثابت منطقي lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ثابت منطقي rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        إذا (lhsIsUnpackArray || rhsIsUnpackArray) {
            إذا (lhsIsUnpackArray && rhsIsUnpackArray) {
                ثابت AstUnpackArrayDType* ثابت lhsp = VN_CAST(lhs, UnpackArrayDType);
                ثابت AstUnpackArrayDType* ثابت rhsp = VN_CAST(rhs, UnpackArrayDType);
                ثابت صحيح lsz = lhsp->elementsConst();
                ثابت صحيح rsz = rhsp->elementsConst();
                إذا (lsz >= 0 && rsz >= 0 && lsz != rsz) أرجع خطأ;
                أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            أرجع خطأ;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ثابت منطقي lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ثابت منطقي rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ثابت منطقي lhs_Is_کتار = VN_IS(lhs, کتار_DType);
        ثابت منطقي rhs_Is_کتار = VN_IS(rhs, کتار_DType);
        ثابت منطقي lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ثابت منطقي rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        إذا (lhsIsDynArray || rhsIsDynArray || lhs_Is_کتار || rhs_Is_کتار || lhsIsAssocArray
            || rhsIsAssocArray) {
            إذا (ثابت AstDynArrayDType* ثابت lhsp = VN_CAST(lhs, DynArrayDType)) {
                إذا (ثابت AstDynArrayDType* ثابت rhsp = VN_CAST(rhs, DynArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت Ast_کتار_DType* ثابت lhsp = VN_CAST(lhs, کتار_DType)) {
                إذا (ثابت Ast_کتار_DType* ثابت rhsp = VN_CAST(rhs, کتار_DType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت AstAssocArrayDType* ثابت lhsp = VN_CAST(lhs, AssocArrayDType)) {
                إذا (ثابت AstAssocArrayDType* ثابت rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->کلی_DTypep(), rhsp->کلی_DTypep());
                }
            }

            أرجع خطأ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        إذا (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            إذا (lhs->پلنوالی() != rhs->پلنوالی()) أرجع خطأ;
            إذا (lhs->isFourstate() != rhs->isFourstate()) أرجع خطأ;
            إذا (lhs->isSigned() != rhs->isSigned()) أرجع خطأ;
            أرجع ریښتیا;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        إذا (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) أرجع خطأ;
        أرجع !(VN_IS(lhs, غوټه_UOr_Struct_DType) || VN_IS(rhs, غوټه_UOr_Struct_DType));
    }

    ساكن منطقي isAggregateType(ثابت Ast_غوټه* nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت Ast_غوټه_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) أرجع خطأ;
        dtypep = dtypep->skipRefp();
        إذا (!dtypep) أرجع خطأ;
        أرجع dtypep->isAggregateType();
    }

    فراغ لیدنه_cmp_eq_gt(Ast_غوټه_Biop* nodep, ثابت ComparisonType cmpType) {
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
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ثابت منطقي lhsIsPat = VN_IS(nodep->lhsp(), نمونه);
            ثابت منطقي rhsIsPat = VN_IS(nodep->rhsp(), نمونه);
            إذا (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } وإلا إذا (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } وإلا {
                user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            }

            ثابت منطقي isAggrLhs = isAggregateType(nodep->lhsp());
            ثابت منطقي isAggrRhs = isAggregateType(nodep->rhsp());

            إذا ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::نورمال_EQ || cmpType == ComparisonType::CASE)) {
                ثابت Ast_غوټه_DType* ثابت lhsDType = nodep->lhsp()->dtypep();
                ثابت Ast_غوټه_DType* ثابت rhsDType = nodep->rhsp()->dtypep();

                إذا (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_غوټه* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_ترتیب(nodep->rhsp(), nodep->lhsp()->dtypep());
            } وإلا إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                إذا (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_غوټه* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::بټ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (Ast_غوټه_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                إذا (Ast_غوټه_Biop* ثابت newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا {
                ثابت منطقي signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                إذا (Ast_غوټه_Biop* ثابت newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ثابت صحيح پلنوالی = std::max(nodep->lhsp()->پلنوالی(), nodep->rhsp()->پلنوالی());
                ثابت صحيح ewidth = std::max(nodep->lhsp()->پلنوالی_Min(), nodep->rhsp()->پلنوالی_Min());
                منطقي warnOn = ریښتیا;
                إذا (!signedFl && پلنوالی == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    إذا ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->پلنوالی() >= nodep->rhsp()->پلنوالی_Min())) {
                        warnOn = خطأ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    إذا ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->پلنوالی_Min() >= nodep->rhsp()->پلنوالی())) {
                        warnOn = خطأ;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->پیدا_کړه_بټ_Or_Logic_DType(پلنوالی, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_سېټ_بټ_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::نورمال_EQ
                                               || cmpType == ComparisonType::نورمال_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    فراغ لیدنه_cmp_real(Ast_غوټه_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه_cmp_string(Ast_غوټه_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_بټ();
        }
    }
    فراغ لیدنه_cmp_type(Ast_غوټه_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, BOTH}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{SELF, BOTH}.p());
            ثابت Ast_Attr_د* ثابت lhsap = VN_AS(nodep->lhsp(), Attr_د);
            ثابت Ast_Attr_د* ثابت rhsap = VN_AS(nodep->rhsp(), Attr_د);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ثابت Ast_غوټه_DType* ثابت lhsDtp = lhsap->dtypep();
            ثابت Ast_غوټه_DType* ثابت rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ثابت منطقي invert = VN_IS(nodep, NeqT);
            ثابت منطقي identical
                = Ast_غوټه::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ثابت منطقي eq = invert ^ identical;
            Ast_غوټه* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::بټ_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        }
    }

    فراغ لیدنه_negate_not(Ast_غوټه_Uniop* nodep, منطقي real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) checkCvtUS(nodep->lhsp(), خطأ);
        }
        إذا (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            إذا (Ast_غوټه_Uniop* ثابت newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_سېټ_Double();
                أرجع;
            }
        } وإلا {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        إذا (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        إذا (m_vup->final()) {
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            // Some warning suppressions
            منطقي lhsWarn = ریښتیا;
            إذا (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->پلنوالی_Min() == (nodep->lhsp()->پلنوالی_Min() + 1)) lhsWarn = خطأ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    فراغ لیدنه_signed_unsigned(Ast_غوټه_Uniop* nodep, VSigning rs_بهر) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ریښتیا);
            ثابت صحيح پلنوالی = nodep->lhsp()->پلنوالی();
            Ast_غوټه_DType* ثابت expDTypep = nodep->پیدا_کړه_بټ_Or_Logic_DType(
                پلنوالی, پلنوالی, rs_بهر, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ لیدنه_ښويدنه_2(Ast_غوټه_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_ښويدنه_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ثابت Ast_غوټه_Biop* ثابت newp = iterate_ښويدنه_final(nodep);
        VL_DANGLING(nodep);
        (فراغ)newp;  // Ununused
    }
    فراغ iterate_ښويدنه_prelim(Ast_غوټه_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            إذا (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_سېټ_Logic_Unsized(nodep->پلنوالی(), nodep->پلنوالی_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_غوټه_Biop* iterate_ښويدنه_final(Ast_غوټه_Biop* nodep) {
        // Nodep maybe edited
        assert_At_بیان(nodep);
        إذا (m_vup->final()) {
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            إذا (VN_IS(nodep, ښويدنه_RS)) {
                إذا (Ast_غوټه_Biop* ثابت newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            منطقي warnOn = ریښتیا;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            إذا (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), غوټه_ټاکل)) warnOn = خطأ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const);
            إذا (shiftp && !shiftp->عدد().isFourState() && shiftp->پلنوالی() <= 32) {
                ثابت int64_t ښويدنه_Val = shiftp->عدد().منزل_SQuad();
                إذا (VN_IS(nodep, ښويدنه_L)) {
                    إذا (ښويدنه_Val > 0 && nodep->پلنوالی() == nodep->lhsp()->پلنوالی() + ښويدنه_Val)
                        warnOn = خطأ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            إذا (nodep->rhsp()->پلنوالی() > 32) {
                إذا (shiftp && shiftp->عدد().most_سېټ_بټ_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_شمېره عدد{shiftp, 32, shiftp->عدد()};
                    Ast_غوټه* ثابت shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(جديد AstConst{shiftrhsp->fileline(), عدد});
                    VL_DO_DANGLING(shiftrhsp->delete_ونه(), shiftrhsp);
                }
            }
        }
        أرجع nodep;  // May edit
    }

    فراغ لیدنه_boolexpr_and_or(Ast_غوټه_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            checkCvtUS(nodep->rhsp(), خطأ);
            ثابت صحيح پلنوالی = std::max(nodep->lhsp()->پلنوالی(), nodep->rhsp()->پلنوالی());
            ثابت صحيح mwidth = std::max(nodep->lhsp()->پلنوالی_Min(), nodep->rhsp()->پلنوالی_Min());
            ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                پلنوالی, mwidth, VSigning::fromBool(expSigned),
                Ast_غوټه_بیان::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        إذا (m_vup->final()) {
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ لیدنه_جمع_کړه_sub_replace(Ast_غوټه_Biop* nodep, منطقي real_ok, ثابت منطقي modOrDiv) {
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
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_بل(nodep->lhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بل(nodep->rhsp(), پلنوالی_VP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) {
                checkCvtUS(nodep->lhsp(), خطأ);
                checkCvtUS(nodep->rhsp(), خطأ);
            }
            إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                إذا (Ast_غوټه_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_سېټ_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                أرجع;
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, جمع_کړه_2)
                            ? (nodep->warn_نور()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                ثابت صحيح پلنوالی = std::max(nodep->lhsp()->پلنوالی(), nodep->rhsp()->پلنوالی());
                ثابت صحيح mwidth = std::max(nodep->lhsp()->پلنوالی_Min(), nodep->rhsp()->پلنوالی_Min());
                ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_سېټ_بټ_Or_Logic_Unsized(
                    پلنوالی, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_غوټه_بیان::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_غوټه_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_غوټه_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            إذا (Ast_غوټه_Biop* ثابت newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            منطقي lhsWarn = ریښتیا;
            منطقي rhsWarn = ریښتیا;
            إذا (VN_IS(nodep, جمع_کړه_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->پلنوالی_Min() == (nodep->lhsp()->پلنوالی_Min() + 1)) lhsWarn = خطأ;
                إذا (subDTypep->پلنوالی_Min() == (nodep->rhsp()->پلنوالی_Min() + 1)) rhsWarn = خطأ;
                إذا (VN_IS(nodep, جمع_کړه_2) && nodep->lhsp()->پلنوالی() == 1
                    && nodep->rhsp()->پلنوالی() != 1)
                    lhsWarn = خطأ;  // do_increment + ...
                إذا (nodep->rhsp()->پلنوالی() == 1 && nodep->lhsp()->پلنوالی() != 1)
                    rhsWarn = خطأ;  // ... + do_increment
            } وإلا إذا (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                إذا (subDTypep->پلنوالی_Min() >= (nodep->lhsp()->پلنوالی_Min())) lhsWarn = خطأ;
                إذا (subDTypep->پلنوالی_Min() >= (nodep->rhsp()->پلنوالی_Min())) rhsWarn = خطأ;
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
    فراغ لیدنه_real_جمع_کړه_sub(Ast_غوټه_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_سېټ_Double();
        }
    }
    فراغ لیدنه_real_neg_ceil(Ast_غوټه_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_بیان(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_سېټ_Double();
        }
    }

    فراغ لیدنه_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->بیان_Format_2(), nodep, "Assumes constant format");
        منطقي دننه_Pct = خطأ;
        Ast_غوټه_بیان* argp = nodep->exprsp();
        string newFormat;
        لكل (حرف ch : nodep->متن()) {
            إذا (!دننه_Pct && ch == '%') {
                دننه_Pct = ریښتیا;
                newFormat += ch;
            } وإلا إذا (دننه_Pct && (std::که_عدد(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } وإلا إذا (!دننه_Pct) {  // Normal text
                newFormat += ch;
            } وإلا {
                دننه_Pct = خطأ;
                Ast_غوټه_بیان* ثابت nextp = argp ? VN_AS(argp->nextp(), غوټه_بیان) : تش_اشاره;
                AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_غوټه_بیان* subargp = fargp ? fargp->exprp() : argp;
                ثابت Ast_غوټه_DType* ثابت dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : تش_اشاره;
                ch = std::tolower(ch);
                بدل (ch) {
                حالة '?':  // Unspecified by user, guess
                    إذا (dtypep->isDouble()) {
                        ch = 'g';
                    } وإلا إذا (dtypep->isString()) {
                        ch = 's';
                    } وإلا إذا (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } وإلا {
                        ch = 'p';
                    }
                    argp = nextp;
                    اكسر;
                حالة '%': اكسر;  // %% - just output a %
                حالة 'm': اكسر;  // %m - auto insert "name"
                حالة 'l': اكسر;  // %m - auto insert "library"
                حالة 'd':
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        إذا (subargp->isDouble()) spliceCvtS(subargp, ریښتیا, 64);
                        argp = nextp;
                    }
                    اكسر;
                حالة 'b':  // FALLTHRU
                حالة 'o':  // FALLTHRU
                حالة 'x':
                    إذا (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    اكسر;
                حالة 't':  // Decimal/float time
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    اكسر;
                حالة 'e':  // FALLTHRU
                حالة 'f':  // FALLTHRU
                حالة 'g':
                    إذا (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        إذا (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    اكسر;
                حالة 'p':  // FALLTHRU
                حالة 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    إذا (subargp && !fargp) {
                        إذا (AstEnumDType* ثابت enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_شا(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    اكسر;
                افتراضي:  // Most operators, just move to next argument
                    argp = nextp;
                    اكسر;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->متن(newFormat);
    }

    ساكن AstEnumDType* formatEnumDType(Ast_غوټه_بیان* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
        إذا (!enumDtp) {
            إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_منزل_Enump(), EnumDType);
            }
        }
        أرجع enumDtp;
    }

    Ast_بیان_Stmt* newEnumCapture(Ast_غوټه_بیان* valuep, AstEnumDType* dtypep) {
        File_کرښه* ثابت flp = valuep->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::XTEMP, m_enum_لنډمهاله_نومونه.get(valuep), dtypep};
        varp->noSubst(ریښتیا);
        إذا (m_ftaskp) {
            varp->funcLocal(ریښتیا);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } وإلا {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->جمع_کړه_Stmtsp(varp);
        }
        iterate(varp);
        Ast_بیان_Stmt* ثابت exprp = جديد Ast_بیان_Stmt{
            flp, جديد Ast_ټاکل{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            جديد AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        أرجع exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_غوټه_بیان* valuep, AstEnumDType* dtypep) {
        إذا (valuep->is_خالص()) valuep = V3Const::constifyEdit(valuep);
        Ast_غوټه_بیان* keyp;
        إذا (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_ونه(خطأ);
        } وإلا {
            Ast_بیان_Stmt* ثابت capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_ونه_خالص(خطأ);
        }
        ثابت VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ثابت argp = جديد AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        أرجع argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    منطقي پلنوالی_Bad(Ast_غوټه* nodep, Ast_غوټه_DType* expDTypep) {
        ثابت صحيح exp_پلنوالی = expDTypep->پلنوالی();
        صحيح exp_پلنوالی_Min = expDTypep->پلنوالی_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        إذا (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) أرجع خطأ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->پلنوالی() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_پلنوالی != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        إذا (exp_پلنوالی_Min == 0) exp_پلنوالی_Min = exp_پلنوالی;
        إذا (nodep->dtypep()->پلنوالی() == exp_پلنوالی) أرجع خطأ;
        إذا (nodep->dtypep()->پلنوالی_Sized() && nodep->پلنوالی() != exp_پلنوالی_Min) أرجع ریښتیا;
        إذا (!nodep->dtypep()->پلنوالی_Sized() && nodep->پلنوالی_Min() > exp_پلنوالی_Min) أرجع ریښتیا;
        أرجع خطأ;
    }

    فراغ fix_پلنوالی_Extend(Ast_غوټه_بیان* nodep, Ast_غوټه_DType* expDTypep, Extend_قاعده extend_قاعده) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<صحيح>(extend_قاعده) << ") old: " << nodep);
        إذا (extend_قاعده == EXTEND_OFF) أرجع;
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        ثابت صحيح exp_پلنوالی = expDTypep->پلنوالی();
        إذا (constp && !constp->عدد().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_شمېره عدد{nodep, exp_پلنوالی, constp->عدد()};
            عدد.isSigned(خطأ);
            Ast_غوټه_بیان* ثابت newp = جديد AstConst{nodep->fileline(), عدد};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ټېل_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا إذا (exp_پلنوالی < nodep->پلنوالی()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            Ast_غوټه_بیان* ثابت newp = جديد AstSel{nodep->fileline(), nodep, 0, exp_پلنوالی};
            newp->did_پلنوالی(ریښتیا);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } وإلا {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            منطقي doSigned = خطأ;
            بدل (extend_قاعده) {
            حالة EXTEND_ZERO: doSigned = خطأ; اكسر;
            حالة EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); اكسر;
            حالة EXTEND_LHS: doSigned = nodep->isSigned(); اكسر;
            افتراضي: nodep->v3fatalSrc("bad case");
            }
            Ast_غوټه_بیان* ثابت newp
                = (doSigned ? static_cast<Ast_غوټه_بیان*>(جديد AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_غوټه_بیان*>(جديد AstExtend{nodep->fileline(), nodep}));
            newp->did_پلنوالی(ریښتیا);
            linker.relink(newp);
            nodep = newp;
        }
        إذا (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_غوټه_بیان* ثابت newp = spliceCvtD(nodep);
            nodep = newp;
        }
        إذا (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_پلنوالی_Signed(exp_پلنوالی, expDTypep->پلنوالی_Min(), expDTypep->numeric());
        } وإلا {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    فراغ fix_پلنوالی_Reduce(Ast_غوټه_بیان* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ثابت صحيح exp_پلنوالی = 1;
        ثابت صحيح expSigned = خطأ;
        إذا (nodep->پلنوالی() == exp_پلنوالی) أرجع;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (constp) {
            V3_شمېره عدد(nodep, exp_پلنوالی);
            عدد.opRedOr(constp->عدد());
            عدد.isSigned(expSigned);
            Ast_غوټه_بیان* ثابت newp = جديد AstConst{nodep->fileline(), عدد};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_ونه(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا {
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            Ast_غوټه_بیان* newp;
            Ast_غوټه_DType* ثابت dtypeSkipp = nodep->dtypep()->skipRefp();
            إذا (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = جديد AstNeq{nodep->fileline(),
                                  جديد AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } وإلا {
                // But, AstRedOr can be faster on wide
                newp = جديد AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_پلنوالی_Signed(exp_پلنوالی, exp_پلنوالی, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    منطقي fixAutoExtend(Ast_غوټه_بیان*& nodepr, صحيح exp_پلنوالی) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        إذا (AstConst* ثابت constp = VN_CAST(nodepr, Const)) {
            إذا (constp->عدد().autoExtend() && !constp->عدد().sized() && constp->پلنوالی() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_شمېره عدد(constp, exp_پلنوالی);
                عدد.opRepl(constp->عدد(), exp_پلنوالی);  // {width{'1}}
                Ast_غوټه_بیان* ثابت newp = جديد AstConst{constp->fileline(), عدد};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ونه(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع ریښتیا;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            وإلا إذا (!constp->عدد().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_پلنوالی > 32 && constp->عدد().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_پلنوالی
                                                  << " bits: " << constp->prettyName());
                V3_شمېره عدد(constp, exp_پلنوالی);
                عدد.opExtendXZ(constp->عدد(), constp->پلنوالی());
                Ast_غوټه_بیان* ثابت newp = جديد AstConst{constp->fileline(), عدد};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ونه(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع ریښتیا;
            }
        }
        أرجع خطأ;  // No change
    }
    منطقي is_بنسټ_Class_Recurse(ثابت AstClass* ثابت cls1p, ثابت AstClass* ثابت cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        إذا (cls1p == cls2p) أرجع ریښتیا;
        لكل (ثابت AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            إذا (is_بنسټ_Class_Recurse(cls1p, cextp->classp())) أرجع ریښتیا;
        }
        أرجع خطأ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ساكن string warnDTypes(ثابت Ast_غوټه* nodep, ثابت Ast_غوټه_DType* lhsDtp,
                             ثابت Ast_غوټه_DType* rhsDtp, ثابت string& lhsName = "Left-hand",
                             ثابت string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        إذا (lhsState == rhsState) lhsState = rhsState = "";
        أرجع "\n" + nodep->warn_نور() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_نور()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ساكن منطقي unpacked_Array_Sizes_سمون(ثابت Ast_غوټه_DType* lhsDtp, ثابت Ast_غوټه_DType* rhsDtp) {
        طالما (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ثابت AstUnpackArrayDType* ثابت lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ثابت AstUnpackArrayDType* ثابت rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            إذا (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                أرجع خطأ;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        أرجع ریښتیا;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    کينډۍ <ډول_نوم T, ډول_نوم N>
    فراغ checkUnpackedArrayAssignmentCompatible(ثابت Ast_غوټه* nodep, ثابت T* ثابت lhsRefp,
                                                ثابت N* ثابت rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::ارزښت || std::is_same<T, Ast_غوټه_Var_Ref>::ارزښت)
                && (std::is_same<N, AstVar>::ارزښت || std::is_same<N, Ast_غوټه_Var_Ref>::ارزښت),
            "Unsupported types provided.");
        إذا (!lhsRefp || !rhsRefp) أرجع;
        std::string lhsName;
        std::string rhsName;
        إذا (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } وإلا {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ثابت Ast_غوټه_DType* ثابت lhsDtp = lhsRefp->dtypep()->skipRefp();
        ثابت Ast_غوټه_DType* ثابت rhsDtp = rhsRefp->dtypep()->skipRefp();
        ثابت منطقي isLhsAggregate = lhsDtp->isAggregateType();
        ثابت منطقي isRhsAggregate = rhsDtp->isAggregateType();
        إذا (!isLhsAggregate && !isRhsAggregate) أرجع;
        إذا (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        } وإلا إذا (VN_IS(lhsDtp, کتار_DType) && VN_IS(rhsDtp, تشه_کتار_DType)) {
            أرجع;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(خطأ),
                                      rhsDim = rhsDtp->dimensions(خطأ);
        // Check if unpacked array dimensions are matching
        إذا (lhsDim.دوهم != rhsDim.دوهم) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        }

        ثابت Ast_غوټه_DType* lhsDtpIterp = lhsDtp;
        ثابت Ast_غوټه_DType* rhsDtpIterp = rhsDtp;
        لكل (uint32_t dim = 0; dim < rhsDim.دوهم; dim++) {
            // Associative arrays are compatible only with each other
            إذا (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                أرجع;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        إذا (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    فراغ check_Class_ټاکل(ثابت Ast_غوټه* nodep, ثابت حرف* side, Ast_غوټه* rhsp,
                          Ast_غوټه_DType* ثابت lhsDTypep, منطقي isOutputArg = خطأ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ثابت Ast_غوټه_DType* ثابت lhsRawDTypep = lhsDTypep->skipRefp();
        ثابت Ast_غوټه_DType* ثابت rhsRawDTypep = rhsp->dtypep()->skipRefp();
        إذا (ثابت AstClassRefDType* ثابت lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            إذا (ثابت AstClassRefDType* ثابت rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ثابت AstClass* ثابت dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ثابت AstClass* ثابت srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                إذا (is_بنسټ_Class_Recurse(dstp, srcp)) أرجع;
            } وإلا إذا (rhsp->isNull()) {
                أرجع;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } وإلا إذا (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        إذا (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_شا(&relinker);
            relinker.relink(
                جديد Ast_Cvt_Unpack_منزل_کتار{rhsp->fileline(), VN_AS(rhsp, غوټه_بیان), lhsDTypep});
        }
    }
    ساكن منطقي similarDTypeRecurse(ثابت Ast_غوټه_DType* ثابت node1p,
                                    ثابت Ast_غوټه_DType* ثابت node2p) {
        أرجع node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    فراغ iterateCheckFileDesc(Ast_غوټه* parentp, Ast_غوټه* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, PRELIM}.p());
        Ast_غوټه_DType* ثابت expDTypep = underp->پیدا_کړه_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckReal(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کړه_Double_DType(), stage);
    }
    فراغ iterateCheckSigned8(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کړه_Signed8_DType(), stage);
    }
    فراغ iterateCheckSigned32(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کړه_Int_DType(), stage);
    }
    فراغ iterateCheckUInt32(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کړه_UInt32_DType(), stage);
    }
    فراغ iterateCheckDelay(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            Ast_غوټه_DType* expDTypep;
            إذا (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->پیدا_کړه_Double_DType();
            } وإلا {
                File_کرښه* ثابت newFl = جديد File_کرښه{underp->fileline()};
                newFl->warnOff(V3_Error_کوډ::WIDTHEXPAND, ریښتیا);
                underp->fileline(newFl);
                expDTypep = parentp->پیدا_کړه_بټ_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, خطأ);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckString(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->پیدا_کړه_String_DType(), stage);
    }
    فراغ iterateCheckTyped(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp,
                           Ast_غوټه_DType* expDTypep, Stage stage) {
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{expDTypep, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckTypedSelfPrelim(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp,
                                     Ast_غوټه_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckIntegralSelf(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, ریښتیا);
    }
    فراغ iterateCheckSelf(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Determ determ,
                          Stage stage, منطقي integralOnly = خطأ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, غوټه_بیان) ? checkCvtUS(VN_AS(underp, غوټه_بیان), integralOnly) : underp;
        Ast_غوټه_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckSizedSelf(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Determ determ,
                               Stage stage, منطقي deferUntyped = خطأ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, غوټه_بیان) ? checkCvtUS(VN_AS(underp, غوټه_بیان), خطأ) : underp;
        Ast_غوټه_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_غوټه_DType* ثابت checkDtp = expDTypep->skip_Ref_منزل_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        إذا (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterate_Check_ټاکل(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* rhsp, Stage stage,
                            Ast_غوټه_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        إذا (AstConst* constp = VN_CAST(rhsp, Const)) {
            إذا (ثابت AstUnpackArrayDType* ثابت arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                إذا (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    إذا (basicp->پلنوالی() == 8 && constp->عدد().isFromString()) {
                        AstConsArray* newp = جديد AstConsArray{
                            constp->fileline(), lhsDTypep,
                            جديد AstConst{constp->fileline(), AstConst::Widthed_ارزښت{}, 8, 0}};
                        لكل (ثابت صحيح aindex : arrayp->declRange().seq_Lo_منزل_Hi()) {
                            صحيح cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_شمېره selected{constp, 8};
                            selected.opSel(constp->عدد(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            إذا (!selected.isFourState()) {
                                إذا (ثابت uint32_t c = selected.منزل_UInt()) {
                                    newp->valuep(aindex,
                                                 جديد AstConst{constp->fileline(),
                                                              AstConst::Widthed_ارزښت{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ټېل_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_ټاکل(parentp, side, rhsp, lhsDTypep);
        ثابت منطقي lhs_بهیر = (VN_IS(parentp, غوټه_ټاکل)
                                && VN_IS(VN_AS(parentp, غوټه_ټاکل)->lhsp(), غوټه_بهیر));
        rhsp = iterateCheck(parentp, side, rhsp, ټاکل_3, FINAL, lhsDTypep,
                            lhs_بهیر ? EXTEND_OFF : EXTEND_LHS, ریښتیا, بهیر_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (فراغ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_غوټه_بیان* convert_Unpacked_ترتیب(Ast_غوټه_بیان* exprp, Ast_غوټه_DType* dtypep) {
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!arrayp || !arrayp->is_سرچپه_د(exprp->dtypep())) أرجع exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_شا(&relinker);
        Ast_غوټه_بیان* ثابت newp = جديد Ast_Cvt_Unpack_منزل_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        أرجع newp;
    }

    ساكن منطقي isLogicalOperator(ثابت Ast_غوټه* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        أرجع VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    فراغ iterateCheckBool(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, پلنوالی_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_غوټه_DType* ثابت underVDTypep = underp ? underp->dtypep()->skipRefp() : تش_اشاره;
        إذا (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_شا(&linker);
            Ast_غوټه* ثابت newp
                = جديد AstNeqD{parentp->fileline(), VN_AS(underp, غوټه_بیان),
                              جديد AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } وإلا إذا (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_شا(&linker);
            Ast_غوټه* ثابت newp
                = جديد AstNeqN{parentp->fileline(), VN_AS(underp, غوټه_بیان),
                              جديد AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } وإلا إذا (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_پلنوالی_Reduce(VN_AS(underp, غوټه_بیان)), underp);  //بدل_شوی
        } وإلا إذا (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(جديد AstConst{parentp->fileline(), AstConst::بټ_False_Erroring{}});
            VL_DO_DANGLING(ټېل_Deletep(underp), underp);
        } وإلا {
            ثابت منطقي bad = پلنوالی_Bad(underp, parentp->پیدا_کړه_بټ_DType());
            إذا (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->پلنوالی(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->پلنوالی()
                                             << (underp->پلنوالی() != underp->پلنوالی_Min()
                                                     ? " or " + cvt_منزل_Str(underp->پلنوالی_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            إذا (Ast_بیان_Stmt* ثابت بیان_Stmtp = VN_CAST(underp, بیان_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_پلنوالی_Reduce(بیان_Stmtp->resultp());
                بیان_Stmtp->dtypeFrom(بیان_Stmtp->resultp());
            } وإلا إذا (underp->پلنوالی() != 1) {
                VL_DO_DANGLING(fix_پلنوالی_Reduce(VN_AS(underp, غوټه_بیان)), underp);  //بدل_شوی
            }
        }
    }

    ساكن فراغ checkEnum(Ast_غوټه* nodep, Ast_غوټه* parentp, ثابت Ast_غوټه_DType* موخه_DTypep,
                          ثابت Ast_غوټه_DType* سرچینه_DTypep, ثابت Ast_غوټه_بیان* سرچینه_Exprp) {
        إذا (ثابت AstEnumDType* ثابت موخه_Enump
            = VN_CAST(موخه_DTypep->skip_Ref_منزل_Enump(), EnumDType)) {
            ثابت VCastable castable
                = Ast_غوټه::computeCastable(موخه_Enump, سرچینه_DTypep, سرچینه_Exprp);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << موخه_DTypep->prettyDTypeNameQ() << " from "
                                             << سرچینه_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_نور()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_غوټه* iterateCheck(Ast_غوټه* parentp, ثابت حرف* side, Ast_غوټه* underp, Determ determ,
                          Stage stage, Ast_غوټه_DType* expDTypep, Extend_قاعده extend_قاعده,
                          منطقي warnOn = ریښتیا, بهیر_Use بهیر_Use_2 = بهیر_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ثابت بهیر_Use ماشوم_بهیر_Use
            = بهیر_Use_2 != بهیر_USE_NONE ? بهیر_Use_2 : بهیر_Use_For_Determ(determ);
        إذا (rhsAccess.isReadOnly()) {
            إذا (Ast_غوټه_بیان* ثابت exprp = VN_CAST(underp, غوټه_بیان)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_ترتیب(exprp, expDTypep);
            }
        }
        إذا (VN_IS(underp, غوټه_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } وإلا إذا (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   پلنوالی_VP{expDTypep, FINAL, ماشوم_بهیر_Use}.p());
        } وإلا إذا (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   پلنوالی_VP{expDTypep, FINAL, ماشوم_بهیر_Use}.p());
        } وإلا إذا (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_غوټه* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, غوټه_بیان));
            underp = userIterateSubtreeReturnEdits(oldp, پلنوالی_VP{SELF, FINAL, ماشوم_بهیر_Use}.p());
        } وإلا إذا (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_غوټه* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, غوټه_بیان), ریښتیا, expDTypep->پلنوالی());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, پلنوالی_VP{SELF, FINAL, ماشوم_بهیر_Use}.p());
        } وإلا إذا (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_غوټه* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, غوټه_بیان));
            underp = userIterateSubtreeReturnEdits(oldp, پلنوالی_VP{SELF, FINAL, ماشوم_بهیر_Use}.p());
        } وإلا {
            ثابت Ast_غوټه_DType* ثابت underDtp = underp->dtypep()->skipRefp();
            إذا (determ != ټاکل_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_غوټه* ثابت newp
                    = جديد AstConst{underp->fileline(), AstConst::بټ_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ټېل_Deletep(underp), underp);
                underp = newp;
            }
            ثابت Ast_غوټه_DType* ثابت expDtp = expDTypep->skipRefp();
            إذا (determ == ټاکل_3 && VN_IS(expDtp, غوټه_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            إذا (determ == ټاکل_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_سمون(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
            ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
            إذا (expBasicp && underBasicp) {
                إذا (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_کوډ::ENUMVALUE)
                    && warnOn) {
                    إذا (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, غوټه_بیان));
                    }
                    إذا (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, تش_اشاره);
                    }
                }
                Ast_غوټه_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                إذا (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, پلنوالی_VP{SELF, FINAL, ماشوم_بهیر_Use}.p());
                } وإلا إذا (determ == ټاکل_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    إذا (underp->isSigned() != subDTypep->isSigned()
                        || underp->پلنوالی() != subDTypep->پلنوالی()) {
                        subDTypep = parentp->پیدا_کړه_بټ_Or_Logic_DType(
                            std::max(subDTypep->پلنوالی(), underp->پلنوالی()),
                            std::max(subDTypep->پلنوالی_Min(), underp->پلنوالی_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, پلنوالی_VP{subDTypep, FINAL, ماشوم_بهیر_Use}.p());
                } وإلا {
                    underp = userIterateSubtreeReturnEdits(
                        underp, پلنوالی_VP{subDTypep, FINAL, ماشوم_بهیر_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                پلنوالی_Check_Sized(parentp, side, VN_AS(underp, غوټه_بیان), expDTypep, extend_قاعده,
                                warnOn);
            } وإلا إذا (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } وإلا إذا (ثابت AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ثابت AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->عدد().isNull()) {
                    // '= null' is ok
                } وإلا إذا (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_له_لارې_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } وإلا إذا (expIfaceRefp->iface_له_لارې_Cellp() != underIfaceRefp->iface_له_لارې_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_له_لارې_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_له_لارې_Cellp()->prettyNameQ() << ").");
                } وإلا إذا (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } وإلا {
                    underp = userIterateSubtreeReturnEdits(
                        underp, پلنوالی_VP{expDTypep, FINAL, ماشوم_بهیر_Use}.p());
                }
            } وإلا {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, پلنوالی_VP{expDTypep, FINAL, ماشوم_بهیر_Use}.p());
            }
        }
        أرجع underp;
    }

    فراغ
    پلنوالی_Check_Sized(Ast_غوټه* parentp, ثابت حرف* side,
                    Ast_غوټه_بیان* underp,  // Node to be checked or have typecast added in front of
                    Ast_غوټه_DType* expDTypep, Extend_قاعده extend_قاعده, منطقي warnOn = ریښتیا) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
        ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
        إذا (expDTypep == underp->dtypep()) {
            أرجع;  // Same type must match
        } وإلا إذا (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#إذا VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#د_که_پای
            أرجع;
        } وإلا {
            ثابت صحيح exp_پلنوالی = expDTypep->پلنوالی();
            صحيح exp_پلنوالی_Min = expDTypep->پلنوالی_Min();
            إذا (exp_پلنوالی_Min == 0) exp_پلنوالی_Min = exp_پلنوالی;
            ثابت منطقي bad = پلنوالی_Bad(underp, expDTypep);
            إذا ((bad || underp->پلنوالی() != exp_پلنوالی) && fixAutoExtend(underp /*ref*/, exp_پلنوالی)) {
                underp = تش_اشاره;  // Changes underp
                أرجع;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            إذا (VN_IS(parentp->backp(), Cast_اندازه)) warnOn = خطأ;
            إذا (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = خطأ;
            إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->عدد().isFromString()
                && exp_پلنوالی > underp->پلنوالی()
                && (((exp_پلنوالی - underp->پلنوالی()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = خطأ;
            }
            إذا ((VN_IS(parentp, جمع_کړه_2) && underp->پلنوالی() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->پلنوالی() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = خطأ;
            }
            إذا (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_پلنوالی, underp->پلنوالی(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_پلنوالی
                        << (exp_پلنوالی != exp_پلنوالی_Min ? " or " + cvt_منزل_Str(exp_پلنوالی_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->پلنوالی()
                        << (underp->پلنوالی() != underp->پلنوالی_Min()
                                ? " or " + cvt_منزل_Str(underp->پلنوالی_Min())
                                : "")
                        << " bits.");
            }
            إذا (bad || underp->پلنوالی() != exp_پلنوالی) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ثابت Ast_غوټه_ټاکل* assignp = VN_CAST(parentp, غوټه_ټاکل);
                ثابت AstPin* pinp = VN_CAST(parentp, Pin);
                إذا (assignp && VN_IS(assignp->lhsp(), غوټه_بهیر)) {
                } وإلا إذا (pinp && pinp->modVarp()->direction() != VDirection::ننوت) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } وإلا إذا (assignp && VN_IS(underp, غوټه_بهیر) && exp_پلنوالی > underp->پلنوالی()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ثابت صحيح ښويدنه_2 = exp_پلنوالی - underp->پلنوالی();
                    underp->unlink_Fr_شا(&linker);
                    AstExtend* ثابت widenedp = جديد AstExtend{underp->fileline(), underp};
                    widenedp->did_پلنوالی(ریښتیا);
                    // Shift left so zeros fill on the right
                    Ast_غوټه_بیان* ثابت shiftedp = جديد Ast_ښويدنه_L{
                        underp->fileline(), widenedp,
                        جديد AstConst{underp->fileline(), static_cast<uint32_t>(ښويدنه_2)}, exp_پلنوالی};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } وإلا {
                    VL_DO_DANGLING(fix_پلنوالی_Extend(underp, expDTypep, extend_قاعده), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_غوټه_بیان* checkCvtUS(Ast_غوټه_بیان* nodep, منطقي fatal) {
        إذا (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            إذا (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } وإلا {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, خطأ, 32);
        }
        أرجع nodep;
    }

    Ast_غوټه_بیان* spliceCvtD(Ast_غوټه_بیان* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        إذا (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            Ast_غوټه_بیان* newp;
            إذا (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = جديد AstISToRD{nodep->fileline(), nodep};
            } وإلا {
                newp = جديد AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_غوټه_بیان* spliceCvtS(Ast_غوټه_بیان* nodep, منطقي warnOn, صحيح پلنوالی) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                إذا (constp->isDouble() && V3_شمېره::epsilonIntegral(constp->عدد().منزل_Double())) {
                    warnOn = خطأ;
                }
            }
            إذا (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_غوټه_بیان* ثابت newp = جديد AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_سېټ_بټ_Sized(پلنوالی, VSigning::SIGNED);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_غوټه_بیان* spliceCvtString(Ast_غوټه_بیان* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_شا(&linker);
            Ast_غوټه_بیان* ثابت newp = جديد AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_غوټه_Biop* replaceWithUOrSVersion(Ast_غوټه_Biop* nodep, منطقي signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (signedFlavorNeeded == nodep->signedFlavor()) أرجع تش_اشاره;
        إذا (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        بدل (nodep->type()) {
        حالة VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::جمع_کړه_2: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        حالة VNType::ښويدنه_L: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع تش_اشاره;
        افتراضي: اكسر;
        }
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* newp = تش_اشاره;
        بدل (nodep->type()) {
        حالة VNType::Gt: newp = جديد AstGtS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::GtS: newp = جديد AstGt{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte: newp = جديد AstGteS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::GteS: newp = جديد AstGte{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt: newp = جديد AstLtS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::LtS: newp = جديد AstLt{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte: newp = جديد AstLteS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::LteS: newp = جديد AstLte{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Div: newp = جديد AstDivS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::DivS: newp = جديد AstDiv{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ModDiv: newp = جديد AstModDivS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ModDivS: newp = جديد AstModDiv{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Mul: newp = جديد AstMulS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::MulS: newp = جديد AstMul{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ښويدنه_R: newp = جديد Ast_ښويدنه_RS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::ښويدنه_RS: newp = جديد Ast_ښويدنه_R{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_غوټه_Biop* replaceWithDVersion(Ast_غوټه_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع تش_اشاره;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* newp = تش_اشاره;
        // No width change on output;...                // All below have bool or double outputs
        بدل (nodep->type()) {
        حالة VNType::جمع_کړه_2: newp = جديد Ast_جمع_کړه_D{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Sub: newp = جديد AstSubD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Pow: newp = جديد AstPowD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Eq:
        حالة VNType::EqCase: newp = جديد AstEqD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Neq:
        حالة VNType::NeqCase: newp = جديد AstNeqD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gt:
        حالة VNType::GtS: newp = جديد AstGtD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte:
        حالة VNType::GteS: newp = جديد AstGteD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt:
        حالة VNType::LtS: newp = جديد AstLtD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte:
        حالة VNType::LteS: newp = جديد AstLteD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Div:
        حالة VNType::DivS: newp = جديد AstDivD{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Mul:
        حالة VNType::MulS: newp = جديد AstMulD{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_غوټه_Biop* replaceWithNVersion(Ast_غوټه_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        إذا (nodep->stringFlavor()) أرجع تش_اشاره;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
        Ast_غوټه_Biop* newp = تش_اشاره;
        // No width change on output;...                // All below have bool or double outputs
        بدل (nodep->type()) {
        حالة VNType::Eq:
        حالة VNType::EqCase:
        حالة VNType::EqWild: newp = جديد AstEqN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Neq:
        حالة VNType::NeqCase:
        حالة VNType::NeqWild: newp = جديد AstNeqN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gt:
        حالة VNType::GtS: newp = جديد AstGtN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Gte:
        حالة VNType::GteS: newp = جديد AstGteN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lt:
        حالة VNType::LtS: newp = جديد AstLtN{fl, lhsp, rhsp}; اكسر;
        حالة VNType::Lte:
        حالة VNType::LteS: newp = جديد AstLteN{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_غوټه_Uniop* replaceWithDVersion(Ast_غوټه_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع تش_اشاره;
        File_کرښه* ثابت fl = nodep->fileline();
        Ast_غوټه_بیان* ثابت lhsp = nodep->lhsp()->unlink_Fr_شا();
        Ast_غوټه_Uniop* newp = تش_اشاره;
        بدل (nodep->type()) {
        حالة VNType::Negate: newp = جديد AstNegateD{fl, lhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
        أرجع newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    فراغ replaceWithSFormat(Ast_لاره_Call* nodep, ثابت string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ثابت argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_غوټه_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_شا(), VarRef);
        Ast_غوټه* ثابت newp = جديد Ast_ټاکل{
            nodep->fileline(), fromp,
            جديد AstSFormatF{nodep->fileline(), format, خطأ, argp->exprp()->unlink_Fr_شا()}};
        fromp->access(VAccess::WRITE);
        ټېل_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_غوټه_DType* iterateEditMoveDTypep(Ast_غوټه* parentp, Ast_غوټه_DType* dtnodep) {
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
        ثابت منطقي child1 = (parentp->get_ماشوم_DTypep() == dtnodep);
        ثابت منطقي child2 = (parentp->getChild2DTypep() == dtnodep);
        إذا (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, تش_اشاره), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_ماشوم_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_پلنوالی(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_شا();
            v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(dtnodep);
        }
        إذا (!dtnodep->did_پلنوالی()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, تش_اشاره);
            UASSERT_OBJ(dtnodep->did_پلنوالی(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        أرجع dtnodep;
    }

    AstConst* dimension_ارزښت(File_کرښه* fileline, Ast_غوټه_DType* nodep, VAttrType attrType,
                             صحيح dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_غوټه_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        لكل (صحيح i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            إذا (ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(dtypep, غوټه_Array_DType)) {
                declRange = adtypep->declRange();
                إذا (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                استمر;
            } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت adtypep
                       = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                اكسر;  // Sub elements don't look like arrays and can't iterate into
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                إذا (adtypep->isRanged()) declRange = adtypep->declRange();
                اكسر;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        AstConst* valp = تش_اشاره;  // If nullptr, construct from val
        صحيح val = 0;
        بدل (attrType) {
        حالة VAttrType::DIM_بټونه: {
            صحيح بټونه = 1;
            طالما (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                إذا (ثابت Ast_غوټه_Array_DType* ثابت adtypep = VN_CAST(dtypep, غوټه_Array_DType)) {
                    بټونه *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    استمر;
                } وإلا إذا (ثابت Ast_غوټه_UOr_Struct_DType* ثابت adtypep
                           = VN_CAST(dtypep, غوټه_UOr_Struct_DType)) {
                    بټونه *= adtypep->isStreamableFixedAggregate() ? adtypep->پلنوالی_بهیر()
                                                                  : adtypep->پلنوالی();
                    اكسر;
                } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                    بټونه *= adtypep->پلنوالی();
                    اكسر;
                }
                اكسر;
            }
            إذا (dim == 0) {
                val = 0;
            } وإلا إذا (dim == 1 && !declRange.ranged()
                       && بټونه == 1) {  // $bits should be sane for non-arrays
                val = nodep->پلنوالی();
            } وإلا {
                val = بټونه;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        حالة VAttrType::DIM_لوړ: val = !declRange.ranged() ? 0 : declRange.hi(); اكسر;
        حالة VAttrType::DIM_کیڼ: val = !declRange.ranged() ? 0 : declRange.کیڼ(); اكسر;
        حالة VAttrType::DIM_ټیټ: val = !declRange.ranged() ? 0 : declRange.lo(); اكسر;
        حالة VAttrType::DIM_ښی: val = !declRange.ranged() ? 0 : declRange.ښی(); اكسر;
        حالة VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            اكسر;
        حالة VAttrType::DIM_اندازه: val = !declRange.ranged() ? 0 : declRange.elements(); اكسر;
        افتراضي: nodep->v3fatalSrc("Missing DIM ATTR type case"); اكسر;
        }
        إذا (!valp) valp = جديد AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_منزل_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        أرجع valp;
    }
    AstVarRef* dimensionVarRefp(Ast_غوټه_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_جدول_Map[std::make_pair(nodep, attrType)];
        إذا (cachedpr) أرجع cachedpr->clone_ونه(خطأ);

        Ast_غوټه_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->پیدا_کړه_Int_DType(), جديد AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(vardtypep);
        userIterate(vardtypep, پلنوالی_VP{SELF, BOTH}.p());
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        لكل (بینښه i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_ارزښت(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::پیدا_کړه(consp);
        VL_DO_DANGLING(consp->delete_ونه(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_پلنوالی(ریښتیا);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_پلنوالی(ریښتیا);
        ټېل_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_ونه(خطأ);
    }
    ساكن uint64_t enum_Max_ارزښت(ثابت Ast_غوټه* errNodep, ثابت AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        لكل (ثابت Ast_Enum_توکی* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
            ثابت AstConst* ثابت vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            إذا (!vconstp->عدد().isAnyXZ() && vconstp->منزل_UQuad() >= maxval)
                maxval = vconstp->منزل_UQuad();
        }
        إذا (adtypep->itemsp()->پلنوالی() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            أرجع 64;
        }
        أرجع maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ثابت nodep, VAttrType attrType, منطقي assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_جدول_Map[std::make_pair(nodep, attrType)];
        إذا (cachedpr) أرجع cachedpr->clone_ونه(خطأ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_کرښه* ثابت flp = nodep->fileline();
        Ast_غوټه_DType* basep;
        إذا (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->پیدا_کړه_String_DType();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->پیدا_کړه_بټ_DType();
        } وإلا {
            basep = nodep->dtypep();
        }
        Ast_غوټه_DType* vardtypep;
        إذا (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = جديد AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } وإلا {
            vardtypep = جديد AstUnpackArrayDType{flp, basep, جديد AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->جمع_کړه_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = تش_اشاره;
        إذا (attrType == VAttrType::ENUM_NAME) {
            defaultp = جديد AstConst{flp, AstConst::String{}, ""};
        } وإلا إذا (attrType == VAttrType::ENUM_بل || attrType == VAttrType::ENUM_PREV) {
            defaultp = جديد AstConst{flp, V3_شمېره{nodep, nodep->پلنوالی(), 0}};
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            defaultp = جديد AstConst{flp, AstConst::بټ_False{}};
        } وإلا {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_غوټه_بیان*> ارزښتونه;
        {
            uint64_t ordinal = 0;
            Ast_Enum_توکی* ثابت firstp = nodep->itemsp();
            ثابت Ast_Enum_توکی* prevp = firstp;  // Prev must start with last item
            طالما (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_توکی);
            لكل (Ast_Enum_توکی* itemp = firstp; itemp;) {
                Ast_Enum_توکی* ثابت nextp = VN_AS(itemp->nextp(), Enum_توکی);
                ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                إذا (!vconstp->عدد().isAnyXZ()) {  // Can 2-state runtime decode
                    ثابت uint64_t i = nodep->isWide() ? ++ordinal : vconstp->منزل_UQuad();
                    إذا (attrType == VAttrType::ENUM_NAME) {
                        ارزښتونه[i] = جديد AstConst{flp, AstConst::String{},
                                                 V3_شمېره::displayedEnumName(itemp)};
                    } وإلا إذا (attrType == VAttrType::ENUM_بل) {
                        ارزښتونه[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_ونه(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_PREV) {
                        ارزښتونه[i] = prevp->valuep()->clone_ونه(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
                        ارزښتونه[i] = جديد AstConst{flp, AstConst::بټ_True{}};
                    } وإلا {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_غوټه_بیان* valuep = تش_اشاره;
        إذا (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ثابت consp = جديد AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            لكل (ثابت خپلکاری& itr : ارزښتونه) {
                V3_شمېره کلی{nodep, nodep->پلنوالی()};
                کلی.سېټ_Quad(itr.لومړی);
                consp->جمع_کړه_Itemsp(جديد Ast_Cons_Assoc_توکی{flp, جديد AstConst{flp, کلی}, itr.دوهم});
            }
            valuep = consp;
        } وإلا {
            AstConsArray* ثابت consp = جديد AstConsArray{flp, vardtypep, defaultp};
            لكل (uint64_t i = 0; i < (msbdim + 1); ++i) {
                إذا (ارزښتونه[i]) consp->valuep(i, ارزښتونه[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::پیدا_کړه(valuep);
        VL_DO_DANGLING(ټېل_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_پلنوالی(ریښتیا);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_پلنوالی(ریښتیا);
        ټېل_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_ونه(خطأ);
    }

    Ast_غوټه_بیان* enumSelect(Ast_غوټه_بیان* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        إذا (attrType == VAttrType::ENUM_NAME) {
            إذا (nodep->is_خالص()) nodep = V3Const::constifyEdit(nodep);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                Ast_غوټه_بیان* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->عدد().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
                أرجع newp;
            }
        }
        إذا (attrType == VAttrType::ENUM_NAME && !nodep->is_خالص() && (m_modep || m_ftaskp)) {
            Ast_بیان_Stmt* ثابت capturep = newEnumCapture(nodep, adtypep);
            Ast_غوټه_بیان* ثابت valuep = capturep->resultp()->unlink_Fr_شا();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_سېټ_String();
            أرجع capturep;
        }
        إذا (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_غوټه_بیان* indexp = جديد AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            لكل (ثابت Ast_Enum_توکی* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_توکی)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                إذا (constp->عدد().isAnyXZ()) استمر;
                indexp = جديد AstCond{nodep->fileline(),
                                     جديد AstEq{nodep->fileline(), nodep->clone_ونه(خطأ),
                                               itemp->valuep()->clone_ونه(خطأ)},
                                     جديد AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, خطأ, ordinal);
            Ast_غوټه_بیان* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_سېټ_String();
            VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
            أرجع newp;
        }
        ثابت uint64_t msbdim = enum_Max_ارزښت(nodep, adtypep);
        ثابت منطقي assoc = msbdim > ENUM_LOOKUP_بټونه;
        Ast_غوټه_بیان* newp;
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, ریښتیا, 0);
            newp = جديد AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } وإلا {
            ثابت صحيح selwidth = V3_شمېره::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(adtypep, attrType, خطأ, (1ULL << selwidth) - 1);
            Ast_غوټه_بیان* ثابت boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->پلنوالی() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? جديد AstGt{nodep->fileline(), nodep->clone_ونه(خطأ),
                                  جديد AstConst{nodep->fileline(),
                                               V3_شمېره{nodep, adtypep->پلنوالی(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : تش_اشاره;
            newp = جديد AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                جديد AstSel{nodep->fileline(), nodep, 0, selwidth}};
            إذا (boundp) {
                newp = جديد AstCond{nodep->fileline(), boundp,
                                   جديد AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        إذا (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_سېټ_String();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_سېټ_بټ();
        } وإلا {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        أرجع newp;
    }
    Ast_غوټه_بیان* enumTestValid(Ast_غوټه_بیان* valp, AstEnumDType* enumDtp) {
        ثابت uint64_t maxval = enum_Max_ارزښت(valp, enumDtp);
        ثابت منطقي assoc = maxval > ENUM_LOOKUP_بټونه;
        Ast_غوټه_بیان* testp = تش_اشاره;
        File_کرښه* ثابت fl_novalue = جديد File_کرښه{valp->fileline()};
        fl_novalue->warnOff(V3_Error_کوډ::ENUMVALUE, ریښتیا);
        fl_novalue->warnOff(V3_Error_کوډ::CMPCONST, ریښتیا);
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ریښتیا, 0);
            testp = جديد AstAssocSel{fl_novalue, tabRefp, valp->clone_ونه_خالص(خطأ)};
        } وإلا {
            ثابت صحيح selwidth = V3_شمېره::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, خطأ, (1ULL << selwidth) - 1);
            File_کرښه* ثابت fl_nowidth = جديد File_کرښه{fl_novalue};
            fl_nowidth->warnOff(V3_Error_کوډ::پلنوالی_2, ریښتیا);
            testp = جديد AstCond{
                fl_novalue,
                جديد AstGt{fl_nowidth, valp->clone_ونه_خالص(خطأ),
                          جديد AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                جديد AstConst{fl_novalue, AstConst::بټ_False{}},
                جديد AstArraySel{fl_novalue, tabRefp,
                                جديد AstSel{fl_novalue, valp->clone_ونه_خالص(خطأ), 0, selwidth}}};
        }
        أرجع testp;
    }

    PatVecMap pat_ویکټور_Map(Ast_نمونه* nodep, ثابت VNumRange& range) {
        PatVecMap patmap;
        صحيح element = range.کیڼ();
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            إذا (patp->keyp()) {
                إذا (ثابت AstConst* ثابت constp = نمونه_Const_Keyp(patp)) {
                    إذا (constp->عدد().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } وإلا {
                        element = constp->منزل_SInt();
                    }
                }
            }
            ثابت منطقي new_ثبت = patmap.emplace(element, patp).دوهم;
            إذا (!new_ثبت) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.کیڼ_منزل_ښی_Inc();
        }
        أرجع patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    فراغ patConcatConvertRecurse(Ast_نمونه* patternp, Ast_غوټه_بیان* nodep) {
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } وإلا {
            AstPatMember* ثابت newp
                = جديد AstPatMember{nodep->fileline(), nodep->unlink_Fr_شا(), تش_اشاره, تش_اشاره};
            newp->isConcat(ریښتیا);
            patternp->جمع_کړه_Itemsp(newp);
        }
    }

    فراغ makeOpenArrayShell(Ast_غوټه_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_غوټه_FTask* ثابت oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_پلار_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_ماشوم(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_غوټه_FTask* ثابت newTaskp = oldTaskp->clone_ونه(خطأ);
        newTaskp->dpi_Open_ماشوم(ریښتیا);
        newTaskp->dpi_Open_پلار_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_منزل_Str(oldTaskp->dpi_Open_پلار()));
        oldTaskp->جمع_کړه_بل_دلته(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        لكل (ثابت خپلکاری& tconnect : tconnects) {
            AstVar* ثابت portp = tconnect.لومړی;
            ثابت AstArg* ثابت argp = tconnect.دوهم;
            ثابت Ast_غوټه* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;  // Argument error we'll find later
            إذا (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_غوټه_DType* ثابت oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_غوټه* nodep) {
                    إذا (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    منطقي markHasOpenArray(Ast_غوټه_FTask* nodep) {
        منطقي hasOpen = خطأ;
        لكل (Ast_غوټه* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(ریښتیا);
                    hasOpen = ریښتیا;
                }
            }
        }
        أرجع hasOpen;
    }
    منطقي hasOpenArrayDTypeRecurse(Ast_غوټه_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        إذا (VN_IS(nodep, UnsizedArrayDType)) أرجع ریښتیا;
        إذا (nodep->subDTypep()) أرجع hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        أرجع خطأ;
    }
    Ast_غوټه_DType* dtypeNotIntAtomOrVecRecurse(Ast_غوټه_DType* nodep, منطقي ranged = خطأ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_منزل_Enump();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (ranged && (!dtp->is_بټ_Logic() || dtp->isRanged()))
                أرجع dtp;  // Packed when already packed
            إذا (dtp->keyword().isIntNumeric()) أرجع تش_اشاره;
            أرجع dtp;
        } وإلا إذا (AstPackArrayDType* ثابت dtp = VN_CAST(nodep, PackArrayDType)) {
            إذا (ranged) أرجع dtp;  // Packed when already packed
            أرجع dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), ریښتیا);
        }
        أرجع nodep;
    }
    Ast_غوټه_DType* dtypeNot4StateIntegralRecurse(Ast_غوټه_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (!dtp->keyword().isFourstate()) أرجع dtp;
            أرجع تش_اشاره;
        } وإلا إذا (Ast_غوټه_Array_DType* ثابت dtp = VN_CAST(nodep, غوټه_Array_DType)) {
            أرجع dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } وإلا إذا (Ast_غوټه_UOr_Struct_DType* ثابت dtp = VN_CAST(nodep, غوټه_UOr_Struct_DType)) {
            لكل (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_غوټه_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                إذا (badDtp) أرجع badDtp;
            }
            أرجع تش_اشاره;
        }
        أرجع nodep;
    }
    فراغ checkForceReleaseLhs(Ast_غوټه* nodep, Ast_غوټه* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        إذا (Ast_غوټه* ثابت selNodep = V3_پلنوالی::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    فراغ assert_At_بیان(Ast_غوټه* nodep) {
        إذا (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    فراغ assertAtStatement(Ast_غوټه* nodep) {
        إذا (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    فراغ checkConstantOrReplace(Ast_غوټه* nodep, منطقي noFourState, ثابت string& پیغام) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp || (noFourState && constp->عدد().isFourState())) {
            nodep->v3error(پیغام);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ټېل_Deletep(nodep), nodep, constp);
            أرجع;
        }
    }
    Ast_غوټه* غوټه_For_Unsized_Warning(Ast_غوټه* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        إذا (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->پلنوالی_Sized()) {
            أرجع nodep->op1p();
        } وإلا إذا (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->پلنوالی_Sized()) {
            أرجع nodep->op2p();
        }
        أرجع nodep;  // By default return this
    }
    AstRefDType* check_Ref_منزل_Typedef_Recurse(Ast_غوټه* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        إذا (VL_LIKELY(!nodep)) أرجع تش_اشاره;
        إذا (خپلکاری* ثابت refp = VN_CAST(nodep, RefDType)) {
            إذا (refp->typedefp() == typedefp) أرجع refp;
        }
        إذا (خپلکاری* ثابت refp = check_Ref_منزل_Typedef_Recurse(nodep->op1p(), typedefp)) أرجع refp;
        إذا (خپلکاری* ثابت refp = check_Ref_منزل_Typedef_Recurse(nodep->op2p(), typedefp)) أرجع refp;
        إذا (خپلکاری* ثابت refp = check_Ref_منزل_Typedef_Recurse(nodep->op3p(), typedefp)) أرجع refp;
        إذا (خپلکاری* ثابت refp = check_Ref_منزل_Typedef_Recurse(nodep->op4p(), typedefp)) أرجع refp;
        أرجع تش_اشاره;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ساكن منطقي isIfaceArrayDType(ثابت Ast_غوټه_DType* dtypep) {
        إذا (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        ثابت AstIfaceRefDType* ثابت irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        أرجع irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ساكن std::string unpacked_Array_اندازه(ثابت Ast_غوټه_DType* dtypep) {
        std::string اندازه;
        dtypep = dtypep->skipRefp();
        طالما (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            إذا (!اندازه.تشه()) اندازه += "x";
            اندازه += std::منزل_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        أرجع اندازه;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ساكن Ast_غوټه_بیان* newIfaceArrayElement(Ast_غوټه_بیان* fromp, صحيح بټ_2) {
        AstConsArray* ثابت consp = VN_CAST(fromp, ConsArray);
        إذا (!consp || !isIfaceArrayDType(consp->dtypep())) أرجع تش_اشاره;
        ثابت AstUnpackArrayDType* ثابت arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (بټ_2 < 0 || بټ_2 >= arrp->elementsConst()) أرجع تش_اشاره;
        أرجع consp->valuep(بټ_2)->clone_ونه(خطأ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_غوټه_بیان* new_Iface_Array_ارزښت(ثابت Ast_غوټه_Var_Ref* refp, Ast_غوټه_DType* dtypep,
                                    ثابت std::string& name, ثابت std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_کرښه* ثابت flp = refp->fileline();
        AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_غوټه_Var_Ref* newp;
            إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(refp, VarXRef)) {
                newp = جديد AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } وإلا {
                newp = جديد AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, پلنوالی_VP{SELF, BOTH}.p());
            أرجع newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ثابت consp = جديد AstConsArray{flp, arrp};
        ثابت VNumRange range = arrp->declRange();
        لكل (ثابت صحيح i : range.seq_کیڼ_منزل_ښی()) {
            ثابت std::string s = name + "__BRA__" + Ast_غوټه::encode_شمېره(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_ارزښت(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        أرجع consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    فراغ replace_وېشل_Iface_Array_Ref(Ast_غوټه_Var_Ref* nodep) {
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_وېشل(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        إذا (varp->is_Iface_پلار()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_ارزښت(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ټېل_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    فراغ expandIfaceArrayPinDimensions(AstPin* nodep, ثابت Ast_غوټه_DType* dtypep,
                                       Ast_غوټه_بیان* conp, ثابت std::string& suffix,
                                       AstVar*& elemVarpr) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        إذا (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ثابت newp = nodep->clone_ونه(خطأ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            إذا (conp) newp->exprp(conp->clone_ونه(خطأ));
            nodep->جمع_کړه_بل_دلته(newp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ثابت VNumRange range = arrp->declRange();
        لكل (صحيح n = range.elements() - 1; n >= 0; --n) {
            ثابت صحيح i = range.کیڼ() + n * range.کیڼ_منزل_ښی_Inc();
            ثابت std::string s = suffix + "__BRA__" + Ast_غوټه::encode_شمېره(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_غوټه_بیان* ثابت elemConnp = [&]() -> Ast_غوټه_بیان* {
                إذا (!conp) أرجع تش_اشاره;
                ثابت AstUnpackArrayDType* ثابت conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ثابت صحيح elements = conArrp->elementsConst();
                // Counted from lo
                Ast_غوټه_بیان* ثابت resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                أرجع resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    فراغ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ثابت portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_وېشل(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        لكل (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_غوټه_بیان* ثابت exprp = VN_AS(nodep->exprp(), غوټه_بیان);
        إذا (exprp) ټېل_Deletep(exprp->unlink_Fr_شا());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ثابت منطقي connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_اندازه(exprp->dtypep()) == unpacked_Array_اندازه(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : تش_اشاره, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ټېل_Deletep(nodep->unlink_Fr_شا()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_غوټه* userIterateSubtreeReturnEdits(Ast_غوټه* nodep, پلنوالی_VP* vup) {
        إذا (!nodep) أرجع تش_اشاره;
        Ast_غوټه* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        أرجع ret;
    }
    فراغ userIterate(Ast_غوټه* nodep, پلنوالی_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    فراغ user_Iterate_And_بل(Ast_غوټه* nodep, پلنوالی_VP* vup) {
        إذا (!nodep) أرجع;
        إذا (nodep->did_پلنوالی()) أرجع;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_بل_Null(nodep);
    }
    فراغ userIterateChildren(Ast_غوټه* nodep, پلنوالی_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    فراغ userIterateChildrenBackwardsConst(Ast_غوټه* nodep, پلنوالی_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

عام:
    // CONSTRUCTORS
    پلنوالی_Visitor(منطقي paramsOnly,  // [in] TRUE if we are considering parameters only.
                 منطقي doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_لنډمهاله_نومونه{"__Venum"}
        , m_inside_لنډمهاله_نومونه{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_غوټه* mainAcceptEdit(Ast_غوټه* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, پلنوالی_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        لكل (AstVar* ثابت varp : m_وېشل_Iface_Array_Varps) {
            VL_DO_DANGLING(ټېل_Deletep(varp->unlink_Fr_شا()), varp);
        }
        m_وېشل_Iface_Array_Varps.clear();
        أرجع nodep;
    }
    ساكن منطقي ښکتنی_As_Fixed_Aggregate(ثابت Ast_غوټه_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->لري_Unpacked_Struct();
    }
    فراغ بهیر_ټاکل_ښکتنی(Ast_غوټه_ټاکل* nodep) {
        Ast_غوټه_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        إذا (Ast_غوټه_بهیر* ثابت streamp = VN_CAST(nodep->rhsp(), غوټه_بهیر)) {
            Ast_غوټه_DType* ثابت lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ثابت صحيح lwidth = lhsDTypeSkippedRefp->پلنوالی_بهیر();
            ثابت صحيح rwidth = streamp->lhsp()->dtypep()->skipRefp()->پلنوالی_بهیر();
            إذا (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            إذا (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), کتار_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), کتار_DType)) {
                ثابت صحيح کتار_Element_اندازه = streamp->lhsp()->dtypep()->subDTypep()->پلنوالی();
                UASSERT_OBJ(کتار_Element_اندازه <= lwidth, nodep, "LHS < RHS");
            }
            إذا (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || ښکتنی_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_شا();
                nodep->rhsp(
                    جديد Ast_Cvt_Packed_منزل_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        إذا (Ast_غوټه_بهیر* ثابت streamp = VN_CAST(nodep->lhsp(), غوټه_بهیر)) {
            ثابت Ast_غوټه_DType* ثابت rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_غوټه_DType* ثابت lhs_بهیر_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ثابت صحيح lwidth = lhs_بهیر_DTypep->پلنوالی_بهیر();
            ثابت صحيح rwidth = rhsDTypep->پلنوالی_بهیر();
            إذا (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            إذا (ښکتنی_As_Fixed_Aggregate(lhs_بهیر_DTypep)) {
                Ast_غوټه_بیان* ثابت بهیر_Exprp = nodep->lhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* ثابت dstp = streamp->lhsp()->unlink_Fr_شا();
                Ast_غوټه_بیان* srcp = nodep->rhsp()->unlink_Fr_شا();
                إذا (VN_IS(streamp, بهیر_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_سېټ_بټ_Or_Logic_Unsized(
                        srcp->پلنوالی(), srcp->پلنوالی_Min(), VSigning::UNSIGNED,
                        Ast_غوټه_بیان::isFourstateOrNoDtype(بهیر_Exprp, srcp));
                    srcp = بهیر_Exprp;
                } وإلا {
                    إذا (srcp->پلنوالی() > lwidth) {
                        srcp = جديد AstSel{streamp->fileline(), srcp, srcp->پلنوالی() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ټېل_Deletep(بهیر_Exprp), بهیر_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد Ast_Cvt_Packed_منزل_Array{srcp->fileline(), srcp, lhs_بهیر_DTypep});
                nodep->dtypeFrom(dstp);
            } وإلا إذا (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_غوټه_بیان* ثابت rhsp = nodep->rhsp()->unlink_Fr_شا();
                nodep->rhsp(جديد Ast_Cvt_Array_منزل_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~پلنوالی_Visitor() override = افتراضي;
};

//######################################################################
// Width class functions

فراغ V3_پلنوالی::پلنوالی(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ثابت پلنوالی_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ثابت VDTypeNameScopes scopes{nodep};
        پلنوالی_Visitor visitor{خطأ, خطأ};
        (فراغ)visitor.mainAcceptEdit(nodep);
        پلنوالی_Remove_Visitor rvisitor;
        (فراغ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ونه("width", 0, dump_ونه_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_غوټه* V3_پلنوالی::پلنوالی_Params_Edit(Ast_غوټه* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    پلنوالی_Visitor visitor{ریښتیا, خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

فراغ V3_پلنوالی::بهیر_ټاکل_ښکتنی_Edit(Ast_غوټه_ټاکل* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    پلنوالی_Visitor visitor{خطأ, خطأ};
    visitor.بهیر_ټاکل_ښکتنی(nodep);
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
Ast_غوټه* V3_پلنوالی::پلنوالی_Generate_Params_Edit(
    Ast_غوټه* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    پلنوالی_Visitor visitor{ریښتیا, ریښتیا};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

Ast_غوټه* V3_پلنوالی::selectNonConstantRecurse(Ast_غوټه* nodep, منطقي دننه_Sel) {
    // If node has a non-constant select, return that select
    Ast_غوټه* resultp = تش_اشاره;
    إذا (Ast_غوټه_Sel* ثابت anodep = VN_CAST(nodep, غوټه_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), دننه_Sel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), ریښتیا);
    } وإلا إذا (AstSel* ثابت anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), دننه_Sel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), ریښتیا);
    } وإلا إذا (Ast_غوټه_Var_Ref* ثابت anodep = VN_CAST(nodep, غوټه_Var_Ref)) {
        إذا (دننه_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) أرجع anodep;
    } وإلا {
        إذا (Ast_غوټه* ثابت refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, دننه_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_غوټه* ثابت refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, دننه_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_غوټه* ثابت refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, دننه_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_غوټه* ثابت refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, دننه_Sel);
    }
    أرجع resultp;
}
