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
#تعریف_کن VERILATOR_V3_عرض_CPP
#ضمن "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

صنف ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ثابت Ast_گره*, AstClass*> m_cache;

    AstClass* پیدا_کن_Cached(Ast_گره* nodep) {
        إذا (!nodep) أرجع اشاره_تهی;
        ثابت خودکار it = m_cache.پیدا_کن(nodep);
        إذا (it != m_cache.انتها()) أرجع it->دومی;

        AstClass* classp = اشاره_تهی;
        إذا (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = پیدا_کن_Cached(nodep->backp());
        } وإلا إذا (AstClass* ثابت parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } وإلا إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } وإلا {
            classp = پیدا_کن_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        أرجع classp;
    }

عام:
    AstClass* پیدا_کن(Ast_گره* nodep) {
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) أرجع classp;
        إذا (AstClassPackage* ثابت packagep = VN_CAST(nodep, ClassPackage)) {
            أرجع packagep->classp();
        }
        أرجع پیدا_کن_Cached(nodep);
    }
};

//######################################################################

تعداد Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& عملگر<<(std::ostream& str, ثابت Stage& rhs) {
    أرجع str << ("-PFB"[static_cast<صحيح>(rhs)]);
}

تعداد Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    تخصیص_بده_3  // Assignment-like where sign comes from RHS only
};
std::ostream& عملگر<<(std::ostream& str, ثابت Determ& rhs) {
    ساكن constexpr ثابت حرف* ثابت s_det[] = {"SELF", "CNTX", "ASSN"};
    أرجع str << s_det[rhs];
}

تعداد جریان_Use : uint8_t {
    جریان_USE_NONE,  // Stream concatenation is not allowed in this context
    جریان_USE_تخصیص_بده,  // Assignment-like context permits stream concatenation
    جریان_USE_CAST,  // Explicit cast context permits stream concatenation
};

#تعریف_کن v3widthWarn(lhs, rhs, پیام_2) \
    v3warn_کد(((lhs) < (rhs)   ? V3_Error_کد::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_کد::WIDTHEXPAND \
                                : V3_Error_کد::عرض_2), \
               پیام_2)

//######################################################################
// Width state, as a visitor of each AstNode

صنف عرض_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_گره_DType* ثابت m_dtypep;  // Parent's data type to resolve to
    ثابت Stage m_stage;  // If true, report errors
    ثابت جریان_Use m_جریان_Use;  // Current expression may be a stream concat

عام:
    عرض_VP(Ast_گره_DType* dtypep, Stage stage, جریان_Use جریان_Use_2 = جریان_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_جریان_Use{جریان_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    عرض_VP(Determ determ, Stage stage, جریان_Use جریان_Use_2 = جریان_USE_NONE)
        : m_dtypep{اشاره_تهی}
        , m_stage{stage}
        , m_جریان_Use{جریان_Use_2} {
        إذا (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    عرض_VP* p() { أرجع هذا; }
    منطقي selfDtm() ثابت { أرجع m_dtypep == اشاره_تهی; }
    جریان_Use جریان_Use_2() ثابت { أرجع m_جریان_Use; }
    Ast_گره_DType* dtypep() ثابت {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        أرجع m_dtypep;
    }
    Ast_گره_DType* dtypeNullp() ثابت { أرجع m_dtypep; }
    Ast_گره_DType* dtypeNullSkipRefp() ثابت {
        Ast_گره_DType* dtp = dtypeNullp();
        إذا (dtp) dtp = dtp->skipRefp();
        أرجع dtp;
    }
    Ast_گره_DType* dtypeOverridep(Ast_گره_DType* ثابت defaultp) ثابت {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        إذا (!m_dtypep) أرجع defaultp;
        إذا (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            أرجع defaultp->پیدا_کن_بیت_Or_Logic_DType(m_dtypep->عرض(), m_dtypep->عرض_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        أرجع m_dtypep;
    }
    صحيح عرض() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->عرض();
    }
    صحيح عرض_Min() ثابت {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        أرجع m_dtypep->عرض_Min();
    }
    منطقي prelim() ثابت { أرجع m_stage & PRELIM; }
    منطقي final() ثابت { أرجع m_stage & FINAL; }
    فراغ dump(std::ostream& str) ثابت {
        إذا (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } وإلا {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_مقصد_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& عملگر<<(std::ostream& str, ثابت عرض_VP* vup) {
    إذا (vup) vup->dump(str);
    أرجع str;
}

//######################################################################

صنف عرض_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    فراغ clear_عرض_Recurse(Ast_گره* nodep) {
        لكل (; nodep; nodep = nodep->nextp()) {
            nodep->did_عرض(خطأ);
            إذا (Ast_گره* ثابت refp = nodep->op1p()) clear_عرض_Recurse(refp);
            إذا (Ast_گره* ثابت refp = nodep->op2p()) clear_عرض_Recurse(refp);
            إذا (Ast_گره* ثابت refp = nodep->op3p()) clear_عرض_Recurse(refp);
            إذا (Ast_گره* ثابت refp = nodep->op4p()) clear_عرض_Recurse(refp);
        }
    }

عام:
    // CONSTRUCTORS
    صریح عرض_Clear_Visitor(AstNetlist* nodep) { clear_عرض_Recurse(nodep); }
    مجازی ~عرض_Clear_Visitor() = افتراضي;
};

//######################################################################

#تعریف_کن accept درون_عرض_Visitor_use_Ast_گره_iterate_instead_از_Ast_گره_accept

//######################################################################

صنف عرض_Visitor final : عام VNVisitor {
    // TYPES
    استفاده PatVecMap = std::map<صحيح, AstPatMember*>;
    استفاده DTypeMap = std::map<ثابت std::string, AstPatMember*>;

    // STATE
    V3_Unique_نامها m_enum_موقت_نامها;  // Captured enum expressions
    V3_Unique_نامها m_inside_موقت_نامها;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ثابت AstModport*, std::unordered_مجموعه<string>>
        m_modport_نامها;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    عرض_VP* m_vup = اشاره_تهی;  // Current node state
    منطقي m_underFork = خطأ;  // Visiting under a fork
    منطقي m_underSExpr = خطأ;  // Visiting under a sequence expression
    منطقي m_underPackedArray = خطأ;  // Visiting under a AstPackArrayDType
    منطقي m_underMemberSel = خطأ;  // Viting under a MemberSel
    Ast_گره* m_seqUnsupp = اشاره_تهی;  // Property has unsupported node
    منطقي m_hasSExpr = خطأ;  // Property has a sequence expression
    ثابت AstCell* m_cellp = اشاره_تهی;  // Current cell for arrayed instantiations
    std::وکتور<AstVar*> m_تقسیم_Iface_Array_Varps;  // Interface arrays split by V3Param
    ثابت Ast_Enum_قلم* m_enumItemp = اشاره_تهی;  // Current enum item
    Ast_گره_FTask* m_ftaskp = اشاره_تهی;  // Current function/task
    Ast_گره_Module* m_modep = اشاره_تهی;  // Current module
    ثابت AstConstraint* m_constraintp = اشاره_تهی;  // Current constraint
    Ast_گره_Procedure* m_procedurep = اشاره_تهی;  // Current final/always
    ثابت AstWith* m_withp = اشاره_تهی;  // Current 'with' statement
    ثابت AstFunc* m_funcp = اشاره_تهی;  // Current function
    ثابت Ast_Attr_از* m_attrp = اشاره_تهی;  // Current attribute
    ثابت Ast_گره_عبارت* m_randomizeFromp = اشاره_تهی;  // Current randomize method call fromp
    ثابت منطقي m_paramsOnly;  // Computing parameter value; limit operation
    ثابت منطقي m_doGenerate;  // Do errors later inside generate statement
    منطقي m_جریان_Concat = خطأ;  // True if visiting arguments of stream concatenation
    منطقي m_درون_Param_Override = خطأ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ثابت Ast_گره_DType*, VAttrType>, AstVarRef*> m_جدول_Map;
    // Queues with given index type
    std::map<ثابت Ast_گره_DType*, Ast_صف_DType*> m_صف_DType_نمایهای;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_مجموعه<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_مجموعه<ثابت AstVar*> m_curModVars;  // Variables declared in current module

    ساكن constexpr صحيح ENUM_LOOKUP_بیتها = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    تعداد Extend_قاعده : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    تعداد صنف ComparisonType : uint8_t {
        نرمال_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        نرمال_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ساكن جریان_Use جریان_Use_For_Determ(Determ determ) {
        أرجع determ == تخصیص_بده_3 ? جریان_USE_تخصیص_بده : جریان_USE_NONE;
    }

    جریان_Use current_جریان_Use() ثابت { أرجع m_vup ? m_vup->جریان_Use_2() : جریان_USE_NONE; }

    ساكن فراغ packIfUnpacked(Ast_گره_عبارت* ثابت nodep) {
        إذا (AstUnpackArrayDType* ثابت unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ثابت صحيح elements_رقم = unpackDTypep->arrayUnpackedElements();
            ثابت صحيح unpack_Min_بیتها = elements_رقم * unpackDTypep->subDTypep()->عرض_Min();
            ثابت صحيح unpack_بیتها = elements_رقم * unpackDTypep->subDTypep()->عرض();
            VNRelinker relinker;
            nodep->unlink_Fr_پشت(&relinker);
            relinker.relink(جديد Ast_Cvt_Array_مقصد_Packed{
                nodep->fileline(), nodep,
                nodep->پیدا_کن_بیت_Or_Logic_DType(unpack_بیتها, unpack_Min_بیتها, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ساكن Ast_گره_DType* fromDTypep(Ast_گره* fromp) {
        إذا (Ast_گره_DType* ثابت dtypep = VN_CAST(fromp, گره_DType))
            أرجع dtypep->skipRefOrNullp();
        أرجع fromp ? fromp->dtypep() : اشاره_تهی;
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
    فراغ بازدید(AstLogNot* nodep) override { بازدید_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    فراغ بازدید(AstLogAnd* nodep) override { بازدید_log_and_or(nodep); }
    فراغ بازدید(AstLogOr* nodep) override { بازدید_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    فراغ بازدید(AstSAnd* nodep) override { بازدید_log_and_or(nodep); }
    فراغ بازدید(AstSIntersect* nodep) override { بازدید_log_and_or(nodep); }
    فراغ بازدید(AstSOr* nodep) override { بازدید_log_and_or(nodep); }
    فراغ بازدید(AstSWithin* nodep) override { بازدید_log_and_or(nodep); }
    فراغ بازدید(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        بازدید_log_and_or(nodep);
    }
    فراغ بازدید(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        بازدید_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    فراغ بازدید(AstRedAnd* nodep) override { بازدید_red_and_or(nodep); }
    فراغ بازدید(AstRedOr* nodep) override { بازدید_red_and_or(nodep); }
    فراغ بازدید(AstRedXor* nodep) override { بازدید_red_and_or(nodep); }
    فراغ بازدید(AstOneHot* nodep) override { بازدید_red_and_or(nodep); }
    فراغ بازدید(AstOneHot0* nodep) override { بازدید_red_and_or(nodep); }
    فراغ بازدید(AstIsUnknown* nodep) override {
        بازدید_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    فراغ بازدید(AstEq* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_EQ); }
    فراغ بازدید(AstNeq* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_EQ); }
    فراغ بازدید(AstGt* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstGte* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstLt* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstLte* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstGtS* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstGteS* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstLtS* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstLteS* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::نرمال_GL); }
    فراغ بازدید(AstEqCase* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::CASE); }
    فراغ بازدید(AstNeqCase* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    فراغ بازدید(AstEqWild* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::WILD); }
    فراغ بازدید(AstNeqWild* nodep) override { بازدید_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    فراغ بازدید(AstEqD* nodep) override { بازدید_cmp_real(nodep); }
    فراغ بازدید(AstNeqD* nodep) override { بازدید_cmp_real(nodep); }
    فراغ بازدید(AstLtD* nodep) override { بازدید_cmp_real(nodep); }
    فراغ بازدید(AstLteD* nodep) override { بازدید_cmp_real(nodep); }
    فراغ بازدید(AstGtD* nodep) override { بازدید_cmp_real(nodep); }
    فراغ بازدید(AstGteD* nodep) override { بازدید_cmp_real(nodep); }
    // ...    String compares
    فراغ بازدید(AstEqN* nodep) override { بازدید_cmp_string(nodep); }
    فراغ بازدید(AstNeqN* nodep) override { بازدید_cmp_string(nodep); }
    فراغ بازدید(AstLtN* nodep) override { بازدید_cmp_string(nodep); }
    فراغ بازدید(AstLteN* nodep) override { بازدید_cmp_string(nodep); }
    فراغ بازدید(AstGtN* nodep) override { بازدید_cmp_string(nodep); }
    فراغ بازدید(AstGteN* nodep) override { بازدید_cmp_string(nodep); }
    // ...    Data type compares
    فراغ بازدید(AstEqT* nodep) override { بازدید_cmp_type(nodep); }
    فراغ بازدید(AstNeqT* nodep) override { بازدید_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    فراغ بازدید(AstAnd* nodep) override { بازدید_boolexpr_and_or(nodep); }
    فراغ بازدید(AstOr* nodep) override { بازدید_boolexpr_and_or(nodep); }
    فراغ بازدید(AstXor* nodep) override { بازدید_boolexpr_and_or(nodep); }
    فراغ بازدید(AstBufIf1* nodep) override {
        بازدید_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    فراغ بازدید(Ast_جمع_کن* nodep) override { بازدید_جمع_کن_sub_replace(nodep, درست, خطأ); }
    فراغ بازدید(AstSub* nodep) override { بازدید_جمع_کن_sub_replace(nodep, درست, خطأ); }
    فراغ بازدید(AstDiv* nodep) override { بازدید_جمع_کن_sub_replace(nodep, درست, درست); }
    فراغ بازدید(AstMul* nodep) override { بازدید_جمع_کن_sub_replace(nodep, درست, خطأ); }
    // These can't promote to real
    فراغ بازدید(AstModDiv* nodep) override { بازدید_جمع_کن_sub_replace(nodep, خطأ, درست); }
    فراغ بازدید(AstModDivS* nodep) override { بازدید_جمع_کن_sub_replace(nodep, خطأ, درست); }
    فراغ بازدید(AstMulS* nodep) override { بازدید_جمع_کن_sub_replace(nodep, خطأ, خطأ); }
    فراغ بازدید(AstDivS* nodep) override { بازدید_جمع_کن_sub_replace(nodep, خطأ, درست); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    فراغ بازدید(AstNegate* nodep) override { بازدید_negate_not(nodep, درست); }
    // Unary never real
    فراغ بازدید(AstNot* nodep) override { بازدید_negate_not(nodep, خطأ); }

    // Real: inputs and output real
    فراغ بازدید(Ast_جمع_کن_D* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    فراغ بازدید(AstSubD* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    فراغ بازدید(AstDivD* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    فراغ بازدید(AstMulD* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    فراغ بازدید(AstPowD* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    فراغ بازدید(Ast_گره_System_Biop_D* nodep) override { بازدید_real_جمع_کن_sub(nodep); }
    // Real: Output real
    فراغ بازدید(AstNegateD* nodep) override { بازدید_real_neg_ceil(nodep); }
    فراغ بازدید(Ast_گره_System_Uniop_D* nodep) override { بازدید_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    فراغ بازدید(AstSigned* nodep) override { بازدید_signed_unsigned(nodep, VSigning::SIGNED); }
    فراغ بازدید(AstUnsigned* nodep) override { بازدید_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    فراغ بازدید(Ast_لغزش_L* nodep) override { بازدید_لغزش_2(nodep); }
    فراغ بازدید(Ast_لغزش_R* nodep) override { بازدید_لغزش_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    فراغ بازدید(Ast_لغزش_RS* nodep) override { بازدید_لغزش_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    فراغ بازدید(Ast_بیتها_مقصد_Real_D* nodep) override { بازدید_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    فراغ بازدید(AstRToIS* nodep) override { بازدید_Os32_Lr(nodep); }
    فراغ بازدید(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->عرض_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    فراغ بازدید(Ast_Real_مقصد_بیتها* nodep) override { بازدید_Ou64_Lr(nodep); }

    // Output integer, input string
    فراغ بازدید(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_Int();
        }
    }
    فراغ بازدید(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_مجموعه_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    فراغ بازدید(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت_Sized(8, VSigning::UNSIGNED);
        }
    }
    فراغ بازدید(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت_Sized(8, VSigning::UNSIGNED);
        }
    }
    فراغ بازدید(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        إذا (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_مجموعه_String();
        }
    }
    فراغ بازدید(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_Int();
        }
    }
    فراغ بازدید(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            إذا (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_مجموعه_Double();
            } وإلا {
                nodep->dtype_مجموعه_Integer();
            }
        }
    }
    فراغ بازدید(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    فراغ بازدید(Ast_زمان* nodep) override { nodep->dtype_مجموعه_Time2_State(); }
    فراغ بازدید(Ast_زمان_D* nodep) override { nodep->dtype_مجموعه_Double(); }
    فراغ بازدید(Ast_زمان_Precision* nodep) override { nodep->dtype_مجموعه_Integer2_State(); }
    فراغ بازدید(Ast_Get_Initial_Random_دانه* nodep) override { nodep->dtype_مجموعه_Int(); }
    فراغ بازدید(Ast_زمان_Unit* nodep) override {
        nodep->replaceWith(
            جديد AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_از_Ten()});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstScopeName* nodep) override {
        nodep->dtype_مجموعه_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ساكن منطقي is_Override_الگو(ثابت Ast_گره* nodep) {
        إذا (ثابت Ast_الگو* ثابت patternp = VN_CAST(nodep, الگو)) {
            أرجع !patternp->فرزند_DTypep();
        }
        ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond);
        أرجع condp && is_Override_الگو(condp->thenp()) && is_Override_الگو(condp->elsep());
    }
    فراغ بازدید(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_عبارت(nodep);
        إذا (m_درون_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ثابت صحيح errors = V3Error::error_شمار();
            {
                VL_RESTORER(m_درون_Param_Override);
                m_درون_Param_Override = خطأ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ثابت AstConst* ثابت constp = VN_CAST(nodep->condp(), Const);
            ثابت منطقي معلوم = constp && !constp->رقم().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            إذا (!معلوم && V3Error::error_شمار() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_گره_عبارت* ثابت keepp
                = (معلوم && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_پشت();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(keepp, عرض_VP{SELF, BOTH}.p());
            أرجع;
        }
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_بعدی(nodep->thenp(), عرض_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_بعدی(nodep->elsep(), عرض_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_گره_DType* ثابت thenDTypep = nodep->thenp()->dtypep();
            Ast_گره_DType* ثابت elseDTypep = nodep->elsep()->dtypep();
            Ast_گره_DType* ثابت thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_گره_DType* ثابت elseDTypeSkippedp = elseDTypep->skipRefp();
            إذا (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->پیدا_کن_Null_DType());
            } وإلا إذا (nodep->thenp()->is_Class_Handle_مقدار()
                       || nodep->elsep()->is_Class_Handle_مقدار()) {
                Ast_گره_DType* commonClassTypep = اشاره_تهی;
                إذا (nodep->thenp()->is_Class_Handle_مقدار() && nodep->elsep()->is_Class_Handle_مقدار()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_گره::getCommonClassTypep(nodep->thenp(), nodep->elsep());
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
                إذا (Ast_گره::computeCastable(elseDTypep, thenDTypep, اشاره_تهی).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } وإلا {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    إذا (!Ast_گره::computeCastable(thenDTypep, elseDTypep, اشاره_تهی)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } وإلا إذا (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_مجموعه_Double();
            } وإلا إذا (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_مجموعه_String();
            } وإلا {
                ثابت منطقي is_Any_نتیجه_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                منطقي handled = خطأ;
                ثابت منطقي isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_گره_DType* ثابت thenEnumDTypep = thenDTypep->skip_Ref_مقصد_Enump();
                إذا (VN_IS(thenDTypeSkippedp, گره_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_درخت(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = درست;
                } وإلا إذا (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_درخت(elseDTypep->skip_Ref_مقصد_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    إذا (is_Any_نتیجه_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = درست;
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
                        nodep->fileline(جديد File_سطر{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_کد::ENUMVALUE, درست);
                    }
                }
                إذا (!handled) {
                    ثابت صحيح عرض = std::max(thenDTypep->عرض(), elseDTypep->عرض());
                    ثابت صحيح عرض_Min = std::max(thenDTypep->عرض_Min(), elseDTypep->عرض_Min());
                    ثابت VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ثابت منطقي isFourstate = is_Any_نتیجه_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    إذا ((عرض == elseDTypeSkippedp->عرض()
                         && عرض_Min == elseDTypeSkippedp->عرض_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } وإلا إذا ((عرض == thenDTypeSkippedp->عرض()
                                && عرض_Min == thenDTypeSkippedp->عرض_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } وإلا {
                        // None of the children dtypes match generate one
                        nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(عرض, عرض_Min, numeric, isFourstate);
                    }
                }
            }
        }
        إذا (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    فراغ بازدید(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, عرض_VP{SELF, BOTH}.p());
        // Conversions
        إذا (Ast_گره_عبارت* ثابت newp = new_Cons_Dyn_Array_Or_صف(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            جمع_کن_Cons_قلم(newp, nodep->lhsp()->unlink_Fr_پشت());
            جمع_کن_Cons_قلم(newp, nodep->rhsp()->unlink_Fr_پشت());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        }
        إذا (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_الگو* ثابت newp = جديد Ast_الگو{nodep->fileline(), اشاره_تهی};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        }

        // Concat handling
        إذا (m_vup->prelim()) {
            إذا (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, صف_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            إذا (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_مجموعه_String();
            } وإلا {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                إذا (m_جریان_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                    nodep->lhsp()->عرض() + nodep->rhsp()->عرض(),
                    nodep->lhsp()->عرض_Min() + nodep->rhsp()->عرض_Min(), VSigning::UNSIGNED,
                    Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->lhsp(), Replicate)) {
                إذا (repp->عرض() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_پشت());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep->rhsp(), Replicate)) {
                إذا (repp->عرض() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_پشت());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
            }
        }
        إذا (m_vup->final()) {
            إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_گره_عبارت* lhsp = nodep->lhsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* rhsp = nodep->rhsp()->unlink_Fr_پشت();
                إذا (!lhsp->isString()) lhsp = جديد AstCvtPackString{lhsp->fileline(), lhsp};
                إذا (!rhsp->isString()) rhsp = جديد AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_گره* ثابت newp = جديد AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            }
            إذا (!nodep->dtypep()->عرض_Sized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ بازدید(AstConcatN* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_String();
        }
        إذا (m_vup->final()) {
            nodep->did_عرض(درست);
            إذا (!nodep->dtypep()->عرض_Sized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    فراغ بازدید(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    فراغ بازدید(AstDelay* nodep) override {
        إذا (nodep->is_چرخه_Delay() && m_underSExpr) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            إذا (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            إذا (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                أرجع;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_چرخه_Delay(nodep, nodep);
            أرجع;
        }
        إذا (Ast_گره_عبارت* ثابت fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            أرجع;
        }
        إذا (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_مجموعه_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_بعدی_Null(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_مجموعه_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } وإلا {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->جمع_کن_بعدی_اینجا(nodep->stmtsp()->unlink_Fr_پشت_With_بعدی());
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    فراغ بازدید(AstFork* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        إذا (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_مجموعه_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ثابت newp = جديد AstBegin{nodep->fileline(), nodep->name(), اشاره_تهی, خطأ};
            nodep->replaceWith(newp);
            إذا (nodep->declsp()) newp->جمع_کن_Declsp(nodep->declsp()->unlink_Fr_پشت_With_بعدی());
            إذا (nodep->stmtsp()) newp->جمع_کن_Stmtsp(nodep->stmtsp()->unlink_Fr_پشت_With_بعدی());
            إذا (nodep->forksp()) newp->جمع_کن_Stmtsp(nodep->forksp()->unlink_Fr_پشت_With_بعدی());
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (v3Global.opt.timing().is_مجموعه_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = درست;
            iterateChildren(nodep);
        } وإلا إذا (v3Global.opt.timing().is_مجموعه_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
        } وإلا {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    فراغ بازدید_Wait_Or_Disable_Fork(Ast_گره* nodep) {
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_مجموعه_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
                أرجع;
            } وإلا إذا (!v3Global.opt.timing().is_مجموعه_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    فراغ بازدید(AstDisableFork* nodep) override { بازدید_Wait_Or_Disable_Fork(nodep); }
    فراغ بازدید(AstWaitFork* nodep) override { بازدید_Wait_Or_Disable_Fork(nodep); }
    فراغ بازدید(Ast_مقصد_پایینی_N* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_String();
        }
    }
    فراغ بازدید(Ast_مقصد_فوقانی_N* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_String();
        }
    }
    فراغ بازدید(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
            إذا (constp) {
                إذا (constp->رقم().isFourState()
                    || (constp->dtypep()->isSigned() && constp->رقم().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } وإلا {
                    times = constp->مقصد_SInt();
                }
            }

            Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
            إذا (VN_IS(vdtypep, صف_DType) || VN_IS(vdtypep, DynArrayDType)
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
                    user_Iterate_And_بعدی(nodep->srcp(), عرض_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_پشت());
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                } وإلا {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_الگو* ثابت newp = جديد Ast_الگو{
                        nodep->fileline(),
                        جديد AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_پشت(),
                                         اشاره_تهی, اشاره_تهی}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
                Ast_گره* ثابت newp
                    = جديد AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_پشت(),
                                        nodep->countp()->unlink_Fr_پشت()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
                nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                    (nodep->srcp()->عرض() * times), (nodep->srcp()->عرض_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->عرض_Sized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ بازدید(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_مجموعه_String();
        }
        إذا (m_vup->final()) {
            إذا (!nodep->dtypep()->عرض_Sized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    فراغ بازدید(Ast_گره_Dist_Biop* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    فراغ بازدید(Ast_گره_Dist_Triop* nodep) override {
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    منطقي جریان_Implicit_Use_Allowed(ثابت Ast_گره_جریان* nodep) ثابت {
        إذا (m_جریان_Concat) أرجع درست;
        إذا (m_vup && m_vup->جریان_Use_2() != جریان_USE_NONE) أرجع درست;

        ثابت Ast_گره* ثابت backp = nodep->backp();
        إذا (!backp) أرجع درست;  // Error elsewhere

        أرجع VN_IS(backp, گره_تخصیص_بده);
    }
    فراغ بازدید(Ast_گره_جریان* nodep) override {
        VL_RESTORER(m_جریان_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            m_جریان_Concat = درست;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_جریان_Concat = خطأ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->rhsp(), Const)) {
                إذا (constp->مقصد_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } وإلا {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ثابت Ast_گره_DType* ثابت lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_مقصد_Enump();
            إذا (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, صف_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, گره_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, گره_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_مجموعه_جریان();
            } وإلا إذا (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_مجموعه_جریان();
            } وإلا {
                nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                    nodep->lhsp()->عرض(), nodep->lhsp()->عرض_Min(), VSigning::UNSIGNED,
                    Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            إذا (!جریان_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_بیشتر() << "... Suggest use a cast");
            }
            إذا (!nodep->dtypep()->عرض_Sized()) {
                // See also error in V3Number
                گره_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    فراغ بازدید(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), درست,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), درست,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        إذا (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ثابت صحيح عرض = nodep->elementsConst();
            إذا (nodep->fromBracket() && nodep->چپ_Const() > nodep->راست_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-عرض + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } وإلا إذا (عرض > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << عرض << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ثابت منطقي درون_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ثابت منطقي درون_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ثابت منطقي درون_Type_جدول = !m_modep;
            ثابت Ast_گره* basep = nodep->backp();
            طالما (VN_IS(basep, Range)) basep = basep->backp();
            إذا (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !درون_Dead_Module && !درون_Parameterized_Template && !درون_Type_جدول) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->چپ_Const() << ":" << nodep->راست_Const()
                                            << "]");
            }
        }
    }

    فراغ بازدید(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        إذا (nodep->did_عرض()) أرجع;
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->lsbp(), عرض_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), خطأ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            صحيح عرض = nodep->عرض_Const();
            إذا (عرض <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_مجموعه_بیت();
                أرجع;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ثابت منطقي isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ثابت منطقي درون_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            إذا (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                عرض = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_مجموعه_بیت_Or_Logic_Sized(عرض, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->عرض_Const(عرض);
                افزودن_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(جديد AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            إذا (nodep->fromp()->عرض() < عرض && !درون_Parameterized_Template) {
                // If it is not a lvalue, extend it
                إذا (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << عرض << " bits from only "
                                                          << nodep->fromp()->عرض()
                                                          << " bit number");
                    Ast_گره_DType* ثابت subDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                        عرض, عرض, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    عرض_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    خطأ /*noerror*/);
                } وإلا {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << عرض << " bits to only "
                                                         << nodep->fromp()->عرض()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            صحيح frommsb = nodep->fromp()->عرض() - 1;
            صحيح fromlsb = 0;
            ثابت صحيح elw = nodep->decl_El_عرض();  // Must adjust to tell user bit ranges
            إذا (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } وإلا {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ثابت صحيح selwidth = V3_عدد::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گره_DType* ثابت selwidthDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, FINAL}.p());
            user_Iterate_And_بعدی(nodep->lsbp(), عرض_VP{SELF, FINAL}.p());
            إذا (عرض_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->عرض() != 32) {
                إذا (!nodep->fileline()->warnIsOff(V3_Error_کد::عرض_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->عرض() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->عرض() / elw)
                            << (nodep->lsbp()->عرض() != nodep->lsbp()->عرض_Min()
                                    ? " or " + cvt_مقصد_Str(nodep->lsbp()->عرض_Min() / elw)
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
                } وإلا إذا (!درون_Parameterized_Template) {
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
                    ثابت صحيح extend_مقصد = nodep->msbConst() + 1;
                    Ast_گره_DType* ثابت subDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                        extend_مقصد, extend_مقصد, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    عرض_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
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
                عرض_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    فراغ بازدید(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            إذا (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep->bitp(), Const)) {
                    ثابت صحيح بیت_2 = constp->مقصد_SInt();
                    Ast_گره_عبارت* newp = newIfaceArrayElement(nodep->fromp(), بیت_2);
                    إذا (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ثابت AstUnpackArrayDType* ثابت arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << بیت_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
            }
            //
            صحيح frommsb;
            صحيح fromlsb;
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            إذا (ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                إذا (fromlsb > frommsb) std::جابجا(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                إذا (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } وإلا {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ثابت صحيح selwidth = V3_عدد::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_گره_DType* ثابت selwidthDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            إذا (عرض_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->عرض() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->عرض(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->عرض()
                                       << (nodep->bitp()->عرض() != nodep->bitp()->عرض_Min()
                                               ? " or " + cvt_مقصد_Str(nodep->bitp()->عرض_Min())
                                               : "")
                                       << " bits.");
                إذا (!nodep->fileline()->warnIsOff(V3_Error_کد::عرض_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            إذا (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                إذا (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->مقصد_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->مقصد_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    إذا (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->مقصد_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                عرض_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                خطأ /*NOWARN*/);
            }
        }
    }

    فراغ بازدید(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->کلید_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ بازدید(AstAlias* nodep) override {
        إذا (!nodep->did_عرض_And_مجموعه()) {
            user_Iterate_And_بعدی(nodep->itemsp(), عرض_VP{SELF, BOTH}.p());
        }

        ثابت خودکار check_If_عبارت_Ok = [هذا](ثابت Ast_گره_عبارت* ثابت exprp) {
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
            إذا (m_aliasedVars.پیدا_کن(varp) != m_aliasedVars.انتها()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                أرجع خطأ;
            } وإلا {
                m_aliasedVars.درج(varp);
            }
            أرجع درست;
        };

        check_If_عبارت_Ok(nodep->itemsp());
        Ast_گره_DType* ثابت اولی_قلم_Dtypep = nodep->itemsp()->dtypep();
        لكل (Ast_گره* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_عبارت_Ok(VN_AS(itemp, گره_عبارت));
            إذا (!اولی_قلم_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << اولی_قلم_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    فراغ بازدید(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
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
                              adtypep->پیدا_کن_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    فراغ بازدید(Ast_برش_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ثابت Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ثابت AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            إذا (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_گره_DType* ثابت newDtp
                = جديد AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          جديد AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newDtp);
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
                        جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}}});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_درخت(خطأ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }

    فراغ بازدید(Ast_Sel_بیت* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->bitp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->thsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->attrp(), عرض_VP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3_عرض::عرض_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    فراغ بازدید(Ast_Sel_استخراج* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->leftp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->rightp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->attrp(), عرض_VP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3_عرض::عرض_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = اشاره_تهی;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    فراغ بازدید(AstSelPlus* nodep) override {
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->bitp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->widthp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->attrp(), عرض_VP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3_عرض::عرض_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = اشاره_تهی;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    فراغ بازدید(AstSelMinus* nodep) override {
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->bitp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->widthp(), عرض_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_بعدی(nodep->attrp(), عرض_VP{SELF, BOTH}.p());
        Ast_گره* ثابت selp = V3_عرض::عرض_Sel_No_Iter_Edit(nodep);
        إذا (selp != nodep) {
            nodep = اشاره_تهی;
            userIterate(selp, m_vup);
            أرجع;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    فراغ بازدید(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        إذا (m_vup && m_vup->prelim()) {
            إذا (VN_IS(nodep->dtypep()->skip_Ref_مقصد_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } وإلا إذا (nodep->رقم().isNull()) {
                nodep->dtypep(nodep->پیدا_کن_Null_DType());
            } وإلا إذا (nodep->رقم().isString()) {
                nodep->dtype_مجموعه_String();
            } وإلا إذا (nodep->رقم().isDouble()) {
                nodep->dtype_Chg_عرض(nodep->رقم().عرض(), nodep->رقم().sized()
                                                               ? nodep->رقم().عرض()
                                                               : nodep->رقم().عرض_مقصد_Fit());
            } وإلا {
                nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                    nodep->رقم().عرض(),
                    nodep->رقم().sized() ? nodep->رقم().عرض() : nodep->رقم().عرض_مقصد_Fit(),
                    nodep->dtypep()->numeric(), nodep->رقم().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    فراغ بازدید(Ast_خالی_صف* nodep) override {
        nodep->dtype_مجموعه_خالی_صف();
        إذا (!VN_IS(nodep->backp(), تخصیص_بده) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    فراغ بازدید(AstFell* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), اشاره_تهی);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstFalling* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstFuture* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), اشاره_تهی);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    فراغ بازدید(AstPast* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeFrom(nodep->exprp());
            إذا (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ درست);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ثابت AstConst* ثابت constp = VN_CAST(nodep->ticksp(), Const);
                إذا (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_پشت()->delete_درخت();
                } وإلا إذا (constp->مقصد_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_پشت()->delete_درخت();
                } وإلا إذا (V3AssertCommon::check_Sva_مقدار_حد(nodep, constp, "$past tick value")) {
                    إذا (constp->مقصد_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->مقصد_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), اشاره_تهی);
        }
    }
    فراغ بازدید(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->countp(), عرض_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
            إذا (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (minConstp->مقصد_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } وإلا إذا (!nodep->unbounded() && !nodep->maxCountp() && minConstp->مقصد_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            إذا (nodep->maxCountp()) {
                user_Iterate_And_بعدی(nodep->maxCountp(), عرض_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
                إذا (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (minConstp && maxConstp->مقصد_SInt() < minConstp->مقصد_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } وإلا إذا (maxConstp->مقصد_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->propp(), عرض_VP{SELF, BOTH}.p());
            Ast_گره_عبارت* propp = nodep->propp();
            إذا (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_بعدی(nodep->loBoundp(), عرض_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            إذا (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_بعدی(nodep->hiBoundp(), عرض_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ثابت منطقي loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ثابت منطقي hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            إذا (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_گره* ثابت boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_مجموعه_بیت();
                أرجع;
            }
            إذا (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_مجموعه_بیت();
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
            إذا (loConstp && loConstp->مقصد_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } وإلا إذا (loConstp) {
                V3AssertCommon::check_Sva_مقدار_حد(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_مقدار_حد(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            إذا (!hiUnbounded && loConstp && hiConstp && hiConstp->مقصد_SInt() < loConstp->مقصد_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            منطقي hasPropertyOp = propp->is_Multi_چرخه_Sva();
            إذا (!hasPropertyOp) {
                propp->foreach([&](ثابت Ast_گره* np) {
                    إذا (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = درست;
                    }
                });
            }
            إذا (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_عرض_Reduce(propp), propp);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstRising* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstRose* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), اشاره_تهی);
            nodep->dtype_مجموعه_بیت();
        }
    }

    فراغ بازدید(AstSampled* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    فراغ بازدید(AstSetuphold* nodep) override {
        File_سطر* ثابت flp = nodep->fileline();
        Ast_تخصیص_بده_W* newp = اشاره_تهی;
        إذا (nodep->delrefp()) {
            newp = convert_Setuphold_مقصد_تخصیص_بده(flp, nodep->refevp(), nodep->delrefp());
        }
        إذا (nodep->deldatap()) {
            إذا (!newp) {
                newp = convert_Setuphold_مقصد_تخصیص_بده(flp, nodep->dataevp(), nodep->deldatap());
            } وإلا {
                newp->جمع_کن_بعدی_اینجا(
                    convert_Setuphold_مقصد_تخصیص_بده(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        إذا (!newp) {
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        nodep->replaceWith(جديد AstAlways{newp});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    Ast_تخصیص_بده_W* convert_Setuphold_مقصد_تخصیص_بده(File_سطر* ثابت flp, Ast_گره_عبارت* ثابت evp,
                                         Ast_گره_عبارت* ثابت delp) {
        Ast_گره_عبارت* ثابت lhsp = delp->clone_درخت_خالص(خطأ);
        Ast_گره_عبارت* ثابت rhsp = evp->clone_درخت_خالص(خطأ);
        UASSERT_OBJ(VN_IS(lhsp, گره_Var_Ref) || VN_IS(lhsp, گره_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        إذا (Ast_گره_Var_Ref* varRefp = VN_CAST(lhsp, گره_Var_Ref)) {
            إذا (varRefp->varp()->direction() == VDirection::درون_داد) {
                VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                أرجع اشاره_تهی;
            }
            varRefp->access(VAccess::WRITE);
        }
        إذا (Ast_گره_Pre_Sel* selp = VN_CAST(lhsp, گره_Pre_Sel)) {
            إذا (Ast_گره_Var_Ref* varRefp = VN_CAST(selp->fromp(), گره_Var_Ref)) {
                إذا (varRefp->varp()->direction() == VDirection::درون_داد) {
                    VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                    أرجع اشاره_تهی;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        أرجع جديد Ast_تخصیص_بده_W{flp, lhsp, rhsp};
    }

    فراغ بازدید(AstStable* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            userIterate(nodep->sentreep(), اشاره_تهی);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstSteady* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ درست);
            nodep->dtype_مجموعه_بیت();
        }
    }

    فراغ بازدید(Ast_Stmt_عبارت* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{SELF, BOTH}.p());
    }

    فراغ بازدید(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }

    فراغ بازدید(AstUntil* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            إذا (nodep->lhsp()->عرض() != 1) fix_عرض_Reduce(nodep->lhsp());
            إذا (nodep->rhsp()->عرض() != 1) fix_عرض_Reduce(nodep->rhsp());
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstAbortOn* nodep) override { بازدید_Abort_Prop(nodep); }

    فراغ بازدید(AstRand* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->urandom()) {
                nodep->dtype_مجموعه_UInt32();  // Says the spec
            } وإلا {
                nodep->dtype_مجموعه_Int();  // Says the spec
            }
            إذا (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    فراغ بازدید(AstSEventually* nodep) override {
        إذا (v3Global.opt.timing().is_مجموعه_False() || !v3Global.opt.timing().is_مجموعه_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Widthed_مقدار{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            أرجع;
        }
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstSGotoRep* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_بعدی(nodep->countp(), عرض_VP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                عرض_Check_Goto_Rep_Range(nodep, "Goto");
            } وإلا {
                عرض_Check_Rep_شمار(nodep);
            }
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstSNonConsRep* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_بعدی(nodep->countp(), عرض_VP{SELF, BOTH}.p());
            إذا (nodep->maxCountp()) {
                عرض_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } وإلا {
                عرض_Check_Rep_شمار(nodep);
            }
            nodep->dtype_مجموعه_بیت();
        }
    }
    قالب <نام_نوع T_Rep>
    فراغ عرض_Check_Rep_شمار(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->countp(), Const);
        إذا (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->dtypep()->isSigned() && constp->رقم().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } وإلا إذا (constp->رقم().most_مجموعه_بیت_P1() > 31) {
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
    قالب <نام_نوع T_Rep>
    فراغ عرض_Check_Goto_Rep_Range(T_Rep* nodep, ثابت حرف* نوع) {
        user_Iterate_And_بعدی(nodep->maxCountp(), عرض_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ثابت AstConst* ثابت minConstp = VN_CAST(nodep->countp(), Const);
        ثابت AstConst* ثابت maxConstp = VN_CAST(nodep->maxCountp(), Const);
        إذا (!minConstp || !maxConstp) {
            nodep->v3error(std::string{نوع}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->مقصد_SInt() < 0) {
            nodep->v3error(std::string{نوع}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (maxConstp->مقصد_SInt() < minConstp->مقصد_SInt()) {
            nodep->v3error(std::string{نوع}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            أرجع;
        }
        إذا (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + نوع
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    فراغ بازدید(AstSThroughout* nodep) override {
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = خطأ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = درست;
            iterate(nodep->rhsp());
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = درست;
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            إذا (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = درست;
        m_hasSExpr = درست;
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            لكل (Ast_Sen_قلم* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_قلم)) {
                user_Iterate_And_بعدی(senp->sensp(), عرض_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstURandomRange* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            nodep->dtype_مجموعه_UInt32();  // Says the spec
            Ast_گره_DType* ثابت expDTypep = nodep->پیدا_کن_UInt32_DType();
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    فراغ بازدید(AstUnbounded* nodep) override {
        nodep->dtype_مجموعه_Int();  // Used in int context
        إذا (VN_IS(nodep->backp(), IsUnbounded)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), BracketArrayDType)) أرجع;  // Ok, leave
        إذا (VN_IS(nodep->backp(), InsideRange)) أرجع;  // Ok, leave
        إذا (ثابت خودکار* ثابت varp = VN_CAST(nodep->backp(), Var)) {
            إذا (varp->isParam()) أرجع;  // Ok, leave
        }
        Ast_گره* backp = nodep->backp();
        إذا (VN_IS(backp, Sub)) backp = backp->backp();
        إذا (ثابت خودکار* ثابت selp = VN_CAST(backp, Sel_استخراج)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), صف_DType)) أرجع;
        }
        إذا (ثابت خودکار* ثابت selp = VN_CAST(backp, Sel_بیت)) {
            إذا (VN_IS(selp->fromp()->dtypep()->skipRefp(), صف_DType)) أرجع;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    فراغ بازدید(AstInferredDisable* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) nodep->dtype_مجموعه_بیت();
    }
    فراغ بازدید(AstIsUnbounded* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        إذا (m_vup && m_vup->prelim()) {
            nodep->dtype_مجموعه_بیت_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        }
        إذا (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            إذا (nodep->عرض() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    فراغ بازدید(AstCLog2* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    قالب <نام_نوع T_Option>
    فراغ عرض_Cover_وزن(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_گره_عبارت* ثابت valuep = nodep->valuep();
        ثابت std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
            إذا (!constp->رقم().isNegative()) أرجع;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->مقصد_SInt()
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
        VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    قالب <نام_نوع T_Option>
    فراغ عرض_Cover_Int(T_Option* nodep) {
        user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    فراغ بازدید(Ast_Cg_Option_تخصیص_بده* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ثابت cgClassp = m_containingClassFinder.پیدا_کن(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        إذا (nodep->optType() == VCoverOptionType::وزن) {
            عرض_Cover_وزن(nodep);
            أرجع;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        إذا (nodep->optType() == VCoverOptionType::AT_کمترین) {
            عرض_Cover_Int(nodep);
            أرجع;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        إذا (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            إذا (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->مقصد_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->مقصد_SInt() << پایان_سطر);
            } وإلا {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } وإلا إذا (nodep->optType() == VCoverOptionType::ادغام_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_گره_عبارت* ثابت valuep = nodep->valuep();
            إذا (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(جديد AstConst{valuep->fileline(), AstConst::بیت_False{}});
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            }
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            أرجع;
        } وإلا إذا (nodep->optType() == VCoverOptionType::DISTRIBUTE_اولی) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ثابت AstConst* ثابت constp = VN_CAST(nodep->valuep(), Const);
            إذا (!constp || !constp->رقم().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }
    فراغ بازدید(AstCoverOption* nodep) override {
        إذا (nodep->optType() == VCoverOptionType::وزن) {
            عرض_Cover_وزن(nodep);
            أرجع;
        }
        إذا (nodep->optType() == VCoverOptionType::AT_کمترین
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            عرض_Cover_Int(nodep);
            أرجع;
        }
        user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstCoverCross* nodep) override {
        user_Iterate_And_بعدی(nodep->itemsp(), اشاره_تهی);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_بعدی(nodep->optionsp(), اشاره_تهی);
        user_Iterate_And_بعدی(nodep->binsp(), اشاره_تهی);
    }
    فراغ بازدید(AstCoverCrossBin* nodep) override {
        user_Iterate_And_بعدی(nodep->selectp(), اشاره_تهی);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    فراغ بازدید(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_بعدی(nodep->binsp(), عرض_VP{nodep->exprp()->dtypep(), BOTH}.p());
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_بعدی(nodep->optionsp(), اشاره_تهی);
    }
    فراغ عرض_Covergroup_Ranges(Ast_گره* rangesp, صحيح fill_عرض) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ثابت خودکار fill = [&](Ast_گره* nodep) {
            إذا (!fill_عرض) أرجع;
            Ast_گره_عبارت* exprp = VN_AS(nodep, گره_عبارت);
            fixAutoExtend(exprp /*ref*/, fill_عرض);
        };
        لكل (Ast_گره *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            إذا (AstInsideRange* ثابت rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, اشاره_تهی);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } وإلا إذا (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } وإلا {
                itemp = userIterateSubtreeReturnEdits(itemp, عرض_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    فراغ بازدید(AstCoverBinsof* nodep) override {
        user_Iterate_And_بعدی(nodep->pointp(), اشاره_تهی);
        عرض_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    فراغ بازدید(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        عرض_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->عرض() : 0);
        إذا (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        إذا (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_بعدی(nodep->arraySizep(), عرض_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_بعدی(nodep->transp(), m_vup);
    }
    فراغ بازدید(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_بعدی(nodep->itemp(), اشاره_تهی);
        إذا (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        إذا (!VN_IS(nodep->subp(), CoverpointRef)) {
            عرض_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->عرض());
        }
        عرض_Cover_Filter(nodep->filterp(), "Bin");
    }
    فراغ بازدید(AstCoverCrossWith* nodep) override {
        user_Iterate_And_بعدی(nodep->selectp(), اشاره_تهی);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_بعدی(nodep->varsp(), اشاره_تهی);
        عرض_Cover_Filter(nodep->filterp(), "Cross bin");
        إذا (Ast_گره_عبارت* ثابت countp = nodep->countp()) {
            إذا (VN_IS(countp, Unbounded)) {
                countp->dtype_مجموعه_Int();  // 'matches $'
            } وإلا {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_بعدی(countp, عرض_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    فراغ عرض_Cover_Filter(Ast_گره_عبارت* filterp, ثابت حرف* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, عرض_VP{SELF, BOTH}.p()), گره_عبارت);
        File_سطر* ثابت fl = filterp->fileline();
        ثابت Ast_گره_DType* ثابت resultp = filterp->dtypep()->skipRefp();
        إذا (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_پشت(&relinker);
            relinker.relink(
                جديد AstNeqD{fl, filterp, جديد AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } وإلا إذا (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(جديد AstConst{fl, AstConst::بیت_False{}});
            VL_DO_DANGLING(افزودن_Deletep(filterp), filterp);
        } وإلا {
            fix_عرض_Reduce(filterp);
        }
    }
    فراغ بازدید(Ast_Cover_Trans_مجموعه* nodep) override { user_Iterate_And_بعدی(nodep->itemsp(), m_vup); }
    فراغ بازدید(Ast_Cover_Trans_قلم* nodep) override {
        عرض_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->عرض() : 0);
    }
    فراغ بازدید(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
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
                             && !VN_AS(nodep->rhsp(), Const)->رقم().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_مجموعه_Logic_Unsized(nodep->عرض(), nodep->عرض_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        إذا (m_vup->final()) {
            Ast_گره_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->پیدا_کن_بیت_Or_Logic_DType(expDTypep->عرض(), expDTypep->عرض_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_گره* newp = اشاره_تهی;  // No change
            إذا (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                    nodep->rhsp()->unlink_Fr_پشت()};
            } وإلا إذا (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = جديد AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                    nodep->rhsp()->unlink_Fr_پشت()};
            } وإلا إذا (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = جديد AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                    nodep->rhsp()->unlink_Fr_پشت()};
            }
            إذا (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }
    فراغ بازدید(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_شمار_بیتها* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح عرض_Min = V3_عدد::log2b(nodep->lhsp()->عرض()) + 1;
            nodep->dtype_مجموعه_بیت_Unsized(32, عرض_Min, VSigning::SIGNED);
        }
    }
    فراغ بازدید(Ast_شمار_Ones* nodep) override {
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ثابت صحيح عرض_Min = V3_عدد::log2b(nodep->lhsp()->عرض()) + 1;
            nodep->dtype_مجموعه_بیت_Unsized(32, عرض_Min, VSigning::SIGNED);
        }
    }
    فراغ بازدید(AstCvtPackString* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH, current_جریان_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        إذا (Ast_گره_DType* ثابت dt = nodep->lhsp()->dtypep()) {
            إذا (VN_IS(dt->skip_Ref_مقصد_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    فراغ بازدید(Ast_Cvt_Packed_مقصد_Array* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH, current_جریان_Use()}.p());
        // Type set in constructor
    }
    فراغ بازدید(Ast_Cvt_Array_مقصد_Array* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH, current_جریان_Use()}.p());
        // Type set in constructor
    }
    فراغ بازدید(Ast_Cvt_Array_مقصد_Packed* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH, current_جریان_Use()}.p());
        // Type set in constructor
    }
    فراغ بازدید(Ast_Cvt_Unpack_مقصد_صف* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // Opaque returns, so arbitrary
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    فراغ بازدید(Ast_Cvt_Unpack_مقصد_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        إذا (m_vup->final()) {
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    فراغ بازدید(Ast_زمان_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        ثابت AstConst* ثابت constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ثابت مزدوج زمان_Prescale = constp->رقم().مقصد_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ثابت مزدوج زمان_2 = زمان_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::RealDouble{}, زمان_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstEventControl* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_مجموعه_True()) {
                iterateChildren(nodep);
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_مجموعه_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_بیشتر()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        إذا (nodep->stmtsp()) nodep->جمع_کن_بعدی_اینجا(nodep->stmtsp()->unlink_Fr_پشت());
        VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
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
            أرجع basep ? " extends class " + basep->dtypeName(درست) : "";
        }
        أرجع "";
    }
    فراغ بازدید(Ast_Attr_از* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
        إذا (nodep->dimp()) user_Iterate_And_بعدی(nodep->dimp(), عرض_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        بدل (nodep->attrType()) {
        حالة VAttrType::DIM_DIMENSIONS:
        حالة VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_گره_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ثابت std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(درست);
            ثابت صحيح val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.دومی
                                                                       : (dim.اولی + dim.دومی));
            nodep->replaceWith(جديد AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            اكسر;
        }
        حالة VAttrType::DIM_بیتها_OR_عدد: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            إذا (VN_IS(nodep->fromp(), گره_عبارت)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_پشت());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            } وإلا {
                Ast_گره* newp = جديد Ast_Attr_از{nodep->fileline(), VAttrType::DIM_بیتها,
                                              nodep->fromp()->unlink_Fr_پشت()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                user_Iterate_And_بعدی(newp, عرض_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            أرجع;
        }
        حالة VAttrType::DIM_بیتها:
        حالة VAttrType::DIM_بالا:
        حالة VAttrType::DIM_INCREMENT:
        حالة VAttrType::DIM_چپ:
        حالة VAttrType::DIM_کم:
        حالة VAttrType::DIM_راست:
        حالة VAttrType::DIM_اندازه: {
            Ast_گره_DType* ثابت dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            إذا (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ثابت Ast_گره_Module* ثابت ownModp = v3Global.rootp()->containingModule(dtypep);
                إذا (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_مجموعه_Int();
                    أرجع;
                }
            }

            إذا (VN_IS(dtypep, صف_DType) || VN_IS(dtypep, DynArrayDType)) {
                بدل (nodep->attrType()) {
                حالة VAttrType::DIM_اندازه: {
                    Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پشت(), گره_عبارت);
                    Ast_گره* ثابت newp
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_اندازه};
                    newp->dtype_مجموعه_Int();
                    newp->did_عرض(درست);
                    newp->protect(خطأ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_چپ:
                حالة VAttrType::DIM_کم: {
                    Ast_گره* ثابت newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_راست:
                حالة VAttrType::DIM_بالا: {
                    Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پشت(), گره_عبارت);
                    Ast_گره_عبارت* ثابت sizep
                        = جديد AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_اندازه};
                    sizep->dtype_مجموعه_Int();
                    sizep->did_عرض(درست);
                    sizep->protect(خطأ);
                    Ast_گره* ثابت newp
                        = جديد AstSub{nodep->fileline(), sizep,
                                     جديد AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_INCREMENT: {
                    Ast_گره_عبارت* ثابت newp
                        = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    اكسر;
                }
                حالة VAttrType::DIM_بیتها: {
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
                ثابت std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(درست);
                ثابت uint32_t msbdim = dimpair.اولی + dimpair.دومی;
                إذا (!nodep->dimp() || msbdim < 1) {
                    إذا (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پشت(), گره_عبارت);
                        Ast_گره* ثابت newp = جديد Ast_لغزش_L{
                            nodep->fileline(), جديد AstLenN{nodep->fileline(), fromp},
                            جديد AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    } وإلا {
                        Ast_گره_DType* ثابت پایه_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(پایه_DTypep, nodep, "Unsized expression");
                        ثابت صحيح dim = 1;
                        AstConst* ثابت newp = dimension_مقدار(nodep->fileline(), پایه_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    }
                } وإلا إذا (VN_IS(nodep->dimp(), Const)) {
                    ثابت صحيح dim = VN_AS(nodep->dimp(), Const)->مقصد_SInt();
                    AstConst* ثابت newp
                        = dimension_مقدار(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                } وإلا {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ثابت tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_گره_عبارت* ثابت dimp = nodep->dimp()->unlink_Fr_پشت();
                    Ast_گره_عبارت* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
            Ast_گره_DType* ثابت protoDtp = nodep->fromp()->dtypep();
            Ast_گره_DType* ثابت declDtp = [&]() {
                إذا (m_ftaskp->fvarp()) أرجع m_ftaskp->fvarp()->dtypep();
                Ast_گره_DType* ثابت voidp = جديد AstVoidDType{m_ftaskp->fileline()};
                افزودن_Deletep(voidp);  // Note voidp used past here
                أرجع voidp;
            }();
            إذا (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_بیشتر()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_بیشتر()
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
            Ast_گره_DType* ثابت dtypep = nodep->fromp()->dtypep();
            string نتیجه = dtypep->prettyDTypeName(درست);
            إذا (ثابت AstClassRefDType* ثابت classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                نتیجه += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << نتیجه << "' from " << dtypep);
            Ast_گره* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{}, نتیجه};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            اكسر;
        }
        حالة VAttrType::TYPEID:
            إذا (Ast_گره_DType* dtypep = VN_CAST(nodep->fromp(), گره_DType)) {
                nodep->dtypep(dtypep);
            } وإلا {
                nodep->dtypep(VN_AS(nodep->fromp(), گره_عبارت)->dtypep());
            }
            اكسر;
        حالة VAttrType::VAR_پایه:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            اكسر;
        افتراضي: {
            // Everything else resolved earlier
            nodep->dtype_مجموعه_بیت_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            اكسر;
        }
        }
    }
    فراغ بازدید(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_متن* nodep) override {
        // Only used in CStmts which don't care....
    }

    فراغ بازدید(AstCellArrayRef* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        user_Iterate_And_بعدی(nodep->selp(), عرض_VP{SELF, PRELIM}.p());
        user_Iterate_And_بعدی(nodep->selp(), عرض_VP{SELF, FINAL}.p());
        nodep->dtype_مجموعه_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_عرض(درست);
    }

    فراغ بازدید(AstCellRef* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;

        إذا (Ast_گره_عبارت* ثابت cellExprp = VN_CAST(nodep->cellp(), گره_عبارت)) {
            user_Iterate_And_بعدی(cellExprp, عرض_VP{SELF, PRELIM}.p());
            user_Iterate_And_بعدی(cellExprp, عرض_VP{SELF, FINAL}.p());
        }

        إذا (Ast_گره_عبارت* ثابت exprp = VN_CAST(nodep->exprp(), گره_عبارت)) {
            user_Iterate_And_بعدی(exprp, عرض_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_بعدی(exprp, عرض_VP{SELF, FINAL}.p());
        } وإلا {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_عرض(درست);
    }

    // DTYPES
    فراغ بازدید(Ast_گره_Array_DType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        إذا (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ثابت basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            إذا (basicp->implicit()) {
                UASSERT_OBJ(basicp->عرض() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->عرض() << " bits");
                AstBasicDType* ثابت newp = جديد AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->عرض_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        إذا (VN_IS(nodep, PackArrayDType)) m_underPackedArray = درست;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_بعدی(nodep->rangep(), عرض_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->عرض_From_Sub(nodep->subDTypep());
            إذا (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(درست);
        } وإلا {
            ثابت Ast_گره_DType* ثابت elemDTypep = nodep->subDTypep()->skipRefp();
            إذا (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ثابت صحيح عرض = nodep->subDTypep()->عرض() * nodep->rangep()->elementsConst();
            nodep->عرض_Force(عرض, عرض);
            إذا (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ثابت Ast_گره_DType* پایه_DTypep = nodep->subDTypep();
                طالما (!VN_IS(پایه_DTypep, RefDType) && پایه_DTypep->subDTypep()) {
                    پایه_DTypep = پایه_DTypep->subDTypep();
                }
                ثابت AstBasicDType* ثابت basicp = nodep->basicp();
                إذا (!VN_IS(پایه_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(درست));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstAssocArrayDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->کلید_DTypep(iterateEditMoveDTypep(nodep, nodep->کلید_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_گره_DType* ثابت childp = nodep->فرزند_DTypep();
        childp->unlink_Fr_پشت();
        Ast_گره* ثابت elementsp = nodep->elementsp()->unlink_Fr_پشت();
        Ast_گره* newp;
        إذا (VN_IS(elementsp, Unbounded)) {
            newp = جديد Ast_صف_DType{nodep->fileline(), VFlag_فرزند_DType{}, childp, اشاره_تهی};
            VL_DO_DANGLING(elementsp->delete_درخت(), elementsp);
        } وإلا إذا (Ast_گره_DType* ثابت keyp = VN_CAST(elementsp, گره_DType)) {
            newp = جديد AstAssocArrayDType{nodep->fileline(), VFlag_فرزند_DType{}, childp, keyp};
        } وإلا {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_سطر* ثابت elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_کد::WIDTHEXPAND, درست);
            // Must be expression that is constant, but we'll determine that later
            newp = جديد AstUnpackArrayDType{
                nodep->fileline(), VFlag_فرزند_DType{}, childp,
                جديد AstRange{nodep->fileline(), جديد AstConst(elementsp->fileline(), 0),
                             جديد AstSub{elementsNewFl, VN_AS(elementsp, گره_عبارت),
                                        جديد AstConst(elementsNewFl, 1)},
                             درست}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, اشاره_تهی), newp);
    }
    فراغ بازدید(AstConstraintRefDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    فراغ بازدید(AstDynArrayDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(Ast_صف_DType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_بعدی(nodep->boundp(), عرض_VP{SELF, BOTH}.p());
        إذا (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_پشت()->delete_درخت();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstVoidDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstUnsizedArrayDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->فرزند_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstWildcardArrayDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstBasicDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (nodep->generic()) أرجع;  // Already perfect
        إذا (nodep->rangep()) {
            user_Iterate_And_بعدی(nodep->rangep(), عرض_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->عرض_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } وإلا إذا (nodep->isRanged()) {
            nodep->عرض_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } وإلا إذا (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->عرض_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstConstDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, اشاره_تهی);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->عرض_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(AstRefDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_عرض(درست);
        إذا (nodep->typeofp()) {  // type(typeofp_expression)
            إذا (Ast_گره_DType* typeofDtp = VN_CAST(nodep->typeofp(), گره_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } وإلا {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_بعدی(nodep->typeofp(), عرض_VP{SELF, BOTH}.p());
                Ast_گره* ثابت typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_پشت()->delete_درخت(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, اشاره_تهی);
        إذا (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), اشاره_تهی);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), اشاره_تهی);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        إذا (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ثابت منطقي درون_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            إذا (درون_Template_Module) {
                nodep->doing_عرض(خطأ);
                أرجع;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->عرض_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_عرض(خطأ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    فراغ عرض_DType_Name_Params(ثابت AstTypedef* nodep) {
        ثابت VDTypeNameScopes* ثابت scopesp = VDTypeNameScopes::currentp();
        إذا (!scopesp) أرجع;  // Computing parameters, as by V3Param, so naming as found
        Ast_گره* scopep = اشاره_تهی;
        لكل (ثابت Ast_گره* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            إذا (ثابت std::وکتور<Ast_گره*>* ثابت paramsp = scopesp->paramsp(scopep)) {
                لكل (Ast_گره* ثابت paramp : *paramsp) {
                    إذا (!paramp->did_عرض()) userIterate(paramp, اشاره_تهی);
                }
            }
        }
    }
    فراغ بازدید(AstTypedef* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        إذا (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(اشاره_تهی);
            VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
            أرجع;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ثابت منطقي declares = nodep->فرزند_DTypep() != اشاره_تهی;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (declares) {
            Ast_گره_DType* ثابت dtypep = nodep->dtypep();
            إذا (Ast_گره_UOr_Struct_DType* ثابت sdtypep = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                عرض_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } وإلا إذا (AstEnumDType* ثابت edtypep = VN_CAST(dtypep, EnumDType)) {
                عرض_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, اشاره_تهی);
    }
    فراغ بازدید(AstParamTypeDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, اشاره_تهی);
        nodep->عرض_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        إذا (nodep->dtypep() && nodep->فرزند_DTypep()) { nodep->فرزند_DTypep(اشاره_تهی); }
    }
    فراغ بازدید(AstRequireDType* nodep) override {
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        إذا (Ast_گره_DType* ثابت dtp = VN_CAST(nodep->lhsp(), گره_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_پشت());
        } وإلا {
            إذا (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(جديد AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        Ast_گره_DType* ثابت مقصد_Dtp = nodep->بالاترین()->dtypep()->skip_Ref_مقصد_Enump();
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        File_سطر* ثابت fl = nodep->fileline();
        ثابت خودکار castable = Ast_گره::computeCastable(مقصد_Dtp, fromDtp, nodep->fromp());
        Ast_گره* newp;
        إذا (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            أرجع;
        } وإلا إذا (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ثابت enumDtp = VN_AS(مقصد_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_گره_عبارت* ثابت testp = enumTestValid(nodep->fromp(), enumDtp);
            File_سطر* ثابت fl_novalue = جديد File_سطر{fl};
            fl_novalue->warnOff(V3_Error_کد::ENUMVALUE, درست);
            newp = جديد AstCond{
                fl, testp,
                جديد Ast_عبارت_Stmt{fl,
                                جديد Ast_تخصیص_بده{fl_novalue, nodep->بالاترین()->unlink_Fr_پشت(),
                                              nodep->fromp()->unlink_Fr_پشت()},
                                جديد AstConst{fl, AstConst::Signed32{}, 1}},
                جديد AstConst{fl, AstConst::Signed32{}, 0}};
        } وإلا إذا ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(مقصد_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << مقصد_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_بیشتر() << "... Suggest static cast");
            newp = جديد Ast_عبارت_Stmt{
                fl,
                جديد Ast_تخصیص_بده{fl, nodep->بالاترین()->unlink_Fr_پشت(),
                              جديد AstCast{fl, nodep->fromp()->unlink_Fr_پشت(), مقصد_Dtp}},
                جديد AstConst{fl, AstConst::Signed32{}, 1}};
        } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << مقصد_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } وإلا {
            newp = جديد AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << مقصد_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_بیشتر()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    فراغ بازدید(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        إذا (AstConst* ثابت constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_پشت();
            Ast_گره* ثابت newp
                = جديد Ast_Cast_اندازه{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا إذا (Ast_گره_DType* ثابت refp = VN_CAST(nodep->dtp(), گره_DType)) {
            refp->unlink_Fr_پشت();
            Ast_گره* ثابت newp = جديد AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                              VFlag_فرزند_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_پشت());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(AstCast* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        إذا (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            إذا (AstSigned* ثابت fromp = VN_CAST(nodep->fromp(), Signed)) {
                إذا (VN_IS(fromp->lhsp(), گره_جریان)) {
                    Ast_گره* ثابت lhsp = fromp->lhsp()->unlink_Fr_پشت();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                }
            }
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, PRELIM, جریان_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_گره_DType* ثابت مقصد_Dtp = nodep->dtypep()->skip_Ref_مقصد_Enump();
            Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
            ثابت خودکار castable = Ast_گره::computeCastable(مقصد_Dtp, fromDtp, nodep->fromp());
            منطقي bad = خطأ;
            إذا (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << مقصد_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = درست;
            } وإلا إذا (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } وإلا إذا (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << مقصد_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_بیشتر() << "... Suggest dynamic $cast");
                bad = درست;
            } وإلا إذا (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << مقصد_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = درست;
            } وإلا {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_گره_عبارت* newp = اشاره_تهی;
            إذا (bad) {
            } وإلا إذا (AstBasicDType* ثابت basicp = مقصد_Dtp->basicp()) {
                إذا (!basicp->isString() && fromDtp->isString()) {
                    newp = جديد AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), مقصد_Dtp};
                } وإلا إذا (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_گره_DType* ثابت origDTypep = nodep->dtypep();
                    إذا (!VN_IS(fromDtp, جریان_DType)) {
                        ثابت صحيح عرض = مقصد_Dtp->عرض();
                        castSized(nodep, nodep->fromp(), عرض);
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
                        newp = جديد AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
                    } وإلا {
                        newp = جديد AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
                    }
                } وإلا إذا (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = جديد AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
                    newp->dtypep(basicp);
                } وإلا إذا (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = جديد AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
                } وإلا إذا (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = جديد AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
                } وإلا {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } وإلا إذا (VN_IS(مقصد_Dtp, صف_DType)) {
                إذا (VN_IS(fromDtp, BasicDType)) {
                    newp = جديد Ast_Cvt_Packed_مقصد_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_پشت(), مقصد_Dtp};
                } وإلا إذا (VN_IS(fromDtp, صف_DType) || VN_IS(fromDtp, جریان_DType)) {
                    صحيح src_Element_بیتها = 1;
                    صحيح dst_Element_بیتها = 1;
                    إذا (Ast_گره_DType* ثابت elemDtp = fromDtp->subDTypep()) {
                        src_Element_بیتها = elemDtp->عرض();
                    }
                    ثابت Ast_صف_DType* ثابت dst_صف_Dtp = VN_AS(مقصد_Dtp, صف_DType);
                    إذا (Ast_گره_DType* ثابت elemDtp = dst_صف_Dtp->subDTypep()) {
                        dst_Element_بیتها = elemDtp->عرض();
                    }
                    newp = جديد Ast_Cvt_Array_مقصد_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_پشت(),
                                                  مقصد_Dtp,
                                                  خطأ,
                                                  1,
                                                  dst_Element_بیتها,
                                                  src_Element_بیتها};
                }
            } وإلا إذا (VN_IS(مقصد_Dtp, ClassRefDType)) {
                // Can just remove cast
            } وإلا {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << مقصد_Dtp->prettyDTypeNameQ());
            }
            إذا (!newp) newp = nodep->fromp()->unlink_Fr_پشت();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        إذا (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, خطأ, جریان_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_گره_عبارت* ثابت underp = nodep->fromp()->unlink_Fr_پشت();
            underp->dtypeFrom(nodep);
            underp->did_عرض(درست);
            Ast_گره_عبارت* ثابت newp = جديد AstCastWrap{nodep->fileline(), underp};
            newp->did_عرض(درست);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(Ast_Cast_اندازه* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            صحيح عرض = nodep->rhsp()->مقصد_SInt();
            إذا (عرض < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << عرض);
                عرض = 1;
            }
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM, جریان_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), عرض);  // lhsp may change
        }
        إذا (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_گره* ثابت underp = nodep->lhsp()->unlink_Fr_پشت();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    فراغ بازدید(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{nodep->dtypep(), BOTH, جریان_USE_CAST}.p());
    }
    فراغ castSized(Ast_گره* nodep, Ast_گره* underp, صحيح عرض) {
        ثابت AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        إذا (!underDtp) underDtp = underp->dtypep()->basicp();
        إذا (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->پیدا_کن_بیت_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ثابت صحيح calc_عرض = std::max(عرض, underDtp->عرض());
            Ast_گره_DType* ثابت calcDtp = nodep->پیدا_کن_بیت_Or_Logic_DType(
                calc_عرض, calc_عرض, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         خطأ, جریان_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_گره_DType* ثابت بیرون_Dtp = nodep->پیدا_کن_بیت_Or_Logic_DType(
                عرض, عرض, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(بیرون_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            عرض_Check_Sized(nodep, "Cast expr", VN_AS(underp, گره_عبارت), بیرون_Dtp, EXTEND_EXP,
                            خطأ);
            VL_DANGLING(underp);
        }
    }
    فراغ بازدید(AstConstraintBefore* nodep) override {
        user_Iterate_And_بعدی(nodep->lhssp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->rhssp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_Constraint_عبارت* nodep) override {
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstConstraintUnique* nodep) override {
        user_Iterate_And_بعدی(nodep->rangesp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_Dist_قلم* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), عرض_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    فراغ بازدید(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        إذا (nodep->did_عرض()) أرجع;
        إذا (nodep->doing_عرض()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            افزودن_Deletep(nodep->valuep()->unlink_Fr_پشت());
            nodep->valuep(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_عرض(درست);
            أرجع;
        }
        nodep->doing_عرض(درست);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        إذا (!m_paramsOnly && nodep->is_Iface_Array_تقسیم()) m_تقسیم_Iface_Array_Varps.افزودن_پشت(nodep);
        إذا (nodep->attrsp()) {
            nodep->attrsp()->foreach([هذا, nodep](Ast_Attr_از* attrp) {
                إذا (attrp->attrType() == VAttrType::VAR_پورت_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    إذا (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), گره_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(افزودن_Deletep(attrp->unlink_Fr_پشت()), attrp);
                }
            });
        }
        إذا (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_گره_DType* dtp = nodep->dtypep();
            Ast_گره_DType* np = اشاره_تهی;
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
                    v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newp);
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
        إذا (VN_IS(nodep->dtypep()->skip_Ref_مقصد_Constp(), ConstDType)) nodep->isConst(درست);
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
                user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                إذا (nodep->valuep()->isDouble()) {
                    nodep->dtype_مجموعه_Double();
                    VL_DANGLING(bdtypep);
                } وإلا إذا (nodep->valuep()->isString()) {
                    nodep->dtype_مجموعه_String();
                    VL_DANGLING(bdtypep);
                } وإلا {
                    صحيح عرض = 0;
                    Ast_گره_DType* ثابت مقدار_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(مقدار_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ثابت AstBasicDType* ثابت مقدار_Bdtypep = مقدار_DTypep->basicp();
                    منطقي issigned = خطأ;
                    إذا (bdtypep->isNosign()) {
                        إذا (مقدار_Bdtypep && مقدار_Bdtypep->isSigned()) issigned = درست;
                    } وإلا {
                        issigned = bdtypep->isSigned();
                    }
                    إذا (مقدار_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        إذا (ثابت خودکار* ثابت constp = VN_CAST(nodep->valuep(), Const)) {
                            إذا (constp->رقم().isString()) {
                                عرض = constp->رقم().مقصد_String().درازی() * 8;
                            }
                        }
                        إذا (عرض < 8) عرض = 8;
                    } وإلا إذا (nodep->valuep()->dtypep()->عرض_Sized()) {
                        عرض = nodep->valuep()->عرض();
                    } وإلا {
                        إذا (nodep->valuep()->عرض() > 32) {
                            nodep->valuep()->v3warn(
                                عرض_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        عرض = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ثابت منطقي is_مقدار_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    إذا (عرض == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->پیدا_کن_بیت_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->عرض_Min(),
                            VSigning::fromBool(issigned), is_مقدار_Fourstate));
                    } وإلا {
                        nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(عرض, nodep->valuep()->عرض_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_مقدار_Fourstate);
                    }
                    didchk = درست;
                }
                iterate_Check_تخصیص_بده(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } وإلا {
                // Or, if nothing assigned, they're integral
                nodep->dtype_مجموعه_Integer();
                VL_DANGLING(bdtypep);
            }
        } وإلا إذا (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_مجموعه_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        إذا (nodep->isNet()) {
            Ast_گره_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
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
            user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_تخصیص_بده(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_بعدی(nodep->delayp(), عرض_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_عرض(درست);
        nodep->doing_عرض(خطأ);
    }
    فراغ بازدید(Ast_گره_Var_Ref* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
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
        إذا (!nodep->varp()->did_عرض()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), اشاره_تهی);
        }
        إذا (!m_paramsOnly && nodep->varp()->is_Iface_Array_تقسیم()) {
            replace_تقسیم_Iface_Array_Ref(nodep);
            أرجع;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        إذا (VN_IS(nodep->backp(), گره_تخصیص_بده) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        إذا (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(درست);
        إذا (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } وإلا إذا (nodep->access().isWriteOrRW() && nodep->varp()->is_درون_داد()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), تخصیص_بده_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ثابت منطقي hierRef = !m_curModVars.شمار(nodep->varp());
            ثابت منطقي net_پورت
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::پورت;
            ثابت منطقي cont_تخصیص_بده = VN_IS(nodep->abovep(), تخصیص_بده_W);
            ثابت منطقي pinConn = VN_IS(nodep->abovep(), Pin);
            إذا (!(hierRef && net_پورت && (cont_تخصیص_بده || pinConn))) {
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
        nodep->did_عرض(درست);
    }

    فراغ بازدید(AstEnumDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_گره_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_گره_DType* ثابت badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        إذا (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->پیدا_کن_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->عرض_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_بعدی(nodep->itemsp(), عرض_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_عدد رقم(nodep, nodep->عرض(), 0);
        ثابت V3_عدد one{nodep, nodep->عرض(), 1};
        منطقي wrapAround = خطأ;
        std::map<ثابت V3_عدد, Ast_Enum_قلم*> inits;
        لكل (Ast_Enum_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            إذا (itemp->valuep()) {
                إذا (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_درخت("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                إذا (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_پشت()->delete_درخت();
                    استمر;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = خطأ;  // Explicit value resets wrap-around tracking
            }
            إذا (!itemp->valuep()) {
                إذا (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                إذا (رقم.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(جديد AstConst{itemp->fileline(), رقم});
            }

            ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
            إذا (constp->رقم().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            رقم.op_تخصیص_بده(constp->رقم());
            // Look for duplicates
            ثابت خودکار pair = inits.emplace(رقم, itemp);
            إذا (!pair.دومی) {  // IEEE says illegal
                ثابت Ast_گره* ثابت otherp = pair.اولی->دومی;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ثابت منطقي prevNeg = constp->رقم().isNegative();
            رقم.op_جمع_کن(one, constp->رقم());
            إذا (basicp->isSigned()) {
                wrapAround = !prevNeg && رقم.isNegative();
            } وإلا {
                wrapAround = رقم.isEqZero();
            }
        }
    }
    فراغ بازدید(Ast_Enum_قلم* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        إذا (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            منطقي warnOn = درست;
            Ast_گره_عبارت* valuep = nodep->valuep();
            إذا (ثابت Ast_جمع_کن* ثابت anodep = VN_CAST(valuep, جمع_کن_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                إذا (anodep->fileline()->equal_اولی_سطر_ستون(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            إذا (ثابت AstConst* ثابت constp = VN_CAST(valuep, Const)) {
                إذا (static_cast<صحيح>(constp->رقم().most_مجموعه_بیت_P1()) > nodep->عرض()) {
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
            nodep->valuep(جديد AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_پشت(),
                                      nodep->dtypep()});
        }
    }
    فراغ بازدید(Ast_Enum_قلم_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        إذا (!nodep->itemp()->did_عرض()) {
            // We need to do the whole enum en masse
            Ast_گره* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            لكل (; enump; enump = enump->backp()) {
                إذا (VN_IS(enump, EnumDType)) اكسر;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    فراغ بازدید(Ast_گره_Cons* nodep) override {}  // Widthed when created
    فراغ بازدید(Ast_گره_Cons_قلم* nodep) override {}  // Widthed when created
    فراغ بازدید(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, عرض_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گره_DType* subDTypep = اشاره_تهی;
        nodep->dtype_مجموعه_بیت();

        إذا (dtype && dtype->isString()) {
            nodep->dtype_مجموعه_String();
            subDTypep = nodep->پیدا_کن_String_DType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtype_مجموعه_Double();
            subDTypep = nodep->پیدا_کن_Double_DType();
        } وإلا {
            // Take width as maximum across all items
            صحيح عرض = nodep->exprp()->عرض();
            صحيح mwidth = nodep->exprp()->عرض_Min();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                عرض = std::max(عرض, itemp->عرض());
                mwidth = std::max(mwidth, itemp->عرض_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                عرض, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        لكل (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_قلم)->rangep();
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, عرض_VP{subDTypep, FINAL}.p());
            } وإلا {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        إذا (m_constraintp) {
            منطقي can_پایینی = درست;
            لكل (Ast_Dist_قلم* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_قلم)) {
                إذا (VN_IS(ditemp->rangep(), InsideRange)) استمر;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ثابت Ast_گره_DType* ثابت قلم_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                إذا (!قلم_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_خالص()) {
                    can_پایینی = خطأ;
                    اكسر;
                }
            }
            إذا (can_پایینی) أرجع;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_گره_عبارت* newp = اشاره_تهی;
        لكل (Ast_Dist_قلم* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_قلم)) {
            Ast_گره_عبارت* inewp
                = inside_قلم(nodep, nodep->exprp()->clone_درخت_خالص(خطأ), itemp->rangep());
            إذا (!inewp) استمر;
            Ast_گره_عبارت* ثابت cmpp
                = جديد AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_پشت(),
                            جديد AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_کد::UNSIGNED, درست);
            cmpp->fileline()->modifyWarnOff(V3_Error_کد::CMPCONST, درست);
            inewp = جديد AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::بیت_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    فراغ بازدید(AstInside* nodep) override {
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, عرض_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_گره_DType* expDTypep = اشاره_تهی;

        إذا (dtype && dtype->isString()) {
            nodep->dtype_مجموعه_String();
            expDTypep = nodep->پیدا_کن_String_DType();
        } وإلا إذا (dtype && dtype->isDouble()) {
            nodep->dtype_مجموعه_Double();
            expDTypep = nodep->پیدا_کن_Double_DType();
        } وإلا {
            // Take width as maximum across all items
            صحيح عرض = nodep->exprp()->عرض();
            صحيح mwidth = nodep->exprp()->عرض_Min();
            منطقي isFourstate = nodep->exprp()->dtypep()->isFourstate();
            لكل (ثابت Ast_گره* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                عرض = std::max(عرض, itemp->عرض());
                mwidth = std::max(mwidth, itemp->عرض_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_مجموعه_بیت();
            ثابت VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(عرض, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        لكل (Ast_گره *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            إذا (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, عرض_VP{expDTypep, FINAL}.p());
            } وإلا إذا (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_گره_عبارت* exprp;
        Ast_عبارت_Stmt* عبارت_Stmtp = اشاره_تهی;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        إذا (!m_constraintp && !nodep->exprp()->is_خالص()) {
            File_سطر* ثابت fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_گره_DType* ثابت عبارت_Dtp = nodep->exprp()->dtypep();
            ثابت صحيح w = عبارت_Dtp->عرض();
            Ast_گره_DType* ثابت موقت_DTypep
                = عبارت_Dtp->عرض_Sized() ? عبارت_Dtp
                                        : nodep->پیدا_کن_بیت_Or_Logic_DType(w, w, عبارت_Dtp->numeric(),
                                                                     عبارت_Dtp->isFourstate());
            AstVar* ثابت varp
                = جديد AstVar{fl, VVarType::XTEMP, m_inside_موقت_نامها.get(nodep), موقت_DTypep};
            exprp = جديد AstVarRef{fl, varp, VAccess::READ};
            عبارت_Stmtp = جديد Ast_عبارت_Stmt{fl,
                                        جديد Ast_تخصیص_بده{fl, جديد AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_پشت()},
                                        exprp};
            إذا (m_ftaskp) {
                varp->funcLocal(درست);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } وإلا {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(عبارت_Stmtp);
        } وإلا {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_گره_عبارت* newp = اشاره_تهی;
        لكل (Ast_گره_عبارت *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), گره_عبارت);  // Will be unlinking
            Ast_گره_عبارت* inewp;
            إذا (عبارت_Stmtp) {
                inewp = inside_قلم(nodep, عبارت_Stmtp, itemp);
                عبارت_Stmtp = اشاره_تهی;
            } وإلا {
                inewp = inside_قلم(nodep, exprp->clone_درخت_خالص(خطأ), itemp);
            }
            إذا (!inewp) استمر;
            newp = newp ? جديد AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        إذا (عبارت_Stmtp) VL_DO_DANGLING(عبارت_Stmtp->delete_درخت(), عبارت_Stmtp);
        إذا (!newp) newp = جديد AstConst{nodep->fileline(), AstConst::بیت_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    Ast_گره_عبارت* inside_قلم(Ast_گره* nodep, Ast_گره_عبارت* exprp, Ast_گره_عبارت* itemp) {
        ثابت Ast_گره_DType* ثابت قلم_Dtp = itemp->dtypep()->skipRefp();
        إذا (AstInsideRange* ثابت irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            أرجع irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_پشت(),
                                             irangep->rhsp()->unlink_Fr_پشت());
        } وإلا إذا (VN_IS(قلم_Dtp, UnpackArrayDType) || VN_IS(قلم_Dtp, DynArrayDType)
                   || VN_IS(قلم_Dtp, صف_DType)) {
            // Unsupported in parameters
            Ast_گره_عبارت* ثابت inewp = جديد AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_پشت(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, قلم_Dtp->subDTypep(), BOTH);
            inewp->dtype_مجموعه_بیت();
            inewp->did_عرض(درست);
            أرجع inewp;
        } وإلا إذا (VN_IS(قلم_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_درخت(), exprp);
            أرجع اشاره_تهی;
        }
        أرجع AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_پشت());
    }
    فراغ بازدید(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        إذا (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } وإلا {
            إذا (m_vup->prelim()) {
                user_Iterate_And_بعدی(nodep->lhsp(), m_vup);
                user_Iterate_And_بعدی(nodep->rhsp(), m_vup);
            }
            إذا (m_vup->final()) {
                Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ثابت صحيح exp_عرض = expDTypep->عرض();
                ثابت منطقي waiveLhs = exp_عرض == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->عرض_Min() >= nodep->lhsp()->عرض();
                ثابت منطقي waiveRhs = exp_عرض == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_عرض >= nodep->rhsp()->عرض_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    فراغ بازدید(AstIfaceRefDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        إذا (nodep->isVirtual()) nodep->iface_از_طریق_Cellp()->مجموعه_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    فراغ بازدید(Ast_گره_UOr_Struct_DType* nodep) override {
        إذا (nodep->doing_عرض()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_مجموعه_بیت();
            nodep->doing_عرض(خطأ);
            أرجع;
        }
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_عرض(درست);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        إذا (ثابت AstUnionDType* ثابت unionp = VN_CAST(nodep, UnionDType)) {
            إذا (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, اشاره_تهی);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(خطأ);
        // Error checks
        لكل (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_گره_DType* ثابت dtp = itemp->subDTypep()->skipRefp();
            إذا (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            إذا ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                افزودن_Deletep(itemp->valuep()->unlink_Fr_پشت());
            }
        }
        ثابت منطقي isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ثابت منطقي درون_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        إذا (VN_IS(nodep, UnionDType) || nodep->packed()) {
            صحيح lsb = 0;
            صحيح عرض = 0;
            منطقي اولی = درست;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            لكل (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            لكل (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                إذا (itemp->isFourstate()) nodep->isFourstate(درست);
                itemp->lsb(lsb);
                إذا (VN_IS(nodep, UnionDType)) {
                    ثابت صحيح قلم_عرض = itemp->عرض();
                    // Skip union size check for template modules with unresolved parameters
                    إذا (!اولی && isHardPackedUnion && قلم_عرض != عرض && !درون_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    عرض = std::max(عرض, قلم_عرض);
                } وإلا {
                    lsb += itemp->عرض();
                    عرض += itemp->عرض();
                }
                اولی = خطأ;
            }
            nodep->عرض_Force(عرض, عرض);  // Signing stays as-is, as parsed from declaration
        } وإلا {
            nodep->عرض_Force(1, 1);
        }
        nodep->doing_عرض(خطأ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    فراغ بازدید(AstThisRef* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->فرزند_DTypep()));
        إذا (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    فراغ بازدید(AstClassRefDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    فراغ بازدید(AstClassOrPackageRef* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        userIterateChildren(nodep, اشاره_تهی);
    }
    فراغ بازدید(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_بیشتر() << "... Suggest use a typedef");
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    فراغ بازدید(AstClassExtends* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        إذا (VN_IS(nodep->فرزند_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->فرزند_DTypep()));
        }
    }
    فراغ بازدید(AstMemberDType* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->عرض_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        إذا (nodep->valuep()) {
            user_Iterate_And_بعدی(nodep->valuep(), عرض_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_تخصیص_بده(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    فراغ بازدید(AstStructSel* nodep) override {
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        إذا (nodep->did_عرض()) أرجع;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = درست;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ثابت AstMemberSel* ثابت fromSel = VN_CAST(nodep->fromp(), MemberSel);
        إذا (AstClocking* ثابت clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->اولی_Abovep(), Clocking)
                                               : اشاره_تهی) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            إذا (AstVar* ثابت varp = memberSelClocking(nodep, clockingp)) {
                إذا (!varp->did_عرض()) userIterate(varp, اشاره_تهی);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_پشت());
                VL_DO_DANGLING(fromp->delete_درخت(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                إذا (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_مجموعه(nodep, nodep->access());
                }
                إذا (AstIfaceRefDType* ثابت adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_عرض(درست);  // Must not visit again: will confuse scopes
                أرجع;
            }
        } وإلا إذا (Ast_گره_UOr_Struct_DType* ثابت adtypep = VN_CAST(fromDtp, گره_UOr_Struct_DType)) {
            إذا (memberSelStruct(nodep, adtypep)) أرجع;
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            إذا (memberSelClass(nodep, adtypep)) أرجع;
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            إذا (Ast_گره* foundp = memberSelIface(nodep, adtypep)) {
                إذا (AstClocking* ثابت clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->did_عرض()) userIterate(varp, اشاره_تهی);
                    AstIface* ثابت ifacep = adtypep->iface_از_طریق_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ثابت AstVarRef* ثابت refp = VN_CAST(nodep->fromp(), VarRef);
                    إذا (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ثابت newp = جديد AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_عرض(درست);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        أرجع;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_عرض(درست);
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
                    v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newDtypep);
                    Ast_گره_عبارت* ثابت fromp = nodep->fromp()->unlink_Fr_پشت();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (AstCell* ثابت cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    إذا (VN_IS(cellp->modp(), Iface)) {
                        ثابت string viftopName = cellp->name() + "__Viftop";
                        Ast_گره_Module* ثابت والد_Ifacep = adtypep->iface_از_طریق_Cellp();
                        AstVar* viftopVarp = اشاره_تهی;
                        لكل (Ast_گره* itemp = والد_Ifacep->stmtsp(); itemp;
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
                        إذا (!viftopVarp->did_عرض()) userIterate(viftopVarp, اشاره_تهی);
                        Ast_گره_DType* subDtypep = viftopVarp->dtypep();
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
                                newDtypep->isVirtual(درست);
                                v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_عرض(درست);
                        أرجع;
                    }
                }
                إذا (Ast_گره_FTask* ثابت ftaskp = VN_CAST(foundp, گره_FTask)) {
                    Ast_روش_Call* ثابت newp = جديد Ast_روش_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
                   || VN_IS(fromDtp, صف_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_گره* ثابت newp = جديد Ast_روش_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_پشت(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            أرجع;
        } وإلا {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False{}});
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }
    منطقي memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        إذا (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_روش_Call* ثابت newp = جديد Ast_روش_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            بازدید(newp);
            أرجع درست;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت اولی_classp = adtypep->classp();
        UASSERT_OBJ(اولی_classp, nodep, "Unlinked");
        لكل (AstClass* classp = اولی_classp; classp;) {
            إذا (Ast_گره* ثابت foundp = m_memberMap.پیدا_کن_Member(classp, nodep->name())) {
                إذا (AstVar* ثابت varp = VN_CAST(foundp, Var)) {
                    إذا (!varp->did_عرض()) userIterate(varp, اشاره_تهی);
                    إذا (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ثابت varRefp
                            = جديد AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                        أرجع درست;
                    }
                    إذا (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_عرض(درست);
                    إذا (nodep->fromp()->same_درخت(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_مجموعه(nodep);
                    أرجع درست;
                }
                إذا (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(جديد AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), constrp});
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع درست;
                }
                إذا (Ast_Enum_قلم_Ref* ثابت adfoundp = VN_CAST(foundp, Enum_قلم_Ref)) {
                    nodep->replaceWith(adfoundp->clone_درخت(خطأ));
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع درست;
                }
                إذا (Ast_گره_FTask* ftaskp = VN_CAST(foundp, گره_FTask)) {
                    Ast_روش_Call* newp = جديد Ast_روش_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع درست;
                }
                إذا (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->پیدا_کن_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    أرجع درست;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی;
        }

        VSpellCheck speller;
        لكل (AstClass* classp = اولی_classp; classp;) {
            لكل (Ast_گره* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                إذا (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_قلم_Ref) || VN_IS(itemp, Var)) {
                    speller.افزودن_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی;
        }
        ثابت string suggest = speller.بهترین_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << اولی_classp->verilogKwd()
                      << " " << اولی_classp->prettyNameQ() << "\n"
                      << (suggest.خالی() ? "" : nodep->fileline()->warn_بیشتر() + suggest));
        أرجع خطأ;  // Caller handles error
    }
    منطقي modport_Allows_قلم(Ast_گره* nodep, ثابت AstIfaceRefDType* adtypep, ثابت Ast_گره* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ثابت AstModport* ثابت modportp = adtypep->modportp();
        إذا (!modportp) أرجع درست;
        إذا (ثابت AstVar* ثابت varp = VN_CAST(itemp, Var)) {
            إذا (varp->isParam()) أرجع درست;
        } وإلا إذا (!VN_IS(itemp, گره_FTask) && !VN_IS(itemp, Clocking)) {
            أرجع درست;
        }
        خودکار pair = m_modport_نامها.emplace(modportp, std::unordered_مجموعه<string>{});
        إذا (pair.دومی) {
            لكل (ثابت Ast_گره* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.اولی->دومی.درج(mpItemp->name());
            }
        }
        إذا (pair.اولی->دومی.شمار(itemp->name())) أرجع درست;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_از_طریق_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        أرجع خطأ;
    }
    Ast_گره* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_گره_Module* ثابت ifacep = adtypep->iface_از_طریق_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        لكل (Ast_گره* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            إذا (itemp->name() == nodep->name()) {
                إذا (!modport_Allows_قلم(nodep, adtypep, itemp)) {
                    أرجع اشاره_تهی;  // Caller handles error
                }
                أرجع itemp;
            }
            إذا (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.افزودن_Candidate(itemp->prettyName());
            }
        }
        ثابت string suggest = speller.بهترین_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.خالی() ? "" : nodep->fileline()->warn_بیشتر() + suggest));
        أرجع اشاره_تهی;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        لكل (Ast_گره* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            إذا (Ast_Clocking_قلم* citemp = VN_CAST(itemp, Clocking_قلم)) {
                إذا (citemp->varp()->name() == nodep->name()) أرجع citemp->varp();
                speller.افزودن_Candidate(citemp->varp()->prettyName());
            }
        }
        ثابت string suggest = speller.بهترین_Candidate_پیام(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.خالی() ? "" : nodep->fileline()->warn_بیشتر() + suggest));
        أرجع اشاره_تهی;  // Caller handles error
    }
    منطقي memberSelStruct(AstMemberSel* nodep, Ast_گره_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        إذا (AstMemberDType* ثابت memberp
            = VN_CAST(m_memberMap.پیدا_کن_Member(adtypep, nodep->name()), MemberDType)) {
            إذا (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } وإلا إذا (adtypep->packed()) {
                AstSel* ثابت newp = جديد AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                                memberp->lsb(), memberp->عرض()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
                newp->did_عرض(درست);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } وإلا {
                AstStructSel* ثابت newp = جديد AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_مقصد_Enump());
                newp->did_عرض(درست);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            أرجع درست;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        أرجع خطأ;
    }

    فراغ بازدید(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    فراغ بازدید(Ast_روش_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        إذا (nodep->did_عرض()) أرجع;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_گره_DType* ثابت fromDtp = nodep->fromp()->dtypep()->skip_Ref_مقصد_Enump();
        AstBasicDType* ثابت basicp = fromDtp ? fromDtp->basicp() : اشاره_تهی;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, عرض_VP{SELF, BOTH}.p());
        إذا (nodep->name() == "rand_mode") {
            روش_Call_Rand_نما(nodep);
        } وإلا إذا (nodep->name() == "constraint_mode") {
            روش_Call_Constraint(nodep, اشاره_تهی);
        } وإلا إذا (AstEnumDType* ثابت adtypep = VN_CAST(fromDtp, EnumDType)) {
            روش_Call_Enum(nodep, adtypep);
        } وإلا إذا (AstAssocArrayDType* ثابت adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            روش_Call_Assoc(nodep, adtypep);
        } وإلا إذا (AstWildcardArrayDType* ثابت adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            روش_Call_Wildcard(nodep, adtypep);
        } وإلا إذا (AstDynArrayDType* ثابت adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            روش_Call_Dyn(nodep, adtypep);
        } وإلا إذا (Ast_صف_DType* ثابت adtypep = VN_CAST(fromDtp, صف_DType)) {
            روش_Call_صف(nodep, adtypep);
        } وإلا إذا (AstClassRefDType* ثابت adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            روش_Call_Class(nodep, adtypep);
        } وإلا إذا (AstIfaceRefDType* ثابت adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            روش_Call_Iface_Ref(nodep, adtypep);
        } وإلا إذا (AstUnpackArrayDType* ثابت adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            روش_Call_Unpack(nodep, adtypep);
        } وإلا إذا (AstConstraintRefDType* ثابت adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            روش_Call_Constraint(nodep, adtypep);
        } وإلا إذا (basicp && basicp->isEvent()) {
            روش_Call_Event(nodep, basicp);
        } وإلا إذا (basicp && basicp->isString()) {
            روش_Call_String(nodep, basicp);
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_مجموعه_Void();
        }
    }
    AstWith* روش_With_Clause(Ast_گره_FTask_Ref* nodep, منطقي required, منطقي arbReturn,
                              Ast_گره_DType* returnDtp, Ast_گره_DType* شاخص_Dtp,
                              Ast_گره_DType* مقدار_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->شاخص_Arg_Refp()->dtypep(شاخص_Dtp);
            withp->مقدار_Arg_Refp()->dtypep(مقدار_Dtp);
            userIterate(withp, عرض_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_پشت();
            أرجع withp;
        }
        إذا (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        أرجع اشاره_تهی;
    }
    فراغ روش_Ok_Arguments(Ast_گره_FTask_Ref* nodep, صحيح minArg, صحيح maxArg) {
        صحيح narg = 0;
        إذا (AstWith* ثابت withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(افزودن_Deletep(withp->unlink_Fr_پشت()), withp);
        }
        لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ثابت منطقي ok = (narg >= minArg) && (narg <= maxArg);
        إذا (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_مقصد_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_مقصد_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            لكل (; narg < minArg; ++narg) {
                nodep->جمع_کن_Argsp(
                    جديد AstArg{nodep->fileline(), "",
                               جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}}});
            }
            لكل (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                طالما (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_پشت();
                VL_DO_DANGLING(argp->delete_درخت(), argp);
            }
        }
    }

    Ast_گره_عبارت* روش_Arg(Ast_روش_Call* nodep, صحيح arg) {
        AstArg* argp = nodep->argsp();
        لكل (صحيح narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        أرجع argp->exprp();
    }

    فراغ روش_Call_Enum(Ast_روش_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        إذا (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = اشاره_تهی;
            روش_Ok_Arguments(nodep, 0, 0);
            إذا (nodep->name() == "num") {
                صحيح اقلام = 0;
                لكل (Ast_گره* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++اقلام;
                newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{}, اقلام);
            } وإلا إذا (nodep->name() == "first") {
                ثابت Ast_Enum_قلم* itemp = adtypep->itemsp();
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_درخت(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } وإلا إذا (nodep->name() == "last") {
                ثابت Ast_Enum_قلم* itemp = adtypep->itemsp();
                طالما (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_قلم);
                إذا (!itemp) {
                    newp = جديد AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } وإلا {
                    newp = VN_AS(itemp->valuep()->clone_درخت(خطأ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            إذا (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                روش_Ok_Arguments(nodep, 0, 0);
            } وإلا إذا (nodep->name() == "next") {
                attrType = VAttrType::ENUM_بعدی;
                روش_Ok_Arguments(nodep, 0, 1);
            } وإلا إذا (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                روش_Ok_Arguments(nodep, 0, 1);
            } وإلا {
                nodep->v3fatalSrc("Bad case");
            }

            إذا (nodep->name() != "name" && nodep->argsp()) {
                Ast_گره_عبارت* stepp = روش_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = روش_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_کد::WIDTHEXPAND, درست);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                إذا (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_گره_عبارت* ثابت newp = جديد AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ثابت uint32_t گام_عرض = VN_AS(stepp, Const)->مقصد_UInt();
                إذا (گام_عرض == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_روش_Call* ثابت newp = جديد Ast_روش_Call{
                        nodep->fileline(),
                        جديد Ast_روش_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    أرجع;
                } وإلا إذا (گام_عرض != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    افزودن_Deletep(nodep->argsp()->unlink_Fr_پشت());
                    Ast_روش_Call* ثابت clonep = nodep->clone_درخت(خطأ);
                    VN_AS(stepp, Const)->رقم().مجموعه_Long(1);
                    AstConst* ثابت constp = جديد AstConst(nodep->fileline(), گام_عرض - 1);
                    AstArg* ثابت argp = جديد AstArg{nodep->fileline(), "", constp};
                    Ast_روش_Call* ثابت newp
                        = جديد Ast_روش_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                    أرجع;
                }
            }
            Ast_گره_عبارت* ثابت newp
                = enumSelect(nodep->fromp()->unlink_Fr_پشت(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    فراغ روش_Call_Wildcard(Ast_روش_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = اشاره_تهی;
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            روش_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ASSOC_اندازه};  // So don't need num()
            newp->dtype_مجموعه_Int();
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
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", روش_Arg(nodep, 0), SELF, BOTH);
            Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_Wildcard_شاخص_عبارت(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ASSOC_EXISTS, شاخص_exprp->unlink_Fr_پشت()};
            newp->dtype_مجموعه_بیت_Unsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            روش_Ok_Arguments(nodep, 0, 1);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_مجموعه_Void();
            } وإلا {
                iterateCheckSelf(nodep, "argument", روش_Arg(nodep, 0), SELF, BOTH);
                Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_Wildcard_شاخص_عبارت(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::ASSOC_ERASE, شاخص_exprp->unlink_Fr_پشت()};
                newp->dtype_مجموعه_Void();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = روش_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   adtypep->پیدا_کن_String_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, درست, خطأ, nodep->پیدا_کن_بیت_DType(),
                                   adtypep->پیدا_کن_String_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, درست, خطأ, adtypep->subDTypep(),
                                   adtypep->پیدا_کن_String_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_عرض(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    فراغ روش_Call_Assoc(Ast_روش_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = اشاره_تهی;
        إذا (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        إذا (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            روش_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ASSOC_اندازه};  // So don't need num()
            newp->dtype_مجموعه_Int();
        } وإلا إذا (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", روش_Arg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_Assoc_شاخص_عبارت(nodep, adtypep);
            روش_Call_LValue_Recurse(nodep, شاخص_exprp, VAccess::READWRITE);
            newp
                = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                     VCMethod::array_روش(nodep->name()),  // first/last/next/prev
                                     شاخص_exprp->unlink_Fr_پشت()};
            newp->dtype_مجموعه_Int();
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", روش_Arg(nodep, 0), adtypep->کلید_DTypep(), BOTH);
            Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_Assoc_شاخص_عبارت(nodep, adtypep);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ASSOC_EXISTS, شاخص_exprp->unlink_Fr_پشت()};
            newp->dtype_مجموعه_بیت_Unsized(32, 1, VSigning::SIGNED);
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            روش_Ok_Arguments(nodep, 0, 1);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_مجموعه_Void();
            } وإلا {
                iterateCheckTyped(nodep, "argument", روش_Arg(nodep, 0), adtypep->کلید_DTypep(),
                                  BOTH);
                Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_Assoc_شاخص_عبارت(nodep, adtypep);
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::ASSOC_ERASE, شاخص_exprp->unlink_Fr_پشت()};
                newp->dtype_مجموعه_Void();
            }
        } وإلا إذا (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp = روش_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = روش_With_Clause(
                nodep, خطأ, درست, اشاره_تهی, nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(صف_DType_نمایهای_By(adtypep->کلید_DTypep()));
            } وإلا {
                newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            }
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ثابت withp = روش_With_Clause(nodep, درست, خطأ, nodep->پیدا_کن_بیت_DType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp = روش_With_Clause(nodep, درست, خطأ, nodep->پیدا_کن_بیت_DType(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(adtypep->کلید_DTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "map") {
            AstWith* ثابت withp = روش_With_Clause(nodep, درست, خطأ, adtypep->subDTypep(),
                                                    adtypep->کلید_DTypep(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_عرض(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گره_عبارت* روش_Call_Assoc_شاخص_عبارت(Ast_روش_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_گره* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", شاخص_exprp, CONTEXT_DET, FINAL, adtypep->کلید_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    Ast_گره_عبارت* روش_Call_Wildcard_شاخص_عبارت(Ast_روش_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_گره* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", شاخص_exprp, CONTEXT_DET, FINAL, adtypep->پیدا_کن_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ روش_Call_LValue_Recurse(Ast_روش_Call* nodep, Ast_گره* childp, ثابت VAccess& access) {
        إذا (AstCMethodHard* ثابت ichildp = VN_CAST(childp, CMethodHard)) {
            منطقي methAt = خطأ;
            منطقي meth_At_پشت = خطأ;
            منطقي methAtWrite = خطأ;
            بدل (ichildp->روش()) {
            حالة VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = درست;
                اكسر;
            حالة VCMethod::ARRAY_AT_پشت:  // FALLTHRU
                meth_At_پشت = درست;
                اكسر;
            حالة VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            حالة VCMethod::DYN_AT_WRITE_APPEND_پشت:  //
                methAtWrite = درست;
                اكسر;
            افتراضي: اكسر;
            }
            إذا (methAt || meth_At_پشت || methAtWrite) {
                ثابت Ast_گره_DType* ثابت fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                إذا (VN_IS(fromDtypep, صف_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    إذا (methAt) {
                        ichildp->روش(VCMethod::ARRAY_AT_WRITE);
                    } وإلا إذا (meth_At_پشت) {
                        ichildp->روش(VCMethod::DYN_AT_WRITE_APPEND_پشت);
                    }
                }
                روش_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                أرجع;
            }
        }
        إذا (Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(childp, گره_Var_Ref)) {
            varrefp->access(access);
        } وإلا إذا (ثابت AstMemberSel* ثابت ichildp = VN_CAST(childp, MemberSel)) {
            روش_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت AstStructSel* ثابت ichildp = VN_CAST(childp, StructSel)) {
            روش_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (ثابت Ast_گره_Sel* ثابت ichildp = VN_CAST(childp, گره_Sel)) {
            روش_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } وإلا إذا (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } وإلا {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* روش_Call_Array(Ast_روش_Call* nodep, Ast_گره_DType* adtypep) {
        AstCMethodHard* newp = اشاره_تهی;

        إذا (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = اشاره_تهی;
            إذا (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = روش_With_Clause(nodep, خطأ, درست, اشاره_تهی, nodep->پیدا_کن_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ثابت withp = روش_With_Clause(
                nodep, خطأ, درست, اشاره_تهی, nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            إذا (nodep->name() == "unique_index") {
                newp->dtypep(newp->پیدا_کن_صف_شاخص_DType());
            } وإلا {
                newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            }
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, درست, خطأ, nodep->پیدا_کن_بیت_DType(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, درست, خطأ, nodep->پیدا_کن_بیت_DType(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->پیدا_کن_صف_شاخص_DType());
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ثابت withp
                = روش_With_Clause(nodep, درست, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(صف_DType_نمایهای_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        }
        أرجع newp;
    }
    فراغ روش_Call_Dyn(Ast_روش_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = اشاره_تهی;
        إذا (nodep->name() == "at") {  // Created internally for []
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWrite") {  // Created internally for []
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "size") {
            روش_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::DYN_اندازه};
            newp->dtype_مجموعه_Int();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete()
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ثابت withp
                = روش_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا ((newp = روش_Call_Array(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_عرض(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    فراغ روش_Call_صف(Ast_روش_Call* nodep, Ast_صف_DType* adtypep) {
        AstCMethodHard* newp = اشاره_تهی;
        إذا (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            روش_Ok_Arguments(nodep, 0, 0);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::DYN_اندازه};
            newp->dtype_مجموعه_Int();
        } وإلا إذا (nodep->name() == "delete") {  // function void delete([input integer index])
            روش_Ok_Arguments(nodep, 0, 1);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            إذا (!nodep->argsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_مجموعه_Void();
            } وإلا {
                iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
                Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_صف_شاخص_عبارت(nodep);
                إذا (شاخص_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                              VCMethod::DYN_برداشت_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_مجموعه_Void();
                } وإلا {
                    newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                              VCMethod::DYN_ERASE, شاخص_exprp->unlink_Fr_پشت()};
                    newp->dtype_مجموعه_Void();
                }
            }
        } وإلا إذا (nodep->name() == "insert") {
            روش_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", روش_Arg(nodep, 0), BOTH);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_گره_عبارت* ثابت شاخص_exprp = روش_Call_صف_شاخص_عبارت(nodep);
            AstArg* ثابت argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            إذا (شاخص_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::DYN_افزودن_FRONT, argp->exprp()->unlink_Fr_پشت()};
                newp->dtype_مجموعه_Void();
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                          VCMethod::DYN_درج, شاخص_exprp->unlink_Fr_پشت()};
                newp->جمع_کن_Pinsp(argp->exprp()->unlink_Fr_پشت());
                newp->dtype_مجموعه_Void();
            }
        } وإلا إذا (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            روش_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } وإلا إذا (nodep->name() == "push_back" || nodep->name() == "push_front") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", روش_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ثابت argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش(nodep->name()),
                                      argp->exprp()->unlink_Fr_پشت()};
            newp->dtype_مجموعه_Void();
        } وإلا إذا (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                      VCMethod::array_روش("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
        } وإلا إذا ((newp = روش_Call_Array(nodep, adtypep))) {
        } وإلا {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        إذا (newp) {
            newp->protect(خطأ);
            newp->did_عرض(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        }
    }
    Ast_گره_عبارت* روش_Call_صف_شاخص_عبارت(Ast_روش_Call* nodep) {
        Ast_گره* ثابت شاخص_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", شاخص_exprp, BOTH);
        VL_DANGLING(شاخص_exprp);  // May have been edited
        أرجع nodep->argsp()->exprp();
    }
    فراغ روش_Call_Warn_Timing(Ast_گره_FTask_Ref* ثابت nodep, ثابت std::string& className) {
        إذا (v3Global.opt.timing().is_مجموعه_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } وإلا إذا (!v3Global.opt.timing().is_مجموعه_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    فراغ روش_Call_Iface_Ref(Ast_روش_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ثابت ifacep = adtypep->iface_از_طریق_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        إذا (Ast_گره_FTask* ثابت ftaskp
            = VN_CAST(m_memberMap.پیدا_کن_Member(ifacep, nodep->name()), گره_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            إذا (!modport_Allows_قلم(nodep, adtypep, ftaskp)) {
                nodep->dtype_مجموعه_Void();
                أرجع;
            }
            إذا (adtypep->isVirtual()) {
                لكل (Ast_گره* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    إذا (AstVar* ثابت mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, اشاره_تهی);
            إذا (ftaskp->isStatic()) {
                AstArg* ثابت argsp = nodep->argsp();
                إذا (argsp) argsp->unlink_Fr_پشت_With_بعدی();
                Ast_گره_FTask_Ref* newp = اشاره_تهی;
                إذا (VN_IS(ftaskp, Task)) {
                    newp = جديد AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } وإلا {
                    newp = جديد AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            } وإلا {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                إذا (VN_IS(ftaskp, Task)) nodep->dtype_مجموعه_Void();
                process_FTask_Ref_آرگومانها(nodep);
            }
            أرجع;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    فراغ handle_Randomize_آرگومانها(Ast_گره_FTask_Ref* ثابت nodep, AstClass* ثابت classp) {
        منطقي has_Non_Null_آرگومانها = خطأ;
        AstConst* nullp = اشاره_تهی;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = اشاره_تهی;
            Ast_گره_عبارت* exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->رقم().isNull()) {
                    nullp = constp;
                    استمر;
                }
            }
            has_Non_Null_آرگومانها = درست;
            AstVar* fromVarp = اشاره_تهی;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            إذا (Ast_روش_Call* روش_Callp = VN_CAST(nodep, روش_Call)) {
                Ast_گره_عبارت* fromp = روش_Callp->fromp();
                طالما (AstMemberSel* ثابت memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ثابت varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            إذا (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
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
                        VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
                    }
                    exprp = اشاره_تهی;
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
                VL_DO_DANGLING(argp->unlink_Fr_پشت()->delete_درخت(), argp);
            }
        }
        إذا (nullp && has_Non_Null_آرگومانها) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    فراغ روش_Call_Class(Ast_روش_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ثابت اولی_classp = adtypep->classp();
        AstWith* withp = اشاره_تهی;
        إذا (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = روش_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کن_Void_DType(),
                                     adtypep->پیدا_کن_بیت_DType(), adtypep);
            لكل (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                إذا (argp->exprp()) userIterate(argp->exprp(), عرض_VP{SELF, BOTH}.p());
            }
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, اولی_classp);
            handle_Randomize_آرگومانها(nodep, اولی_classp);
        } وإلا إذا (nodep->name() == "srandom") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, اولی_classp);
        }
        UASSERT_OBJ(اولی_classp, nodep, "Unlinked");
        لكل (AstClass* classp = اولی_classp; classp;) {
            إذا (nodep->fileline()->timingOn()) {
                إذا (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ثابت packagep = get_قلم_Package(classp);
                    // Check if it's std
                    إذا (packagep && packagep->name() == "std") {
                        إذا (classp->name() == "process") {
                            روش_Call_Warn_Timing(nodep, "process");
                        } وإلا إذا (classp->name() == "semaphore" && nodep->name() == "get") {
                            روش_Call_Warn_Timing(nodep, "semaphore");
                        } وإلا إذا (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            روش_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            إذا (Ast_گره_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.پیدا_کن_Member(classp, nodep->name()), گره_FTask)) {
                userIterate(ftaskp, اشاره_تهی);
                إذا (ftaskp->isStatic()) {
                    AstArg* ثابت argsp = nodep->argsp();
                    إذا (argsp) argsp->unlink_Fr_پشت_With_بعدی();
                    Ast_گره_FTask_Ref* newp = اشاره_تهی;
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
                    VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                } وإلا {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    إذا (VN_IS(ftaskp, Task)) {
                        إذا (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_عبارت)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_مجموعه_Void();
                    }
                    إذا (withp) nodep->withp(withp);
                    process_FTask_Ref_آرگومانها(nodep);
                }
                أرجع;
            } وإلا إذا (nodep->name() == "get_randstate") {
                روش_Ok_Arguments(nodep, 0, 0);
                اولی_classp->پایه_Most_Classp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                         VCMethod::RNG_GET_RANDSTATE, اشاره_تهی};
                newp->usePtr(درست);
                newp->dtype_مجموعه_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                روش_Ok_Arguments(nodep, 1, 1);
                Ast_گره_عبارت* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گره_عبارت* ثابت exprp = nodep->argsp()->exprp();
                اولی_classp->پایه_Most_Classp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                         VCMethod::RNG_مجموعه_RANDSTATE, exprp->unlink_Fr_پشت()};
                newp->usePtr(درست);
                newp->dtype_مجموعه_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(درست);
                nodep->dtypep(nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی;
        }
        {
            VSpellCheck speller;
            لكل (AstClass* classp = اولی_classp; classp;) {
                لكل (Ast_گره* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    إذا (VN_IS(itemp, گره_FTask)) speller.افزودن_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : اشاره_تهی;
            }
            ثابت string suggest = speller.بهترین_Candidate_پیام(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << اولی_classp->verilogKwd() << " " << اولی_classp->prettyNameQ()
                           << "\n"
                           << (suggest.خالی() ? "" : nodep->fileline()->warn_بیشتر() + suggest));
        }
        nodep->dtype_مجموعه_Int();  // Guess on error
    }
    فراغ روش_Call_Constraint(Ast_روش_Call* nodep, AstConstraintRefDType*) {
        إذا (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            روش_Ok_Arguments(nodep, 0, 1);
            إذا (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", روش_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::INT));
            } وإلا {
                nodep->dtype_مجموعه_Void();
            }
            v3Global.useRandomizeMethods(درست);
        } وإلا {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ روش_Call_Rand_نما(Ast_روش_Call* nodep) {
        روش_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        إذا (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            nodep->dtype_مجموعه_Void();
        } وإلا {
            nodep->dtypep(nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(درست);
    }
    فراغ روش_Call_Unpack(Ast_روش_Call* nodep, AstUnpackArrayDType* adtypep) {
        تعداد : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_جمع,
            ARRAY_حاصلضرب
        } روش_Id;

        روش_Id = UNKNOWN;
        إذا (nodep->name() == "or") {
            روش_Id = ARRAY_OR;
        } وإلا إذا (nodep->name() == "and") {
            روش_Id = ARRAY_AND;
        } وإلا إذا (nodep->name() == "xor") {
            روش_Id = ARRAY_XOR;
        } وإلا إذا (nodep->name() == "sum") {
            روش_Id = ARRAY_جمع;
        } وإلا إذا (nodep->name() == "product") {
            روش_Id = ARRAY_حاصلضرب;
        }

        إذا (روش_Id) {
            AstWith* ثابت withp
                = روش_With_Clause(nodep, خطأ, خطأ, adtypep->subDTypep(),
                                   nodep->پیدا_کن_UInt32_DType(), adtypep->subDTypep());
            روش_Ok_Arguments(nodep, 0, 0);
            إذا (withp) {
                روش_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(),
                                         VCMethod::array_روش("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                إذا (!nodep->اولی_Abovep()) newp->dtype_مجموعه_Void();
                newp->protect(خطأ);
                newp->did_عرض(درست);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
                أرجع;
            } وإلا {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_سطر* ثابت fl = nodep->fileline();
                Ast_گره_عبارت* newp = اشاره_تهی;
                لكل (صحيح i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_گره_عبارت* ثابت arrayRef = nodep->fromp()->clone_درخت_خالص(خطأ);
                    Ast_گره_عبارت* ثابت selector = جديد AstArraySel{fl, arrayRef, i};
                    إذا (!newp) {
                        newp = selector;
                    } وإلا {
                        بدل (روش_Id) {
                        حالة ARRAY_OR: newp = جديد AstOr{fl, newp, selector}; اكسر;
                        حالة ARRAY_AND: newp = جديد AstAnd{fl, newp, selector}; اكسر;
                        حالة ARRAY_XOR: newp = جديد AstXor{fl, newp, selector}; اكسر;
                        حالة ARRAY_جمع: newp = جديد Ast_جمع_کن{fl, newp, selector}; اكسر;
                        حالة ARRAY_حاصلضرب: newp = جديد AstMul{fl, newp, selector}; اكسر;
                        افتراضي: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        } وإلا إذا (AstCMethodHard* newp = روش_Call_Array(nodep, adtypep)) {
            newp->protect(خطأ);
            newp->did_عرض(درست);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    فراغ روش_Call_Event(Ast_روش_Call* nodep, AstBasicDType*) {
        // Method call on event
        إذا (nodep->name() == "triggered") {
            روش_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ثابت callp = جديد AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_مجموعه_بیت();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    فراغ روش_Call_String(Ast_روش_Call* nodep, AstBasicDType*) {
        // Method call on string
        إذا (nodep->name() == "len") {
            // Constant value
            روش_Ok_Arguments(nodep, 0, 0);
            Ast_گره* ثابت newp = جديد AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        } وإلا إذا (nodep->name() == "itoa") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } وإلا إذا (nodep->name() == "hextoa") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } وإلا إذا (nodep->name() == "octtoa") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } وإلا إذا (nodep->name() == "bintoa") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } وإلا إذا (nodep->name() == "realtoa") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } وإلا إذا (nodep->name() == "tolower") {
            روش_Ok_Arguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جديد Ast_مقصد_پایینی_N{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (nodep->name() == "toupper") {
            روش_Ok_Arguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جديد Ast_مقصد_فوقانی_N{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (nodep->name() == "compare" || nodep->name() == "icompare") {
            ثابت منطقي ignoreCase = nodep->name()[0] == 'i';
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            AstArg* ثابت argp = nodep->argsp();
            Ast_گره_عبارت* ثابت lhs = nodep->fromp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت rhs = argp->exprp()->unlink_Fr_پشت();
            Ast_گره* ثابت newp = جديد AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (nodep->name() == "putc") {
            روش_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", روش_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", روش_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گره_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پشت(), VarRef);
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت thsp = arg1p->exprp()->unlink_Fr_پشت();
            AstVarRef* ثابت varrefp
                = جديد AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_گره* ثابت newp = جديد Ast_تخصیص_بده{
                nodep->fileline(), fromp, جديد AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            افزودن_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } وإلا إذا (nodep->name() == "getc") {
            روش_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", روش_Arg(nodep, 0), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            Ast_گره_عبارت* ثابت lhsp = nodep->fromp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت newp = جديد AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا إذا (nodep->name() == "substr") {
            روش_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", روش_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", روش_Arg(nodep, 1), BOTH);
            AstArg* ثابت arg0p = nodep->argsp();
            AstArg* ثابت arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_گره_عبارت* ثابت lhsp = nodep->fromp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت rhsp = arg0p->exprp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت thsp = arg1p->exprp()->unlink_Fr_پشت();
            Ast_گره_عبارت* ثابت newp = جديد AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
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
            روش_Ok_Arguments(nodep, 0, 0);
            Ast_گره* ثابت newp
                = جديد AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_پشت(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_صف_DType* صف_DType_نمایهای_By(Ast_گره_DType* شاخص_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        إذا (Ast_صف_DType* ثابت queuep = m_صف_DType_نمایهای[شاخص_DTypep]) {
            أرجع queuep;
        } وإلا {
            Ast_صف_DType* ثابت newp
                = جديد Ast_صف_DType{شاخص_DTypep->fileline(), شاخص_DTypep, اشاره_تهی};
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newp);
            m_صف_DType_نمایهای[شاخص_DTypep] = newp;
            أرجع newp;
        }
    }

    فراغ بازدید(AstNew* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        AstClass* classp = اشاره_تهی;
        منطقي تخصیص_بده_2 = خطأ;
        إذا (VN_IS(nodep->backp(), تخصیص_بده)) {  // assignment case
            تخصیص_بده_2 = درست;
            Ast_گره* warnp = اشاره_تهی;
            AstClassRefDType* refp = اشاره_تهی;

            إذا (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                إذا (AstClass* ثابت clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ثابت adtypep
                        = جديد AstClassRefDType{nodep->fileline(), clsp, اشاره_تهی};
                    v3Global.rootp()->typeTablep()->جمع_کن_Typesp(adtypep);
                    refp = adtypep;
                }
            } وإلا {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : اشاره_تهی;
            }
            إذا (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, گره_DType)
                           ? ("data type "s + VN_AS(warnp, گره_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                أرجع;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            إذا (Ast_گره_FTask* ثابت ftaskp
                = VN_CAST(m_memberMap.پیدا_کن_Member(classp, "new"), Func)) {
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
        process_FTask_Ref_آرگومانها(nodep);
        إذا (!تخصیص_بده_2) nodep->dtypeFrom(nodep->taskp());
    }
    فراغ بازدید(AstNewCopy* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        AstClassRefDType* ثابت refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        إذا (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            أرجع;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        إذا (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    فراغ بازدید(AstNewDynamic* nodep) override {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        AstDynArrayDType* ثابت adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        إذا (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            أرجع;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        إذا (!VN_IS(nodep->backp(), گره_تخصیص_بده)) {
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
                consp->sizep(nodep->sizep()->unlink_Fr_پشت());
                nodep->replaceWith(consp->unlink_Fr_پشت());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }

    فراغ بازدید(Ast_Tagged_عبارت* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_مجموعه_بیت();
        userIterateChildren(nodep, m_vup);
    }
    فراغ بازدید(Ast_Tagged_الگو* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_مجموعه_بیت();
        userIterateChildren(nodep, m_vup);
    }
    فراغ بازدید(Ast_الگو_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_مجموعه_بیت();
    }
    فراغ بازدید(Ast_الگو_ستاره* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_مجموعه_بیت();
    }
    فراغ بازدید(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_مجموعه_بیت();
        userIterateChildren(nodep, m_vup);
    }

    فراغ بازدید(Ast_الگو* nodep) override {
        إذا (m_درون_Param_Override && !nodep->فرزند_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_درون_Param_Override);
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_درون_Param_Override = خطأ;
                user_Iterate_And_بعدی(patp->repp(), عرض_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                إذا (!VN_IS(patp->keyp(), متن_2) && !VN_IS(patp->keyp(), گره_DType)) {
                    user_Iterate_And_بعدی(patp->keyp(), عرض_VP{SELF, BOTH}.p());
                }
                لكل (Ast_گره *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_درون_Param_Override = VN_IS(valuep, الگو);
                    userIterate(valuep, عرض_VP{SELF, BOTH}.p());
                }
            }
            أرجع;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_درون_Param_Override);
        m_درون_Param_Override = خطأ;
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        UINFO(9, "PATTERN " << nodep);
        إذا (nodep->فرزند_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        إذا (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_گره_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            إذا (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                أرجع;
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False{}});
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
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
                ثابت صحيح times = بازدید_Pat_Member_Rep(patp);
                لكل (صحيح i = 1; i < times; i++) {
                    AstPatMember* ثابت newp = patp->clone_درخت(خطأ);
                    patp->جمع_کن_بعدی_اینجا(newp);
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
                    patp->unlink_Fr_پشت(&relinkHandle);
                    طالما (Ast_گره_عبارت* ثابت movep = VN_AS(patp->lhssp()->nextp(), گره_عبارت)) {
                        movep->unlink_Fr_پشت();  // Not unlinkFrBackWithNext, just one
                        Ast_گره* newkeyp = اشاره_تهی;
                        إذا (patp->keyp()) newkeyp = patp->keyp()->clone_درخت(درست);
                        AstPatMember* ثابت newp
                            = جديد AstPatMember{patp->fileline(), movep, newkeyp, اشاره_تهی};
                        patp->جمع_کن_بعدی(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = اشاره_تهی;
            لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ثابت nextp = VN_AS(patp->nextp(), PatMember);
                إذا (patp->isDefault()) {
                    إذا (defaultp) {
                        VL_DO_DANGLING(افزودن_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_پشت();
                }
                patp = nextp;
            }
            طالما (ثابت AstConstDType* ثابت vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, عرض_VP{SELF, BOTH}.p());

            إذا (خودکار* ثابت vdtypep = VN_CAST(dtypep, StructDType)) {
                الگو_Struct(nodep, vdtypep, defaultp);
            } وإلا إذا (خودکار* ثابت vdtypep = VN_CAST(dtypep, گره_Array_DType)) {
                الگو_Array(nodep, vdtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ثابت consp = جديد AstConsAssoc{nodep->fileline()};
                الگو_Keyed<Ast_Cons_Assoc_قلم>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ثابت consp = جديد AstConsWildcard{nodep->fileline()};
                الگو_Keyed<Ast_Cons_Wildcard_قلم>(nodep, consp, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, صف_DType)) {
                الگو_Dyn_Array_Or_صف(nodep, dtypep, defaultp);
            } وإلا إذا (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                الگو_Basic(nodep, dtypep, defaultp);
            } وإلا {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            إذا (defaultp) VL_DO_DANGLING(افزودن_Deletep(defaultp), defaultp);
        }
    }
    فراغ الگو_Struct(Ast_الگو* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        استفاده PatMap = std::map<ثابت AstMemberDType*, AstPatMember*>;
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
                            memp = VN_CAST(m_memberMap.پیدا_کن_Member(vdtypep, textp->متن()),
                                           MemberDType);
                            إذا (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->متن() << "' not found as member");
                                اكسر;
                            } وإلا {
                                ثابت std::pair<PatMap::iterator, منطقي> ret
                                    = patmap.emplace(memp, patp);
                                إذا (!ret.دومی) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), متن_2)->متن());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } وإلا إذا (Ast_گره_DType* nodedtypep = VN_CAST(patp->keyp(), گره_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, عرض_VP{SELF, BOTH}.p());
                            ثابت string dtype = nodedtypep->dtypep()->prettyDTypeName(درست);
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
                        إذا (!ret.دومی) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), متن_2)->متن());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } وإلا {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } طالما (خطأ);

                //بعدی
                إذا (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_گره_عبارت* newp = اشاره_تهی;
        إذا (vdtypep->packed()) {
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خودکار it = patmap.پیدا_کن(memp);
                إذا (it == patmap.انتها()) {  // default or default_type assignment
                    إذا (AstStructDType* ثابت memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_الگو_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } وإلا {
                        AstPatMember* ثابت patp
                            = default_Patp_الگو_Struct(nodep, memp, defaultp, dtypemap);
                        newp = مقدار_Concat_الگو_Struct(patp, newp, memp, nodep);
                        إذا (patp) VL_DO_DANGLING(افزودن_Deletep(patp), patp);
                    }
                } وإلا {  // member assignment
                    newp = مقدار_Concat_الگو_Struct(it->دومی, newp, memp, nodep);
                }
            }
        } وإلا {  // Unpacked
            AstConsStruct* ثابت consp = جديد AstConsStruct{nodep->fileline(), vdtypep};
            لكل (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ثابت خودکار it = patmap.پیدا_کن(memp);
                AstPatMember* patp = اشاره_تهی;
                إذا (it == patmap.انتها()) {  // Default or default_type assignment
                    patp = default_Patp_الگو_Struct(nodep, memp, defaultp, dtypemap);
                    افزودن_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    افزودن_Deletep(patp);  // patp used below
                } وإلا {
                    patp = it->دومی;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_گره_عبارت* ثابت valuep = الگو_Member_مقدار_Iterate(patp);
                consp->جمع_کن_Itemsp(جديد Ast_Cons_Struct_قلم{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_گره_عبارت* nestedvalue_Concat_الگو_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_گره_عبارت* newp,
                                                 Ast_الگو* nodep, ثابت DTypeMap& dtypemap) {
        لكل (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            إذا (AstStructDType* ثابت memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_الگو_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } وإلا {
                AstPatMember* ثابت patp
                    = default_Patp_الگو_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = مقدار_Concat_الگو_Struct(patp, newp, memp_nested, nodep);
                إذا (patp) VL_DO_DANGLING(افزودن_Deletep(patp), patp);
            }
        }
        أرجع newp;
    }

    AstPatMember* default_Patp_الگو_Struct(Ast_الگو* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ثابت DTypeMap& dtypemap) {
        ثابت string memp_DType = memp->virtRefDTypep()->prettyDTypeName(درست);
        ثابت خودکار it = dtypemap.پیدا_کن(memp_DType);
        إذا (it != dtypemap.انتها()) {
            // default_value for data_type
            أرجع it->دومی->clone_درخت(خطأ);
        }
        إذا (defaultp) {
            // default_value for any unmatched member yet
            أرجع defaultp->clone_درخت(خطأ);
        }
        إذا (memp->valuep()) {
            أرجع جديد AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_درخت(خطأ), گره_عبارت),
                                    جديد Ast_متن{nodep->fileline(), memp->name()}, اشاره_تهی};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        أرجع اشاره_تهی;
    }

    Ast_گره_عبارت* مقدار_Concat_الگو_Struct(AstPatMember* patp, Ast_گره_عبارت* newp,
                                           AstMemberDType* memp, Ast_الگو* nodep) {
        إذا (patp) {
            patp->dtypep(memp);
            Ast_گره_عبارت* ثابت valuep = الگو_Member_مقدار_Iterate(patp);
            إذا (!newp) {
                newp = valuep;
            } وإلا {
                AstConcat* ثابت concatp = جديد AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_کد::WIDTHCONCAT, درست);
                valuep->fileline()->modifyWarnOff(V3_Error_کد::WIDTHCONCAT, درست);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        أرجع newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_گره_DType* elemDTypep,
                                       ثابت DTypeMap& dtypemap = DTypeMap{}) {
        Ast_گره_عبارت* ثابت valuep = defaultp->lhssp()->clone_درخت(خطأ);
        Ast_گره_DType* ثابت elemDTypeSkipRefp = elemDTypep->skipRefp();
        ثابت AstStructDType* ثابت structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ثابت منطقي unpackedStruct = structp && !structp->packed();

        إذا (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            أرجع defaultp->clone_درخت(خطأ);
        }
        إذا (VN_IS(valuep, الگو)) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            أرجع defaultp->clone_درخت(خطأ);
        }
        إذا (!valuep->dtypep()) userIterate(valuep, عرض_VP{SELF, BOTH}.p());
        منطقي کامل_Element = unpackedStruct;
        إذا (Ast_گره_DType* ثابت مقدار_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            کامل_Element = unpackedStruct ? !مقدار_DTypep->skipRefp()->isIntegralOrPacked()
                                          : مقدار_DTypep->similarDType(elemDTypeSkipRefp);
        }
        إذا (کامل_Element) {
            VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            أرجع defaultp->clone_درخت(خطأ);
        }

        AstPatMember* ثابت nestedDefaultp
            = جديد AstPatMember{defaultp->fileline(), valuep, اشاره_تهی, اشاره_تهی};
        nestedDefaultp->isDefault(درست);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        لكل (ثابت خودکار& مدخل : dtypemap) nestedDefaultp->جمع_کن_بعدی(مدخل.دومی->clone_درخت(خطأ));
        Ast_الگو* ثابت recursivePatternp = جديد Ast_الگو{defaultp->fileline(), nestedDefaultp};
        أرجع جديد AstPatMember{defaultp->fileline(), recursivePatternp, اشاره_تهی, اشاره_تهی};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_گره_عبارت* الگو_Default_Valuep(AstPatMember* defaultp, Ast_گره_DType* elemDTypep) {
        إذا (!defaultp) أرجع اشاره_تهی;
        AstPatMember* ثابت elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_گره_عبارت* ثابت valuep = الگو_Member_مقدار_Iterate(elemDefaultp);
        VL_DO_DANGLING(افزودن_Deletep(elemDefaultp), elemDefaultp);
        أرجع valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    فراغ الگو_Concat_Array(Ast_الگو* nodep, AstUnpackArrayDType* arrayDtp) {
        ثابت VNumRange range = arrayDtp->declRange();
        ثابت صحيح elements = range.elements();
        AstConsArray* ثابت newp = جديد AstConsArray{nodep->fileline(), arrayDtp};
        صحيح entn = 0;  // Number of elements given so far, from the left
        منطقي error = خطأ;  // Reported an error, so the value is not complete
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گره_عبارت* ثابت valuep = الگو_Member_مقدار_Iterate(patp);
            ثابت uint64_t شاخص = range.ascending() ? entn : elements - 1 - entn;
            ثابت AstUnpackArrayDType* ثابت spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            إذا (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_مقادیر(شاخص, valuep);
                استمر;
            }
            ثابت VCastable castable = Ast_گره::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = درست;
            }
            newp->valuep(شاخص, valuep);
            ++entn;
        }
        إذا (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = درست;
        }
        إذا (error) {
            VL_DO_DANGLING(افزودن_Deletep(newp), newp);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}});
        } وإلا {
            nodep->replaceWith(newp);
        }
    }
    فراغ الگو_Array(Ast_الگو* nodep, Ast_گره_Array_DType* arrayDtp, AstPatMember* defaultp) {
        إذا (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            الگو_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            أرجع;
        }
        ثابت VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_وکتور_Map(nodep, range);
        UINFO(9, "ent " << range.چپ() << " to " << range.راست());
        Ast_گره* newp = اشاره_تهی;
        منطقي missed = خطأ;  // Some element missed, so not a complete value
        لكل (صحيح entn = 0, ent = range.چپ(); entn < range.elements();
             ++entn, ent += range.چپ_مقصد_راست_Inc()) {
            Ast_گره_عبارت* valuep = اشاره_تهی;
            ثابت خودکار it = patmap.پیدا_کن(ent);
            إذا (it == patmap.انتها()) {
                إذا (defaultp) {
                    valuep = الگو_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = درست;
                }
            } وإلا {
                AstPatMember* ثابت patp = it->دومی;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = الگو_Member_مقدار_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ثابت AstUnpackArrayDType* ثابت memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                إذا (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_بیشتر()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                    valuep = جديد AstConst{patp->fileline(), AstConst::بیت_False_Erroring{}};
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
                            = جديد AstConcat{valuep->fileline(), VN_AS(newp, گره_عبارت), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        إذا (!patmap.خالی()) nodep->v3error("Assignment pattern with too many elements");
        إذا (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(افزودن_Deletep(newp), newp);
            newp = جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}};
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
    AstConst* الگو_Const_Keyp(AstPatMember* patp) {
        إذا (VN_IS(patp->keyp(), گره_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            أرجع اشاره_تهی;
        }
        إذا (patp->کلید_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());  // May change
        AstConst* ثابت constp = VN_CAST(patp->کلید_Exprp(), Const);
        إذا (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        أرجع constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* الگو_Scalar_Keyp(AstPatMember* patp, Ast_گره_DType* کلید_DTypep) {
        إذا (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            أرجع اشاره_تهی;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        إذا (کلید_DTypep->skipRefp()->isIntegralOrPacked() && patp->کلید_Exprp()) {
            userIterate(patp->کلید_Exprp(), عرض_VP{کلید_DTypep, PRELIM}.p());
            ثابت صحيح عرض = std::max(patp->کلید_Exprp()->عرض(), کلید_DTypep->عرض());
            Ast_گره_DType* ثابت contextDTypep
                = کلید_DTypep->پیدا_کن_بیت_Or_Logic_DType(عرض, عرض, کلید_DTypep->numeric(), درست);
            iterateCheck(patp, "key", patp->کلید_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, خطأ);
        }
        AstConst* ثابت constp = الگو_Const_Keyp(patp);
        إذا (!constp) أرجع اشاره_تهی;
        إذا (constp->رقم().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            أرجع اشاره_تهی;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, کلید_DTypep, EXTEND_EXP, خطأ);
        Ast_گره_عبارت* ثابت convp = patp->کلید_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_گره_عبارت* ثابت keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        أرجع VN_AS(keyp->unlink_Fr_پشت(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_گره_عبارت* الگو_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        Ast_گره_DType* ثابت کلید_DTypep = VN_AS(arrayDtp, AssocArrayDType)->کلید_DTypep();
        إذا (!کلید_DTypep->skipRefp()->isAggregateType() || !patp->کلید_Exprp()) {
            أرجع الگو_Scalar_Keyp(patp, کلید_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->کلید_Exprp(), کلید_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->کلید_Exprp());
        Ast_گره_عبارت* ثابت foldedp = patp->کلید_Exprp();
        إذا (!foldedp->is_Constant_درخت()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            أرجع اشاره_تهی;
        }
        إذا (foldedp->exists([](ثابت AstConst* constp) { أرجع constp->رقم().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            أرجع اشاره_تهی;
        }
        أرجع foldedp->unlink_Fr_پشت();
    }
    AstConst* الگو_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        أرجع الگو_Scalar_Keyp(patp, arrayDtp->پیدا_کن_String_DType());
    }
    AstConst* الگو_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_گره_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        أرجع الگو_Scalar_Keyp(patp, arrayDtp->پیدا_کن_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    قالب <نام_نوع T_قلم, نام_نوع T_Cons>
    فراغ مرتب_سازی_Cons_اقلام(T_Cons* nodep) {
        std::وکتور<T_قلم*> itemps;
        طالما (T_قلم* ثابت itemp = nodep->itemsp()) {
            itemp->unlink_Fr_پشت();
            itemps.افزودن_پشت(itemp);
        }
        ثابت خودکار کلید_از = [](ثابت T_قلم* itemp) -> ثابت V3_عدد& {
            ثابت Ast_گره* ثابت keyp = itemp->keyp();
            أرجع VN_AS(keyp, Const)->رقم();
        };
        std::stable_مرتب_سازی(itemps.begin(), itemps.انتها(), [&](ثابت T_قلم* ap, ثابت T_قلم* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            أرجع کلید_از(ap).isLtXZ(کلید_از(bp));
        });
        ثابت T_قلم* firstp = اشاره_تهی;  // First item with the current key
        لكل (T_قلم* ثابت itemp : itemps) {
            إذا (firstp && کلید_از(firstp).isCaseEq(کلید_از(itemp))) {
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
            nodep->جمع_کن_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    قالب <نام_نوع T_قلم, نام_نوع T_Cons>
    فراغ الگو_Keyed(Ast_الگو* nodep, T_Cons* consp, Ast_گره_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(الگو_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ثابت خودکار keyp = الگو_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_گره_عبارت* ثابت valuep = الگو_Member_مقدار_Iterate(patp);
            إذا (keyp) {
                consp->جمع_کن_Itemsp(جديد T_قلم{patp->fileline(), keyp, valuep});
            } وإلا {
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ثابت AstAssocArrayDType* ثابت assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        إذا (!assocDtp || !assocDtp->کلید_DTypep()->skipRefp()->isAggregateType()) {
            مرتب_سازی_Cons_اقلام<T_قلم>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_گره_عبارت* new_Cons_Dyn_Array_Or_صف(File_سطر* fileline, ثابت Ast_گره_DType* dtypep) {
        إذا (VN_IS(dtypep, DynArrayDType)) أرجع جديد AstConsDynArray{fileline};
        إذا (VN_IS(dtypep, صف_DType)) أرجع جديد Ast_Cons_صف{fileline};
        أرجع اشاره_تهی;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    قالب <نام_نوع T_قلم, نام_نوع T_Cons>
    فراغ جمع_کن_Cons_قلم(T_Cons* consp, Ast_گره_عبارت* valuep) {
        Ast_گره_DType* ثابت arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ثابت منطقي isElement
            = Ast_گره::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), اشاره_تهی)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ثابت Ast_صف_DType* ثابت sub_صف_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), صف_DType);
        إذا (!isElement && !(sub_صف_Dtp && sub_صف_Dtp->boundp())) {
            T_Cons* ثابت subp = Ast_گره::cast<T_Cons>(valuep);
            إذا (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                إذا (subp->itemsp()) consp->جمع_کن_Itemsp(subp->itemsp()->unlink_Fr_پشت_With_بعدی());
                VL_DO_DANGLING(افزودن_Deletep(subp), subp);
                أرجع;
            }
            إذا (Ast_Cons_صف* ثابت queuep = VN_CAST(valuep, Cons_صف)) {
                جمع_کن_Cons_اقلام(consp, queuep);
                أرجع;
            }
            إذا (AstConsDynArray* ثابت dynp = VN_CAST(valuep, ConsDynArray)) {
                جمع_کن_Cons_اقلام(consp, dynp);
                أرجع;
            }
        }
        T_قلم* ثابت itemp = جديد T_قلم{valuep->fileline(), valuep};
        consp->جمع_کن_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        إذا (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } وإلا {
            user_Iterate_And_بعدی(valuep, عرض_VP{arrayDtp, FINAL}.p());
        }
    }
    فراغ جمع_کن_Cons_قلم(Ast_گره_عبارت* consp, Ast_گره_عبارت* valuep) {
        إذا (AstConsDynArray* ثابت dynp = VN_CAST(consp, ConsDynArray)) {
            جمع_کن_Cons_قلم<Ast_Cons_Dyn_Array_قلم>(dynp, valuep);
        } وإلا {
            جمع_کن_Cons_قلم<Ast_Cons_صف_قلم>(VN_AS(consp, Cons_صف), valuep);
        }
    }
    قالب <نام_نوع T_Sub>
    فراغ جمع_کن_Cons_اقلام(Ast_گره_عبارت* consp, T_Sub* subp) {
        طالما (خودکار* ثابت itemp = subp->itemsp()) {
            Ast_گره_عبارت* ثابت valuep = itemp->valuep()->unlink_Fr_پشت();
            VL_DO_DANGLING(افزودن_Deletep(itemp->unlink_Fr_پشت()), itemp);
            جمع_کن_Cons_قلم(consp, valuep);
        }
        VL_DO_DANGLING(افزودن_Deletep(subp), subp);
    }
    فراغ الگو_Dyn_Array_Or_صف(Ast_الگو* nodep, Ast_گره_DType* arrayp, AstPatMember* defaultp) {
        Ast_گره_عبارت* ثابت newp = new_Cons_Dyn_Array_Or_صف(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ثابت AstPatMember* ثابت firstp = VN_AS(nodep->itemsp(), PatMember);
        ثابت منطقي keyed = defaultp || (firstp && firstp->keyp());
        إذا (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            الگو_Keyed<Ast_Cons_Dyn_Array_قلم>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
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
            جمع_کن_Cons_قلم(newp, الگو_Member_مقدار_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    فراغ الگو_Basic(Ast_الگو* nodep, Ast_گره_DType* vdtypep, AstPatMember* defaultp) {
        ثابت AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ثابت VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_وکتور_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_گره_عبارت* newp = اشاره_تهی;
        لكل (ثابت صحيح ent : range.seq_Hi_مقصد_Lo()) {
            Ast_گره_عبارت* valuep = اشاره_تهی;
            ثابت خودکار it = patmap.پیدا_کن(ent);
            إذا (it == patmap.انتها()) {
                إذا (defaultp) {
                    valuep = الگو_Default_Valuep(defaultp, nodep->پیدا_کن_بیت_DType());
                } وإلا {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } وإلا {
                AstPatMember* ثابت patp = it->دومی;
                patmap.erase(it);
                patp->dtypep(nodep->پیدا_کن_بیت_DType());
                valuep = الگو_Member_مقدار_Iterate(patp);
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
        إذا (!patmap.خالی()) nodep->v3error("Assignment pattern with too many elements");
        إذا (newp) {
            nodep->replaceWith(newp);
        } وإلا {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_گره_عبارت* الگو_Member_مقدار_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, عرض_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_گره_عبارت* valuep = patp->lhssp()->unlink_Fr_پشت();
        إذا (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            إذا (AstConst* ثابت newp = V3_عرض_Commit::new_If_Const_Commit_اندازه(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        أرجع valuep;
    }

    ساكن فراغ checkEventAssignment(ثابت Ast_گره_تخصیص_بده* ثابت asgnp) {
        string unsupEvtAsgn;
        إذا (!استفاده_میکند_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        إذا (VN_IS(asgnp->rhsp(), CReset)) أرجع;
        إذا (asgnp->rhsp()->dtypep()->isEvent() && !استفاده_میکند_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.خالی() ? "from" : " and from");
        }
        إذا (!unsupEvtAsgn.خالی()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_بیشتر()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_بیشتر()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ساكن منطقي استفاده_میکند_Dynamic_Scheduler(Ast_گره* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        طالما (درست) {
            AstVarRef* ثابت vrefp = VN_CAST(nodep, VarRef);
            إذا (vrefp) أرجع استفاده_میکند_Dynamic_Scheduler(vrefp);
            إذا (VN_IS(nodep, MemberSel)) {
                أرجع درست;
            } وإلا إذا (Ast_گره_Sel* selp = VN_CAST(nodep, گره_Sel)) {
                nodep = selp->fromp();
            } وإلا {
                أرجع خطأ;
            }
        }
    }

    ساكن منطقي استفاده_میکند_Dynamic_Scheduler(AstVarRef* vrefp) {
        أرجع VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    فراغ بازدید(AstPatMember* nodep) override {
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, عرض_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), تخصیص_بده_3, FINAL, vdtypep, EXTEND_LHS);
    }
    صحيح بازدید_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        إذا (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ثابت AstConst* ثابت constp = VN_CAST(nodep->repp(), Const);
            إذا (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } وإلا {
                times = constp->مقصد_UInt();
            }
            إذا (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_پشت_With_بعدی()
                ->delete_درخت();  // Done with replicate before cloning
        }
        أرجع times;
    }

    فراغ بازدید(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_گره* propp = nodep->propp();
            طالما (AstLogNot* ثابت notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ثابت untilp = VN_CAST(propp, Until);
            إذا (untilp && VN_IS(nodep->backp(), گره_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_مجموعه_False() || !v3Global.opt.timing().is_مجموعه_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(جديد AstConst{untilp->fileline(), AstConst::بیت_False{}});
                VL_DO_DANGLING(افزودن_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_بعدی(nodep->sensesp(), اشاره_تهی);
            إذا (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_مجموعه_بیت();
        }
    }

    منطقي اولی_New_Statement_Ok_Recurse(Ast_گره* nodep) {
        إذا (AstVar* ثابت varp = VN_CAST(nodep, Var)) {
            إذا (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) أرجع درست;
        }
        إذا (Ast_تخصیص_بده* ثابت aitemp = VN_CAST(nodep, تخصیص_بده)) {
            إذا (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) أرجع درست;
        }
        إذا (AstInitialStaticStmt* ثابت aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            لكل (Ast_گره* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!اولی_New_Statement_Ok_Recurse(stmtp)) أرجع خطأ;
            }
            أرجع درست;
        }
        إذا (AstInitialAutomaticStmt* ثابت aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            لكل (Ast_گره* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                إذا (!اولی_New_Statement_Ok_Recurse(stmtp)) أرجع خطأ;
            }
            أرجع درست;
        }
        أرجع خطأ;
    }

    //--------------------
    // Top levels

    قالب <نام_نوع Case_قلم>
    فراغ handleCaseType(Ast_گره* casep, Ast_گره_عبارت* exprp, Case_قلم* itemsp) {
        Ast_Attr_از* ثابت exprap = VN_CAST(exprp, Attr_از);
        إذا (!exprap) أرجع;
        إذا (exprap->attrType() != VAttrType::TYPEID) أرجع;

        ثابت Ast_گره_DType* ثابت عبارت_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << عبارت_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_سطر* ثابت newfl = casep->fileline();
        newfl->warnOff(V3_Error_کد::CASEINCOMPLETE, درست);  // Side effect of transform
        newfl->warnOff(V3_Error_کد::CASEOVERLAP, درست);  // Side effect of transform
        casep->fileline(newfl);
        لكل (Case_قلم* itemp = itemsp; itemp; itemp = Ast_گره::as<Case_قلم>(itemp->nextp())) {
            إذا (itemp->isDefault()) استمر;
            منطقي اصابت = خطأ;
            لكل (Ast_گره* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ثابت Ast_Attr_از* ثابت condAttrp = VN_CAST(condp, Attr_از);
                إذا (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } وإلا {
                    Ast_گره_DType* ثابت condDtp = condAttrp->dtypep();
                    إذا (Ast_گره::computeCastable(عبارت_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        اصابت = درست;
                        اكسر;
                    }
                }
            }
            افزودن_Deletep(itemp->condsp()->unlink_Fr_پشت_With_بعدی());
            // Item condition becomes constant 1 if hits else 0
            itemp->جمع_کن_Condsp(جديد AstConst{newfl, AstConst::بیت_True{}, اصابت});
        }
        exprap->replaceWith(جديد AstConst{newfl, AstConst::بیت_True{}});
        VL_DO_DANGLING(افزودن_Deletep(exprap), exprap);
    }

    قالب <نام_نوع Case_قلم>
    فراغ handleCase(Ast_گره* casep, Ast_گره_عبارت* exprp, Case_قلم* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_گره_DType* subDTypep = exprp->dtypep();
        لكل (Case_قلم* itemp = itemsp; itemp; itemp = Ast_گره::as<Case_قلم>(itemp->nextp())) {
            لكل (Ast_گره* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                إذا (condp->dtypep() == subDTypep) استمر;

                إذا (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->پیدا_کن_Double_DType();
                } وإلا إذا (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->پیدا_کن_String_DType();
                } وإلا {
                    ثابت صحيح عرض = std::max(subDTypep->عرض(), condp->عرض());
                    ثابت صحيح mwidth = std::max(subDTypep->عرض_Min(), condp->عرض_Min());
                    ثابت منطقي issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->پیدا_کن_بیت_Or_Logic_DType(
                        عرض, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        لكل (Case_قلم* itemp = itemsp; itemp; itemp = Ast_گره::as<Case_قلم>(itemp->nextp())) {
            لكل (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    فراغ بازدید(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_Gen_Case_قلم *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_قلم);
            لكل (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, عرض_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ بازدید(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بعدی(nodep->initsp(), اشاره_تهی);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_بعدی(nodep->incsp(), اشاره_تهی);
    }
    فراغ بازدید(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    فراغ بازدید(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
        لكل (Ast_Case_قلم *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_قلم);  // Prelim may cause the node to get replaced
            user_Iterate_And_بعدی(itemp->stmtsp(), اشاره_تهی);
            لكل (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, عرض_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    فراغ بازدید(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_گره_DType* ثابت قلم_DTypep = nodep->پیدا_کن_UInt32_DType();
        لكل (Ast_Case_قلم *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_قلم);  // Prelim may cause the node to get replaced
            user_Iterate_And_بعدی(itemp->stmtsp(), اشاره_تهی);
            لكل (Ast_گره *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, قلم_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    فراغ بازدید(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_بعدی(nodep->stmtsp(), اشاره_تهی);
    }
    فراغ بازدید(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    فراغ بازدید(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    فراغ بازدید(Ast_گره_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_بعدی(nodep->thensp(), اشاره_تهی);
        user_Iterate_And_بعدی(nodep->elsesp(), اشاره_تهی);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    فراغ بازدید(Ast_عبارت_Stmt* nodep) override {
        user_Iterate_And_بعدی(nodep->stmtsp(), اشاره_تهی);
        // expected result is same as parent's expected result
        user_Iterate_And_بعدی(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    فراغ بازدید(Ast_گره_Foreach* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        nodep->did_عرض(درست);
        ثابت Ast_Foreach_سرآیند* ثابت headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_بعدی(headerp->fromp(), عرض_VP{SELF, BOTH}.p());
        Ast_گره_عبارت* ثابت fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_گره_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        لكل (Ast_گره *argsp = headerp->elementsp(), *بعدی_argsp; argsp; argsp = بعدی_argsp) {
            بعدی_argsp = argsp->nextp();
            ثابت منطقي خالی = VN_IS(argsp, خالی_2);
            AstVar* ثابت varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || خالی, argsp, "Missing foreach loop variable");
            إذا (varp) varp->استفاده_شده_Loop_Idx(درست);
            إذا (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            إذا (VN_IS(fromDtp, گره_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, صف_DType)) {
                // Nothing special here
            } وإلا إذا (AstBasicDType* ثابت adtypep = VN_CAST(fromDtp, BasicDType)) {
                إذا (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
                    أرجع;
                }
            } وإلا إذا (ثابت AstAssocArrayDType* ثابت adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->کلید_DTypep());
            } وإلا {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
                أرجع;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_گره* ثابت bodyp = nodep->bodyp();
        user_Iterate_And_بعدی(bodyp, اشاره_تهی);
        إذا (AstForeach* ثابت loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_مقصد_While(loopp), loopp, nodep);
            أرجع;
        }
    }

    فراغ بازدید(Ast_گره_تخصیص_بده* nodep) override {
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
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->عرض_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            إذا (ثابت AstIfaceRefDType* ثابت irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                إذا (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
                    أرجع;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            جریان_تخصیص_بده_پایینی(nodep);
            Ast_گره_DType* ثابت lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            إذا (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ثابت Ast_گره_DType* ثابت lhsDtp = lhsDTypep->skipRefp();
                ثابت Ast_گره_DType* ثابت rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
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
                checkUnpackedArrayAssignmentCompatible<Ast_گره_Var_Ref, Ast_گره_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), گره_Var_Ref), VN_CAST(nodep->rhsp(), گره_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                إذا (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), گره_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_تخصیص_بده(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        إذا (VN_IS(nodep, تخصیص_بده_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        إذا (Ast_گره* ثابت controlp = nodep->timingControlp()) {
            إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_پشت_With_بعدی()->delete_درخت(), controlp);
            } وإلا إذا (nodep->fileline()->timingOn() && v3Global.opt.timing().is_مجموعه_True()) {
                iterateNull(controlp);
            } وإلا {
                إذا (nodep->fileline()->timingOn()) {
                    إذا (v3Global.opt.timing().is_مجموعه_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } وإلا {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_پشت_With_بعدی()->delete_درخت(), controlp);
            }
        }
        إذا (VN_IS(nodep->rhsp(), خالی_صف)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ثابت Ast_گره_DType* ثابت lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            إذا (!VN_IS(lhsDtp, صف_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
                أرجع;
            }
            Ast_روش_Call* ثابت newp
                = جديد Ast_روش_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(), "delete"};
            newp->dtype_مجموعه_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            // Need to now convert it
            بازدید(newp);
            أرجع;
        }
        إذا (ثابت AstNewDynamic* ثابت dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            إذا (!dynp->rhsp()) {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_پشت()};
            } وإلا {
                newp = جديد AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_پشت()};
                newp->جمع_کن_Pinsp(dynp->rhsp()->unlink_Fr_پشت());
            }
            newp->did_عرض(درست);
            newp->protect(خطأ);
            newp->dtype_مجموعه_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        إذا (AstCMethodHard* ثابت slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod تخصیص_بده_روش;
            إذا (slicep->روش() == VCMethod::DYN_برش) {
                تخصیص_بده_روش = VCMethod::DYN_برش_تخصیص_بده;
            } وإلا إذا (slicep->روش() == VCMethod::DYN_برش_پشت_پشت) {
                تخصیص_بده_روش = VCMethod::DYN_برش_تخصیص_بده_پشت_پشت;
            } وإلا إذا (slicep->روش() == VCMethod::DYN_برش_FRONT_پشت) {
                تخصیص_بده_روش = VCMethod::DYN_برش_تخصیص_بده_FRONT_پشت;
            } وإلا {
                تخصیص_بده_روش = VCMethod::_ENUM_MAX;  // not a slice
            }
            إذا (تخصیص_بده_روش != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_گره_عبارت* ثابت fromp = slicep->fromp()->unlink_Fr_پشت();
                // Collect existing slice index pins (lsb, msb)
                Ast_گره_عبارت* ثابت lsbp = slicep->pinsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* ثابت msbp = slicep->pinsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
                AstCMethodHard* ثابت newp
                    = جديد AstCMethodHard{nodep->fileline(), fromp, تخصیص_بده_روش};
                newp->جمع_کن_Pinsp(lsbp);
                newp->جمع_کن_Pinsp(msbp);
                newp->جمع_کن_Pinsp(rhsp);
                newp->did_عرض(درست);
                newp->protect(خطأ);
                newp->dtype_مجموعه_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            }
        }

        إذا (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.مجموعه_Assigns_Events();
        }
    }

    فراغ بازدید(AstRSRule* nodep) override {
        إذا (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_بعدی(nodep->prodlistsp(), اشاره_تهی);
        user_Iterate_And_بعدی(nodep->وزن_Stmtsp(), اشاره_تهی);
    }

    فراغ بازدید(AstRelease* nodep) override {
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->عرض_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    فراغ بازدید(AstDeassign* nodep) override {
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->عرض_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ساكن منطقي isFormatNonNumericArg(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        أرجع dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, صف_DType)  //
               || (VN_IS(dtypep, گره_UOr_Struct_DType)
                   && !VN_AS(dtypep, گره_UOr_Struct_DType)->packed());
    }

    فراغ checkFormatNumericArg(Ast_گره* argp, حرف ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        إذا (std::tolower(ch) == 'd') أرجع;
        // In contrast, %x of a class reference, causes errors on several simulators.
        إذا (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_بیشتر() << "... Suggest use '%s'");
    }

    فراغ بازدید(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    فراغ بازدید(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بعدی(nodep->fmtp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_مقصد_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_مجموعه_String();
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    فراغ بازدید(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        إذا (m_vup && !m_vup->prelim()) أرجع;  // Can be called as statement or function
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;
        //
        UINFO(9, "  Display in " << nodep->متن());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        إذا (!nodep->عبارت_Format_2()) {  // Non-constant format, can't check format string types
            بازدید_sformatf_format(nodep);
        }

        Ast_گره_عبارت* oldExprsp = nodep->exprsp();
        إذا (nodep->عبارت_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), گره_عبارت);
        }
        إذا (oldExprsp) oldExprsp->unlink_Fr_پشت_With_بعدی();
        طالما (Ast_گره_عبارت* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), گره_عبارت);
            إذا (oldExprsp) oldExprsp->unlink_Fr_پشت_With_بعدی();
            إذا (VN_IS(argp, SFormatArg)) {
                nodep->جمع_کن_Exprsp(argp);
                استمر;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_گره_DType* ثابت dtypep = argp->dtypep()->skipRefp();
            إذا (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } وإلا إذا (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } وإلا إذا (isFormatNonNumericArg(dtypep)) {
                إذا (AstClassRefDType* ثابت classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    إذا (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(درست);
                    }
                } وإلا {
                    // Class handles inside containers print type names, not their members.
                    Ast_گره_DType* گره_Dtypep = dtypep;
                    طالما (گره_Dtypep) {
                        گره_Dtypep = گره_Dtypep->skipRefp();
                        إذا (Ast_گره_UOr_Struct_DType* ثابت uOrStructDTypep
                            = VN_CAST(گره_Dtypep, گره_UOr_Struct_DType)) {
                            uOrStructDTypep->مجموعه_Emit_مقصد_String();
                            v3Global.hasPrintedObjects(درست);
                        }
                        گره_Dtypep = گره_Dtypep->subDTypep();
                    }
                }
                Ast_گره_عبارت* ثابت newp = جديد Ast_مقصد_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } وإلا إذا (nodep->عبارت_Format_2()) {
                إذا (AstEnumDType* ثابت enumDtp = formatEnumDType(argp)) {
                    nodep->جمع_کن_Exprsp(newFormatEnumArg(argp, enumDtp));
                    استمر;
                }
            }
            إذا (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            إذا (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->جمع_کن_Exprsp(argp);
            } وإلا {
                nodep->جمع_کن_Exprsp(جديد AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->متن());
    }
    فراغ بازدید(AstCReset* nodep) override {
        assert_At_عبارت(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    فراغ بازدید(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    فراغ بازدید(AstConstraintRef* nodep) override { userIterateChildren(nodep, اشاره_تهی); }
    فراغ بازدید(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        إذا (!m_paramsOnly) {
            nodep->foreach([هذا](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    جديد AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string متن = VString::dequotePercent(nodep->fmtp()->متن());
            إذا (متن.خالی()) متن = "Elaboration system task message (IEEE 1800-2023 20.11)";
            بدل (nodep->displayType()) {
            حالة VDisplayType::DT_INFO: nodep->v3warn(USERINFO, متن); اكسر;
            حالة VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, متن); اكسر;
            حالة VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, متن); اكسر;
            حالة VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, متن); اكسر;
            افتراضي: UASSERT_OBJ(خطأ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
        }
    }
    فراغ بازدید(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    فراغ بازدید(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    فراغ بازدید(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    فراغ بازدید(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ بازدید(AstFError* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_بعدی(nodep->strp(), عرض_VP{SELF, BOTH}.p());
        }
    }
    فراغ بازدید(AstFEof* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    فراغ بازدید(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ بازدید(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ بازدید(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    فراغ بازدید(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->جابه_جایی(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    فراغ بازدید(AstFGetC* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    فراغ بازدید(AstFGetS* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_بعدی(nodep->strgp(), عرض_VP{SELF, BOTH}.p());
        }
    }
    فراغ بازدید(AstFUngetC* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    فراغ بازدید(AstFRead* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->memp(), عرض_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            إذا (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            إذا (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    فراغ بازدید(AstFScanF* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_بعدی(nodep->exprsp(), عرض_VP{SELF, BOTH}.p());
        }
    }
    فراغ بازدید(AstSScanF* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->fromp(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->exprsp(), عرض_VP{SELF, BOTH}.p());
        }
    }
    فراغ بازدید(Ast_پشته_Trace_F* nodep) override { nodep->dtype_مجموعه_String(); }
    فراغ بازدید(AstSysIgnore* nodep) override {
        user_Iterate_And_بعدی(nodep->exprsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstSystemF* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_گره_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_بعدی(nodep->memp(), عرض_VP{SELF, BOTH}.p());
        ثابت Ast_گره_DType* subp = اشاره_تهی;
        إذا (ثابت AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            إذا (!adtypep->کلید_DTypep()->skipRefp()->basicp()
                || !adtypep->کلید_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
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
        user_Iterate_And_بعدی(nodep->lsbp(), عرض_VP{SELF, BOTH}.p());
        user_Iterate_And_بعدی(nodep->msbp(), عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(Ast_Test_Plus_آرگومانها* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    فراغ بازدید(Ast_مقدار_Plus_آرگومانها* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_بعدی(nodep->outp(), عرض_VP{SELF, BOTH}.p());
        }
    }
    فراغ بازدید(Ast_زمان_Format* nodep) override {
        assertAtStatement(nodep);
        إذا (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        إذا (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        إذا (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        إذا (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    فراغ بازدید(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بعدی(nodep->passsp(), اشاره_تهی);
        user_Iterate_And_بعدی(nodep->failsp(), اشاره_تهی);
    }
    فراغ بازدید(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
    }
    فراغ بازدید(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بعدی(nodep->passsp(), اشاره_تهی);
        user_Iterate_And_بعدی(nodep->failsp(), اشاره_تهی);
    }
    فراغ بازدید(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_بعدی(nodep->passsp(), اشاره_تهی);
    }
    فراغ بازدید(AstRestrict* nodep) override {
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
        Ast_گره_DType* ثابت modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_گره_DType* ثابت conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        إذا (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) أرجع درست;
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
        ثابت صحيح from_چپ = idx / subElems;

        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ثابت uint32_t conDims = conDTypep->dimensions(خطأ).دومی;
        ثابت uint32_t modDims = modDTypep->dimensions(خطأ).دومی;

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
                user_Iterate_And_بعدی(nodep->exprp(),
                                   عرض_VP{conDTypep, FINAL, جریان_USE_تخصیص_بده}.p());
                أرجع درست;
            }
            // ArraySel index is counted from lo
            ثابت صحيح بیت_2 = arrp->declRange().ascending() ? from_چپ : elements - 1 - from_چپ;
            // Connection is self-determined
            user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{conDTypep, FINAL, جریان_USE_تخصیص_بده}.p());
            Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->exprp(), گره_عبارت)->unlink_Fr_پشت();
            إذا (Ast_گره_عبارت* ثابت elemp = newIfaceArrayElement(fromp, بیت_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(افزودن_Deletep(fromp), fromp);
            } وإلا {
                AstArraySel* ثابت selp = جديد AstArraySel{fromp->fileline(), fromp, بیت_2};
                selp->did_عرض(درست);
                nodep->exprp(selp);
            }
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        إذا (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) أرجع خطأ;

        // Packed connection
        ثابت صحيح modwidth = modDTypep->عرض();
        ثابت صحيح conwidth = conDTypep->عرض();

        // Connection has the width of the port, connect it to every element
        إذا (conwidth == modwidth) أرجع خطأ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        إذا (conwidth == modwidth * elements * subElems) {
            إذا (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->چپ_Const() << ":"
                                                     << rangep->راست_Const() << "]");
            }
            ثابت صحيح selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{conDTypep, FINAL, جریان_USE_تخصیص_بده}.p());
            Ast_گره_عبارت* ثابت fromp = VN_AS(nodep->exprp(), گره_عبارت)->unlink_Fr_پشت();
            AstSel* ثابت selp = جديد AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_چپ), selwidth};
            selp->did_عرض(درست);
            nodep->exprp(selp);
            أرجع pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{conDTypep, FINAL, جریان_USE_تخصیص_بده}.p());
        أرجع درست;
    }

    فراغ بازدید(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        إذا (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ثابت concatp = VN_CAST(nodep->exprp(), Replicate);
            ثابت AstConst* ثابت countp = concatp ? VN_CAST(concatp->countp(), Const) : اشاره_تهی;
            ثابت Ast_گره_DType* ثابت dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            إذا (countp && countp->مقصد_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_الگو* ثابت patternp = جديد Ast_الگو{concatp->fileline(), اشاره_تهی};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(افزودن_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_درون_Param_Override);
            m_درون_Param_Override = is_Override_الگو(nodep->exprp());
            userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
        } وإلا إذا (!m_paramsOnly) {
            إذا (!nodep->modVarp()->did_عرض()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), اشاره_تهی);
            }
            إذا (!nodep->exprp()) {  // No-connect
                إذا (nodep->modVarp()->is_Iface_Array_تقسیم()) expandIfaceArrayPin(nodep);
                أرجع;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ثابت VDirection pinDirection = nodep->modVarp()->direction();
            إذا (VN_IS(nodep->exprp(), گره_جریان) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_گره_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_گره_DType* conDTypep = nodep->exprp()->dtypep();
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
            Ast_گره_DType* subDTypep = modDTypep;
            ثابت صحيح modwidth = modDTypep->عرض();
            صحيح conwidth = conDTypep->عرض();
            إذا (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_بعدی(nodep->exprp(),
                                   عرض_VP{subDTypep, FINAL, جریان_USE_تخصیص_بده}.p());
            } وإلا {
                إذا (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    إذا (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        إذا (nodep->modVarp()->is_Iface_Array_تقسیم()) expandIfaceArrayPin(nodep);
                        أرجع;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->عرض();
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
                ثابت Ast_گره_Array_DType* ثابت عبارت_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ثابت Ast_گره_Array_DType* ثابت modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                إذا (عبارت_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_اندازه(conDTypep) != unpacked_Array_اندازه(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_اندازه(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_اندازه(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا إذا ((عبارت_Arrayp && !modArrayp) || (!عبارت_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (عبارت_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } وإلا {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_گره_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), گره_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_تخصیص_بده(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            إذا (nodep->modVarp()->is_Iface_Array_تقسیم()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    فراغ بازدید(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        إذا (!m_paramsOnly) {
            إذا (VN_IS(nodep->modp(), Not_یافت_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_مسیر_Looked_پیام(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            إذا (nodep->rangep()) user_Iterate_And_بعدی(nodep->rangep(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->pinsp(), اشاره_تهی);
            // If element of an instance array, pins are now connected, no longer need the ranges
            إذا (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                افزودن_Deletep(nodep->rangep()->unlink_Fr_پشت_With_بعدی());
            }
        }
        user_Iterate_And_بعدی(nodep->paramsp(), اشاره_تهی);
    }
    فراغ بازدید(AstGatePin* nodep) override {
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->rangep(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ثابت Ast_گره_DType* ثابت pinDTypep = nodep->exprp()->dtypep();
            ثابت صحيح رقم_Insts = nodep->rangep()->elementsConst();
            ثابت صحيح modwidth = رقم_Insts;
            ثابت صحيح pinwidth = pinDTypep->عرض();
            إذا (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_گره_DType* ثابت subDTypep = nodep->پیدا_کن_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{subDTypep, FINAL}.p());
                Ast_گره* ثابت newp
                    = جديد AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_پشت(),
                                       static_cast<uint32_t>(رقم_Insts)};
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
                user_Iterate_And_بعدی(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_پشت());
            }
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }
    فراغ بازدید(Ast_گره_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        إذا (nodep->did_عرض()) أرجع;
        إذا (nodep->doing_عرض()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(درست);
            nodep->did_عرض(درست);
            أرجع;
        }
        إذا (nodep->class_روش() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } وإلا إذا (nodep->class_روش() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        إذا (nodep->class_روش() && nodep->name() == "new") {
            إذا (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            إذا (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_گره* firstp = اشاره_تهی;
            لكل (Ast_گره* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                إذا (Ast_Stmt_عبارت* ثابت sep = VN_CAST(itemp, Stmt_عبارت)) {
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
                إذا (اولی_New_Statement_Ok_Recurse(itemp)) استمر;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_عرض(درست);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        إذا (nodep->fvarp()) userIterate(nodep->fvarp(), اشاره_تهی);
        إذا (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_مجموعه_Void();
        } وإلا إذا (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } وإلا إذا (VN_IS(nodep, Property)) {
            nodep->dtype_مجموعه_بیت();
        }
        عرض_VP* vup = اشاره_تهی;
        إذا (VN_IS(nodep, Property)) vup = عرض_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_عرض(درست);
        nodep->doing_عرض(خطأ);
        إذا (nodep->dpiImport() && !nodep->dpi_Open_والد() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_والد_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        إذا (nodep->fvarp() && !nodep->dpiImport() && !nodep->خالص_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_بعدی([&](ثابت AstVarRef* varrefp) {
                   أرجع varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::وکتور<AstVar*> ports;
        std::وکتور<Ast_Attr_از*> protos;
        Ast_Attr_از* protop = اشاره_تهی;  // Base prototype
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_پشت(portp); }
            } وإلا إذا (Ast_Attr_از* ثابت attrp = VN_CAST(stmtp, Attr_از)) {
                إذا (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_پشت(attrp); }
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
                    Ast_Attr_از* ثابت rprotop = protos[i];
                    Ast_گره_DType* ثابت declDtp = portp->dtypep();
                    Ast_گره_DType* ثابت protoDtp = rprotop->fromp()->dtypep();
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
                                << protoDtp->warn_بیشتر() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_بیشتر() << "... Declaration data type: "
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
    فراغ بازدید(AstConstraint* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, اشاره_تهی);
        nodep->did_عرض(درست);
    }
    فراغ بازدید(AstProperty* nodep) override {
        إذا (nodep->did_عرض()) أرجع;
        إذا (nodep->doing_عرض()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(درست);
            nodep->did_عرض(درست);
            أرجع;
        }
        nodep->doing_عرض(درست);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_مجموعه_بیت();
        لكل (Ast_گره* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            إذا (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, اشاره_تهی);
            } وإلا إذا (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } وإلا إذا (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, اشاره_تهی);
            } وإلا {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_عرض(درست);
        nodep->doing_عرض(خطأ);
    }
    فراغ بازدید(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    فراغ بازدید(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        إذا (nodep->did_عرض()) أرجع;
        nodep->dtype_مجموعه_بیت();
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (VN_IS(stmtp, Var)) {
                userIterate(stmtp, اشاره_تهی);
            } وإلا إذا (VN_IS(stmtp, گره_عبارت)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } وإلا {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_عرض(درست);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_قلم_Package(Ast_گره* pkgItemp) {
        طالما (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        أرجع VN_CAST(pkgItemp->backp(), Package);
    }
    فراغ بازدید(AstFuncRef* nodep) override {
        بازدید(static_cast<Ast_گره_FTask_Ref*>(nodep));
        إذا (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            إذا (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_عبارت))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_مجموعه_Void();
        } وإلا {
            nodep->dtypeFrom(nodep->taskp());
        }
        إذا (nodep->fileline()->timingOn()) {
            Ast_گره_Module* ثابت classp = nodep->classOrPackagep();
            إذا (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ثابت packagep = get_قلم_Package(classp);
                إذا (packagep && packagep->name() == "std") {
                    روش_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ساكن منطقي are_Same_اندازه(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ثابت std::وکتور<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ثابت std::وکتور<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        إذا (dims0.اندازه() != dims1.اندازه()) أرجع خطأ;
        لكل (اندازه_t i = 0; i < dims0.اندازه(); ++i) {
            إذا (dims0[i]->elementsConst() != dims1[i]->elementsConst()) أرجع خطأ;
        }
        أرجع درست;
    }
    // Makes sure that port and pin have same size and same datatype
    فراغ check_Unpacked_Array_آرگومانها(AstVar* portp, Ast_گره* pinp) {
        إذا (AstUnpackArrayDType* ثابت پورت_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            إذا (AstUnpackArrayDType* ثابت pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                إذا (!are_Same_اندازه(پورت_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << پورت_Dtypep->prettyDTypeNameQ() << ")");
                }
                ثابت AstBasicDType* ثابت پورت_Basicp = پورت_Dtypep->basicp();
                ثابت AstBasicDType* ثابت pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ثابت منطقي sameElemDType
                    = پورت_Basicp && pinBasicp
                          ? پورت_Basicp->عرض() == pinBasicp->عرض()
                                && پورت_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(پورت_Dtypep->elemDTypep());
                إذا (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, پورت_Dtypep, "Argument", "Parameter"));
                }
            } وإلا {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    فراغ process_FTask_Ref_آرگومانها(Ast_گره_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->did_عرض()) أرجع;
        userIterate(nodep->taskp(), اشاره_تهی);
        //
        // And do the arguments to the task/function too
        افعل {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ثابت V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            إذا (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            لكل (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.اولی;
                AstArg* ثابت argp = tconnect.دومی;
                Ast_گره_عبارت* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                إذا (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_پشت_With_بعدی(&handle);  // Format + additional args, if any
                    Ast_گره_عبارت* argsp = اشاره_تهی;
                    طالما (AstArg* ثابت nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->جمع_کن_بعدی(nextargp->exprp()->unlink_Fr_پشت_With_بعدی());
                        nextargp->unlink_Fr_پشت()->delete_درخت();  // Remove the call's Arg wrapper
                    }
                    string format;
                    إذا (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->رقم().مقصد_String();
                    } وإلا {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(افزودن_Deletep(argp), argp);
                    AstSFormatF* ثابت newp
                        = جديد AstSFormatF{nodep->fileline(), format, خطأ, argsp};
                    إذا (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(جديد AstScopeName{newp->fileline(), درست});
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
                    pinp->unlink_Fr_پشت(&handle);  // No next, that's the next pin
                    Ast_گره_عبارت* ثابت newp = جديد AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                إذا (portp->isWritable()) {
                    V3LinkLValue::link_LValue_مجموعه(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, عرض_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } طالما (خطأ);
        // Stage 2
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خودکار& tconnect : tconnects) {
                AstVar* ثابت portp = tconnect.اولی;
                ثابت AstArg* ثابت argp = tconnect.دومی;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Change data types based on above accept completion
                إذا (nodep->taskp()->dpiImport()) check_Unpacked_Array_آرگومانها(portp, pinp);
                إذا (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.اولی;
                ثابت AstArg* ثابت argp = tconnect.دومی;
                Ast_گره_عبارت* ثابت pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, عرض_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        إذا (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            لكل (ثابت خودکار& tconnect : tconnects) {
                ثابت AstVar* ثابت portp = tconnect.اولی;
                ثابت AstArg* ثابت argp = tconnect.دومی;
                Ast_گره_عبارت* pinp = argp->exprp();
                إذا (!pinp) استمر;  // Argument error we'll find later
                Ast_گره_DType* ثابت پورت_DTypep = portp->dtypep()->skip_Ref_مقصد_Enump();
                إذا (VN_IS(pinp, خالی_صف)) {
                    Ast_گره_عبارت* newp = new_Cons_Dyn_Array_Or_صف(pinp->fileline(), پورت_DTypep);
                    إذا (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = جديد AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(پورت_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ثابت Ast_گره_DType* ثابت pinDTypep = pinp->dtypep()->skip_Ref_مقصد_Enump();
                ثابت AstIfaceRefDType* ثابت پورت_Ifacep
                    = VN_CAST(پورت_DTypep->elemDTypep(درست), IfaceRefDType);
                ثابت VCastable ifaceCastable
                    = پورت_Ifacep ? Ast_گره::computeCastable(پورت_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ درست)
                                 : VCastable{VCastable::UNSUPPORTED};
                ثابت منطقي matchingRefDTypes = پورت_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(پورت_DTypep, pinDTypep);
                ثابت منطقي pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->رقم().isNull();
                إذا ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    إذا (پورت_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } وإلا {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << پورت_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } وإلا إذا (پورت_Ifacep && پورت_Ifacep->isVirtual() && portp->is_درون_داد() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } وإلا إذا (portp->isWritable() && pinp->عرض() != portp->عرض()) {
                    pinp->v3widthWarn(portp->عرض(), pinp->عرض(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->عرض()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->عرض() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_پشت(&relinkHandle);
                    Ast_گره_عبارت* ثابت newp = جديد AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                إذا (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(پورت_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && پورت_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << پورت_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    استمر;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                إذا (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } وإلا إذا (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_تخصیص_بده(nodep, "Function Argument", pinp, پورت_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, عرض_VP{پورت_DTypep, FINAL, جریان_USE_تخصیص_بده}.p());
                } وإلا {
                    iterate_Check_تخصیص_بده(nodep, "Function Argument", pinp, FINAL, پورت_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    فراغ handle_Std_Randomize_آرگومانها(Ast_گره_FTask_Ref* ثابت nodep) {
        AstConst* nullp = اشاره_تهی;
        لكل (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_گره_عبارت* ثابت exprp = argp->exprp();
            إذا (AstConst* ثابت constp = VN_CAST(exprp, Const)) {
                إذا (constp->رقم().isNull()) {
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
                إذا (methodp->روش() == VCMethod::ARRAY_AT
                    || methodp->روش() == VCMethod::ARRAY_AT_WRITE) {
                    استمر;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        إذا (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    فراغ بازدید(Ast_گره_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        إذا (nodep->did_عرض()) أرجع;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = اشاره_تهی;
        إذا (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(درست);
            nodep->dtypep(nodep->پیدا_کن_Basic_DType(VBasicDTypeKwd::INT));
            أرجع;  // Handled in V3Randomize
        } وإلا إذا (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ثابت classp = VN_CAST(nodep->classOrPackagep(), Class);
            إذا (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(درست);
                Ast_گره_DType* ثابت adtypep = nodep->پیدا_کن_بیت_DType();
                withp = روش_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کن_Void_DType(),
                                         adtypep->پیدا_کن_بیت_DType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_بعدی(argp->exprp(), عرض_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_آرگومانها(nodep);  // Provided args should be in current scope
                process_FTask_Ref_آرگومانها(nodep);
                إذا (withp) nodep->withp(withp);
                nodep->did_عرض(درست);
                أرجع;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            إذا (nodep->name() == "randomize") {
                AstClassRefDType* ثابت adtypep
                    = جديد AstClassRefDType{nodep->fileline(), classp, اشاره_تهی};
                v3Global.rootp()->typeTablep()->جمع_کن_Typesp(adtypep);
                withp = روش_With_Clause(nodep, خطأ, خطأ, adtypep->پیدا_کن_Void_DType(),
                                         adtypep->پیدا_کن_بیت_DType(), adtypep);
                لكل (ثابت AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_بعدی(argp->exprp(), عرض_VP{SELF, BOTH}.p());
                }
                handle_Randomize_آرگومانها(nodep, classp);
            } وإلا إذا (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } وإلا إذا (nodep->name() == "get_randstate") {
                روش_Ok_Arguments(nodep, 0, 0);
                classp->پایه_Most_Classp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_مجموعه_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            } وإلا إذا (nodep->name() == "set_randstate") {
                روش_Ok_Arguments(nodep, 1, 1);
                Ast_گره_عبارت* ثابت expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_گره_عبارت* ثابت exprp = nodep->argsp()->exprp();
                classp->پایه_Most_Classp()->needRNG(درست);
                v3Global.useRandomizeMethods(درست);
                AstCExpr* ثابت newp
                    = جديد AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->جمع_کن(exprp->unlink_Fr_پشت());
                newp->جمع_کن(")");
                newp->dtype_مجموعه_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                UASSERT_OBJ(خطأ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        إذا (nodep->did_عرض()) {
            إذا (withp) nodep->withp(withp);
            أرجع;
        }
        إذا ((nodep->taskp()->class_روش() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            منطقي allow = خطأ;
            إذا (m_ftaskp && m_ftaskp->class_روش() && !m_ftaskp->isStatic()) {
                إذا (ثابت AstFuncRef* ثابت funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } وإلا إذا (ثابت AstTaskRef* ثابت taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                إذا (!allow) {
                    ثابت AstClass* callerClassp = m_containingClassFinder.پیدا_کن(m_ftaskp);
                    إذا (!callerClassp) {
                        callerClassp = m_containingClassFinder.پیدا_کن(m_ftaskp->classOrPackagep());
                    }
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.پیدا_کن(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            إذا (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ثابت AstClass* ثابت cgClassp = m_containingClassFinder.پیدا_کن(nodep);
                إذا (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ثابت AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    إذا (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.پیدا_کن(nodep->taskp());
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
        process_FTask_Ref_آرگومانها(nodep);
        إذا (withp) nodep->withp(withp);
        nodep->did_عرض(درست);
        // See steps that follow in visit(AstFuncRef*)
    }
    فراغ بازدید(Ast_گره_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, اشاره_تهی);
    }
    فراغ بازدید(Ast_Sen_قلم* nodep) override {
        إذا (nodep->is_Combo_ستاره()) أرجع;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        إذا (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            خودکار* ثابت concatOrReplp = VN_CAST(nodep->sensp(), گره_Biop);
            خودکار* ثابت rhsp = concatOrReplp->rhsp()->unlink_Fr_پشت();
            إذا (nodep->یال_Type() == VEdgeType::ET_تغییریافته) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                خودکار* ثابت lhsp = concatOrReplp->lhsp()->unlink_Fr_پشت();
                nodep->جمع_کن_بعدی_اینجا(جديد Ast_Sen_قلم{lhsp->fileline(), nodep->یال_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(جديد Ast_Sen_قلم{rhsp->fileline(), nodep->یال_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_درخت(), nodep);
        } وإلا {
            userIterateChildren(nodep, عرض_VP{SELF, BOTH}.p());
            إذا (nodep->یال_Type().an_یال()) {
                Ast_گره_DType* ثابت sensDtp = nodep->sensp()->dtypep()->skipRefp();
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
    فراغ بازدید(Ast_Clocking_قلم* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ثابت refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, عرض_VP{SELF, PRELIM}.p());
    }
    فراغ بازدید(AstWait* nodep) override {
        إذا (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_پشت()->delete_درخت(), nodep);
            أرجع;
        }
        إذا (nodep->fileline()->timingOn()) {
            إذا (v3Global.opt.timing().is_مجموعه_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                إذا (Ast_گره_Var_Ref* ثابت varrefp = VN_CAST(nodep->condp(), گره_Var_Ref)) {
                    إذا (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_بیشتر() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                أرجع;
            } وإلا إذا (v3Global.opt.timing().is_مجموعه_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } وإلا {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        إذا (Ast_گره* ثابت stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_پشت_With_بعدی();
            nodep->replaceWith(stmtsp);
        } وإلا {
            nodep->unlink_Fr_پشت();
        }
        VL_DO_DANGLING(nodep->delete_درخت(), nodep);
    }
    فراغ بازدید(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->شاخص_Arg_Refp(), اشاره_تهی);
        userIterateChildren(nodep->مقدار_Arg_Refp(), اشاره_تهی);
        إذا (!nodep->exprp()) {
            nodep->dtype_مجموعه_Void();
        } وإلا {
            إذا (!nodep->exprp()->hasDType()) {
                user_Iterate_And_بعدی(nodep->exprp(), اشاره_تهی);
            } وإلا {
                إذا (vdtypep) {
                    user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{nodep->dtypep(), PRELIM}.p());
                } وإلا {  // 'sort with' allows arbitrary type
                    user_Iterate_And_بعدی(nodep->exprp(), عرض_VP{SELF, PRELIM}.p());
                }
            }

            إذا (!nodep->exprp()->hasDType()) {
                nodep->dtype_مجموعه_Void();
            } وإلا {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_تخصیص_بده(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    فراغ بازدید(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        إذا (nodep->شاخص()) {
            nodep->dtypeFrom(m_withp->شاخص_Arg_Refp());
        } وإلا {
            nodep->dtypeFrom(m_withp->مقدار_Arg_Refp());
        }
    }
    فراغ بازدید(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, اشاره_تهی);
    }

    //--------------------
    // Default
    فراغ بازدید(Ast_گره_عبارت* nodep) override {
        إذا (!nodep->did_عرض()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, اشاره_تهی);
    }
    فراغ بازدید(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << پایان_سطر);
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        إذا (Ast_گره_DType* ثابت vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } وإلا إذا (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->عرض() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->عرض() << " bits -> " << nodep->عرض()
                                    << " bits" << پایان_سطر);

        // Final processing
        user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, FINAL}.p());
        nodep->did_عرض(درست);
    }
    فراغ بازدید_Class(AstClass* nodep) {
        إذا (nodep->did_عرض_And_مجموعه()) أرجع;

        // If the class is std::process
        إذا (nodep->name() == "process") {
            AstPackage* ثابت packagep = get_قلم_Package(nodep);
            إذا (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                إذا (AstVar* ثابت varp
                    = VN_CAST(m_memberMap.پیدا_کن_Member(nodep, "m_process"), Var)) {
                    Ast_گره_DType* ثابت dtypep = varp->get_فرزند_DTypep();
                    إذا (!varp->dtypep()) {
                        VL_DO_DANGLING(افزودن_Deletep(dtypep->unlink_Fr_پشت()), dtypep);
                    }
                    AstBasicDType* ثابت newdtypep = جديد AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->جمع_کن_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                إذا (Ast_گره_FTask* ثابت ftaskp
                    = VN_CAST(m_memberMap.پیدا_کن_Member(nodep, "self"), گره_FTask)) {
                    ftaskp->مجموعه_Need_Process();
                    v3Global.مجموعه_استفاده_میکند_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_بعدی(nodep->extendsp(), اشاره_تهی);
        userIterateChildren(nodep, اشاره_تهی);  // First size all members
    }
    فراغ بازدید(Ast_گره_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_موقت_نامها);
        VL_RESTORER_COPY(m_inside_موقت_نامها);
        إذا (AstClass* ثابت classp = VN_CAST(nodep, Class)) {
            بازدید_Class(classp);
        } وإلا {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([هذا](ثابت AstVar* varp) { m_curModVars.درج(varp); });
            userIterateChildren(nodep, اشاره_تهی);
        }
    }
    فراغ بازدید(Ast_گره* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, اشاره_تهی);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    فراغ بازدید_Or_Lu64(Ast_گره_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_مجموعه_Double();
            Ast_گره_DType* ثابت subDTypep = nodep->پیدا_کن_بیت_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    فراغ بازدید(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_مجموعه_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM}.p());
            إذا (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    جديد AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_پشت()});
                VL_DO_DANGLING(nodep->delete_درخت(), nodep);
            }
        }
    }
    فراغ بازدید(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_مجموعه_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM}.p());
        }
    }
    فراغ بازدید_Os32_Lr(Ast_گره_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_Integer2_State();
        }
    }
    فراغ بازدید_Ou64_Lr(Ast_گره_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_UInt64();
        }
    }

    فراغ بازدید_log_not(AstLogNot* nodep) {
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
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_مجموعه_بیت_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            إذا (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ثابت newp
                    = جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            }
        }
    }
    فراغ بازدید_log_and_or(Ast_گره_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت_Or_Logic(
                Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    فراغ بازدید_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید_red_and_or(Ast_گره_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_مجموعه_بیت_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    فراغ بازدید_red_unknown(Ast_گره_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            nodep->dtype_مجموعه_بیت();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    منطقي isEquivalentDType(ثابت Ast_گره_DType* lhs, ثابت Ast_گره_DType* rhs) {
        // a) If two types match, they are equivalent.
        إذا (!lhs || !rhs) أرجع خطأ;
        lhs = lhs->skip_Ref_مقصد_Enump();
        rhs = rhs->skip_Ref_مقصد_Enump();
        إذا (lhs == rhs) أرجع درست;
        // If both are basic types, check if they are the same type
        إذا (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ثابت خودکار* lb = VN_CAST(lhs, BasicDType);
            ثابت خودکار* rb = VN_CAST(rhs, BasicDType);
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
        ثابت منطقي lhs_Is_صف = VN_IS(lhs, صف_DType);
        ثابت منطقي rhs_Is_صف = VN_IS(rhs, صف_DType);
        ثابت منطقي lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ثابت منطقي rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        إذا (lhsIsDynArray || rhsIsDynArray || lhs_Is_صف || rhs_Is_صف || lhsIsAssocArray
            || rhsIsAssocArray) {
            إذا (ثابت AstDynArrayDType* ثابت lhsp = VN_CAST(lhs, DynArrayDType)) {
                إذا (ثابت AstDynArrayDType* ثابت rhsp = VN_CAST(rhs, DynArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت Ast_صف_DType* ثابت lhsp = VN_CAST(lhs, صف_DType)) {
                إذا (ثابت Ast_صف_DType* ثابت rhsp = VN_CAST(rhs, صف_DType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            إذا (ثابت AstAssocArrayDType* ثابت lhsp = VN_CAST(lhs, AssocArrayDType)) {
                إذا (ثابت AstAssocArrayDType* ثابت rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    أرجع isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->کلید_DTypep(), rhsp->کلید_DTypep());
                }
            }

            أرجع خطأ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        إذا (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            إذا (lhs->عرض() != rhs->عرض()) أرجع خطأ;
            إذا (lhs->isFourstate() != rhs->isFourstate()) أرجع خطأ;
            إذا (lhs->isSigned() != rhs->isSigned()) أرجع خطأ;
            أرجع درست;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        إذا (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) أرجع خطأ;
        أرجع !(VN_IS(lhs, گره_UOr_Struct_DType) || VN_IS(rhs, گره_UOr_Struct_DType));
    }

    ساكن منطقي isAggregateType(ثابت Ast_گره* nodep) {
        إذا (!nodep) أرجع خطأ;
        ثابت Ast_گره_DType* dtypep = nodep->dtypep();
        إذا (!dtypep) أرجع خطأ;
        dtypep = dtypep->skipRefp();
        إذا (!dtypep) أرجع خطأ;
        أرجع dtypep->isAggregateType();
    }

    فراغ بازدید_cmp_eq_gt(Ast_گره_Biop* nodep, ثابت ComparisonType cmpType) {
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
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ثابت منطقي lhsIsPat = VN_IS(nodep->lhsp(), الگو);
            ثابت منطقي rhsIsPat = VN_IS(nodep->rhsp(), الگو);
            إذا (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } وإلا إذا (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } وإلا {
                user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            }

            ثابت منطقي isAggrLhs = isAggregateType(nodep->lhsp());
            ثابت منطقي isAggrRhs = isAggregateType(nodep->rhsp());

            إذا ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::نرمال_EQ || cmpType == ComparisonType::CASE)) {
                ثابت Ast_گره_DType* ثابت lhsDType = nodep->lhsp()->dtypep();
                ثابت Ast_گره_DType* ثابت rhsDType = nodep->rhsp()->dtypep();

                إذا (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_گره* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_ترتیب(nodep->rhsp(), nodep->lhsp()->dtypep());
            } وإلا إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                إذا (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_گره* ثابت newp
                        = جديد AstConst{nodep->fileline(), AstConst::بیت_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                    أرجع;
                }
                إذا (Ast_گره_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                إذا (Ast_گره_Biop* ثابت newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } وإلا {
                ثابت منطقي signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                إذا (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ثابت صحيح عرض = std::max(nodep->lhsp()->عرض(), nodep->rhsp()->عرض());
                ثابت صحيح ewidth = std::max(nodep->lhsp()->عرض_Min(), nodep->rhsp()->عرض_Min());
                منطقي warnOn = درست;
                إذا (!signedFl && عرض == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    إذا ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->عرض() >= nodep->rhsp()->عرض_Min())) {
                        warnOn = خطأ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    إذا ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->عرض_Min() >= nodep->rhsp()->عرض())) {
                        warnOn = خطأ;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->پیدا_کن_بیت_Or_Logic_DType(عرض, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->پیدا_کن_بیت_Or_Logic_DType(عرض, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_مجموعه_بیت_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::نرمال_EQ
                                               || cmpType == ComparisonType::نرمال_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    فراغ بازدید_cmp_real(Ast_گره_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید_cmp_string(Ast_گره_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_بیت();
        }
    }
    فراغ بازدید_cmp_type(Ast_گره_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, BOTH}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{SELF, BOTH}.p());
            ثابت Ast_Attr_از* ثابت lhsap = VN_AS(nodep->lhsp(), Attr_از);
            ثابت Ast_Attr_از* ثابت rhsap = VN_AS(nodep->rhsp(), Attr_از);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ثابت Ast_گره_DType* ثابت lhsDtp = lhsap->dtypep();
            ثابت Ast_گره_DType* ثابت rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ثابت منطقي invert = VN_IS(nodep, NeqT);
            ثابت منطقي identical
                = Ast_گره::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ثابت منطقي eq = invert ^ identical;
            Ast_گره* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::بیت_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        }
    }

    فراغ بازدید_negate_not(Ast_گره_Uniop* nodep, منطقي real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) checkCvtUS(nodep->lhsp(), خطأ);
        }
        إذا (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            إذا (Ast_گره_Uniop* ثابت newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_مجموعه_Double();
                أرجع;
            }
        } وإلا {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        إذا (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        إذا (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            // Some warning suppressions
            منطقي lhsWarn = درست;
            إذا (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->عرض_Min() == (nodep->lhsp()->عرض_Min() + 1)) lhsWarn = خطأ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    فراغ بازدید_signed_unsigned(Ast_گره_Uniop* nodep, VSigning rs_بیرون) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), درست);
            ثابت صحيح عرض = nodep->lhsp()->عرض();
            Ast_گره_DType* ثابت expDTypep = nodep->پیدا_کن_بیت_Or_Logic_DType(
                عرض, عرض, rs_بیرون, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ بازدید_لغزش_2(Ast_گره_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_لغزش_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ثابت Ast_گره_Biop* ثابت newp = iterate_لغزش_final(nodep);
        VL_DANGLING(nodep);
        (فراغ)newp;  // Ununused
    }
    فراغ iterate_لغزش_prelim(Ast_گره_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            إذا (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_مجموعه_Logic_Unsized(nodep->عرض(), nodep->عرض_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_گره_Biop* iterate_لغزش_final(Ast_گره_Biop* nodep) {
        // Nodep maybe edited
        assert_At_عبارت(nodep);
        إذا (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            إذا (VN_IS(nodep, لغزش_RS)) {
                إذا (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            منطقي warnOn = درست;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            إذا (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), گره_تخصیص_بده)) warnOn = خطأ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ثابت shiftp = VN_CAST(nodep->rhsp(), Const);
            إذا (shiftp && !shiftp->رقم().isFourState() && shiftp->عرض() <= 32) {
                ثابت int64_t لغزش_Val = shiftp->رقم().مقصد_SQuad();
                إذا (VN_IS(nodep, لغزش_L)) {
                    إذا (لغزش_Val > 0 && nodep->عرض() == nodep->lhsp()->عرض() + لغزش_Val)
                        warnOn = خطأ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            إذا (nodep->rhsp()->عرض() > 32) {
                إذا (shiftp && shiftp->رقم().most_مجموعه_بیت_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_عدد رقم{shiftp, 32, shiftp->رقم()};
                    Ast_گره* ثابت shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(جديد AstConst{shiftrhsp->fileline(), رقم});
                    VL_DO_DANGLING(shiftrhsp->delete_درخت(), shiftrhsp);
                }
            }
        }
        أرجع nodep;  // May edit
    }

    فراغ بازدید_boolexpr_and_or(Ast_گره_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), خطأ);
            checkCvtUS(nodep->rhsp(), خطأ);
            ثابت صحيح عرض = std::max(nodep->lhsp()->عرض(), nodep->rhsp()->عرض());
            ثابت صحيح mwidth = std::max(nodep->lhsp()->عرض_Min(), nodep->rhsp()->عرض_Min());
            ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                عرض, mwidth, VSigning::fromBool(expSigned),
                Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        إذا (m_vup->final()) {
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    فراغ بازدید_جمع_کن_sub_replace(Ast_گره_Biop* nodep, منطقي real_ok, ثابت منطقي modOrDiv) {
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
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_بعدی(nodep->lhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_بعدی(nodep->rhsp(), عرض_VP{CONTEXT_DET, PRELIM}.p());
            إذا (!real_ok) {
                checkCvtUS(nodep->lhsp(), خطأ);
                checkCvtUS(nodep->rhsp(), خطأ);
            }
            إذا (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                إذا (Ast_گره_Biop* ثابت newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_مجموعه_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                أرجع;
            } وإلا إذا (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, جمع_کن_2)
                            ? (nodep->warn_بیشتر()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع;
            } وإلا {
                ثابت صحيح عرض = std::max(nodep->lhsp()->عرض(), nodep->rhsp()->عرض());
                ثابت صحيح mwidth = std::max(nodep->lhsp()->عرض_Min(), nodep->rhsp()->عرض_Min());
                ثابت منطقي expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_مجموعه_بیت_Or_Logic_Unsized(
                    عرض, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_گره_عبارت::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        إذا (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_گره_DType* ثابت expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_گره_DType* ثابت subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            إذا (Ast_گره_Biop* ثابت newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            منطقي lhsWarn = درست;
            منطقي rhsWarn = درست;
            إذا (VN_IS(nodep, جمع_کن_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                إذا (subDTypep->عرض_Min() == (nodep->lhsp()->عرض_Min() + 1)) lhsWarn = خطأ;
                إذا (subDTypep->عرض_Min() == (nodep->rhsp()->عرض_Min() + 1)) rhsWarn = خطأ;
                إذا (VN_IS(nodep, جمع_کن_2) && nodep->lhsp()->عرض() == 1
                    && nodep->rhsp()->عرض() != 1)
                    lhsWarn = خطأ;  // do_increment + ...
                إذا (nodep->rhsp()->عرض() == 1 && nodep->lhsp()->عرض() != 1)
                    rhsWarn = خطأ;  // ... + do_increment
            } وإلا إذا (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                إذا (subDTypep->عرض_Min() >= (nodep->lhsp()->عرض_Min())) lhsWarn = خطأ;
                إذا (subDTypep->عرض_Min() >= (nodep->rhsp()->عرض_Min())) rhsWarn = خطأ;
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
    فراغ بازدید_real_جمع_کن_sub(Ast_گره_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_مجموعه_Double();
        }
    }
    فراغ بازدید_real_neg_ceil(Ast_گره_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_عبارت(nodep);
        إذا (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_مجموعه_Double();
        }
    }

    فراغ بازدید_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->عبارت_Format_2(), nodep, "Assumes constant format");
        منطقي درون_Pct = خطأ;
        Ast_گره_عبارت* argp = nodep->exprsp();
        string newFormat;
        لكل (حرف ch : nodep->متن()) {
            إذا (!درون_Pct && ch == '%') {
                درون_Pct = درست;
                newFormat += ch;
            } وإلا إذا (درون_Pct && (std::آیا_رقم(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } وإلا إذا (!درون_Pct) {  // Normal text
                newFormat += ch;
            } وإلا {
                درون_Pct = خطأ;
                Ast_گره_عبارت* ثابت nextp = argp ? VN_AS(argp->nextp(), گره_عبارت) : اشاره_تهی;
                AstSFormatArg* ثابت fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_گره_عبارت* subargp = fargp ? fargp->exprp() : argp;
                ثابت Ast_گره_DType* ثابت dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : اشاره_تهی;
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
                        إذا (subargp->isDouble()) spliceCvtS(subargp, درست, 64);
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
                            subargp->unlink_Fr_پشت(&relinker);
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

    ساكن AstEnumDType* formatEnumDType(Ast_گره_عبارت* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
        إذا (!enumDtp) {
            إذا (ثابت AstVarRef* ثابت varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_مقصد_Enump(), EnumDType);
            }
        }
        أرجع enumDtp;
    }

    Ast_عبارت_Stmt* newEnumCapture(Ast_گره_عبارت* valuep, AstEnumDType* dtypep) {
        File_سطر* ثابت flp = valuep->fileline();
        AstVar* ثابت varp = جديد AstVar{flp, VVarType::XTEMP, m_enum_موقت_نامها.get(valuep), dtypep};
        varp->noSubst(درست);
        إذا (m_ftaskp) {
            varp->funcLocal(درست);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } وإلا {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->جمع_کن_Stmtsp(varp);
        }
        iterate(varp);
        Ast_عبارت_Stmt* ثابت exprp = جديد Ast_عبارت_Stmt{
            flp, جديد Ast_تخصیص_بده{flp, جديد AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            جديد AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        أرجع exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_گره_عبارت* valuep, AstEnumDType* dtypep) {
        إذا (valuep->is_خالص()) valuep = V3Const::constifyEdit(valuep);
        Ast_گره_عبارت* keyp;
        إذا (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_درخت(خطأ);
        } وإلا {
            Ast_عبارت_Stmt* ثابت capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_درخت_خالص(خطأ);
        }
        ثابت VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ثابت argp = جديد AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        أرجع argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    منطقي عرض_Bad(Ast_گره* nodep, Ast_گره_DType* expDTypep) {
        ثابت صحيح exp_عرض = expDTypep->عرض();
        صحيح exp_عرض_Min = expDTypep->عرض_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        إذا (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) أرجع خطأ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->عرض() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_عرض != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        إذا (exp_عرض_Min == 0) exp_عرض_Min = exp_عرض;
        إذا (nodep->dtypep()->عرض() == exp_عرض) أرجع خطأ;
        إذا (nodep->dtypep()->عرض_Sized() && nodep->عرض() != exp_عرض_Min) أرجع درست;
        إذا (!nodep->dtypep()->عرض_Sized() && nodep->عرض_Min() > exp_عرض_Min) أرجع درست;
        أرجع خطأ;
    }

    فراغ fix_عرض_Extend(Ast_گره_عبارت* nodep, Ast_گره_DType* expDTypep, Extend_قاعده extend_قاعده) {
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
        ثابت صحيح exp_عرض = expDTypep->عرض();
        إذا (constp && !constp->رقم().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_عدد رقم{nodep, exp_عرض, constp->رقم()};
            رقم.isSigned(خطأ);
            Ast_گره_عبارت* ثابت newp = جديد AstConst{nodep->fileline(), رقم};
            constp->replaceWith(newp);
            VL_DO_DANGLING(افزودن_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا إذا (exp_عرض < nodep->عرض()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            Ast_گره_عبارت* ثابت newp = جديد AstSel{nodep->fileline(), nodep, 0, exp_عرض};
            newp->did_عرض(درست);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } وإلا {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            منطقي doSigned = خطأ;
            بدل (extend_قاعده) {
            حالة EXTEND_ZERO: doSigned = خطأ; اكسر;
            حالة EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); اكسر;
            حالة EXTEND_LHS: doSigned = nodep->isSigned(); اكسر;
            افتراضي: nodep->v3fatalSrc("bad case");
            }
            Ast_گره_عبارت* ثابت newp
                = (doSigned ? static_cast<Ast_گره_عبارت*>(جديد AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_گره_عبارت*>(جديد AstExtend{nodep->fileline(), nodep}));
            newp->did_عرض(درست);
            linker.relink(newp);
            nodep = newp;
        }
        إذا (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_گره_عبارت* ثابت newp = spliceCvtD(nodep);
            nodep = newp;
        }
        إذا (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_عرض_Signed(exp_عرض, expDTypep->عرض_Min(), expDTypep->numeric());
        } وإلا {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    فراغ fix_عرض_Reduce(Ast_گره_عبارت* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ثابت صحيح exp_عرض = 1;
        ثابت صحيح expSigned = خطأ;
        إذا (nodep->عرض() == exp_عرض) أرجع;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (constp) {
            V3_عدد رقم(nodep, exp_عرض);
            رقم.opRedOr(constp->رقم());
            رقم.isSigned(expSigned);
            Ast_گره_عبارت* ثابت newp = جديد AstConst{nodep->fileline(), رقم};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_درخت(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } وإلا {
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            Ast_گره_عبارت* newp;
            Ast_گره_DType* ثابت dtypeSkipp = nodep->dtypep()->skipRefp();
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
        nodep->dtype_Chg_عرض_Signed(exp_عرض, exp_عرض, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    منطقي fixAutoExtend(Ast_گره_عبارت*& nodepr, صحيح exp_عرض) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        إذا (AstConst* ثابت constp = VN_CAST(nodepr, Const)) {
            إذا (constp->رقم().autoExtend() && !constp->رقم().sized() && constp->عرض() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_عدد رقم(constp, exp_عرض);
                رقم.opRepl(constp->رقم(), exp_عرض);  // {width{'1}}
                Ast_گره_عبارت* ثابت newp = جديد AstConst{constp->fileline(), رقم};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع درست;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            وإلا إذا (!constp->رقم().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_عرض > 32 && constp->رقم().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_عرض
                                                  << " bits: " << constp->prettyName());
                V3_عدد رقم(constp, exp_عرض);
                رقم.opExtendXZ(constp->رقم(), constp->عرض());
                Ast_گره_عبارت* ثابت newp = جديد AstConst{constp->fileline(), رقم};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_درخت(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                أرجع درست;
            }
        }
        أرجع خطأ;  // No change
    }
    منطقي is_پایه_Class_Recurse(ثابت AstClass* ثابت cls1p, ثابت AstClass* ثابت cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        إذا (cls1p == cls2p) أرجع درست;
        لكل (ثابت AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            إذا (is_پایه_Class_Recurse(cls1p, cextp->classp())) أرجع درست;
        }
        أرجع خطأ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ساكن string warnDTypes(ثابت Ast_گره* nodep, ثابت Ast_گره_DType* lhsDtp,
                             ثابت Ast_گره_DType* rhsDtp, ثابت string& lhsName = "Left-hand",
                             ثابت string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        إذا (lhsState == rhsState) lhsState = rhsState = "";
        أرجع "\n" + nodep->warn_بیشتر() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_بیشتر()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ساكن منطقي unpacked_Array_Sizes_تطابق(ثابت Ast_گره_DType* lhsDtp, ثابت Ast_گره_DType* rhsDtp) {
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
        أرجع درست;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    قالب <نام_نوع T, نام_نوع N>
    فراغ checkUnpackedArrayAssignmentCompatible(ثابت Ast_گره* nodep, ثابت T* ثابت lhsRefp,
                                                ثابت N* ثابت rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::مقدار || std::is_same<T, Ast_گره_Var_Ref>::مقدار)
                && (std::is_same<N, AstVar>::مقدار || std::is_same<N, Ast_گره_Var_Ref>::مقدار),
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

        ثابت Ast_گره_DType* ثابت lhsDtp = lhsRefp->dtypep()->skipRefp();
        ثابت Ast_گره_DType* ثابت rhsDtp = rhsRefp->dtypep()->skipRefp();
        ثابت منطقي isLhsAggregate = lhsDtp->isAggregateType();
        ثابت منطقي isRhsAggregate = rhsDtp->isAggregateType();
        إذا (!isLhsAggregate && !isRhsAggregate) أرجع;
        إذا (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        } وإلا إذا (VN_IS(lhsDtp, صف_DType) && VN_IS(rhsDtp, خالی_صف_DType)) {
            أرجع;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(خطأ),
                                      rhsDim = rhsDtp->dimensions(خطأ);
        // Check if unpacked array dimensions are matching
        إذا (lhsDim.دومی != rhsDim.دومی) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            أرجع;
        }

        ثابت Ast_گره_DType* lhsDtpIterp = lhsDtp;
        ثابت Ast_گره_DType* rhsDtpIterp = rhsDtp;
        لكل (uint32_t dim = 0; dim < rhsDim.دومی; dim++) {
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
    فراغ check_Class_تخصیص_بده(ثابت Ast_گره* nodep, ثابت حرف* side, Ast_گره* rhsp,
                          Ast_گره_DType* ثابت lhsDTypep, منطقي isOutputArg = خطأ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ثابت Ast_گره_DType* ثابت lhsRawDTypep = lhsDTypep->skipRefp();
        ثابت Ast_گره_DType* ثابت rhsRawDTypep = rhsp->dtypep()->skipRefp();
        إذا (ثابت AstClassRefDType* ثابت lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            إذا (ثابت AstClassRefDType* ثابت rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ثابت AstClass* ثابت dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ثابت AstClass* ثابت srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                إذا (is_پایه_Class_Recurse(dstp, srcp)) أرجع;
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
            rhsp->unlink_Fr_پشت(&relinker);
            relinker.relink(
                جديد Ast_Cvt_Unpack_مقصد_صف{rhsp->fileline(), VN_AS(rhsp, گره_عبارت), lhsDTypep});
        }
    }
    ساكن منطقي similarDTypeRecurse(ثابت Ast_گره_DType* ثابت node1p,
                                    ثابت Ast_گره_DType* ثابت node2p) {
        أرجع node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    فراغ iterateCheckFileDesc(Ast_گره* parentp, Ast_گره* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, PRELIM}.p());
        Ast_گره_DType* ثابت expDTypep = underp->پیدا_کن_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckReal(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کن_Double_DType(), stage);
    }
    فراغ iterateCheckSigned8(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کن_Signed8_DType(), stage);
    }
    فراغ iterateCheckSigned32(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کن_Int_DType(), stage);
    }
    فراغ iterateCheckUInt32(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->پیدا_کن_UInt32_DType(), stage);
    }
    فراغ iterateCheckDelay(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            Ast_گره_DType* expDTypep;
            إذا (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->پیدا_کن_Double_DType();
            } وإلا {
                File_سطر* ثابت newFl = جديد File_سطر{underp->fileline()};
                newFl->warnOff(V3_Error_کد::WIDTHEXPAND, درست);
                underp->fileline(newFl);
                expDTypep = parentp->پیدا_کن_بیت_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, خطأ);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckString(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->پیدا_کن_String_DType(), stage);
    }
    فراغ iterateCheckTyped(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp,
                           Ast_گره_DType* expDTypep, Stage stage) {
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, عرض_VP{expDTypep, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckTypedSelfPrelim(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp,
                                     Ast_گره_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, PRELIM}.p());
        }
        إذا (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckIntegralSelf(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, درست);
    }
    فراغ iterateCheckSelf(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Determ determ,
                          Stage stage, منطقي integralOnly = خطأ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, گره_عبارت) ? checkCvtUS(VN_AS(underp, گره_عبارت), integralOnly) : underp;
        Ast_گره_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (فراغ)underp;  // cppcheck
    }
    فراغ iterateCheckSizedSelf(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Determ determ,
                               Stage stage, منطقي deferUntyped = خطأ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        إذا (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, گره_عبارت) ? checkCvtUS(VN_AS(underp, گره_عبارت), خطأ) : underp;
        Ast_گره_DType* ثابت expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_گره_DType* ثابت checkDtp = expDTypep->skip_Ref_مقصد_Enump();
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
    فراغ iterate_Check_تخصیص_بده(Ast_گره* parentp, ثابت حرف* side, Ast_گره* rhsp, Stage stage,
                            Ast_گره_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        إذا (AstConst* constp = VN_CAST(rhsp, Const)) {
            إذا (ثابت AstUnpackArrayDType* ثابت arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                إذا (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    إذا (basicp->عرض() == 8 && constp->رقم().isFromString()) {
                        AstConsArray* newp = جديد AstConsArray{
                            constp->fileline(), lhsDTypep,
                            جديد AstConst{constp->fileline(), AstConst::Widthed_مقدار{}, 8, 0}};
                        لكل (ثابت صحيح aindex : arrayp->declRange().seq_Lo_مقصد_Hi()) {
                            صحيح cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_عدد selected{constp, 8};
                            selected.opSel(constp->رقم(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            إذا (!selected.isFourState()) {
                                إذا (ثابت uint32_t c = selected.مقصد_UInt()) {
                                    newp->valuep(aindex,
                                                 جديد AstConst{constp->fileline(),
                                                              AstConst::Widthed_مقدار{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_تخصیص_بده(parentp, side, rhsp, lhsDTypep);
        ثابت منطقي lhs_جریان = (VN_IS(parentp, گره_تخصیص_بده)
                                && VN_IS(VN_AS(parentp, گره_تخصیص_بده)->lhsp(), گره_جریان));
        rhsp = iterateCheck(parentp, side, rhsp, تخصیص_بده_3, FINAL, lhsDTypep,
                            lhs_جریان ? EXTEND_OFF : EXTEND_LHS, درست, جریان_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (فراغ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_گره_عبارت* convert_Unpacked_ترتیب(Ast_گره_عبارت* exprp, Ast_گره_DType* dtypep) {
        ثابت AstUnpackArrayDType* ثابت arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        إذا (!arrayp || !arrayp->is_معکوس_از(exprp->dtypep())) أرجع exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_پشت(&relinker);
        Ast_گره_عبارت* ثابت newp = جديد Ast_Cvt_Unpack_مقصد_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        أرجع newp;
    }

    ساكن منطقي isLogicalOperator(ثابت Ast_گره* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        أرجع VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    فراغ iterateCheckBool(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, عرض_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_گره_DType* ثابت underVDTypep = underp ? underp->dtypep()->skipRefp() : اشاره_تهی;
        إذا (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_پشت(&linker);
            Ast_گره* ثابت newp
                = جديد AstNeqD{parentp->fileline(), VN_AS(underp, گره_عبارت),
                              جديد AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } وإلا إذا (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_پشت(&linker);
            Ast_گره* ثابت newp
                = جديد AstNeqN{parentp->fileline(), VN_AS(underp, گره_عبارت),
                              جديد AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } وإلا إذا (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_عرض_Reduce(VN_AS(underp, گره_عبارت)), underp);  //تغییریافته
        } وإلا إذا (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(جديد AstConst{parentp->fileline(), AstConst::بیت_False_Erroring{}});
            VL_DO_DANGLING(افزودن_Deletep(underp), underp);
        } وإلا {
            ثابت منطقي bad = عرض_Bad(underp, parentp->پیدا_کن_بیت_DType());
            إذا (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->عرض(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->عرض()
                                             << (underp->عرض() != underp->عرض_Min()
                                                     ? " or " + cvt_مقصد_Str(underp->عرض_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            إذا (Ast_عبارت_Stmt* ثابت عبارت_Stmtp = VN_CAST(underp, عبارت_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_عرض_Reduce(عبارت_Stmtp->resultp());
                عبارت_Stmtp->dtypeFrom(عبارت_Stmtp->resultp());
            } وإلا إذا (underp->عرض() != 1) {
                VL_DO_DANGLING(fix_عرض_Reduce(VN_AS(underp, گره_عبارت)), underp);  //تغییریافته
            }
        }
    }

    ساكن فراغ checkEnum(Ast_گره* nodep, Ast_گره* parentp, ثابت Ast_گره_DType* هدف_DTypep,
                          ثابت Ast_گره_DType* منبع_DTypep, ثابت Ast_گره_عبارت* منبع_Exprp) {
        إذا (ثابت AstEnumDType* ثابت هدف_Enump
            = VN_CAST(هدف_DTypep->skip_Ref_مقصد_Enump(), EnumDType)) {
            ثابت VCastable castable
                = Ast_گره::computeCastable(هدف_Enump, منبع_DTypep, منبع_Exprp);
            إذا (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << هدف_DTypep->prettyDTypeNameQ() << " from "
                                             << منبع_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_بیشتر()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_گره* iterateCheck(Ast_گره* parentp, ثابت حرف* side, Ast_گره* underp, Determ determ,
                          Stage stage, Ast_گره_DType* expDTypep, Extend_قاعده extend_قاعده,
                          منطقي warnOn = درست, جریان_Use جریان_Use_2 = جریان_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ثابت جریان_Use فرزند_جریان_Use
            = جریان_Use_2 != جریان_USE_NONE ? جریان_Use_2 : جریان_Use_For_Determ(determ);
        إذا (rhsAccess.isReadOnly()) {
            إذا (Ast_گره_عبارت* ثابت exprp = VN_CAST(underp, گره_عبارت)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_ترتیب(exprp, expDTypep);
            }
        }
        إذا (VN_IS(underp, گره_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } وإلا إذا (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   عرض_VP{expDTypep, FINAL, فرزند_جریان_Use}.p());
        } وإلا إذا (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   عرض_VP{expDTypep, FINAL, فرزند_جریان_Use}.p());
        } وإلا إذا (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, گره_عبارت));
            underp = userIterateSubtreeReturnEdits(oldp, عرض_VP{SELF, FINAL, فرزند_جریان_Use}.p());
        } وإلا إذا (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, گره_عبارت), درست, expDTypep->عرض());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, عرض_VP{SELF, FINAL, فرزند_جریان_Use}.p());
        } وإلا إذا (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_گره* ثابت oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, گره_عبارت));
            underp = userIterateSubtreeReturnEdits(oldp, عرض_VP{SELF, FINAL, فرزند_جریان_Use}.p());
        } وإلا {
            ثابت Ast_گره_DType* ثابت underDtp = underp->dtypep()->skipRefp();
            إذا (determ != تخصیص_بده_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_گره* ثابت newp
                    = جديد AstConst{underp->fileline(), AstConst::بیت_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(افزودن_Deletep(underp), underp);
                underp = newp;
            }
            ثابت Ast_گره_DType* ثابت expDtp = expDTypep->skipRefp();
            إذا (determ == تخصیص_بده_3 && VN_IS(expDtp, گره_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            إذا (determ == تخصیص_بده_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_تطابق(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ثابت AstBasicDType* ثابت expBasicp = expDTypep->basicp();
            ثابت AstBasicDType* ثابت underBasicp = underp->dtypep()->basicp();
            إذا (expBasicp && underBasicp) {
                إذا (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_کد::ENUMVALUE)
                    && warnOn) {
                    إذا (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, گره_عبارت));
                    }
                    إذا (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, اشاره_تهی);
                    }
                }
                Ast_گره_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                إذا (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, عرض_VP{SELF, FINAL, فرزند_جریان_Use}.p());
                } وإلا إذا (determ == تخصیص_بده_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    إذا (underp->isSigned() != subDTypep->isSigned()
                        || underp->عرض() != subDTypep->عرض()) {
                        subDTypep = parentp->پیدا_کن_بیت_Or_Logic_DType(
                            std::max(subDTypep->عرض(), underp->عرض()),
                            std::max(subDTypep->عرض_Min(), underp->عرض_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, عرض_VP{subDTypep, FINAL, فرزند_جریان_Use}.p());
                } وإلا {
                    underp = userIterateSubtreeReturnEdits(
                        underp, عرض_VP{subDTypep, FINAL, فرزند_جریان_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                عرض_Check_Sized(parentp, side, VN_AS(underp, گره_عبارت), expDTypep, extend_قاعده,
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
                إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->رقم().isNull()) {
                    // '= null' is ok
                } وإلا إذا (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_از_طریق_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } وإلا إذا (expIfaceRefp->iface_از_طریق_Cellp() != underIfaceRefp->iface_از_طریق_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_از_طریق_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_از_طریق_Cellp()->prettyNameQ() << ").");
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
                        underp, عرض_VP{expDTypep, FINAL, فرزند_جریان_Use}.p());
                }
            } وإلا {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, عرض_VP{expDTypep, FINAL, فرزند_جریان_Use}.p());
            }
        }
        أرجع underp;
    }

    فراغ
    عرض_Check_Sized(Ast_گره* parentp, ثابت حرف* side,
                    Ast_گره_عبارت* underp,  // Node to be checked or have typecast added in front of
                    Ast_گره_DType* expDTypep, Extend_قاعده extend_قاعده, منطقي warnOn = درست) {
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
#پایان_اگر
            أرجع;
        } وإلا {
            ثابت صحيح exp_عرض = expDTypep->عرض();
            صحيح exp_عرض_Min = expDTypep->عرض_Min();
            إذا (exp_عرض_Min == 0) exp_عرض_Min = exp_عرض;
            ثابت منطقي bad = عرض_Bad(underp, expDTypep);
            إذا ((bad || underp->عرض() != exp_عرض) && fixAutoExtend(underp /*ref*/, exp_عرض)) {
                underp = اشاره_تهی;  // Changes underp
                أرجع;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            إذا (VN_IS(parentp->backp(), Cast_اندازه)) warnOn = خطأ;
            إذا (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = خطأ;
            إذا (VN_IS(underp, Const) && VN_AS(underp, Const)->رقم().isFromString()
                && exp_عرض > underp->عرض()
                && (((exp_عرض - underp->عرض()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = خطأ;
            }
            إذا ((VN_IS(parentp, جمع_کن_2) && underp->عرض() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->عرض() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = خطأ;
            }
            إذا (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_عرض, underp->عرض(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_عرض
                        << (exp_عرض != exp_عرض_Min ? " or " + cvt_مقصد_Str(exp_عرض_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->عرض()
                        << (underp->عرض() != underp->عرض_Min()
                                ? " or " + cvt_مقصد_Str(underp->عرض_Min())
                                : "")
                        << " bits.");
            }
            إذا (bad || underp->عرض() != exp_عرض) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ثابت Ast_گره_تخصیص_بده* assignp = VN_CAST(parentp, گره_تخصیص_بده);
                ثابت AstPin* pinp = VN_CAST(parentp, Pin);
                إذا (assignp && VN_IS(assignp->lhsp(), گره_جریان)) {
                } وإلا إذا (pinp && pinp->modVarp()->direction() != VDirection::درون_داد) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } وإلا إذا (assignp && VN_IS(underp, گره_جریان) && exp_عرض > underp->عرض()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ثابت صحيح لغزش_2 = exp_عرض - underp->عرض();
                    underp->unlink_Fr_پشت(&linker);
                    AstExtend* ثابت widenedp = جديد AstExtend{underp->fileline(), underp};
                    widenedp->did_عرض(درست);
                    // Shift left so zeros fill on the right
                    Ast_گره_عبارت* ثابت shiftedp = جديد Ast_لغزش_L{
                        underp->fileline(), widenedp,
                        جديد AstConst{underp->fileline(), static_cast<uint32_t>(لغزش_2)}, exp_عرض};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } وإلا {
                    VL_DO_DANGLING(fix_عرض_Extend(underp, expDTypep, extend_قاعده), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_گره_عبارت* checkCvtUS(Ast_گره_عبارت* nodep, منطقي fatal) {
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

    Ast_گره_عبارت* spliceCvtD(Ast_گره_عبارت* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        إذا (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            Ast_گره_عبارت* newp;
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
    Ast_گره_عبارت* spliceCvtS(Ast_گره_عبارت* nodep, منطقي warnOn, صحيح عرض) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                إذا (constp->isDouble() && V3_عدد::epsilonIntegral(constp->رقم().مقصد_Double())) {
                    warnOn = خطأ;
                }
            }
            إذا (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_گره_عبارت* ثابت newp = جديد AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_مجموعه_بیت_Sized(عرض, VSigning::SIGNED);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_گره_عبارت* spliceCvtString(Ast_گره_عبارت* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        إذا (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_پشت(&linker);
            Ast_گره_عبارت* ثابت newp = جديد AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            أرجع newp;
        } وإلا {
            أرجع nodep;
        }
    }
    Ast_گره_Biop* replaceWithUOrSVersion(Ast_گره_Biop* nodep, منطقي signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (signedFlavorNeeded == nodep->signedFlavor()) أرجع اشاره_تهی;
        إذا (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        بدل (nodep->type()) {
        حالة VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::جمع_کن_2: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        حالة VNType::لغزش_L: nodep->dtypeChgSigned(signedFlavorNeeded); أرجع اشاره_تهی;
        افتراضي: اكسر;
        }
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
        Ast_گره_Biop* newp = اشاره_تهی;
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
        حالة VNType::لغزش_R: newp = جديد Ast_لغزش_RS{fl, lhsp, rhsp}; اكسر;
        حالة VNType::لغزش_RS: newp = جديد Ast_لغزش_R{fl, lhsp, rhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گره_Biop* replaceWithDVersion(Ast_گره_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع اشاره_تهی;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
        Ast_گره_Biop* newp = اشاره_تهی;
        // No width change on output;...                // All below have bool or double outputs
        بدل (nodep->type()) {
        حالة VNType::جمع_کن_2: newp = جديد Ast_جمع_کن_D{fl, lhsp, rhsp}; اكسر;
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
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گره_Biop* replaceWithNVersion(Ast_گره_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        إذا (nodep->stringFlavor()) أرجع اشاره_تهی;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
        Ast_گره_Biop* newp = اشاره_تهی;
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
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        أرجع newp;
    }
    Ast_گره_Uniop* replaceWithDVersion(Ast_گره_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        إذا (nodep->doubleFlavor()) أرجع اشاره_تهی;
        File_سطر* ثابت fl = nodep->fileline();
        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp()->unlink_Fr_پشت();
        Ast_گره_Uniop* newp = اشاره_تهی;
        بدل (nodep->type()) {
        حالة VNType::Negate: newp = جديد AstNegateD{fl, lhsp}; اكسر;
        افتراضي:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            اكسر;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
        أرجع newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    فراغ replaceWithSFormat(Ast_روش_Call* nodep, ثابت string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ثابت argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_گره_Var_Ref* ثابت fromp = VN_AS(nodep->fromp()->unlink_Fr_پشت(), VarRef);
        Ast_گره* ثابت newp = جديد Ast_تخصیص_بده{
            nodep->fileline(), fromp,
            جديد AstSFormatF{nodep->fileline(), format, خطأ, argp->exprp()->unlink_Fr_پشت()}};
        fromp->access(VAccess::WRITE);
        افزودن_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_گره_DType* iterateEditMoveDTypep(Ast_گره* parentp, Ast_گره_DType* dtnodep) {
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
        ثابت منطقي child1 = (parentp->get_فرزند_DTypep() == dtnodep);
        ثابت منطقي child2 = (parentp->getChild2DTypep() == dtnodep);
        إذا (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, اشاره_تهی), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_فرزند_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_عرض(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_پشت();
            v3Global.rootp()->typeTablep()->جمع_کن_Typesp(dtnodep);
        }
        إذا (!dtnodep->did_عرض()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, اشاره_تهی);
            UASSERT_OBJ(dtnodep->did_عرض(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        أرجع dtnodep;
    }

    AstConst* dimension_مقدار(File_سطر* fileline, Ast_گره_DType* nodep, VAttrType attrType,
                             صحيح dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_گره_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        لكل (صحيح i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            إذا (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType)) {
                declRange = adtypep->declRange();
                إذا (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                استمر;
            } وإلا إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                       = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                اكسر;  // Sub elements don't look like arrays and can't iterate into
            } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                إذا (adtypep->isRanged()) declRange = adtypep->declRange();
                اكسر;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        AstConst* valp = اشاره_تهی;  // If nullptr, construct from val
        صحيح val = 0;
        بدل (attrType) {
        حالة VAttrType::DIM_بیتها: {
            صحيح بیتها = 1;
            طالما (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                إذا (ثابت Ast_گره_Array_DType* ثابت adtypep = VN_CAST(dtypep, گره_Array_DType)) {
                    بیتها *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    استمر;
                } وإلا إذا (ثابت Ast_گره_UOr_Struct_DType* ثابت adtypep
                           = VN_CAST(dtypep, گره_UOr_Struct_DType)) {
                    بیتها *= adtypep->isStreamableFixedAggregate() ? adtypep->عرض_جریان()
                                                                  : adtypep->عرض();
                    اكسر;
                } وإلا إذا (ثابت AstBasicDType* ثابت adtypep = VN_CAST(dtypep, BasicDType)) {
                    بیتها *= adtypep->عرض();
                    اكسر;
                }
                اكسر;
            }
            إذا (dim == 0) {
                val = 0;
            } وإلا إذا (dim == 1 && !declRange.ranged()
                       && بیتها == 1) {  // $bits should be sane for non-arrays
                val = nodep->عرض();
            } وإلا {
                val = بیتها;
            }
            اكسر;  // LCOV_EXCL_LINE
        }
        حالة VAttrType::DIM_بالا: val = !declRange.ranged() ? 0 : declRange.hi(); اكسر;
        حالة VAttrType::DIM_چپ: val = !declRange.ranged() ? 0 : declRange.چپ(); اكسر;
        حالة VAttrType::DIM_کم: val = !declRange.ranged() ? 0 : declRange.lo(); اكسر;
        حالة VAttrType::DIM_راست: val = !declRange.ranged() ? 0 : declRange.راست(); اكسر;
        حالة VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            اكسر;
        حالة VAttrType::DIM_اندازه: val = !declRange.ranged() ? 0 : declRange.elements(); اكسر;
        افتراضي: nodep->v3fatalSrc("Missing DIM ATTR type case"); اكسر;
        }
        إذا (!valp) valp = جديد AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_مقصد_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        أرجع valp;
    }
    AstVarRef* dimensionVarRefp(Ast_گره_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_جدول_Map[std::make_pair(nodep, attrType)];
        إذا (cachedpr) أرجع cachedpr->clone_درخت(خطأ);

        Ast_گره_Array_DType* ثابت vardtypep = جديد AstUnpackArrayDType{
            nodep->fileline(), nodep->پیدا_کن_Int_DType(), جديد AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(vardtypep);
        userIterate(vardtypep, عرض_VP{SELF, BOTH}.p());
        AstConsArray* ثابت consp = جديد AstConsArray{nodep->fileline(), vardtypep};
        لكل (بیعلامت i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_مقدار(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::پیدا_کن(consp);
        VL_DO_DANGLING(consp->delete_درخت(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_عرض(درست);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_عرض(درست);
        افزودن_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_درخت(خطأ);
    }
    ساكن uint64_t enum_Max_مقدار(ثابت Ast_گره* errNodep, ثابت AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        لكل (ثابت Ast_Enum_قلم* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
            ثابت AstConst* ثابت vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            إذا (!vconstp->رقم().isAnyXZ() && vconstp->مقصد_UQuad() >= maxval)
                maxval = vconstp->مقصد_UQuad();
        }
        إذا (adtypep->itemsp()->عرض() > 64) {
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
        إذا (cachedpr) أرجع cachedpr->clone_درخت(خطأ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_سطر* ثابت flp = nodep->fileline();
        Ast_گره_DType* basep;
        إذا (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->پیدا_کن_String_DType();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->پیدا_کن_بیت_DType();
        } وإلا {
            basep = nodep->dtypep();
        }
        Ast_گره_DType* vardtypep;
        إذا (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = جديد AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } وإلا {
            vardtypep = جديد AstUnpackArrayDType{flp, basep, جديد AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->جمع_کن_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = اشاره_تهی;
        إذا (attrType == VAttrType::ENUM_NAME) {
            defaultp = جديد AstConst{flp, AstConst::String{}, ""};
        } وإلا إذا (attrType == VAttrType::ENUM_بعدی || attrType == VAttrType::ENUM_PREV) {
            defaultp = جديد AstConst{flp, V3_عدد{nodep, nodep->عرض(), 0}};
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            defaultp = جديد AstConst{flp, AstConst::بیت_False{}};
        } وإلا {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_گره_عبارت*> مقادیر;
        {
            uint64_t ordinal = 0;
            Ast_Enum_قلم* ثابت firstp = nodep->itemsp();
            ثابت Ast_Enum_قلم* prevp = firstp;  // Prev must start with last item
            طالما (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_قلم);
            لكل (Ast_Enum_قلم* itemp = firstp; itemp;) {
                Ast_Enum_قلم* ثابت nextp = VN_AS(itemp->nextp(), Enum_قلم);
                ثابت AstConst* ثابت vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                إذا (!vconstp->رقم().isAnyXZ()) {  // Can 2-state runtime decode
                    ثابت uint64_t i = nodep->isWide() ? ++ordinal : vconstp->مقصد_UQuad();
                    إذا (attrType == VAttrType::ENUM_NAME) {
                        مقادیر[i] = جديد AstConst{flp, AstConst::String{},
                                                 V3_عدد::displayedEnumName(itemp)};
                    } وإلا إذا (attrType == VAttrType::ENUM_بعدی) {
                        مقادیر[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_درخت(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_PREV) {
                        مقادیر[i] = prevp->valuep()->clone_درخت(خطأ);  // A const
                    } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
                        مقادیر[i] = جديد AstConst{flp, AstConst::بیت_True{}};
                    } وإلا {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_گره_عبارت* valuep = اشاره_تهی;
        إذا (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ثابت consp = جديد AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            لكل (ثابت خودکار& itr : مقادیر) {
                V3_عدد کلید{nodep, nodep->عرض()};
                کلید.مجموعه_Quad(itr.اولی);
                consp->جمع_کن_Itemsp(جديد Ast_Cons_Assoc_قلم{flp, جديد AstConst{flp, کلید}, itr.دومی});
            }
            valuep = consp;
        } وإلا {
            AstConsArray* ثابت consp = جديد AstConsArray{flp, vardtypep, defaultp};
            لكل (uint64_t i = 0; i < (msbdim + 1); ++i) {
                إذا (مقادیر[i]) consp->valuep(i, مقادیر[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::پیدا_کن(valuep);
        VL_DO_DANGLING(افزودن_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_عرض(درست);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_عرض(درست);
        افزودن_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        أرجع cachedpr->clone_درخت(خطأ);
    }

    Ast_گره_عبارت* enumSelect(Ast_گره_عبارت* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        إذا (attrType == VAttrType::ENUM_NAME) {
            إذا (nodep->is_خالص()) nodep = V3Const::constifyEdit(nodep);
            إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
                Ast_گره_عبارت* ثابت newp = جديد AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->رقم().displayedEnumName(adtypep)};
                VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
                أرجع newp;
            }
        }
        إذا (attrType == VAttrType::ENUM_NAME && !nodep->is_خالص() && (m_modep || m_ftaskp)) {
            Ast_عبارت_Stmt* ثابت capturep = newEnumCapture(nodep, adtypep);
            Ast_گره_عبارت* ثابت valuep = capturep->resultp()->unlink_Fr_پشت();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_مجموعه_String();
            أرجع capturep;
        }
        إذا (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_گره_عبارت* indexp = جديد AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            لكل (ثابت Ast_Enum_قلم* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_قلم)) {
                ثابت AstConst* ثابت constp = VN_AS(itemp->valuep(), Const);
                إذا (constp->رقم().isAnyXZ()) استمر;
                indexp = جديد AstCond{nodep->fileline(),
                                     جديد AstEq{nodep->fileline(), nodep->clone_درخت(خطأ),
                                               itemp->valuep()->clone_درخت(خطأ)},
                                     جديد AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, خطأ, ordinal);
            Ast_گره_عبارت* ثابت newp = جديد AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_مجموعه_String();
            VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
            أرجع newp;
        }
        ثابت uint64_t msbdim = enum_Max_مقدار(nodep, adtypep);
        ثابت منطقي assoc = msbdim > ENUM_LOOKUP_بیتها;
        Ast_گره_عبارت* newp;
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(adtypep, attrType, درست, 0);
            newp = جديد AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } وإلا {
            ثابت صحيح selwidth = V3_عدد::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(adtypep, attrType, خطأ, (1ULL << selwidth) - 1);
            Ast_گره_عبارت* ثابت boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->عرض() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? جديد AstGt{nodep->fileline(), nodep->clone_درخت(خطأ),
                                  جديد AstConst{nodep->fileline(),
                                               V3_عدد{nodep, adtypep->عرض(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : اشاره_تهی;
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
            newp->dtype_مجموعه_String();
        } وإلا إذا (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_مجموعه_بیت();
        } وإلا {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        أرجع newp;
    }
    Ast_گره_عبارت* enumTestValid(Ast_گره_عبارت* valp, AstEnumDType* enumDtp) {
        ثابت uint64_t maxval = enum_Max_مقدار(valp, enumDtp);
        ثابت منطقي assoc = maxval > ENUM_LOOKUP_بیتها;
        Ast_گره_عبارت* testp = اشاره_تهی;
        File_سطر* ثابت fl_novalue = جديد File_سطر{valp->fileline()};
        fl_novalue->warnOff(V3_Error_کد::ENUMVALUE, درست);
        fl_novalue->warnOff(V3_Error_کد::CMPCONST, درست);
        إذا (assoc) {
            AstVarRef* ثابت tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, درست, 0);
            testp = جديد AstAssocSel{fl_novalue, tabRefp, valp->clone_درخت_خالص(خطأ)};
        } وإلا {
            ثابت صحيح selwidth = V3_عدد::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ثابت tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, خطأ, (1ULL << selwidth) - 1);
            File_سطر* ثابت fl_nowidth = جديد File_سطر{fl_novalue};
            fl_nowidth->warnOff(V3_Error_کد::عرض_2, درست);
            testp = جديد AstCond{
                fl_novalue,
                جديد AstGt{fl_nowidth, valp->clone_درخت_خالص(خطأ),
                          جديد AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                جديد AstConst{fl_novalue, AstConst::بیت_False{}},
                جديد AstArraySel{fl_novalue, tabRefp,
                                جديد AstSel{fl_novalue, valp->clone_درخت_خالص(خطأ), 0, selwidth}}};
        }
        أرجع testp;
    }

    PatVecMap pat_وکتور_Map(Ast_الگو* nodep, ثابت VNumRange& range) {
        PatVecMap patmap;
        صحيح element = range.چپ();
        لكل (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            إذا (patp->keyp()) {
                إذا (ثابت AstConst* ثابت constp = الگو_Const_Keyp(patp)) {
                    إذا (constp->رقم().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } وإلا {
                        element = constp->مقصد_SInt();
                    }
                }
            }
            ثابت منطقي new_مدخل = patmap.emplace(element, patp).دومی;
            إذا (!new_مدخل) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.چپ_مقصد_راست_Inc();
        }
        أرجع patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    فراغ patConcatConvertRecurse(Ast_الگو* patternp, Ast_گره_عبارت* nodep) {
        إذا (AstConcat* ثابت concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } وإلا {
            AstPatMember* ثابت newp
                = جديد AstPatMember{nodep->fileline(), nodep->unlink_Fr_پشت(), اشاره_تهی, اشاره_تهی};
            newp->isConcat(درست);
            patternp->جمع_کن_Itemsp(newp);
        }
    }

    فراغ makeOpenArrayShell(Ast_گره_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_گره_FTask* ثابت oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_والد_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_فرزند(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_گره_FTask* ثابت newTaskp = oldTaskp->clone_درخت(خطأ);
        newTaskp->dpi_Open_فرزند(درست);
        newTaskp->dpi_Open_والد_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_مقصد_Str(oldTaskp->dpi_Open_والد()));
        oldTaskp->جمع_کن_بعدی_اینجا(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ثابت V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        لكل (ثابت خودکار& tconnect : tconnects) {
            AstVar* ثابت portp = tconnect.اولی;
            ثابت AstArg* ثابت argp = tconnect.دومی;
            ثابت Ast_گره* ثابت pinp = argp->exprp();
            إذا (!pinp) استمر;  // Argument error we'll find later
            إذا (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_گره_DType* ثابت oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_گره* nodep) {
                    إذا (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    منطقي markHasOpenArray(Ast_گره_FTask* nodep) {
        منطقي hasOpen = خطأ;
        لكل (Ast_گره* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstVar* ثابت portp = VN_CAST(stmtp, Var)) {
                إذا (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(درست);
                    hasOpen = درست;
                }
            }
        }
        أرجع hasOpen;
    }
    منطقي hasOpenArrayDTypeRecurse(Ast_گره_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        إذا (VN_IS(nodep, UnsizedArrayDType)) أرجع درست;
        إذا (nodep->subDTypep()) أرجع hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        أرجع خطأ;
    }
    Ast_گره_DType* dtypeNotIntAtomOrVecRecurse(Ast_گره_DType* nodep, منطقي ranged = خطأ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_مقصد_Enump();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (ranged && (!dtp->is_بیت_Logic() || dtp->isRanged()))
                أرجع dtp;  // Packed when already packed
            إذا (dtp->keyword().isIntNumeric()) أرجع اشاره_تهی;
            أرجع dtp;
        } وإلا إذا (AstPackArrayDType* ثابت dtp = VN_CAST(nodep, PackArrayDType)) {
            إذا (ranged) أرجع dtp;  // Packed when already packed
            أرجع dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), درست);
        }
        أرجع nodep;
    }
    Ast_گره_DType* dtypeNot4StateIntegralRecurse(Ast_گره_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        إذا (AstBasicDType* ثابت dtp = VN_CAST(nodep, BasicDType)) {
            إذا (!dtp->keyword().isFourstate()) أرجع dtp;
            أرجع اشاره_تهی;
        } وإلا إذا (Ast_گره_Array_DType* ثابت dtp = VN_CAST(nodep, گره_Array_DType)) {
            أرجع dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } وإلا إذا (Ast_گره_UOr_Struct_DType* ثابت dtp = VN_CAST(nodep, گره_UOr_Struct_DType)) {
            لكل (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_گره_DType* ثابت badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                إذا (badDtp) أرجع badDtp;
            }
            أرجع اشاره_تهی;
        }
        أرجع nodep;
    }
    فراغ checkForceReleaseLhs(Ast_گره* nodep, Ast_گره* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        إذا (Ast_گره* ثابت selNodep = V3_عرض::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    فراغ assert_At_عبارت(Ast_گره* nodep) {
        إذا (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    فراغ assertAtStatement(Ast_گره* nodep) {
        إذا (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    فراغ checkConstantOrReplace(Ast_گره* nodep, منطقي noFourState, ثابت string& پیام) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ثابت constp = VN_CAST(nodep, Const);
        إذا (!constp || (noFourState && constp->رقم().isFourState())) {
            nodep->v3error(پیام);
            nodep->replaceWith(جديد AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(افزودن_Deletep(nodep), nodep, constp);
            أرجع;
        }
    }
    Ast_گره* گره_For_Unsized_Warning(Ast_گره* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        إذا (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->عرض_Sized()) {
            أرجع nodep->op1p();
        } وإلا إذا (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->عرض_Sized()) {
            أرجع nodep->op2p();
        }
        أرجع nodep;  // By default return this
    }
    AstRefDType* check_Ref_مقصد_Typedef_Recurse(Ast_گره* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        إذا (VL_LIKELY(!nodep)) أرجع اشاره_تهی;
        إذا (خودکار* ثابت refp = VN_CAST(nodep, RefDType)) {
            إذا (refp->typedefp() == typedefp) أرجع refp;
        }
        إذا (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op1p(), typedefp)) أرجع refp;
        إذا (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op2p(), typedefp)) أرجع refp;
        إذا (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op3p(), typedefp)) أرجع refp;
        إذا (خودکار* ثابت refp = check_Ref_مقصد_Typedef_Recurse(nodep->op4p(), typedefp)) أرجع refp;
        أرجع اشاره_تهی;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ساكن منطقي isIfaceArrayDType(ثابت Ast_گره_DType* dtypep) {
        إذا (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) أرجع خطأ;
        ثابت AstIfaceRefDType* ثابت irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        أرجع irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ساكن std::string unpacked_Array_اندازه(ثابت Ast_گره_DType* dtypep) {
        std::string اندازه;
        dtypep = dtypep->skipRefp();
        طالما (ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            إذا (!اندازه.خالی()) اندازه += "x";
            اندازه += std::مقصد_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        أرجع اندازه;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ساكن Ast_گره_عبارت* newIfaceArrayElement(Ast_گره_عبارت* fromp, صحيح بیت_2) {
        AstConsArray* ثابت consp = VN_CAST(fromp, ConsArray);
        إذا (!consp || !isIfaceArrayDType(consp->dtypep())) أرجع اشاره_تهی;
        ثابت AstUnpackArrayDType* ثابت arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        إذا (بیت_2 < 0 || بیت_2 >= arrp->elementsConst()) أرجع اشاره_تهی;
        أرجع consp->valuep(بیت_2)->clone_درخت(خطأ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_گره_عبارت* new_Iface_Array_مقدار(ثابت Ast_گره_Var_Ref* refp, Ast_گره_DType* dtypep,
                                    ثابت std::string& name, ثابت std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_سطر* ثابت flp = refp->fileline();
        AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        إذا (!arrp) {
            AstVar* ثابت varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_گره_Var_Ref* newp;
            إذا (ثابت AstVarXRef* ثابت xrefp = VN_CAST(refp, VarXRef)) {
                newp = جديد AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } وإلا {
                newp = جديد AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, عرض_VP{SELF, BOTH}.p());
            أرجع newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ثابت consp = جديد AstConsArray{flp, arrp};
        ثابت VNumRange range = arrp->declRange();
        لكل (ثابت صحيح i : range.seq_چپ_مقصد_راست()) {
            ثابت std::string s = name + "__BRA__" + Ast_گره::encode_عدد(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_مقدار(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        أرجع consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    فراغ replace_تقسیم_Iface_Array_Ref(Ast_گره_Var_Ref* nodep) {
        AstVar* ثابت varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_تقسیم(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        إذا (varp->is_Iface_والد()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_مقدار(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(افزودن_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    فراغ expandIfaceArrayPinDimensions(AstPin* nodep, ثابت Ast_گره_DType* dtypep,
                                       Ast_گره_عبارت* conp, ثابت std::string& suffix,
                                       AstVar*& elemVarpr) {
        ثابت AstUnpackArrayDType* ثابت arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        إذا (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ثابت newp = nodep->clone_درخت(خطأ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            إذا (conp) newp->exprp(conp->clone_درخت(خطأ));
            nodep->جمع_کن_بعدی_اینجا(newp);
            أرجع;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ثابت VNumRange range = arrp->declRange();
        لكل (صحيح n = range.elements() - 1; n >= 0; --n) {
            ثابت صحيح i = range.چپ() + n * range.چپ_مقصد_راست_Inc();
            ثابت std::string s = suffix + "__BRA__" + Ast_گره::encode_عدد(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_گره_عبارت* ثابت elemConnp = [&]() -> Ast_گره_عبارت* {
                إذا (!conp) أرجع اشاره_تهی;
                ثابت AstUnpackArrayDType* ثابت conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ثابت صحيح elements = conArrp->elementsConst();
                // Counted from lo
                Ast_گره_عبارت* ثابت resultp
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
        UASSERT_OBJ(portp->is_Iface_Array_تقسیم(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        لكل (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_گره_عبارت* ثابت exprp = VN_AS(nodep->exprp(), گره_عبارت);
        إذا (exprp) افزودن_Deletep(exprp->unlink_Fr_پشت());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ثابت منطقي connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_اندازه(exprp->dtypep()) == unpacked_Array_اندازه(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : اشاره_تهی, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(افزودن_Deletep(nodep->unlink_Fr_پشت()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_گره* userIterateSubtreeReturnEdits(Ast_گره* nodep, عرض_VP* vup) {
        إذا (!nodep) أرجع اشاره_تهی;
        Ast_گره* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        أرجع ret;
    }
    فراغ userIterate(Ast_گره* nodep, عرض_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    فراغ user_Iterate_And_بعدی(Ast_گره* nodep, عرض_VP* vup) {
        إذا (!nodep) أرجع;
        إذا (nodep->did_عرض()) أرجع;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_بعدی_Null(nodep);
    }
    فراغ userIterateChildren(Ast_گره* nodep, عرض_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    فراغ userIterateChildrenBackwardsConst(Ast_گره* nodep, عرض_VP* vup) {
        إذا (!nodep) أرجع;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

عام:
    // CONSTRUCTORS
    عرض_Visitor(منطقي paramsOnly,  // [in] TRUE if we are considering parameters only.
                 منطقي doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_موقت_نامها{"__Venum"}
        , m_inside_موقت_نامها{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_گره* mainAcceptEdit(Ast_گره* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, عرض_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        لكل (AstVar* ثابت varp : m_تقسیم_Iface_Array_Varps) {
            VL_DO_DANGLING(افزودن_Deletep(varp->unlink_Fr_پشت()), varp);
        }
        m_تقسیم_Iface_Array_Varps.clear();
        أرجع nodep;
    }
    ساكن منطقي پایینی_As_Fixed_Aggregate(ثابت Ast_گره_DType* ثابت dtypep) {
        أرجع dtypep->isStreamableFixedAggregate() && dtypep->شامل_است_Unpacked_Struct();
    }
    فراغ جریان_تخصیص_بده_پایینی(Ast_گره_تخصیص_بده* nodep) {
        Ast_گره_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        إذا (Ast_گره_جریان* ثابت streamp = VN_CAST(nodep->rhsp(), گره_جریان)) {
            Ast_گره_DType* ثابت lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ثابت صحيح lwidth = lhsDTypeSkippedRefp->عرض_جریان();
            ثابت صحيح rwidth = streamp->lhsp()->dtypep()->skipRefp()->عرض_جریان();
            إذا (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            إذا (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), صف_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), صف_DType)) {
                ثابت صحيح صف_Element_اندازه = streamp->lhsp()->dtypep()->subDTypep()->عرض();
                UASSERT_OBJ(صف_Element_اندازه <= lwidth, nodep, "LHS < RHS");
            }
            إذا (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || پایینی_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_پشت();
                nodep->rhsp(
                    جديد Ast_Cvt_Packed_مقصد_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        إذا (Ast_گره_جریان* ثابت streamp = VN_CAST(nodep->lhsp(), گره_جریان)) {
            ثابت Ast_گره_DType* ثابت rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_گره_DType* ثابت lhs_جریان_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ثابت صحيح lwidth = lhs_جریان_DTypep->عرض_جریان();
            ثابت صحيح rwidth = rhsDTypep->عرض_جریان();
            إذا (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            إذا (پایینی_As_Fixed_Aggregate(lhs_جریان_DTypep)) {
                Ast_گره_عبارت* ثابت جریان_Exprp = nodep->lhsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* ثابت dstp = streamp->lhsp()->unlink_Fr_پشت();
                Ast_گره_عبارت* srcp = nodep->rhsp()->unlink_Fr_پشت();
                إذا (VN_IS(streamp, جریان_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_مجموعه_بیت_Or_Logic_Unsized(
                        srcp->عرض(), srcp->عرض_Min(), VSigning::UNSIGNED,
                        Ast_گره_عبارت::isFourstateOrNoDtype(جریان_Exprp, srcp));
                    srcp = جریان_Exprp;
                } وإلا {
                    إذا (srcp->عرض() > lwidth) {
                        srcp = جديد AstSel{streamp->fileline(), srcp, srcp->عرض() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(افزودن_Deletep(جریان_Exprp), جریان_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(جديد Ast_Cvt_Packed_مقصد_Array{srcp->fileline(), srcp, lhs_جریان_DTypep});
                nodep->dtypeFrom(dstp);
            } وإلا إذا (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_گره_عبارت* ثابت rhsp = nodep->rhsp()->unlink_Fr_پشت();
                nodep->rhsp(جديد Ast_Cvt_Array_مقصد_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~عرض_Visitor() override = افتراضي;
};

//######################################################################
// Width class functions

فراغ V3_عرض::عرض(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ثابت عرض_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ثابت VDTypeNameScopes scopes{nodep};
        عرض_Visitor visitor{خطأ, خطأ};
        (فراغ)visitor.mainAcceptEdit(nodep);
        عرض_Remove_Visitor rvisitor;
        (فراغ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("width", 0, dump_درخت_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_گره* V3_عرض::عرض_Params_Edit(Ast_گره* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    عرض_Visitor visitor{درست, خطأ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

فراغ V3_عرض::جریان_تخصیص_بده_پایینی_Edit(Ast_گره_تخصیص_بده* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    عرض_Visitor visitor{خطأ, خطأ};
    visitor.جریان_تخصیص_بده_پایینی(nodep);
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
Ast_گره* V3_عرض::عرض_Generate_Params_Edit(
    Ast_گره* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    عرض_Visitor visitor{درست, درست};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    أرجع nodep;
}

Ast_گره* V3_عرض::selectNonConstantRecurse(Ast_گره* nodep, منطقي درون_Sel) {
    // If node has a non-constant select, return that select
    Ast_گره* resultp = اشاره_تهی;
    إذا (Ast_گره_Sel* ثابت anodep = VN_CAST(nodep, گره_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), درون_Sel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), درست);
    } وإلا إذا (AstSel* ثابت anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), درون_Sel);
        إذا (resultp) أرجع resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), درست);
    } وإلا إذا (Ast_گره_Var_Ref* ثابت anodep = VN_CAST(nodep, گره_Var_Ref)) {
        إذا (درون_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) أرجع anodep;
    } وإلا {
        إذا (Ast_گره* ثابت refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, درون_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گره* ثابت refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, درون_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گره* ثابت refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, درون_Sel);
        إذا (resultp) أرجع resultp;
        إذا (Ast_گره* ثابت refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, درون_Sel);
    }
    أرجع resultp;
}
