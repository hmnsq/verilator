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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3Width.h"

#अन्तर्भाव "V3Assert.h"
#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Begin.h"
#अन्तर्भाव "V3Const.h"
#अन्तर्भाव "V3ConstPool.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3LinkLValue.h"
#अन्तर्भाव "V3MemberMap.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3Randomize.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3Task.h"
#अन्तर्भाव "V3UniqueNames.h"
#अन्तर्भाव "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#परिभाषय VERILATOR_V3_विस्तारः_CPP
#अन्तर्भाव "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

वर्ग ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ध्रुव Ast_ग्रन्थिः*, AstClass*> m_cache;

    AstClass* findCached(Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम शून्यसूचकः;
        ध्रुव स्वचालित it = m_cache.find(nodep);
        यदि (it != m_cache.अन्तः_2()) प्रत्यागम it->second;

        AstClass* classp = शून्यसूचकः;
        यदि (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = findCached(nodep->backp());
        } अन्यथा यदि (AstClass* ध्रुव parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } अन्यथा यदि (AstClassPackage* ध्रुव packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } अन्यथा {
            classp = findCached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        प्रत्यागम classp;
    }

सार्वजनिक:
    AstClass* find(Ast_ग्रन्थिः* nodep) {
        यदि (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) प्रत्यागम classp;
        यदि (AstClassPackage* ध्रुव packagep = VN_CAST(nodep, ClassPackage)) {
            प्रत्यागम packagep->classp();
        }
        प्रत्यागम findCached(nodep);
    }
};

//######################################################################

गणना Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& संक्रियकः<<(std::ostream& str, ध्रुव Stage& rhs) {
    प्रत्यागम str << ("-PFB"[static_cast<पूर्णाङ्क>(rhs)]);
}

गणना Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    नियोजय_3  // Assignment-like where sign comes from RHS only
};
std::ostream& संक्रियकः<<(std::ostream& str, ध्रुव Determ& rhs) {
    स्थिर constexpr ध्रुव वर्ण* ध्रुव s_det[] = {"SELF", "CNTX", "ASSN"};
    प्रत्यागम str << s_det[rhs];
}

गणना प्रवाहः_Use : uint8_t {
    प्रवाहः_USE_NONE,  // Stream concatenation is not allowed in this context
    प्रवाहः_USE_नियोजय,  // Assignment-like context permits stream concatenation
    प्रवाहः_USE_CAST,  // Explicit cast context permits stream concatenation
};

#परिभाषय v3widthWarn(lhs, rhs, सन्देशः_2) \
    v3warn_संहिता(((lhs) < (rhs)   ? V3_Error_संहिता::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_संहिता::WIDTHEXPAND \
                                : V3_Error_संहिता::विस्तारः_2), \
               सन्देशः_2)

//######################################################################
// Width state, as a visitor of each AstNode

वर्ग विस्तारः_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ग्रन्थिः_DType* ध्रुव m_dtypep;  // Parent's data type to resolve to
    ध्रुव Stage m_stage;  // If true, report errors
    ध्रुव प्रवाहः_Use m_प्रवाहः_Use;  // Current expression may be a stream concat

सार्वजनिक:
    विस्तारः_VP(Ast_ग्रन्थिः_DType* dtypep, Stage stage, प्रवाहः_Use प्रवाहः_Use_2 = प्रवाहः_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_प्रवाहः_Use{प्रवाहः_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    विस्तारः_VP(Determ determ, Stage stage, प्रवाहः_Use प्रवाहः_Use_2 = प्रवाहः_USE_NONE)
        : m_dtypep{शून्यसूचकः}
        , m_stage{stage}
        , m_प्रवाहः_Use{प्रवाहः_Use_2} {
        यदि (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    विस्तारः_VP* p() { प्रत्यागम एतत्; }
    तार्किक selfDtm() ध्रुव { प्रत्यागम m_dtypep == शून्यसूचकः; }
    प्रवाहः_Use प्रवाहः_Use_2() ध्रुव { प्रत्यागम m_प्रवाहः_Use; }
    Ast_ग्रन्थिः_DType* dtypep() ध्रुव {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        प्रत्यागम m_dtypep;
    }
    Ast_ग्रन्थिः_DType* dtypeNullp() ध्रुव { प्रत्यागम m_dtypep; }
    Ast_ग्रन्थिः_DType* dtypeNullSkipRefp() ध्रुव {
        Ast_ग्रन्थिः_DType* dtp = dtypeNullp();
        यदि (dtp) dtp = dtp->skipRefp();
        प्रत्यागम dtp;
    }
    Ast_ग्रन्थिः_DType* dtypeOverridep(Ast_ग्रन्थिः_DType* ध्रुव defaultp) ध्रुव {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        यदि (!m_dtypep) प्रत्यागम defaultp;
        यदि (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            प्रत्यागम defaultp->findBitOrLogicDType(m_dtypep->विस्तारः(), m_dtypep->विस्तारः_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        प्रत्यागम m_dtypep;
    }
    पूर्णाङ्क विस्तारः() ध्रुव {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        प्रत्यागम m_dtypep->विस्तारः();
    }
    पूर्णाङ्क विस्तारः_Min() ध्रुव {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        प्रत्यागम m_dtypep->विस्तारः_Min();
    }
    तार्किक prelim() ध्रुव { प्रत्यागम m_stage & PRELIM; }
    तार्किक final() ध्रुव { प्रत्यागम m_stage & FINAL; }
    शून्य dump(std::ostream& str) ध्रुव {
        यदि (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } अन्यथा {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_गन्तव्यम्_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& संक्रियकः<<(std::ostream& str, ध्रुव विस्तारः_VP* vup) {
    यदि (vup) vup->dump(str);
    प्रत्यागम str;
}

//######################################################################

वर्ग विस्तारः_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    शून्य clear_विस्तारः_Recurse(Ast_ग्रन्थिः* nodep) {
        प्रति (; nodep; nodep = nodep->nextp()) {
            nodep->did_विस्तारः(असत्य);
            यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) clear_विस्तारः_Recurse(refp);
            यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) clear_विस्तारः_Recurse(refp);
            यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) clear_विस्तारः_Recurse(refp);
            यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) clear_विस्तारः_Recurse(refp);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट विस्तारः_Clear_Visitor(AstNetlist* nodep) { clear_विस्तारः_Recurse(nodep); }
    आभास ~विस्तारः_Clear_Visitor() = स्वभाव;
};

//######################################################################

#परिभाषय accept अन्तर्_विस्तारः_Visitor_use_Ast_ग्रन्थिः_iterate_instead_सम्बन्धि_Ast_ग्रन्थिः_accept

//######################################################################

वर्ग विस्तारः_Visitor final : सार्वजनिक VNVisitor {
    // TYPES
    उपयुज्य PatVecMap = std::map<पूर्णाङ्क, AstPatMember*>;
    उपयुज्य DTypeMap = std::map<ध्रुव std::string, AstPatMember*>;

    // STATE
    V3_Unique_नामानि m_enum_अस्थायी_नामानि;  // Captured enum expressions
    V3_Unique_नामानि m_inside_अस्थायी_नामानि;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ध्रुव AstModport*, std::unordered_समुच्चयः<string>>
        m_modport_नामानि;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    विस्तारः_VP* m_vup = शून्यसूचकः;  // Current node state
    तार्किक m_underFork = असत्य;  // Visiting under a fork
    तार्किक m_underSExpr = असत्य;  // Visiting under a sequence expression
    तार्किक m_underPackedArray = असत्य;  // Visiting under a AstPackArrayDType
    तार्किक m_underMemberSel = असत्य;  // Viting under a MemberSel
    Ast_ग्रन्थिः* m_seqUnsupp = शून्यसूचकः;  // Property has unsupported node
    तार्किक m_hasSExpr = असत्य;  // Property has a sequence expression
    ध्रुव AstCell* m_cellp = शून्यसूचकः;  // Current cell for arrayed instantiations
    std::सदिशम्<AstVar*> m_विभज_Iface_Array_Varps;  // Interface arrays split by V3Param
    ध्रुव AstEnumItem* m_enumItemp = शून्यसूचकः;  // Current enum item
    Ast_ग्रन्थिः_FTask* m_ftaskp = शून्यसूचकः;  // Current function/task
    Ast_ग्रन्थिः_Module* m_modep = शून्यसूचकः;  // Current module
    ध्रुव AstConstraint* m_constraintp = शून्यसूचकः;  // Current constraint
    Ast_ग्रन्थिः_Procedure* m_procedurep = शून्यसूचकः;  // Current final/always
    ध्रुव AstWith* m_withp = शून्यसूचकः;  // Current 'with' statement
    ध्रुव AstFunc* m_funcp = शून्यसूचकः;  // Current function
    ध्रुव Ast_Attr_सम्बन्धि* m_attrp = शून्यसूचकः;  // Current attribute
    ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* m_randomizeFromp = शून्यसूचकः;  // Current randomize method call fromp
    ध्रुव तार्किक m_paramsOnly;  // Computing parameter value; limit operation
    ध्रुव तार्किक m_doGenerate;  // Do errors later inside generate statement
    तार्किक m_प्रवाहः_Concat = असत्य;  // True if visiting arguments of stream concatenation
    तार्किक m_अन्तर्_Param_Override = असत्य;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ध्रुव Ast_ग्रन्थिः_DType*, VAttrType>, AstVarRef*> m_सारणी_Map;
    // Queues with given index type
    std::map<ध्रुव Ast_ग्रन्थिः_DType*, AstQueueDType*> m_queue_DType_अनुक्रमितम्;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_समुच्चयः<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_समुच्चयः<ध्रुव AstVar*> m_curModVars;  // Variables declared in current module

    स्थिर constexpr पूर्णाङ्क ENUM_LOOKUP_द्वयङ्काः = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    गणना Extend_नियमः : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    गणना वर्ग ComparisonType : uint8_t {
        प्रसामान्यम्_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        प्रसामान्यम्_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    स्थिर प्रवाहः_Use प्रवाहः_Use_For_Determ(Determ determ) {
        प्रत्यागम determ == नियोजय_3 ? प्रवाहः_USE_नियोजय : प्रवाहः_USE_NONE;
    }

    प्रवाहः_Use current_प्रवाहः_Use() ध्रुव { प्रत्यागम m_vup ? m_vup->प्रवाहः_Use_2() : प्रवाहः_USE_NONE; }

    स्थिर शून्य packIfUnpacked(Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nodep) {
        यदि (AstUnpackArrayDType* ध्रुव unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ध्रुव पूर्णाङ्क elements_अङ्कः = unpackDTypep->arrayUnpackedElements();
            ध्रुव पूर्णाङ्क unpack_Min_द्वयङ्काः = elements_अङ्कः * unpackDTypep->subDTypep()->विस्तारः_Min();
            ध्रुव पूर्णाङ्क unpack_द्वयङ्काः = elements_अङ्कः * unpackDTypep->subDTypep()->विस्तारः();
            VNRelinker relinker;
            nodep->unlink_Fr_पृष्ठभागः(&relinker);
            relinker.relink(नूतन Ast_Cvt_Array_गन्तव्यम्_Packed{
                nodep->fileline(), nodep,
                nodep->findBitOrLogicDType(unpack_द्वयङ्काः, unpack_Min_द्वयङ्काः, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    स्थिर Ast_ग्रन्थिः_DType* fromDTypep(Ast_ग्रन्थिः* fromp) {
        यदि (Ast_ग्रन्थिः_DType* ध्रुव dtypep = VN_CAST(fromp, ग्रन्थिः_DType))
            प्रत्यागम dtypep->skipRefOrNullp();
        प्रत्यागम fromp ? fromp->dtypep() : शून्यसूचकः;
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
    शून्य दर्शय(AstLogNot* nodep) override { दर्शय_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    शून्य दर्शय(AstLogAnd* nodep) override { दर्शय_log_and_or(nodep); }
    शून्य दर्शय(AstLogOr* nodep) override { दर्शय_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    शून्य दर्शय(AstSAnd* nodep) override { दर्शय_log_and_or(nodep); }
    शून्य दर्शय(AstSIntersect* nodep) override { दर्शय_log_and_or(nodep); }
    शून्य दर्शय(AstSOr* nodep) override { दर्शय_log_and_or(nodep); }
    शून्य दर्शय(AstSWithin* nodep) override { दर्शय_log_and_or(nodep); }
    शून्य दर्शय(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        दर्शय_log_and_or(nodep);
    }
    शून्य दर्शय(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        दर्शय_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    शून्य दर्शय(AstRedAnd* nodep) override { दर्शय_red_and_or(nodep); }
    शून्य दर्शय(AstRedOr* nodep) override { दर्शय_red_and_or(nodep); }
    शून्य दर्शय(AstRedXor* nodep) override { दर्शय_red_and_or(nodep); }
    शून्य दर्शय(AstOneHot* nodep) override { दर्शय_red_and_or(nodep); }
    शून्य दर्शय(AstOneHot0* nodep) override { दर्शय_red_and_or(nodep); }
    शून्य दर्शय(AstIsUnknown* nodep) override {
        दर्शय_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    शून्य दर्शय(AstEq* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_EQ); }
    शून्य दर्शय(AstNeq* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_EQ); }
    शून्य दर्शय(AstGt* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstGte* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstLt* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstLte* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstGtS* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstGteS* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstLtS* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstLteS* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::प्रसामान्यम्_GL); }
    शून्य दर्शय(AstEqCase* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::CASE); }
    शून्य दर्शय(AstNeqCase* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    शून्य दर्शय(AstEqWild* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::WILD); }
    शून्य दर्शय(AstNeqWild* nodep) override { दर्शय_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    शून्य दर्शय(AstEqD* nodep) override { दर्शय_cmp_real(nodep); }
    शून्य दर्शय(AstNeqD* nodep) override { दर्शय_cmp_real(nodep); }
    शून्य दर्शय(AstLtD* nodep) override { दर्शय_cmp_real(nodep); }
    शून्य दर्शय(AstLteD* nodep) override { दर्शय_cmp_real(nodep); }
    शून्य दर्शय(AstGtD* nodep) override { दर्शय_cmp_real(nodep); }
    शून्य दर्शय(AstGteD* nodep) override { दर्शय_cmp_real(nodep); }
    // ...    String compares
    शून्य दर्शय(AstEqN* nodep) override { दर्शय_cmp_string(nodep); }
    शून्य दर्शय(AstNeqN* nodep) override { दर्शय_cmp_string(nodep); }
    शून्य दर्शय(AstLtN* nodep) override { दर्शय_cmp_string(nodep); }
    शून्य दर्शय(AstLteN* nodep) override { दर्शय_cmp_string(nodep); }
    शून्य दर्शय(AstGtN* nodep) override { दर्शय_cmp_string(nodep); }
    शून्य दर्शय(AstGteN* nodep) override { दर्शय_cmp_string(nodep); }
    // ...    Data type compares
    शून्य दर्शय(AstEqT* nodep) override { दर्शय_cmp_type(nodep); }
    शून्य दर्शय(AstNeqT* nodep) override { दर्शय_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    शून्य दर्शय(AstAnd* nodep) override { दर्शय_boolexpr_and_or(nodep); }
    शून्य दर्शय(AstOr* nodep) override { दर्शय_boolexpr_and_or(nodep); }
    शून्य दर्शय(AstXor* nodep) override { दर्शय_boolexpr_and_or(nodep); }
    शून्य दर्शय(AstBufIf1* nodep) override {
        दर्शय_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    शून्य दर्शय(Ast_सङ्कलय* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, सत्य, असत्य); }
    शून्य दर्शय(AstSub* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, सत्य, असत्य); }
    शून्य दर्शय(AstDiv* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, सत्य, सत्य); }
    शून्य दर्शय(AstMul* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, सत्य, असत्य); }
    // These can't promote to real
    शून्य दर्शय(AstModDiv* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, असत्य, सत्य); }
    शून्य दर्शय(AstModDivS* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, असत्य, सत्य); }
    शून्य दर्शय(AstMulS* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, असत्य, असत्य); }
    शून्य दर्शय(AstDivS* nodep) override { दर्शय_सङ्कलय_sub_replace(nodep, असत्य, सत्य); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    शून्य दर्शय(AstNegate* nodep) override { दर्शय_negate_not(nodep, सत्य); }
    // Unary never real
    शून्य दर्शय(AstNot* nodep) override { दर्शय_negate_not(nodep, असत्य); }

    // Real: inputs and output real
    शून्य दर्शय(Ast_सङ्कलय_D* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    शून्य दर्शय(AstSubD* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    शून्य दर्शय(AstDivD* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    शून्य दर्शय(AstMulD* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    शून्य दर्शय(AstPowD* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_System_Biop_D* nodep) override { दर्शय_real_सङ्कलय_sub(nodep); }
    // Real: Output real
    शून्य दर्शय(AstNegateD* nodep) override { दर्शय_real_neg_ceil(nodep); }
    शून्य दर्शय(Ast_ग्रन्थिः_System_Uniop_D* nodep) override { दर्शय_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    शून्य दर्शय(AstSigned* nodep) override { दर्शय_signed_unsigned(nodep, VSigning::SIGNED); }
    शून्य दर्शय(AstUnsigned* nodep) override { दर्शय_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    शून्य दर्शय(Ast_सरणम्_L* nodep) override { दर्शय_सरणम्_2(nodep); }
    शून्य दर्शय(Ast_सरणम्_R* nodep) override { दर्शय_सरणम्_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    शून्य दर्शय(Ast_सरणम्_RS* nodep) override { दर्शय_सरणम्_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    शून्य दर्शय(Ast_द्वयङ्काः_गन्तव्यम्_Real_D* nodep) override { दर्शय_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    शून्य दर्शय(AstRToIS* nodep) override { दर्शय_Os32_Lr(nodep); }
    शून्य दर्शय(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->विस्तारः_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    शून्य दर्शय(Ast_Real_गन्तव्यम्_द्वयङ्काः* nodep) override { दर्शय_Ou64_Lr(nodep); }

    // Output integer, input string
    शून्य दर्शय(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        यदि (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_Int();
        }
    }
    शून्य दर्शय(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        यदि (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_समुच्चयः_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    शून्य दर्शय(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        यदि (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit_Sized(8, VSigning::UNSIGNED);
        }
    }
    शून्य दर्शय(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        यदि (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit_Sized(8, VSigning::UNSIGNED);
        }
    }
    शून्य दर्शय(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        यदि (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_समुच्चयः_String();
        }
    }
    शून्य दर्शय(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Int();
        }
    }
    शून्य दर्शय(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            यदि (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_समुच्चयः_Double();
            } अन्यथा {
                nodep->dtype_समुच्चयः_Integer();
            }
        }
    }
    शून्य दर्शय(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    शून्य दर्शय(AstTime* nodep) override { nodep->dtype_समुच्चयः_Time2_State(); }
    शून्य दर्शय(AstTimeD* nodep) override { nodep->dtype_समुच्चयः_Double(); }
    शून्य दर्शय(AstTimePrecision* nodep) override { nodep->dtype_समुच्चयः_Integer2_State(); }
    शून्य दर्शय(Ast_Get_Initial_Random_बीजम्* nodep) override { nodep->dtype_समुच्चयः_Int(); }
    शून्य दर्शय(AstTimeUnit* nodep) override {
        nodep->replaceWith(
            नूतन AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_सम्बन्धि_Ten()});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstScopeName* nodep) override {
        nodep->dtype_समुच्चयः_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    स्थिर तार्किक is_Override_आदर्शः(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (ध्रुव Ast_आदर्शः* ध्रुव patternp = VN_CAST(nodep, आदर्शः)) {
            प्रत्यागम !patternp->childDTypep();
        }
        ध्रुव AstCond* ध्रुव condp = VN_CAST(nodep, Cond);
        प्रत्यागम condp && is_Override_आदर्शः(condp->thenp()) && is_Override_आदर्शः(condp->elsep());
    }
    शून्य दर्शय(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_अन्तर्_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ध्रुव पूर्णाङ्क errors = V3Error::error_गणनम्();
            {
                VL_RESTORER(m_अन्तर्_Param_Override);
                m_अन्तर्_Param_Override = असत्य;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->condp(), Const);
            ध्रुव तार्किक ज्ञातम् = constp && !constp->अङ्कः().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            यदि (!ज्ञातम् && V3Error::error_गणनम्() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव keepp
                = (ज्ञातम् && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_पृष्ठभागः();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(keepp, विस्तारः_VP{SELF, BOTH}.p());
            प्रत्यागम;
        }
        यदि (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_अग्र(nodep->thenp(), विस्तारः_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_अग्र(nodep->elsep(), विस्तारः_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_ग्रन्थिः_DType* ध्रुव thenDTypep = nodep->thenp()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव elseDTypep = nodep->elsep()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव elseDTypeSkippedp = elseDTypep->skipRefp();
            यदि (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->findNullDType());
            } अन्यथा यदि (nodep->thenp()->is_Class_Handle_मानम्()
                       || nodep->elsep()->is_Class_Handle_मानम्()) {
                Ast_ग्रन्थिः_DType* commonClassTypep = शून्यसूचकः;
                यदि (nodep->thenp()->is_Class_Handle_मानम्() && nodep->elsep()->is_Class_Handle_मानम्()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ग्रन्थिः::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                यदि (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } अन्यथा {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } अन्यथा यदि (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                यदि (Ast_ग्रन्थिः::computeCastable(elseDTypep, thenDTypep, शून्यसूचकः).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } अन्यथा {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    यदि (!Ast_ग्रन्थिः::computeCastable(thenDTypep, elseDTypep, शून्यसूचकः)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } अन्यथा यदि (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_समुच्चयः_Double();
            } अन्यथा यदि (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_समुच्चयः_String();
            } अन्यथा {
                ध्रुव तार्किक is_Any_परिणाम_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                तार्किक handled = असत्य;
                ध्रुव तार्किक isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ग्रन्थिः_DType* ध्रुव thenEnumDTypep = thenDTypep->skip_Ref_गन्तव्यम्_Enump();
                यदि (VN_IS(thenDTypeSkippedp, ग्रन्थिः_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_वृक्षः(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = सत्य;
                } अन्यथा यदि (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_वृक्षः(elseDTypep->skip_Ref_गन्तव्यम्_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    यदि (is_Any_परिणाम_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = सत्य;
                    } अन्यथा {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(नूतन File_पङ्क्तिः{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_संहिता::ENUMVALUE, सत्य);
                    }
                }
                यदि (!handled) {
                    ध्रुव पूर्णाङ्क विस्तारः = std::max(thenDTypep->विस्तारः(), elseDTypep->विस्तारः());
                    ध्रुव पूर्णाङ्क विस्तारः_Min = std::max(thenDTypep->विस्तारः_Min(), elseDTypep->विस्तारः_Min());
                    ध्रुव VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ध्रुव तार्किक isFourstate = is_Any_परिणाम_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    यदि ((विस्तारः == elseDTypeSkippedp->विस्तारः()
                         && विस्तारः_Min == elseDTypeSkippedp->विस्तारः_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } अन्यथा यदि ((विस्तारः == thenDTypeSkippedp->विस्तारः()
                                && विस्तारः_Min == thenDTypeSkippedp->विस्तारः_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } अन्यथा {
                        // None of the children dtypes match generate one
                        nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(विस्तारः, विस्तारः_Min, numeric, isFourstate);
                    }
                }
            }
        }
        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    शून्य दर्शय(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, विस्तारः_VP{SELF, BOTH}.p());
        // Conversions
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = newConsDynArrayOrQueue(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            सङ्कलय_Cons_Item(newp, nodep->lhsp()->unlink_Fr_पृष्ठभागः());
            सङ्कलय_Cons_Item(newp, nodep->rhsp()->unlink_Fr_पृष्ठभागः());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        यदि (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_आदर्शः* ध्रुव newp = नूतन Ast_आदर्शः{nodep->fileline(), शून्यसूचकः};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            प्रत्यागम;
        }

        // Concat handling
        यदि (m_vup->prelim()) {
            यदि (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, QueueDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            यदि (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_समुच्चयः_String();
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                यदि (m_प्रवाहः_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                    nodep->lhsp()->विस्तारः() + nodep->rhsp()->विस्तारः(),
                    nodep->lhsp()->विस्तारः_Min() + nodep->rhsp()->विस्तारः_Min(), VSigning::UNSIGNED,
                    Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            यदि (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep->lhsp(), Replicate)) {
                यदि (repp->विस्तारः() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_पृष्ठभागः());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
            यदि (ध्रुव AstReplicate* ध्रुव repp = VN_CAST(nodep->rhsp(), Replicate)) {
                यदि (repp->विस्तारः() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_पृष्ठभागः());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
        }
        यदि (m_vup->final()) {
            यदि (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                यदि (!lhsp->isString()) lhsp = नूतन AstCvtPackString{lhsp->fileline(), lhsp};
                यदि (!rhsp->isString()) rhsp = नूतन AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
            यदि (!nodep->dtypep()->विस्तारः_Sized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    शून्य दर्शय(AstConcatN* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_String();
        }
        यदि (m_vup->final()) {
            nodep->did_विस्तारः(सत्य);
            यदि (!nodep->dtypep()->विस्तारः_Sized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    शून्य दर्शय(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    शून्य दर्शय(AstDelay* nodep) override {
        यदि (nodep->is_आवृत्तिः_Delay() && m_underSExpr) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            यदि (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            यदि (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                प्रत्यागम;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_आवृत्तिः_Delay(nodep, nodep);
            प्रत्यागम;
        }
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            प्रत्यागम;
        }
        यदि (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        यदि (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        यदि (nodep->fileline()->timingOn()) {
            यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_अग्र_Null(nodep->stmtsp());
                प्रत्यागम;
            } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } अन्यथा {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        यदि (nodep->stmtsp()) nodep->सङ्कलय_अग्र_अत्र(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    शून्य दर्शय(AstFork* nodep) override {
        यदि (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        यदि (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_समुच्चयः_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ध्रुव newp = नूतन AstBegin{nodep->fileline(), nodep->name(), शून्यसूचकः, असत्य};
            nodep->replaceWith(newp);
            यदि (nodep->declsp()) newp->सङ्कलय_Declsp(nodep->declsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            यदि (nodep->stmtsp()) newp->सङ्कलय_Stmtsp(nodep->stmtsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            यदि (nodep->forksp()) newp->सङ्कलय_Stmtsp(nodep->forksp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = सत्य;
            iterateChildren(nodep);
        } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
        } अन्यथा {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    शून्य दर्शय_Wait_Or_Disable_Fork(Ast_ग्रन्थिः* nodep) {
        यदि (nodep->fileline()->timingOn()) {
            यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
                प्रत्यागम;
            } अन्यथा यदि (!v3Global.opt.timing().is_समुच्चयः_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    शून्य दर्शय(AstDisableFork* nodep) override { दर्शय_Wait_Or_Disable_Fork(nodep); }
    शून्य दर्शय(AstWaitFork* nodep) override { दर्शय_Wait_Or_Disable_Fork(nodep); }
    शून्य दर्शय(Ast_गन्तव्यम्_अधस्तनम्_N* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_String();
        }
    }
    शून्य दर्शय(Ast_गन्तव्यम्_ऊर्ध्वस्थम्_N* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_String();
        }
    }
    शून्य दर्शय(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
            यदि (constp) {
                यदि (constp->अङ्कः().isFourState()
                    || (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } अन्यथा {
                    times = constp->गन्तव्यम्_SInt();
                }
            }

            Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
            यदि (VN_IS(vdtypep, QueueDType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                यदि (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                यदि (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_अग्र(nodep->srcp(), विस्तारः_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_पृष्ठभागः());
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                } अन्यथा {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_आदर्शः* ध्रुव newp = नूतन Ast_आदर्शः{
                        nodep->fileline(),
                        नूतन AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_पृष्ठभागः(),
                                         शून्यसूचकः, शून्यसूचकः}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    प्रत्यागम;
                }
            }
            यदि (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            यदि (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } अन्यथा {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            यदि ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_पृष्ठभागः(),
                                        nodep->countp()->unlink_Fr_पृष्ठभागः()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा {
                यदि (!constp) nodep->v3error("Replication value isn't a constant.");
                यदि (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                    (nodep->srcp()->विस्तारः() * times), (nodep->srcp()->विस्तारः_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        यदि (m_vup->final()) {
            यदि (!nodep->dtypep()->विस्तारः_Sized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य दर्शय(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_समुच्चयः_String();
        }
        यदि (m_vup->final()) {
            यदि (!nodep->dtypep()->विस्तारः_Sized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Biop* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Dist_Triop* nodep) override {
        यदि (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    तार्किक प्रवाहः_Implicit_Use_Allowed(ध्रुव Ast_ग्रन्थिः_प्रवाहः* nodep) ध्रुव {
        यदि (m_प्रवाहः_Concat) प्रत्यागम सत्य;
        यदि (m_vup && m_vup->प्रवाहः_Use_2() != प्रवाहः_USE_NONE) प्रत्यागम सत्य;

        ध्रुव Ast_ग्रन्थिः* ध्रुव backp = nodep->backp();
        यदि (!backp) प्रत्यागम सत्य;  // Error elsewhere

        प्रत्यागम VN_IS(backp, ग्रन्थिः_नियोजय);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_प्रवाहः* nodep) override {
        VL_RESTORER(m_प्रवाहः_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            m_प्रवाहः_Concat = सत्य;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_प्रवाहः_Concat = असत्य;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->rhsp(), Const)) {
                यदि (constp->गन्तव्यम्_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } अन्यथा {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
            यदि (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, QueueDType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ग्रन्थिः_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ग्रन्थिः_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_समुच्चयः_प्रवाहः();
            } अन्यथा यदि (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_समुच्चयः_प्रवाहः();
            } अन्यथा {
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                    nodep->lhsp()->विस्तारः(), nodep->lhsp()->विस्तारः_Min(), VSigning::UNSIGNED,
                    Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        यदि (m_vup->final()) {
            यदि (!प्रवाहः_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_अधिकम्() << "... Suggest use a cast");
            }
            यदि (!nodep->dtypep()->विस्तारः_Sized()) {
                // See also error in V3Number
                ग्रन्थिः_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    शून्य दर्शय(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), सत्य,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), सत्य,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        यदि (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ध्रुव पूर्णाङ्क विस्तारः = nodep->elementsConst();
            यदि (nodep->fromBracket() && nodep->वामः_Const() > nodep->दक्षिणः_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-विस्तारः + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } अन्यथा यदि (विस्तारः > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << विस्तारः << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ध्रुव तार्किक अन्तर्_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ध्रुव तार्किक अन्तर्_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ध्रुव तार्किक अन्तर्_Type_सारणी = !m_modep;
            ध्रुव Ast_ग्रन्थिः* basep = nodep->backp();
            यावत् (VN_IS(basep, Range)) basep = basep->backp();
            यदि (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !अन्तर्_Dead_Module && !अन्तर्_Parameterized_Template && !अन्तर्_Type_सारणी) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->वामः_Const() << ":" << nodep->दक्षिणः_Const()
                                            << "]");
            }
        }
    }

    शून्य दर्शय(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->lsbp(), विस्तारः_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), असत्य);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            पूर्णाङ्क विस्तारः = nodep->विस्तारः_Const();
            यदि (विस्तारः <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_समुच्चयः_Bit();
                प्रत्यागम;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ध्रुव तार्किक isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ध्रुव तार्किक अन्तर्_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            यदि (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                विस्तारः = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Sized(विस्तारः, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->विस्तारः_Const(विस्तारः);
                प्रक्षिप_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(नूतन AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            यदि (nodep->fromp()->विस्तारः() < विस्तारः && !अन्तर्_Parameterized_Template) {
                // If it is not a lvalue, extend it
                यदि (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << विस्तारः << " bits from only "
                                                          << nodep->fromp()->विस्तारः()
                                                          << " bit number");
                    Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitOrLogicDType(
                        विस्तारः, विस्तारः, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    विस्तारः_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    असत्य /*noerror*/);
                } अन्यथा {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << विस्तारः << " bits to only "
                                                         << nodep->fromp()->विस्तारः()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            पूर्णाङ्क frommsb = nodep->fromp()->विस्तारः() - 1;
            पूर्णाङ्क fromlsb = 0;
            ध्रुव पूर्णाङ्क elw = nodep->decl_El_विस्तारः();  // Must adjust to tell user bit ranges
            यदि (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } अन्यथा {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ग्रन्थिः_DType* ध्रुव selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, FINAL}.p());
            user_Iterate_And_अग्र(nodep->lsbp(), विस्तारः_VP{SELF, FINAL}.p());
            यदि (विस्तारः_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->विस्तारः() != 32) {
                यदि (!nodep->fileline()->warnIsOff(V3_Error_संहिता::विस्तारः_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->विस्तारः() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->विस्तारः() / elw)
                            << (nodep->lsbp()->विस्तारः() != nodep->lsbp()->विस्तारः_Min()
                                    ? " or " + cvt_गन्तव्यम्_Str(nodep->lsbp()->विस्तारः_Min() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            यदि (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                यदि (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } अन्यथा यदि (!अन्तर्_Parameterized_Template) {
                    यदि (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } अन्यथा {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                यदि (!isWriteSelect) {
                    // Extend it
                    ध्रुव पूर्णाङ्क extend_गन्तव्यम् = nodep->msbConst() + 1;
                    Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitOrLogicDType(
                        extend_गन्तव्यम्, extend_गन्तव्यम्, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    विस्तारः_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    असत्य /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            यदि (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                विस्तारः_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                असत्य /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    शून्य दर्शय(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            यदि (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->bitp(), Const)) {
                    ध्रुव पूर्णाङ्क bit = constp->गन्तव्यम्_SInt();
                    Ast_ग्रन्थिः_व्यञ्जकम्* newp = newIfaceArrayElement(nodep->fromp(), bit);
                    यदि (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ध्रुव AstUnpackArrayDType* ध्रुव arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << bit + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
            }
            //
            पूर्णाङ्क frommsb;
            पूर्णाङ्क fromlsb;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                यदि (fromlsb > frommsb) std::swap(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                यदि (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } अन्यथा {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ग्रन्थिः_DType* ध्रुव selwidthDTypep = nodep->findBitOrLogicDType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            यदि (विस्तारः_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->विस्तारः() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->विस्तारः(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->विस्तारः()
                                       << (nodep->bitp()->विस्तारः() != nodep->bitp()->विस्तारः_Min()
                                               ? " or " + cvt_गन्तव्यम्_Str(nodep->bitp()->विस्तारः_Min())
                                               : "")
                                       << " bits.");
                यदि (!nodep->fileline()->warnIsOff(V3_Error_संहिता::विस्तारः_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            यदि (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                यदि (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->गन्तव्यम्_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->गन्तव्यम्_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    यदि (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->गन्तव्यम्_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                विस्तारः_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                असत्य /*NOWARN*/);
            }
        }
    }

    शून्य दर्शय(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstAssocArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, AssocArrayDType);
            यदि (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->कुञ्चिका_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    शून्य दर्शय(AstAlias* nodep) override {
        यदि (!nodep->did_विस्तारः_And_समुच्चयः()) {
            user_Iterate_And_अग्र(nodep->itemsp(), विस्तारः_VP{SELF, BOTH}.p());
        }

        ध्रुव स्वचालित check_If_व्यञ्जकम्_Ok = [एतत्](ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp) {
            यदि (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                प्रत्यागम असत्य;
            }

            ध्रुव AstVarRef* ध्रुव varRefp = VN_CAST(exprp, VarRef);
            यदि (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                प्रत्यागम असत्य;
            }
            AstVar* ध्रुव varp = varRefp->varp();
            यदि (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                प्रत्यागम असत्य;
            }
            यदि (m_aliasedVars.find(varp) != m_aliasedVars.अन्तः_2()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                प्रत्यागम असत्य;
            } अन्यथा {
                m_aliasedVars.निवेशय(varp);
            }
            प्रत्यागम सत्य;
        };

        check_If_व्यञ्जकम्_Ok(nodep->itemsp());
        Ast_ग्रन्थिः_DType* ध्रुव firstItemDtypep = nodep->itemsp()->dtypep();
        प्रति (Ast_ग्रन्थिः* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_व्यञ्जकम्_Ok(VN_AS(itemp, ग्रन्थिः_व्यञ्जकम्));
            यदि (!firstItemDtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << firstItemDtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    शून्य दर्शय(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            यदि (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ध्रुव AstBasicDType* ध्रुव basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            यदि (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->findStringDType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    शून्य दर्शय(Ast_खण्डिका_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ध्रुव AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            यदि (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ग्रन्थिः_DType* ध्रुव newDtp
                = नूतन AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          नूतन AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newDtp);
            nodep->dtypeFrom(newDtp);

            यदि (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ध्रुव पूर्णाङ्क subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ध्रुव VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                यदि ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(नूतन AstConsArray{
                        nodep->fileline(), newDtp,
                        नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                } अन्यथा यदि ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            यदि (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ध्रुव fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ध्रुव newp = नूतन AstConsArray{nodep->fileline(), newDtp};
                ध्रुव VNumRange range = nodep->declRange();
                प्रति (पूर्णाङ्क n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_वृक्षः(असत्य));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }

    शून्य दर्शय(AstSelBit* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->thsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3_विस्तारः::विस्तारः_Sel_No_Iter_Edit(nodep);
        यदि (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            प्रत्यागम;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    शून्य दर्शय(AstSelExtract* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->leftp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->rightp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3_विस्तारः::विस्तारः_Sel_No_Iter_Edit(nodep);
        यदि (selp != nodep) {
            nodep = शून्यसूचकः;
            userIterate(selp, m_vup);
            प्रत्यागम;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    शून्य दर्शय(AstSelPlus* nodep) override {
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->widthp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3_विस्तारः::विस्तारः_Sel_No_Iter_Edit(nodep);
        यदि (selp != nodep) {
            nodep = शून्यसूचकः;
            userIterate(selp, m_vup);
            प्रत्यागम;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    शून्य दर्शय(AstSelMinus* nodep) override {
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->bitp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->widthp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_अग्र(nodep->attrp(), विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः* ध्रुव selp = V3_विस्तारः::विस्तारः_Sel_No_Iter_Edit(nodep);
        यदि (selp != nodep) {
            nodep = शून्यसूचकः;
            userIterate(selp, m_vup);
            प्रत्यागम;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    शून्य दर्शय(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        यदि (m_vup && m_vup->prelim()) {
            यदि (VN_IS(nodep->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } अन्यथा यदि (nodep->अङ्कः().isNull()) {
                nodep->dtypep(nodep->findNullDType());
            } अन्यथा यदि (nodep->अङ्कः().isString()) {
                nodep->dtype_समुच्चयः_String();
            } अन्यथा यदि (nodep->अङ्कः().isDouble()) {
                nodep->dtype_Chg_विस्तारः(nodep->अङ्कः().विस्तारः(), nodep->अङ्कः().sized()
                                                               ? nodep->अङ्कः().विस्तारः()
                                                               : nodep->अङ्कः().विस्तारः_गन्तव्यम्_Fit());
            } अन्यथा {
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                    nodep->अङ्कः().विस्तारः(),
                    nodep->अङ्कः().sized() ? nodep->अङ्कः().विस्तारः() : nodep->अङ्कः().विस्तारः_गन्तव्यम्_Fit(),
                    nodep->dtypep()->numeric(), nodep->अङ्कः().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    शून्य दर्शय(Ast_रिक्तानि_Queue* nodep) override {
        nodep->dtype_समुच्चयः_रिक्तानि_Queue();
        यदि (!VN_IS(nodep->backp(), नियोजय) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    शून्य दर्शय(AstFell* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), शून्यसूचकः);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstFalling* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstFuture* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), शून्यसूचकः);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    शून्य दर्शय(AstPast* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeFrom(nodep->exprp());
            यदि (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ सत्य);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->ticksp(), Const);
                यदि (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                } अन्यथा यदि (constp->गन्तव्यम्_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                } अन्यथा यदि (V3AssertCommon::check_Sva_मानम्_सीमा(nodep, constp, "$past tick value")) {
                    यदि (constp->गन्तव्यम्_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->गन्तव्यम्_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), शून्यसूचकः);
        }
    }
    शून्य दर्शय(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->countp(), विस्तारः_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ध्रुव AstConst* ध्रुव minConstp = VN_CAST(nodep->countp(), Const);
            यदि (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा यदि (minConstp->गन्तव्यम्_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } अन्यथा यदि (!nodep->unbounded() && !nodep->maxCountp() && minConstp->गन्तव्यम्_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            यदि (nodep->maxCountp()) {
                user_Iterate_And_अग्र(nodep->maxCountp(), विस्तारः_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ध्रुव AstConst* ध्रुव maxConstp = VN_CAST(nodep->maxCountp(), Const);
                यदि (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } अन्यथा यदि (minConstp && maxConstp->गन्तव्यम्_SInt() < minConstp->गन्तव्यम्_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } अन्यथा यदि (maxConstp->गन्तव्यम्_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->propp(), विस्तारः_VP{SELF, BOTH}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* propp = nodep->propp();
            यदि (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_अग्र(nodep->loBoundp(), विस्तारः_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            यदि (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_अग्र(nodep->hiBoundp(), विस्तारः_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ध्रुव तार्किक loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ध्रुव तार्किक hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            यदि (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ग्रन्थिः* ध्रुव boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_समुच्चयः_Bit();
                प्रत्यागम;
            }
            यदि (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_समुच्चयः_Bit();
                प्रत्यागम;
            }
            ध्रुव AstConst* ध्रुव loConstp = VN_CAST(nodep->loBoundp(), Const);
            ध्रुव AstConst* ध्रुव hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            यदि (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            यदि (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            यदि (loConstp && loConstp->गन्तव्यम्_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } अन्यथा यदि (loConstp) {
                V3AssertCommon::check_Sva_मानम्_सीमा(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            यदि (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_मानम्_सीमा(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            यदि (!hiUnbounded && loConstp && hiConstp && hiConstp->गन्तव्यम्_SInt() < loConstp->गन्तव्यम्_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            तार्किक hasPropertyOp = propp->is_Multi_आवृत्तिः_Sva();
            यदि (!hasPropertyOp) {
                propp->foreach([&](ध्रुव Ast_ग्रन्थिः* np) {
                    यदि (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = सत्य;
                    }
                });
            }
            यदि (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_विस्तारः_Reduce(propp), propp);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstRising* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstRose* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), शून्यसूचकः);
            nodep->dtype_समुच्चयः_Bit();
        }
    }

    शून्य दर्शय(AstSampled* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    शून्य दर्शय(AstSetuphold* nodep) override {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_नियोजय_W* newp = शून्यसूचकः;
        यदि (nodep->delrefp()) {
            newp = convert_Setuphold_गन्तव्यम्_नियोजय(flp, nodep->refevp(), nodep->delrefp());
        }
        यदि (nodep->deldatap()) {
            यदि (!newp) {
                newp = convert_Setuphold_गन्तव्यम्_नियोजय(flp, nodep->dataevp(), nodep->deldatap());
            } अन्यथा {
                newp->सङ्कलय_अग्र_अत्र(
                    convert_Setuphold_गन्तव्यम्_नियोजय(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        यदि (!newp) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        nodep->replaceWith(नूतन AstAlways{newp});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    Ast_नियोजय_W* convert_Setuphold_गन्तव्यम्_नियोजय(File_पङ्क्तिः* ध्रुव flp, Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव evp,
                                         Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव delp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = delp->clone_वृक्षः_विशुद्धम्(असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = evp->clone_वृक्षः_विशुद्धम्(असत्य);
        UASSERT_OBJ(VN_IS(lhsp, ग्रन्थिः_Var_Ref) || VN_IS(lhsp, ग्रन्थिः_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        यदि (Ast_ग्रन्थिः_Var_Ref* varRefp = VN_CAST(lhsp, ग्रन्थिः_Var_Ref)) {
            यदि (varRefp->varp()->direction() == VDirection::निवेशः) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                प्रत्यागम शून्यसूचकः;
            }
            varRefp->access(VAccess::WRITE);
        }
        यदि (Ast_ग्रन्थिः_Pre_Sel* selp = VN_CAST(lhsp, ग्रन्थिः_Pre_Sel)) {
            यदि (Ast_ग्रन्थिः_Var_Ref* varRefp = VN_CAST(selp->fromp(), ग्रन्थिः_Var_Ref)) {
                यदि (varRefp->varp()->direction() == VDirection::निवेशः) {
                    VL_DO_DANGLING(प्रक्षिप_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                    प्रत्यागम शून्यसूचकः;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        प्रत्यागम नूतन Ast_नियोजय_W{flp, lhsp, rhsp};
    }

    शून्य दर्शय(AstStable* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            userIterate(nodep->sentreep(), शून्यसूचकः);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstSteady* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ सत्य);
            nodep->dtype_समुच्चयः_Bit();
        }
    }

    शून्य दर्शय(Ast_Stmt_व्यञ्जकम्* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{SELF, BOTH}.p());
    }

    शून्य दर्शय(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }

    शून्य दर्शय(AstUntil* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            यदि (nodep->lhsp()->विस्तारः() != 1) fix_विस्तारः_Reduce(nodep->lhsp());
            यदि (nodep->rhsp()->विस्तारः() != 1) fix_विस्तारः_Reduce(nodep->rhsp());
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstAbortOn* nodep) override { दर्शय_Abort_Prop(nodep); }

    शून्य दर्शय(AstRand* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            यदि (nodep->urandom()) {
                nodep->dtype_समुच्चयः_UInt32();  // Says the spec
            } अन्यथा {
                nodep->dtype_समुच्चयः_Int();  // Says the spec
            }
            यदि (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    शून्य दर्शय(AstSEventually* nodep) override {
        यदि (v3Global.opt.timing().is_समुच्चयः_False() || !v3Global.opt.timing().is_समुच्चयः_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::Widthed_मानम्{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstSGotoRep* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_अग्र(nodep->countp(), विस्तारः_VP{SELF, BOTH}.p());
            यदि (nodep->maxCountp()) {
                विस्तारः_Check_Goto_Rep_Range(nodep, "Goto");
            } अन्यथा {
                विस्तारः_Check_Rep_गणनम्(nodep);
            }
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstSNonConsRep* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_अग्र(nodep->countp(), विस्तारः_VP{SELF, BOTH}.p());
            यदि (nodep->maxCountp()) {
                विस्तारः_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } अन्यथा {
                विस्तारः_Check_Rep_गणनम्(nodep);
            }
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    प्रतिरूपम् <प्रकारनाम T_Rep>
    शून्य विस्तारः_Check_Rep_गणनम्(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->countp(), Const);
        यदि (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा यदि (constp->dtypep()->isSigned() && constp->अङ्कः().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } अन्यथा यदि (constp->अङ्कः().most_समुच्चयः_Bit_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } अन्यथा यदि (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    प्रतिरूपम् <प्रकारनाम T_Rep>
    शून्य विस्तारः_Check_Goto_Rep_Range(T_Rep* nodep, ध्रुव वर्ण* विधा) {
        user_Iterate_And_अग्र(nodep->maxCountp(), विस्तारः_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ध्रुव AstConst* ध्रुव minConstp = VN_CAST(nodep->countp(), Const);
        ध्रुव AstConst* ध्रुव maxConstp = VN_CAST(nodep->maxCountp(), Const);
        यदि (!minConstp || !maxConstp) {
            nodep->v3error(std::string{विधा}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            प्रत्यागम;
        }
        यदि (minConstp->गन्तव्यम्_SInt() < 0) {
            nodep->v3error(std::string{विधा}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            प्रत्यागम;
        }
        यदि (maxConstp->गन्तव्यम्_SInt() < minConstp->गन्तव्यम्_SInt()) {
            nodep->v3error(std::string{विधा}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            प्रत्यागम;
        }
        यदि (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + विधा
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    शून्य दर्शय(AstSThroughout* nodep) override {
        m_hasSExpr = सत्य;
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = असत्य;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = सत्य;
            iterate(nodep->rhsp());
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सत्य;
        m_hasSExpr = सत्य;
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            यदि (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = सत्य;
        m_hasSExpr = सत्य;
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            प्रति (AstSenItem* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), SenItem)) {
                user_Iterate_And_अग्र(senp->sensp(), विस्तारः_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstURandomRange* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            nodep->dtype_समुच्चयः_UInt32();  // Says the spec
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = nodep->findUInt32DType();
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    शून्य दर्शय(AstUnbounded* nodep) override {
        nodep->dtype_समुच्चयः_Int();  // Used in int context
        यदि (VN_IS(nodep->backp(), IsUnbounded)) प्रत्यागम;  // Ok, leave
        यदि (VN_IS(nodep->backp(), BracketArrayDType)) प्रत्यागम;  // Ok, leave
        यदि (VN_IS(nodep->backp(), InsideRange)) प्रत्यागम;  // Ok, leave
        यदि (ध्रुव स्वचालित* ध्रुव varp = VN_CAST(nodep->backp(), Var)) {
            यदि (varp->isParam()) प्रत्यागम;  // Ok, leave
        }
        Ast_ग्रन्थिः* backp = nodep->backp();
        यदि (VN_IS(backp, Sub)) backp = backp->backp();
        यदि (ध्रुव स्वचालित* ध्रुव selp = VN_CAST(backp, SelExtract)) {
            यदि (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) प्रत्यागम;
        }
        यदि (ध्रुव स्वचालित* ध्रुव selp = VN_CAST(backp, SelBit)) {
            यदि (VN_IS(selp->fromp()->dtypep()->skipRefp(), QueueDType)) प्रत्यागम;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    शून्य दर्शय(AstInferredDisable* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) nodep->dtype_समुच्चयः_Bit();
    }
    शून्य दर्शय(AstIsUnbounded* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        यदि (m_vup && m_vup->prelim()) {
            nodep->dtype_समुच्चयः_Bit_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        }
        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            यदि (nodep->विस्तारः() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    शून्य दर्शय(AstCLog2* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    प्रतिरूपम् <प्रकारनाम T_Option>
    शून्य विस्तारः_Cover_Weight(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->valuep();
        ध्रुव std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
            यदि (!constp->अङ्कः().isNegative()) प्रत्यागम;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->गन्तव्यम्_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } अन्यथा यदि (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } अन्यथा {
            प्रत्यागम;
        }
        // Continue with the default weight
        valuep->replaceWith(नूतन AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    प्रतिरूपम् <प्रकारनाम T_Option>
    शून्य विस्तारः_Cover_Int(T_Option* nodep) {
        user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    शून्य दर्शय(Ast_Cg_Option_नियोजय* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ध्रुव cgClassp = m_containingClassFinder.find(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        यदि (nodep->optType() == VCoverOptionType::WEIGHT) {
            विस्तारः_Cover_Weight(nodep);
            प्रत्यागम;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        यदि (nodep->optType() == VCoverOptionType::AT_न्यूनतमम्) {
            विस्तारः_Cover_Int(nodep);
            प्रत्यागम;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        यदि (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            यदि (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->गन्तव्यम्_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->गन्तव्यम्_SInt() << पङ्क्त्यन्तः);
            } अन्यथा {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } अन्यथा यदि (nodep->optType() == VCoverOptionType::MERGE_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = nodep->valuep();
            यदि (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(नूतन AstConst{valuep->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            }
            प्रत्यागम;
        } अन्यथा यदि (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            प्रत्यागम;
        } अन्यथा यदि (nodep->optType() == VCoverOptionType::DISTRIBUTE_FIRST) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->valuep(), Const);
            यदि (!constp || !constp->अङ्कः().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }
    शून्य दर्शय(AstCoverOption* nodep) override {
        यदि (nodep->optType() == VCoverOptionType::WEIGHT) {
            विस्तारः_Cover_Weight(nodep);
            प्रत्यागम;
        }
        यदि (nodep->optType() == VCoverOptionType::AT_न्यूनतमम्
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            विस्तारः_Cover_Int(nodep);
            प्रत्यागम;
        }
        user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstCoverCross* nodep) override {
        user_Iterate_And_अग्र(nodep->itemsp(), शून्यसूचकः);
        यदि (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_अग्र(nodep->optionsp(), शून्यसूचकः);
        user_Iterate_And_अग्र(nodep->binsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstCoverCrossBin* nodep) override {
        user_Iterate_And_अग्र(nodep->selectp(), शून्यसूचकः);
        यदि (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    शून्य दर्शय(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_अग्र(nodep->binsp(), विस्तारः_VP{nodep->exprp()->dtypep(), BOTH}.p());
        यदि (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_अग्र(nodep->optionsp(), शून्यसूचकः);
    }
    शून्य विस्तारः_Covergroup_Ranges(Ast_ग्रन्थिः* rangesp, पूर्णाङ्क fill_विस्तारः) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ध्रुव स्वचालित fill = [&](Ast_ग्रन्थिः* nodep) {
            यदि (!fill_विस्तारः) प्रत्यागम;
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = VN_AS(nodep, ग्रन्थिः_व्यञ्जकम्);
            fixAutoExtend(exprp /*ref*/, fill_विस्तारः);
        };
        प्रति (Ast_ग्रन्थिः *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            यदि (AstInsideRange* ध्रुव rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, शून्यसूचकः);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } अन्यथा यदि (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } अन्यथा {
                itemp = userIterateSubtreeReturnEdits(itemp, विस्तारः_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    शून्य दर्शय(AstCoverBinsof* nodep) override {
        user_Iterate_And_अग्र(nodep->pointp(), शून्यसूचकः);
        विस्तारः_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    शून्य दर्शय(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        विस्तारः_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->विस्तारः() : 0);
        यदि (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        यदि (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_अग्र(nodep->arraySizep(), विस्तारः_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_अग्र(nodep->transp(), m_vup);
    }
    शून्य दर्शय(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_अग्र(nodep->itemp(), शून्यसूचकः);
        यदि (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        यदि (!VN_IS(nodep->subp(), CoverpointRef)) {
            विस्तारः_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->विस्तारः());
        }
        विस्तारः_Cover_Filter(nodep->filterp(), "Bin");
    }
    शून्य दर्शय(AstCoverCrossWith* nodep) override {
        user_Iterate_And_अग्र(nodep->selectp(), शून्यसूचकः);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_अग्र(nodep->varsp(), शून्यसूचकः);
        विस्तारः_Cover_Filter(nodep->filterp(), "Cross bin");
        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव countp = nodep->countp()) {
            यदि (VN_IS(countp, Unbounded)) {
                countp->dtype_समुच्चयः_Int();  // 'matches $'
            } अन्यथा {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_अग्र(countp, विस्तारः_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    शून्य विस्तारः_Cover_Filter(Ast_ग्रन्थिः_व्यञ्जकम्* filterp, ध्रुव वर्ण* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, विस्तारः_VP{SELF, BOTH}.p()), ग्रन्थिः_व्यञ्जकम्);
        File_पङ्क्तिः* ध्रुव fl = filterp->fileline();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव resultp = filterp->dtypep()->skipRefp();
        यदि (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_पृष्ठभागः(&relinker);
            relinker.relink(
                नूतन AstNeqD{fl, filterp, नूतन AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } अन्यथा यदि (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(नूतन AstConst{fl, AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(filterp), filterp);
        } अन्यथा {
            fix_विस्तारः_Reduce(filterp);
        }
    }
    शून्य दर्शय(Ast_Cover_Trans_समुच्चयः* nodep) override { user_Iterate_And_अग्र(nodep->itemsp(), m_vup); }
    शून्य दर्शय(AstCoverTransItem* nodep) override {
        विस्तारः_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->विस्तारः() : 0);
    }
    शून्य दर्शय(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            यदि (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                प्रत्यागम;
            }
            checkCvtUS(nodep->lhsp(), असत्य);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            यदि (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->अङ्कः().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_समुच्चयः_Logic_Unsized(nodep->विस्तारः(), nodep->विस्तारः_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->findBitOrLogicDType(expDTypep->विस्तारः(), expDTypep->विस्तारः_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ग्रन्थिः* newp = शून्यसूचकः;  // No change
            यदि (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                    nodep->rhsp()->unlink_Fr_पृष्ठभागः()};
            } अन्यथा यदि (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                    nodep->rhsp()->unlink_Fr_पृष्ठभागः()};
            } अन्यथा यदि (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = नूतन AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                    nodep->rhsp()->unlink_Fr_पृष्ठभागः()};
            }
            यदि (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }
    शून्य दर्शय(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(Ast_गणनम्_द्वयङ्काः* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ध्रुव पूर्णाङ्क विस्तारः_Min = V3_सङ्ख्या::log2b(nodep->lhsp()->विस्तारः()) + 1;
            nodep->dtype_समुच्चयः_Bit_Unsized(32, विस्तारः_Min, VSigning::SIGNED);
        }
    }
    शून्य दर्शय(Ast_गणनम्_Ones* nodep) override {
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ध्रुव पूर्णाङ्क विस्तारः_Min = V3_सङ्ख्या::log2b(nodep->lhsp()->विस्तारः()) + 1;
            nodep->dtype_समुच्चयः_Bit_Unsized(32, विस्तारः_Min, VSigning::SIGNED);
        }
    }
    शून्य दर्शय(AstCvtPackString* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH, current_प्रवाहः_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        यदि (Ast_ग्रन्थिः_DType* ध्रुव dt = nodep->lhsp()->dtypep()) {
            यदि (VN_IS(dt->skip_Ref_गन्तव्यम्_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    शून्य दर्शय(Ast_Cvt_Packed_गन्तव्यम्_Array* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH, current_प्रवाहः_Use()}.p());
        // Type set in constructor
    }
    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Array* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH, current_प्रवाहः_Use()}.p());
        // Type set in constructor
    }
    शून्य दर्शय(Ast_Cvt_Array_गन्तव्यम्_Packed* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH, current_प्रवाहः_Use()}.p());
        // Type set in constructor
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Queue* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // Opaque returns, so arbitrary
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    शून्य दर्शय(Ast_Cvt_Unpack_गन्तव्यम्_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        यदि (m_vup->final()) {
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    शून्य दर्शय(AstTimeImport* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ध्रुव द्विगुण timePrescale = constp->अङ्कः().गन्तव्यम्_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ध्रुव द्विगुण time = timePrescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::RealDouble{}, time};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstEventControl* nodep) override {
        यदि (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        यदि (nodep->fileline()->timingOn()) {
            यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
                iterateChildren(nodep);
                प्रत्यागम;
            } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_अधिकम्()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } अन्यथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        यदि (nodep->stmtsp()) nodep->सङ्कलय_अग्र_अत्र(nodep->stmtsp()->unlink_Fr_पृष्ठभागः());
        VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    स्थिर string typenameExtends(ध्रुव AstClass* classp) {
        // An interface class may extend several, so those are not named
        यदि (!classp || classp->isInterfaceClass()) प्रत्यागम "";
        प्रति (ध्रुव AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            यदि (extendsp->isImplements()) अनुवर्त;
            ध्रुव AstClass* ध्रुव basep = extendsp->classOrNullp();
            प्रत्यागम basep ? " extends class " + basep->dtypeName(सत्य) : "";
        }
        प्रत्यागम "";
    }
    शून्य दर्शय(Ast_Attr_सम्बन्धि* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
        यदि (nodep->dimp()) user_Iterate_And_अग्र(nodep->dimp(), विस्तारः_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        विकल्प (nodep->attrType()) {
        पक्ष VAttrType::DIM_DIMENSIONS:
        पक्ष VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ध्रुव std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(सत्य);
            ध्रुव पूर्णाङ्क val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.second
                                                                       : (dim.first + dim.second));
            nodep->replaceWith(नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            भङ्ग;
        }
        पक्ष VAttrType::DIM_द्वयङ्काः_OR_सङ्ख्या: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            यदि (VN_IS(nodep->fromp(), ग्रन्थिः_व्यञ्जकम्)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_पृष्ठभागः());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            } अन्यथा {
                Ast_ग्रन्थिः* newp = नूतन Ast_Attr_सम्बन्धि{nodep->fileline(), VAttrType::DIM_द्वयङ्काः,
                                              nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                user_Iterate_And_अग्र(newp, विस्तारः_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            प्रत्यागम;
        }
        पक्ष VAttrType::DIM_द्वयङ्काः:
        पक्ष VAttrType::DIM_उच्चः:
        पक्ष VAttrType::DIM_INCREMENT:
        पक्ष VAttrType::DIM_वामः:
        पक्ष VAttrType::DIM_नीचः:
        पक्ष VAttrType::DIM_दक्षिणः:
        पक्ष VAttrType::DIM_आकार: {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            यदि (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ध्रुव Ast_ग्रन्थिः_Module* ध्रुव ownModp = v3Global.rootp()->containingModule(dtypep);
                यदि (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_समुच्चयः_Int();
                    प्रत्यागम;
                }
            }

            यदि (VN_IS(dtypep, QueueDType) || VN_IS(dtypep, DynArrayDType)) {
                विकल्प (nodep->attrType()) {
                पक्ष VAttrType::DIM_आकार: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_आकार};
                    newp->dtype_समुच्चयः_Int();
                    newp->did_विस्तारः(सत्य);
                    newp->protect(असत्य);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_वामः:
                पक्ष VAttrType::DIM_नीचः: {
                    Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_दक्षिणः:
                पक्ष VAttrType::DIM_उच्चः: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव sizep
                        = नूतन AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_आकार};
                    sizep->dtype_समुच्चयः_Int();
                    sizep->did_विस्तारः(सत्य);
                    sizep->protect(असत्य);
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstSub{nodep->fileline(), sizep,
                                     नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_INCREMENT: {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                        = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    भङ्ग;
                }
                पक्ष VAttrType::DIM_द्वयङ्काः: {
                    यदि (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } अन्यथा {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    भङ्ग;
                }
                स्वभाव: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } अन्यथा {
                ध्रुव std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(सत्य);
                ध्रुव uint32_t msbdim = dimpair.first + dimpair.second;
                यदि (!nodep->dimp() || msbdim < 1) {
                    यदि (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), ग्रन्थिः_व्यञ्जकम्);
                        Ast_ग्रन्थिः* ध्रुव newp = नूतन Ast_सरणम्_L{
                            nodep->fileline(), नूतन AstLenN{nodep->fileline(), fromp},
                            नूतन AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    } अन्यथा {
                        Ast_ग्रन्थिः_DType* ध्रुव आधारः_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(आधारः_DTypep, nodep, "Unsized expression");
                        ध्रुव पूर्णाङ्क dim = 1;
                        AstConst* ध्रुव newp = dimension_मानम्(nodep->fileline(), आधारः_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    }
                } अन्यथा यदि (VN_IS(nodep->dimp(), Const)) {
                    ध्रुव पूर्णाङ्क dim = VN_AS(nodep->dimp(), Const)->गन्तव्यम्_SInt();
                    AstConst* ध्रुव newp
                        = dimension_मानम्(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                } अन्यथा {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ध्रुव tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dimp = nodep->dimp()->unlink_Fr_पृष्ठभागः();
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                }
            }
            भङ्ग;
        }
        पक्ष VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            भङ्ग;
        पक्ष VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ग्रन्थिः_DType* ध्रुव protoDtp = nodep->fromp()->dtypep();
            Ast_ग्रन्थिः_DType* ध्रुव declDtp = [&]() {
                यदि (m_ftaskp->fvarp()) प्रत्यागम m_ftaskp->fvarp()->dtypep();
                Ast_ग्रन्थिः_DType* ध्रुव voidp = नूतन AstVoidDType{m_ftaskp->fileline()};
                प्रक्षिप_Deletep(voidp);  // Note voidp used past here
                प्रत्यागम voidp;
            }();
            यदि (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_अधिकम्()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_अधिकम्()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            भङ्ग;
        }
        पक्ष VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->fromp()->dtypep();
            string परिणाम = dtypep->prettyDTypeName(सत्य);
            यदि (ध्रुव AstClassRefDType* ध्रुव classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                परिणाम += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << परिणाम << "' from " << dtypep);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::String{}, परिणाम};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            भङ्ग;
        }
        पक्ष VAttrType::TYPEID:
            यदि (Ast_ग्रन्थिः_DType* dtypep = VN_CAST(nodep->fromp(), ग्रन्थिः_DType)) {
                nodep->dtypep(dtypep);
            } अन्यथा {
                nodep->dtypep(VN_AS(nodep->fromp(), ग्रन्थिः_व्यञ्जकम्)->dtypep());
            }
            भङ्ग;
        पक्ष VAttrType::VAR_आधारः:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            भङ्ग;
        स्वभाव: {
            // Everything else resolved earlier
            nodep->dtype_समुच्चयः_Bit_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            भङ्ग;
        }
        }
    }
    शून्य दर्शय(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstText* nodep) override {
        // Only used in CStmts which don't care....
    }

    शून्य दर्शय(AstCellArrayRef* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        user_Iterate_And_अग्र(nodep->selp(), विस्तारः_VP{SELF, PRELIM}.p());
        user_Iterate_And_अग्र(nodep->selp(), विस्तारः_VP{SELF, FINAL}.p());
        nodep->dtype_समुच्चयः_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_विस्तारः(सत्य);
    }

    शून्य दर्शय(AstCellRef* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;

        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cellExprp = VN_CAST(nodep->cellp(), ग्रन्थिः_व्यञ्जकम्)) {
            user_Iterate_And_अग्र(cellExprp, विस्तारः_VP{SELF, PRELIM}.p());
            user_Iterate_And_अग्र(cellExprp, विस्तारः_VP{SELF, FINAL}.p());
        }

        यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)) {
            user_Iterate_And_अग्र(exprp, विस्तारः_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_अग्र(exprp, विस्तारः_VP{SELF, FINAL}.p());
        } अन्यथा {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_विस्तारः(सत्य);
    }

    // DTYPES
    शून्य दर्शय(Ast_ग्रन्थिः_Array_DType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed

        यदि (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ध्रुव basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            यदि (basicp->implicit()) {
                UASSERT_OBJ(basicp->विस्तारः() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->विस्तारः() << " bits");
                AstBasicDType* ध्रुव newp = नूतन AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->विस्तारः_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        यदि (VN_IS(nodep, PackArrayDType)) m_underPackedArray = सत्य;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_अग्र(nodep->rangep(), विस्तारः_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        यदि (AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->विस्तारः_From_Sub(nodep->subDTypep());
            यदि (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(सत्य);
        } अन्यथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव elemDTypep = nodep->subDTypep()->skipRefp();
            यदि (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ध्रुव पूर्णाङ्क विस्तारः = nodep->subDTypep()->विस्तारः() * nodep->rangep()->elementsConst();
            nodep->विस्तारः_Force(विस्तारः, विस्तारः);
            यदि (!VL_RESTORER_पूर्व(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ध्रुव Ast_ग्रन्थिः_DType* आधारः_DTypep = nodep->subDTypep();
                यावत् (!VN_IS(आधारः_DTypep, RefDType) && आधारः_DTypep->subDTypep()) {
                    आधारः_DTypep = आधारः_DTypep->subDTypep();
                }
                ध्रुव AstBasicDType* ध्रुव basicp = nodep->basicp();
                यदि (!VN_IS(आधारः_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(सत्य));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstAssocArrayDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->कुञ्चिका_DTypep(iterateEditMoveDTypep(nodep, nodep->कुञ्चिका_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ग्रन्थिः_DType* ध्रुव childp = nodep->childDTypep();
        childp->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः* ध्रुव elementsp = nodep->elementsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः* newp;
        यदि (VN_IS(elementsp, Unbounded)) {
            newp = नूतन AstQueueDType{nodep->fileline(), VFlagChildDType{}, childp, शून्यसूचकः};
            VL_DO_DANGLING(elementsp->delete_वृक्षः(), elementsp);
        } अन्यथा यदि (Ast_ग्रन्थिः_DType* ध्रुव keyp = VN_CAST(elementsp, ग्रन्थिः_DType)) {
            newp = नूतन AstAssocArrayDType{nodep->fileline(), VFlagChildDType{}, childp, keyp};
        } अन्यथा {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_पङ्क्तिः* ध्रुव elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_संहिता::WIDTHEXPAND, सत्य);
            // Must be expression that is constant, but we'll determine that later
            newp = नूतन AstUnpackArrayDType{
                nodep->fileline(), VFlagChildDType{}, childp,
                नूतन AstRange{nodep->fileline(), नूतन AstConst(elementsp->fileline(), 0),
                             नूतन AstSub{elementsNewFl, VN_AS(elementsp, ग्रन्थिः_व्यञ्जकम्),
                                        नूतन AstConst(elementsNewFl, 1)},
                             सत्य}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, शून्यसूचकः), newp);
    }
    शून्य दर्शय(AstConstraintRefDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    शून्य दर्शय(AstDynArrayDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstQueueDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_अग्र(nodep->boundp(), विस्तारः_VP{SELF, BOTH}.p());
        यदि (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstVoidDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstUnsizedArrayDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        यदि (nodep->childDTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstWildcardArrayDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstBasicDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        यदि (nodep->generic()) प्रत्यागम;  // Already perfect
        यदि (nodep->rangep()) {
            user_Iterate_And_अग्र(nodep->rangep(), विस्तारः_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->विस्तारः_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } अन्यथा यदि (nodep->isRanged()) {
            nodep->विस्तारः_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } अन्यथा यदि (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->विस्तारः_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstConstDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, शून्यसूचकः);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->विस्तारः_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(AstRefDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_विस्तारः(सत्य);
        यदि (nodep->typeofp()) {  // type(typeofp_expression)
            यदि (Ast_ग्रन्थिः_DType* typeofDtp = VN_CAST(nodep->typeofp(), ग्रन्थिः_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } अन्यथा {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_अग्र(nodep->typeofp(), विस्तारः_VP{SELF, BOTH}.p());
                Ast_ग्रन्थिः* ध्रुव typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, शून्यसूचकः);
        यदि (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), शून्यसूचकः);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), शून्यसूचकः);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        यदि (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ध्रुव तार्किक अन्तर्_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            यदि (अन्तर्_Template_Module) {
                nodep->doing_विस्तारः(असत्य);
                प्रत्यागम;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->विस्तारः_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_विस्तारः(असत्य);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    शून्य विस्तारः_DType_Name_Params(ध्रुव AstTypedef* nodep) {
        ध्रुव VDTypeNameScopes* ध्रुव scopesp = VDTypeNameScopes::currentp();
        यदि (!scopesp) प्रत्यागम;  // Computing parameters, as by V3Param, so naming as found
        Ast_ग्रन्थिः* scopep = शून्यसूचकः;
        प्रति (ध्रुव Ast_ग्रन्थिः* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            यदि (ध्रुव std::सदिशम्<Ast_ग्रन्थिः*>* ध्रुव paramsp = scopesp->paramsp(scopep)) {
                प्रति (Ast_ग्रन्थिः* ध्रुव paramp : *paramsp) {
                    यदि (!paramp->did_विस्तारः()) userIterate(paramp, शून्यसूचकः);
                }
            }
        }
    }
    शून्य दर्शय(AstTypedef* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        यदि (स्वचालित* ध्रुव refp = check_Ref_गन्तव्यम्_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(शून्यसूचकः);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
            प्रत्यागम;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ध्रुव तार्किक declares = nodep->childDTypep() != शून्यसूचकः;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        यदि (declares) {
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->dtypep();
            यदि (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव sdtypep = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                विस्तारः_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } अन्यथा यदि (AstEnumDType* ध्रुव edtypep = VN_CAST(dtypep, EnumDType)) {
                विस्तारः_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, शून्यसूचकः);
    }
    शून्य दर्शय(AstParamTypeDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, शून्यसूचकः);
        nodep->विस्तारः_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        यदि (nodep->dtypep() && nodep->childDTypep()) { nodep->childDTypep(शून्यसूचकः); }
    }
    शून्य दर्शय(AstRequireDType* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        यदि (Ast_ग्रन्थिः_DType* ध्रुव dtp = VN_CAST(nodep->lhsp(), ग्रन्थिः_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_पृष्ठभागः());
        } अन्यथा {
            यदि (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(नूतन AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः_DType* ध्रुव गन्तव्यम्_Dtp = nodep->शिखरम्()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        ध्रुव स्वचालित castable = Ast_ग्रन्थिः::computeCastable(गन्तव्यम्_Dtp, fromDtp, nodep->fromp());
        Ast_ग्रन्थिः* newp;
        यदि (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            प्रत्यागम;
        } अन्यथा यदि (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ध्रुव enumDtp = VN_AS(गन्तव्यम्_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव testp = enumTestValid(nodep->fromp(), enumDtp);
            File_पङ्क्तिः* ध्रुव fl_novalue = नूतन File_पङ्क्तिः{fl};
            fl_novalue->warnOff(V3_Error_संहिता::ENUMVALUE, सत्य);
            newp = नूतन AstCond{
                fl, testp,
                नूतन Ast_व्यञ्जकम्_Stmt{fl,
                                नूतन Ast_नियोजय{fl_novalue, nodep->शिखरम्()->unlink_Fr_पृष्ठभागः(),
                                              nodep->fromp()->unlink_Fr_पृष्ठभागः()},
                                नूतन AstConst{fl, AstConst::Signed32{}, 1}},
                नूतन AstConst{fl, AstConst::Signed32{}, 0}};
        } अन्यथा यदि ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(गन्तव्यम्_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << गन्तव्यम्_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_अधिकम्() << "... Suggest static cast");
            newp = नूतन Ast_व्यञ्जकम्_Stmt{
                fl,
                नूतन Ast_नियोजय{fl, nodep->शिखरम्()->unlink_Fr_पृष्ठभागः(),
                              नूतन AstCast{fl, nodep->fromp()->unlink_Fr_पृष्ठभागः(), गन्तव्यम्_Dtp}},
                नूतन AstConst{fl, AstConst::Signed32{}, 1}};
        } अन्यथा यदि (castable == VCastable::INCOMPATIBLE) {
            newp = नूतन AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << गन्तव्यम्_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } अन्यथा {
            newp = नूतन AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << गन्तव्यम्_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_अधिकम्()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    शून्य दर्शय(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        यदि (AstConst* ध्रुव constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन Ast_Cast_आकार{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा यदि (Ast_ग्रन्थिः_DType* ध्रुव refp = VN_CAST(nodep->dtp(), ग्रन्थिः_DType)) {
            refp->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                              VFlagChildDType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_पृष्ठभागः());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(AstCast* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        यदि (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            यदि (AstSigned* ध्रुव fromp = VN_CAST(nodep->fromp(), Signed)) {
                यदि (VN_IS(fromp->lhsp(), ग्रन्थिः_प्रवाहः)) {
                    Ast_ग्रन्थिः* ध्रुव lhsp = fromp->lhsp()->unlink_Fr_पृष्ठभागः();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_वृक्षः(), fromp);
                }
            }
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, PRELIM, प्रवाहः_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ग्रन्थिः_DType* ध्रुव गन्तव्यम्_Dtp = nodep->dtypep()->skip_Ref_गन्तव्यम्_Enump();
            Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
            ध्रुव स्वचालित castable = Ast_ग्रन्थिः::computeCastable(गन्तव्यम्_Dtp, fromDtp, nodep->fromp());
            तार्किक bad = असत्य;
            यदि (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << गन्तव्यम्_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = सत्य;
            } अन्यथा यदि (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } अन्यथा यदि (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << गन्तव्यम्_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_अधिकम्() << "... Suggest dynamic $cast");
                bad = सत्य;
            } अन्यथा यदि (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << गन्तव्यम्_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = सत्य;
            } अन्यथा {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
            यदि (bad) {
            } अन्यथा यदि (AstBasicDType* ध्रुव basicp = गन्तव्यम्_Dtp->basicp()) {
                यदि (!basicp->isString() && fromDtp->isString()) {
                    newp = नूतन AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), गन्तव्यम्_Dtp};
                } अन्यथा यदि (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ग्रन्थिः_DType* ध्रुव origDTypep = nodep->dtypep();
                    यदि (!VN_IS(fromDtp, प्रवाहः_DType)) {
                        ध्रुव पूर्णाङ्क विस्तारः = गन्तव्यम्_Dtp->विस्तारः();
                        castSized(nodep, nodep->fromp(), विस्तारः);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } अन्यथा {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 असत्य);
                }
                यदि (newp) {
                } अन्यथा यदि (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    यदि (nodep->fromp()->isSigned()) {
                        newp = नूतन AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                    } अन्यथा {
                        newp = नूतन AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                    }
                } अन्यथा यदि (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = नूतन AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                    newp->dtypep(basicp);
                } अन्यथा यदि (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = नूतन AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                } अन्यथा यदि (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = नूतन AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
                } अन्यथा {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } अन्यथा यदि (VN_IS(गन्तव्यम्_Dtp, QueueDType)) {
                यदि (VN_IS(fromDtp, BasicDType)) {
                    newp = नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_पृष्ठभागः(), गन्तव्यम्_Dtp};
                } अन्यथा यदि (VN_IS(fromDtp, QueueDType) || VN_IS(fromDtp, प्रवाहः_DType)) {
                    पूर्णाङ्क src_Element_द्वयङ्काः = 1;
                    पूर्णाङ्क dst_Element_द्वयङ्काः = 1;
                    यदि (Ast_ग्रन्थिः_DType* ध्रुव elemDtp = fromDtp->subDTypep()) {
                        src_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    ध्रुव AstQueueDType* ध्रुव dstQueueDtp = VN_AS(गन्तव्यम्_Dtp, QueueDType);
                    यदि (Ast_ग्रन्थिः_DType* ध्रुव elemDtp = dstQueueDtp->subDTypep()) {
                        dst_Element_द्वयङ्काः = elemDtp->विस्तारः();
                    }
                    newp = नूतन Ast_Cvt_Array_गन्तव्यम्_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                                  गन्तव्यम्_Dtp,
                                                  असत्य,
                                                  1,
                                                  dst_Element_द्वयङ्काः,
                                                  src_Element_द्वयङ्काः};
                }
            } अन्यथा यदि (VN_IS(गन्तव्यम्_Dtp, ClassRefDType)) {
                // Can just remove cast
            } अन्यथा {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << गन्तव्यम्_Dtp->prettyDTypeNameQ());
            }
            यदि (!newp) newp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        यदि (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, असत्य, प्रवाहः_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव underp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            underp->dtypeFrom(nodep);
            underp->did_विस्तारः(सत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCastWrap{nodep->fileline(), underp};
            newp->did_विस्तारः(सत्य);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(Ast_Cast_आकार* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            पूर्णाङ्क विस्तारः = nodep->rhsp()->गन्तव्यम्_SInt();
            यदि (विस्तारः < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << विस्तारः);
                विस्तारः = 1;
            }
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM, प्रवाहः_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), विस्तारः);  // lhsp may change
        }
        यदि (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ग्रन्थिः* ध्रुव underp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    शून्य दर्शय(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{nodep->dtypep(), BOTH, प्रवाहः_USE_CAST}.p());
    }
    शून्य castSized(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* underp, पूर्णाङ्क विस्तारः) {
        ध्रुव AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        यदि (!underDtp) underDtp = underp->dtypep()->basicp();
        यदि (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->findBitDType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ध्रुव पूर्णाङ्क calc_विस्तारः = std::max(विस्तारः, underDtp->विस्तारः());
            Ast_ग्रन्थिः_DType* ध्रुव calcDtp = nodep->findBitOrLogicDType(
                calc_विस्तारः, calc_विस्तारः, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         असत्य, प्रवाहः_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ग्रन्थिः_DType* ध्रुव बहिर्गमनम्_Dtp = nodep->findBitOrLogicDType(
                विस्तारः, विस्तारः, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(बहिर्गमनम्_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            विस्तारः_Check_Sized(nodep, "Cast expr", VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), बहिर्गमनम्_Dtp, EXTEND_EXP,
                            असत्य);
            VL_DANGLING(underp);
        }
    }
    शून्य दर्शय(AstConstraintBefore* nodep) override {
        user_Iterate_And_अग्र(nodep->lhssp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->rhssp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(Ast_Constraint_व्यञ्जकम्* nodep) override {
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstConstraintUnique* nodep) override {
        user_Iterate_And_अग्र(nodep->rangesp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstDistItem* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), विस्तारः_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    शून्य दर्शय(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        यदि (nodep->doing_विस्तारः()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            प्रक्षिप_Deletep(nodep->valuep()->unlink_Fr_पृष्ठभागः());
            nodep->valuep(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_विस्तारः(सत्य);
            प्रत्यागम;
        }
        nodep->doing_विस्तारः(सत्य);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        यदि (!m_paramsOnly && nodep->is_Iface_Array_विभज()) m_विभज_Iface_Array_Varps.प्रक्षिप_पृष्ठभागः(nodep);
        यदि (nodep->attrsp()) {
            nodep->attrsp()->foreach([एतत्, nodep](Ast_Attr_सम्बन्धि* attrp) {
                यदि (attrp->attrType() == VAttrType::VAR_पत्तनम्_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    यदि (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ग्रन्थिः_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(attrp->unlink_Fr_पृष्ठभागः()), attrp);
                }
            });
        }
        यदि (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ग्रन्थिः_DType* dtp = nodep->dtypep();
            Ast_ग्रन्थिः_DType* np = शून्यसूचकः;
            यावत् (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                यदि (ध्रुव AstDynArrayDType* ध्रुव unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    यदि (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } अन्यथा {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ध्रुव newp
                        = नूतन AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    यदि (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } अन्यथा {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newp);
                    np = newp;
                } अन्यथा {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        यदि (AstWildcardArrayDType* ध्रुव wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        यदि (VN_IS(nodep->dtypep()->skip_Ref_गन्तव्यम्_Constp(), ConstDType)) nodep->isConst(सत्य);
        // Parameters if implicit untyped inherit from what they are assigned to
        ध्रुव AstBasicDType* ध्रुव bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        तार्किक didchk = असत्य;
        ध्रुव तार्किक implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        यदि (implicitParam) {
            यदि (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                यदि (nodep->valuep()->isDouble()) {
                    nodep->dtype_समुच्चयः_Double();
                    VL_DANGLING(bdtypep);
                } अन्यथा यदि (nodep->valuep()->isString()) {
                    nodep->dtype_समुच्चयः_String();
                    VL_DANGLING(bdtypep);
                } अन्यथा {
                    पूर्णाङ्क विस्तारः = 0;
                    Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(मानम्_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ध्रुव AstBasicDType* ध्रुव मानम्_Bdtypep = मानम्_DTypep->basicp();
                    तार्किक issigned = असत्य;
                    यदि (bdtypep->isNosign()) {
                        यदि (मानम्_Bdtypep && मानम्_Bdtypep->isSigned()) issigned = सत्य;
                    } अन्यथा {
                        issigned = bdtypep->isSigned();
                    }
                    यदि (मानम्_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        यदि (ध्रुव स्वचालित* ध्रुव constp = VN_CAST(nodep->valuep(), Const)) {
                            यदि (constp->अङ्कः().isString()) {
                                विस्तारः = constp->अङ्कः().गन्तव्यम्_String().दीर्घता_2() * 8;
                            }
                        }
                        यदि (विस्तारः < 8) विस्तारः = 8;
                    } अन्यथा यदि (nodep->valuep()->dtypep()->विस्तारः_Sized()) {
                        विस्तारः = nodep->valuep()->विस्तारः();
                    } अन्यथा {
                        यदि (nodep->valuep()->विस्तारः() > 32) {
                            nodep->valuep()->v3warn(
                                विस्तारः_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        विस्तारः = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ध्रुव तार्किक is_मानम्_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    यदि (विस्तारः == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->findBitOrLogicRangeDType(
                            VNumRange{0, 0}, nodep->valuep()->विस्तारः_Min(),
                            VSigning::fromBool(issigned), is_मानम्_Fourstate));
                    } अन्यथा {
                        nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(विस्तारः, nodep->valuep()->विस्तारः_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_मानम्_Fourstate);
                    }
                    didchk = सत्य;
                }
                iterate_Check_नियोजय(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } अन्यथा {
                // Or, if nothing assigned, they're integral
                nodep->dtype_समुच्चयः_Integer();
                VL_DANGLING(bdtypep);
            }
        } अन्यथा यदि (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_समुच्चयः_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        यदि (nodep->isNet()) {
            Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            यदि (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        यदि (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_नियोजय(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_अग्र(nodep->delayp(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_विस्तारः(सत्य);
        nodep->doing_विस्तारः(असत्य);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Var_Ref* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        यदि (!nodep->varp()) {
            यदि (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, असत्य,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                प्रत्यागम;
            } अन्यथा {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        यदि (!nodep->varp()->did_विस्तारः()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), शून्यसूचकः);
        }
        यदि (!m_paramsOnly && nodep->varp()->is_Iface_Array_विभज()) {
            replace_विभज_Iface_Array_Ref(nodep);
            प्रत्यागम;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        यदि (VN_IS(nodep->backp(), ग्रन्थिः_नियोजय) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        यदि (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(सत्य);
        यदि (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } अन्यथा यदि (nodep->access().isWriteOrRW() && nodep->varp()->is_निवेशः()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), नियोजय_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ध्रुव तार्किक hierRef = !m_curModVars.गणनम्(nodep->varp());
            ध्रुव तार्किक net_पत्तनम्
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::पत्तनम्;
            ध्रुव तार्किक cont_नियोजय = VN_IS(nodep->abovep(), नियोजय_W);
            ध्रुव तार्किक pinConn = VN_IS(nodep->abovep(), Pin);
            यदि (!(hierRef && net_पत्तनम् && (cont_नियोजय || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } अन्यथा यदि (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        यदि (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->did_विस्तारः(सत्य);
    }

    शून्य दर्शय(AstEnumDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ग्रन्थिः_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        यदि (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->findIntDType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->विस्तारः_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_अग्र(nodep->itemsp(), विस्तारः_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_सङ्ख्या अङ्कः(nodep, nodep->विस्तारः(), 0);
        ध्रुव V3_सङ्ख्या one{nodep, nodep->विस्तारः(), 1};
        तार्किक wrapAround = असत्य;
        std::map<ध्रुव V3_सङ्ख्या, AstEnumItem*> inits;
        प्रति (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            यदि (itemp->valuep()) {
                यदि (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_वृक्षः("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                यदि (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();
                    अनुवर्त;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = असत्य;  // Explicit value resets wrap-around tracking
            }
            यदि (!itemp->valuep()) {
                यदि (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                यदि (अङ्कः.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(नूतन AstConst{itemp->fileline(), अङ्कः});
            }

            ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
            यदि (constp->अङ्कः().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            अङ्कः.op_नियोजय(constp->अङ्कः());
            // Look for duplicates
            ध्रुव स्वचालित pair = inits.emplace(अङ्कः, itemp);
            यदि (!pair.second) {  // IEEE says illegal
                ध्रुव Ast_ग्रन्थिः* ध्रुव otherp = pair.first->second;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ध्रुव तार्किक पूर्व_Neg = constp->अङ्कः().isNegative();
            अङ्कः.op_सङ्कलय(one, constp->अङ्कः());
            यदि (basicp->isSigned()) {
                wrapAround = !पूर्व_Neg && अङ्कः.isNegative();
            } अन्यथा {
                wrapAround = अङ्कः.isEqZero();
            }
        }
    }
    शून्य दर्शय(AstEnumItem* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        यदि (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            तार्किक warnOn = सत्य;
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = nodep->valuep();
            यदि (ध्रुव Ast_सङ्कलय* ध्रुव anodep = VN_CAST(valuep, सङ्कलय_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                यदि (anodep->fileline()->equal_First_पङ्क्तिः_Col(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(valuep, Const)) {
                यदि (static_cast<पूर्णाङ्क>(constp->अङ्कः().most_समुच्चयः_Bit_P1()) > nodep->विस्तारः()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = असत्य;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(नूतन AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_पृष्ठभागः(),
                                      nodep->dtypep()});
        }
    }
    शून्य दर्शय(AstEnumItemRef* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        यदि (!nodep->itemp()->did_विस्तारः()) {
            // We need to do the whole enum en masse
            Ast_ग्रन्थिः* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            प्रति (; enump; enump = enump->backp()) {
                यदि (VN_IS(enump, EnumDType)) भङ्ग;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Cons* nodep) override {}  // Widthed when created
    शून्य दर्शय(Ast_ग्रन्थिः_Cons_Item* nodep) override {}  // Widthed when created
    शून्य दर्शय(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
        प्रति (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, विस्तारः_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ग्रन्थिः_DType* subDTypep = शून्यसूचकः;
        nodep->dtype_समुच्चयः_Bit();

        यदि (dtype && dtype->isString()) {
            nodep->dtype_समुच्चयः_String();
            subDTypep = nodep->findStringDType();
        } अन्यथा यदि (dtype && dtype->isDouble()) {
            nodep->dtype_समुच्चयः_Double();
            subDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णाङ्क विस्तारः = nodep->exprp()->विस्तारः();
            पूर्णाङ्क mwidth = nodep->exprp()->विस्तारः_Min();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            प्रति (ध्रुव Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                विस्तारः = std::max(विस्तारः, itemp->विस्तारः());
                mwidth = std::max(mwidth, itemp->विस्तारः_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->findBitOrLogicDType(
                विस्तारः, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        प्रति (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, DistItem)->rangep();
            यदि (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, विस्तारः_VP{subDTypep, FINAL}.p());
            } अन्यथा {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        यदि (m_constraintp) {
            तार्किक can_अधस्तनम् = सत्य;
            प्रति (AstDistItem* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), DistItem)) {
                यदि (VN_IS(ditemp->rangep(), InsideRange)) अनुवर्त;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDtp = ditemp->rangep()->dtypep()->skipRefp();
                यदि (!itemDtp->isIntegralOrPacked() || !ditemp->rangep()->is_विशुद्धम्()) {
                    can_अधस्तनम् = असत्य;
                    भङ्ग;
                }
            }
            यदि (can_अधस्तनम्) प्रत्यागम;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        प्रति (AstDistItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), DistItem)) {
            Ast_ग्रन्थिः_व्यञ्जकम्* inewp
                = insideItem(nodep, nodep->exprp()->clone_वृक्षः_विशुद्धम्(असत्य), itemp->rangep());
            यदि (!inewp) अनुवर्त;
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव cmpp
                = नूतन AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_पृष्ठभागः(),
                            नूतन AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_संहिता::UNSIGNED, सत्य);
            cmpp->fileline()->modifyWarnOff(V3_Error_संहिता::CMPCONST, सत्य);
            inewp = नूतन AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? नूतन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        यदि (!newp) newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    शून्य दर्शय(AstInside* nodep) override {
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
        प्रति (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, विस्तारः_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ग्रन्थिः_DType* expDTypep = शून्यसूचकः;

        यदि (dtype && dtype->isString()) {
            nodep->dtype_समुच्चयः_String();
            expDTypep = nodep->findStringDType();
        } अन्यथा यदि (dtype && dtype->isDouble()) {
            nodep->dtype_समुच्चयः_Double();
            expDTypep = nodep->findDoubleDType();
        } अन्यथा {
            // Take width as maximum across all items
            पूर्णाङ्क विस्तारः = nodep->exprp()->विस्तारः();
            पूर्णाङ्क mwidth = nodep->exprp()->विस्तारः_Min();
            तार्किक isFourstate = nodep->exprp()->dtypep()->isFourstate();
            प्रति (ध्रुव Ast_ग्रन्थिः* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                विस्तारः = std::max(विस्तारः, itemp->विस्तारः());
                mwidth = std::max(mwidth, itemp->विस्तारः_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_समुच्चयः_Bit();
            ध्रुव VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->findBitOrLogicDType(विस्तारः, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        प्रति (Ast_ग्रन्थिः *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            यदि (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, विस्तारः_VP{expDTypep, FINAL}.p());
            } अन्यथा यदि (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* exprp;
        Ast_व्यञ्जकम्_Stmt* व्यञ्जकम्_Stmtp = शून्यसूचकः;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        यदि (!m_constraintp && !nodep->exprp()->is_विशुद्धम्()) {
            File_पङ्क्तिः* ध्रुव fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = nodep->exprp()->dtypep();
            ध्रुव पूर्णाङ्क w = व्यञ्जकम्_Dtp->विस्तारः();
            Ast_ग्रन्थिः_DType* ध्रुव अस्थायी_DTypep
                = व्यञ्जकम्_Dtp->विस्तारः_Sized() ? व्यञ्जकम्_Dtp
                                        : nodep->findBitOrLogicDType(w, w, व्यञ्जकम्_Dtp->numeric(),
                                                                     व्यञ्जकम्_Dtp->isFourstate());
            AstVar* ध्रुव varp
                = नूतन AstVar{fl, VVarType::XTEMP, m_inside_अस्थायी_नामानि.get(nodep), अस्थायी_DTypep};
            exprp = नूतन AstVarRef{fl, varp, VAccess::READ};
            व्यञ्जकम्_Stmtp = नूतन Ast_व्यञ्जकम्_Stmt{fl,
                                        नूतन Ast_नियोजय{fl, नूतन AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_पृष्ठभागः()},
                                        exprp};
            यदि (m_ftaskp) {
                varp->funcLocal(सत्य);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } अन्यथा {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(व्यञ्जकम्_Stmtp);
        } अन्यथा {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        प्रति (Ast_ग्रन्थिः_व्यञ्जकम् *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ग्रन्थिः_व्यञ्जकम्);  // Will be unlinking
            Ast_ग्रन्थिः_व्यञ्जकम्* inewp;
            यदि (व्यञ्जकम्_Stmtp) {
                inewp = insideItem(nodep, व्यञ्जकम्_Stmtp, itemp);
                व्यञ्जकम्_Stmtp = शून्यसूचकः;
            } अन्यथा {
                inewp = insideItem(nodep, exprp->clone_वृक्षः_विशुद्धम्(असत्य), itemp);
            }
            यदि (!inewp) अनुवर्त;
            newp = newp ? नूतन AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        यदि (व्यञ्जकम्_Stmtp) VL_DO_DANGLING(व्यञ्जकम्_Stmtp->delete_वृक्षः(), व्यञ्जकम्_Stmtp);
        यदि (!newp) newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* insideItem(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_व्यञ्जकम्* itemp) {
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव itemDtp = itemp->dtypep()->skipRefp();
        यदि (AstInsideRange* ध्रुव irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            प्रत्यागम irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                             irangep->rhsp()->unlink_Fr_पृष्ठभागः());
        } अन्यथा यदि (VN_IS(itemDtp, UnpackArrayDType) || VN_IS(itemDtp, DynArrayDType)
                   || VN_IS(itemDtp, QueueDType)) {
            // Unsupported in parameters
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव inewp = नूतन AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_पृष्ठभागः(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, itemDtp->subDTypep(), BOTH);
            inewp->dtype_समुच्चयः_Bit();
            inewp->did_विस्तारः(सत्य);
            प्रत्यागम inewp;
        } अन्यथा यदि (VN_IS(itemDtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_वृक्षः(), exprp);
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_पृष्ठभागः());
    }
    शून्य दर्शय(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        यदि (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } अन्यथा {
            यदि (m_vup->prelim()) {
                user_Iterate_And_अग्र(nodep->lhsp(), m_vup);
                user_Iterate_And_अग्र(nodep->rhsp(), m_vup);
            }
            यदि (m_vup->final()) {
                Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ध्रुव पूर्णाङ्क exp_विस्तारः = expDTypep->विस्तारः();
                ध्रुव तार्किक waiveLhs = exp_विस्तारः == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->विस्तारः_Min() >= nodep->lhsp()->विस्तारः();
                ध्रुव तार्किक waiveRhs = exp_विस्तारः == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_विस्तारः >= nodep->rhsp()->विस्तारः_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    शून्य दर्शय(AstIfaceRefDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        यदि (nodep->isVirtual()) nodep->iface_द्वारा_Cellp()->समुच्चयः_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    शून्य दर्शय(Ast_ग्रन्थिः_UOr_Struct_DType* nodep) override {
        यदि (nodep->doing_विस्तारः()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_समुच्चयः_Bit();
            nodep->doing_विस्तारः(असत्य);
            प्रत्यागम;
        }
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_विस्तारः(सत्य);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        यदि (ध्रुव AstUnionDType* ध्रुव unionp = VN_CAST(nodep, UnionDType)) {
            यदि (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, शून्यसूचकः);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(असत्य);
        // Error checks
        प्रति (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ग्रन्थिः_DType* ध्रुव dtp = itemp->subDTypep()->skipRefp();
            यदि (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            यदि ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                प्रक्षिप_Deletep(itemp->valuep()->unlink_Fr_पृष्ठभागः());
            }
        }
        ध्रुव तार्किक isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ध्रुव तार्किक अन्तर्_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        यदि (VN_IS(nodep, UnionDType) || nodep->packed()) {
            पूर्णाङ्क lsb = 0;
            पूर्णाङ्क विस्तारः = 0;
            तार्किक first = सत्य;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            प्रति (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            प्रति (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                यदि (itemp->isFourstate()) nodep->isFourstate(सत्य);
                itemp->lsb(lsb);
                यदि (VN_IS(nodep, UnionDType)) {
                    ध्रुव पूर्णाङ्क item_विस्तारः = itemp->विस्तारः();
                    // Skip union size check for template modules with unresolved parameters
                    यदि (!first && isHardPackedUnion && item_विस्तारः != विस्तारः && !अन्तर्_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    विस्तारः = std::max(विस्तारः, item_विस्तारः);
                } अन्यथा {
                    lsb += itemp->विस्तारः();
                    विस्तारः += itemp->विस्तारः();
                }
                first = असत्य;
            }
            nodep->विस्तारः_Force(विस्तारः, विस्तारः);  // Signing stays as-is, as parsed from declaration
        } अन्यथा {
            nodep->विस्तारः_Force(1, 1);
        }
        nodep->doing_विस्तारः(असत्य);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    शून्य दर्शय(AstThisRef* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        यदि (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    शून्य दर्शय(AstClassRefDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    शून्य दर्शय(AstClassOrPackageRef* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        userIterateChildren(nodep, शून्यसूचकः);
    }
    शून्य दर्शय(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_अधिकम्() << "... Suggest use a typedef");
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    शून्य दर्शय(AstClassExtends* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        यदि (VN_IS(nodep->childDTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->childDTypep()));
        }
    }
    शून्य दर्शय(AstMemberDType* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->विस्तारः_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        यदि (nodep->valuep()) {
            user_Iterate_And_अग्र(nodep->valuep(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_नियोजय(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    शून्य दर्शय(AstStructSel* nodep) override {
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = सत्य;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ध्रुव AstMemberSel* ध्रुव fromSel = VN_CAST(nodep->fromp(), MemberSel);
        यदि (AstClocking* ध्रुव clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->firstAbovep(), Clocking)
                                               : शून्यसूचकः) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            यदि (AstVar* ध्रुव varp = memberSelClocking(nodep, clockingp)) {
                यदि (!varp->did_विस्तारः()) userIterate(varp, शून्यसूचकः);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_पृष्ठभागः());
                VL_DO_DANGLING(fromp->delete_वृक्षः(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                यदि (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_समुच्चयः(nodep, nodep->access());
                }
                यदि (AstIfaceRefDType* ध्रुव adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_विस्तारः(सत्य);  // Must not visit again: will confuse scopes
                प्रत्यागम;
            }
        } अन्यथा यदि (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep = VN_CAST(fromDtp, ग्रन्थिः_UOr_Struct_DType)) {
            यदि (memberSelStruct(nodep, adtypep)) प्रत्यागम;
        } अन्यथा यदि (AstClassRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            यदि (memberSelClass(nodep, adtypep)) प्रत्यागम;
        } अन्यथा यदि (AstIfaceRefDType* ध्रुव adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            यदि (Ast_ग्रन्थिः* foundp = memberSelIface(nodep, adtypep)) {
                यदि (AstClocking* ध्रुव clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                यदि (AstVar* ध्रुव varp = VN_CAST(foundp, Var)) {
                    यदि (!varp->did_विस्तारः()) userIterate(varp, शून्यसूचकः);
                    AstIface* ध्रुव ifacep = adtypep->iface_द्वारा_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ध्रुव AstVarRef* ध्रुव refp = VN_CAST(nodep->fromp(), VarRef);
                    यदि (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ध्रुव newp = नूतन AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_विस्तारः(सत्य);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                        प्रत्यागम;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_विस्तारः(सत्य);
                    प्रत्यागम;
                }
                यदि (AstModport* ध्रुव modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ध्रुव newDtypep = नूतन AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newDtypep);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                यदि (AstCell* ध्रुव cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    यदि (VN_IS(cellp->modp(), Iface)) {
                        ध्रुव string viftopName = cellp->name() + "__Viftop";
                        Ast_ग्रन्थिः_Module* ध्रुव parentIfacep = adtypep->iface_द्वारा_Cellp();
                        AstVar* viftopVarp = शून्यसूचकः;
                        प्रति (Ast_ग्रन्थिः* itemp = parentIfacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            यदि (AstVar* ध्रुव vp = VN_CAST(itemp, Var)) {
                                यदि (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    भङ्ग;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        यदि (!viftopVarp->did_विस्तारः()) userIterate(viftopVarp, शून्यसूचकः);
                        Ast_ग्रन्थिः_DType* subDtypep = viftopVarp->dtypep();
                        यदि (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            यदि (AstIfaceRefDType* ध्रुव subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ध्रुव newDtypep = नूतन AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(सत्य);
                                v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_विस्तारः(सत्य);
                        प्रत्यागम;
                    }
                }
                यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp = VN_CAST(foundp, ग्रन्थिः_FTask)) {
                    Ast_विधानम्_Call* ध्रुव newp = नूतन Ast_विधानम्_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    प्रत्यागम;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } अन्यथा यदि (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, QueueDType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ग्रन्थिः* ध्रुव newp = नूतन Ast_विधानम्_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_पृष्ठभागः(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            प्रत्यागम;
        } अन्यथा {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }
    तार्किक memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        यदि (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_विधानम्_Call* ध्रुव newp = नूतन Ast_विधानम्_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            दर्शय(newp);
            प्रत्यागम सत्य;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ध्रुव first_classp = adtypep->classp();
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        प्रति (AstClass* classp = first_classp; classp;) {
            यदि (Ast_ग्रन्थिः* ध्रुव foundp = m_memberMap.findMember(classp, nodep->name())) {
                यदि (AstVar* ध्रुव varp = VN_CAST(foundp, Var)) {
                    यदि (!varp->did_विस्तारः()) userIterate(varp, शून्यसूचकः);
                    यदि (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ध्रुव varRefp
                            = नूतन AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                        प्रत्यागम सत्य;
                    }
                    यदि (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_विस्तारः(सत्य);
                    यदि (nodep->fromp()->same_वृक्षः(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_समुच्चयः(nodep);
                    प्रत्यागम सत्य;
                }
                यदि (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(नूतन AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), constrp});
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम सत्य;
                }
                यदि (AstEnumItemRef* ध्रुव adfoundp = VN_CAST(foundp, EnumItemRef)) {
                    nodep->replaceWith(adfoundp->clone_वृक्षः(असत्य));
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम सत्य;
                }
                यदि (Ast_ग्रन्थिः_FTask* ftaskp = VN_CAST(foundp, ग्रन्थिः_FTask)) {
                    Ast_विधानम्_Call* newp = नूतन Ast_विधानम्_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम सत्य;
                }
                यदि (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->findConstraintRefDType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    प्रत्यागम सत्य;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः;
        }

        VSpellCheck speller;
        प्रति (AstClass* classp = first_classp; classp;) {
            प्रति (Ast_ग्रन्थिः* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                यदि (VN_IS(itemp, Constraint) || VN_IS(itemp, EnumItemRef) || VN_IS(itemp, Var)) {
                    speller.प्रक्षिप_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः;
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << first_classp->verilogKwd()
                      << " " << first_classp->prettyNameQ() << "\n"
                      << (suggest.रिक्तानि() ? "" : nodep->fileline()->warn_अधिकम्() + suggest));
        प्रत्यागम असत्य;  // Caller handles error
    }
    तार्किक modportAllowsItem(Ast_ग्रन्थिः* nodep, ध्रुव AstIfaceRefDType* adtypep, ध्रुव Ast_ग्रन्थिः* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ध्रुव AstModport* ध्रुव modportp = adtypep->modportp();
        यदि (!modportp) प्रत्यागम सत्य;
        यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(itemp, Var)) {
            यदि (varp->isParam()) प्रत्यागम सत्य;
        } अन्यथा यदि (!VN_IS(itemp, ग्रन्थिः_FTask) && !VN_IS(itemp, Clocking)) {
            प्रत्यागम सत्य;
        }
        स्वचालित pair = m_modport_नामानि.emplace(modportp, std::unordered_समुच्चयः<string>{});
        यदि (pair.second) {
            प्रति (ध्रुव Ast_ग्रन्थिः* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.first->second.निवेशय(mpItemp->name());
            }
        }
        यदि (pair.first->second.गणनम्(itemp->name())) प्रत्यागम सत्य;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_द्वारा_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        प्रत्यागम असत्य;
    }
    Ast_ग्रन्थिः* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ग्रन्थिः_Module* ध्रुव ifacep = adtypep->iface_द्वारा_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        प्रति (Ast_ग्रन्थिः* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            यदि (itemp->name() == nodep->name()) {
                यदि (!modportAllowsItem(nodep, adtypep, itemp)) {
                    प्रत्यागम शून्यसूचकः;  // Caller handles error
                }
                प्रत्यागम itemp;
            }
            यदि (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.प्रक्षिप_Candidate(itemp->prettyName());
            }
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.रिक्तानि() ? "" : nodep->fileline()->warn_अधिकम्() + suggest));
        प्रत्यागम शून्यसूचकः;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        प्रति (Ast_ग्रन्थिः* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            यदि (AstClockingItem* citemp = VN_CAST(itemp, ClockingItem)) {
                यदि (citemp->varp()->name() == nodep->name()) प्रत्यागम citemp->varp();
                speller.प्रक्षिप_Candidate(citemp->varp()->prettyName());
            }
        }
        ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.रिक्तानि() ? "" : nodep->fileline()->warn_अधिकम्() + suggest));
        प्रत्यागम शून्यसूचकः;  // Caller handles error
    }
    तार्किक memberSelStruct(AstMemberSel* nodep, Ast_ग्रन्थिः_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        यदि (AstMemberDType* ध्रुव memberp
            = VN_CAST(m_memberMap.findMember(adtypep, nodep->name()), MemberDType)) {
            यदि (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } अन्यथा यदि (adtypep->packed()) {
                AstSel* ध्रुव newp = नूतन AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                                memberp->lsb(), memberp->विस्तारः()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_गन्तव्यम्_Enump());
                newp->did_विस्तारः(सत्य);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } अन्यथा {
                AstStructSel* ध्रुव newp = नूतन AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_गन्तव्यम्_Enump());
                newp->did_विस्तारः(सत्य);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            प्रत्यागम सत्य;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        प्रत्यागम असत्य;
    }

    शून्य दर्शय(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    शून्य दर्शय(Ast_विधानम्_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ग्रन्थिः_DType* ध्रुव fromDtp = nodep->fromp()->dtypep()->skip_Ref_गन्तव्यम्_Enump();
        AstBasicDType* ध्रुव basicp = fromDtp ? fromDtp->basicp() : शून्यसूचकः;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, विस्तारः_VP{SELF, BOTH}.p());
        यदि (nodep->name() == "rand_mode") {
            विधानम्_Call_Rand_बहुलकम्(nodep);
        } अन्यथा यदि (nodep->name() == "constraint_mode") {
            विधानम्_Call_Constraint(nodep, शून्यसूचकः);
        } अन्यथा यदि (AstEnumDType* ध्रुव adtypep = VN_CAST(fromDtp, EnumDType)) {
            विधानम्_Call_Enum(nodep, adtypep);
        } अन्यथा यदि (AstAssocArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            विधानम्_Call_Assoc(nodep, adtypep);
        } अन्यथा यदि (AstWildcardArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            विधानम्_Call_Wildcard(nodep, adtypep);
        } अन्यथा यदि (AstDynArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            विधानम्_Call_Dyn(nodep, adtypep);
        } अन्यथा यदि (AstQueueDType* ध्रुव adtypep = VN_CAST(fromDtp, QueueDType)) {
            विधानम्_Call_Queue(nodep, adtypep);
        } अन्यथा यदि (AstClassRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            विधानम्_Call_Class(nodep, adtypep);
        } अन्यथा यदि (AstIfaceRefDType* ध्रुव adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            विधानम्_Call_Iface_Ref(nodep, adtypep);
        } अन्यथा यदि (AstUnpackArrayDType* ध्रुव adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            विधानम्_Call_Unpack(nodep, adtypep);
        } अन्यथा यदि (AstConstraintRefDType* ध्रुव adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            विधानम्_Call_Constraint(nodep, adtypep);
        } अन्यथा यदि (basicp && basicp->isEvent()) {
            विधानम्_Call_Event(nodep, basicp);
        } अन्यथा यदि (basicp && basicp->isString()) {
            विधानम्_Call_String(nodep, basicp);
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_समुच्चयः_Void();
        }
    }
    AstWith* विधानम्_With_Clause(Ast_ग्रन्थिः_FTask_Ref* nodep, तार्किक required, तार्किक arbReturn,
                              Ast_ग्रन्थिः_DType* returnDtp, Ast_ग्रन्थिः_DType* सूचकांक_Dtp,
                              Ast_ग्रन्थिः_DType* मानम्_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        यदि (AstWith* ध्रुव withp = nodep->withp()) {
            withp->सूचकांक_Arg_Refp()->dtypep(सूचकांक_Dtp);
            withp->मानम्_Arg_Refp()->dtypep(मानम्_Dtp);
            userIterate(withp, विस्तारः_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_पृष्ठभागः();
            प्रत्यागम withp;
        }
        यदि (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        प्रत्यागम शून्यसूचकः;
    }
    शून्य विधानम्_Ok_Arguments(Ast_ग्रन्थिः_FTask_Ref* nodep, पूर्णाङ्क minArg, पूर्णाङ्क maxArg) {
        पूर्णाङ्क narg = 0;
        यदि (AstWith* ध्रुव withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(प्रक्षिप_Deletep(withp->unlink_Fr_पृष्ठभागः()), withp);
        }
        प्रति (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ध्रुव तार्किक ok = (narg >= minArg) && (narg <= maxArg);
        यदि (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_गन्तव्यम्_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_गन्तव्यम्_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            प्रति (; narg < minArg; ++narg) {
                nodep->सङ्कलय_Argsp(
                    नूतन AstArg{nodep->fileline(), "",
                               नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}}});
            }
            प्रति (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                यावत् (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_पृष्ठभागः();
                VL_DO_DANGLING(argp->delete_वृक्षः(), argp);
            }
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* विधानम्_Arg(Ast_विधानम्_Call* nodep, पूर्णाङ्क arg) {
        AstArg* argp = nodep->argsp();
        प्रति (पूर्णाङ्क narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        प्रत्यागम argp->exprp();
    }

    शून्य विधानम्_Call_Enum(Ast_विधानम्_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        यदि (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = शून्यसूचकः;
            विधानम्_Ok_Arguments(nodep, 0, 0);
            यदि (nodep->name() == "num") {
                पूर्णाङ्क वस्तूनि = 0;
                प्रति (Ast_ग्रन्थिः* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++वस्तूनि;
                newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{}, वस्तूनि);
            } अन्यथा यदि (nodep->name() == "first") {
                ध्रुव AstEnumItem* itemp = adtypep->itemsp();
                यदि (!itemp) {
                    newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्षः(असत्य), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } अन्यथा यदि (nodep->name() == "last") {
                ध्रुव AstEnumItem* itemp = adtypep->itemsp();
                यावत् (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), EnumItem);
                यदि (!itemp) {
                    newp = नूतन AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } अन्यथा {
                    newp = VN_AS(itemp->valuep()->clone_वृक्षः(असत्य), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            यदि (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                विधानम्_Ok_Arguments(nodep, 0, 0);
            } अन्यथा यदि (nodep->name() == "next") {
                attrType = VAttrType::ENUM_अग्र;
                विधानम्_Ok_Arguments(nodep, 0, 1);
            } अन्यथा यदि (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_पूर्व;
                विधानम्_Ok_Arguments(nodep, 0, 1);
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
            }

            यदि (nodep->name() != "name" && nodep->argsp()) {
                Ast_ग्रन्थिः_व्यञ्जकम्* stepp = विधानम्_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = विधानम्_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_संहिता::WIDTHEXPAND, सत्य);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                यदि (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ध्रुव uint32_t चरणः_विस्तारः = VN_AS(stepp, Const)->गन्तव्यम्_UInt();
                यदि (चरणः_विस्तारः == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_विधानम्_Call* ध्रुव newp = नूतन Ast_विधानम्_Call{
                        nodep->fileline(),
                        नूतन Ast_विधानम्_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    प्रत्यागम;
                } अन्यथा यदि (चरणः_विस्तारः != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    प्रक्षिप_Deletep(nodep->argsp()->unlink_Fr_पृष्ठभागः());
                    Ast_विधानम्_Call* ध्रुव clonep = nodep->clone_वृक्षः(असत्य);
                    VN_AS(stepp, Const)->अङ्कः().समुच्चयः_Long(1);
                    AstConst* ध्रुव constp = नूतन AstConst(nodep->fileline(), चरणः_विस्तारः - 1);
                    AstArg* ध्रुव argp = नूतन AstArg{nodep->fileline(), "", constp};
                    Ast_विधानम्_Call* ध्रुव newp
                        = नूतन Ast_विधानम्_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                    प्रत्यागम;
                }
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                = enumSelect(nodep->fromp()->unlink_Fr_पृष्ठभागः(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    शून्य विधानम्_Call_Wildcard(Ast_विधानम्_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = शून्यसूचकः;
        यदि (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ASSOC_आकार};  // So don't need num()
            newp->dtype_समुच्चयः_Int();
        } अन्यथा यदि (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } अन्यथा यदि (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", विधानम्_Arg(nodep, 0), SELF, BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Wildcard_सूचकांक_व्यञ्जकम्(nodep, adtypep);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ASSOC_EXISTS, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
            newp->dtype_समुच्चयः_Bit_Unsized(32, 1, VSigning::SIGNED);
        } अन्यथा यदि (nodep->name() == "delete") {  // function void delete([input integer index])
            विधानम्_Ok_Arguments(nodep, 0, 1);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            यदि (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_समुच्चयः_Void();
            } अन्यथा {
                iterateCheckSelf(nodep, "argument", विधानम्_Arg(nodep, 0), SELF, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Wildcard_सूचकांक_व्यञ्जकम्(nodep, adtypep);
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::ASSOC_ERASE, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
                newp->dtype_समुच्चयः_Void();
            }
        } अन्यथा यदि (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा यदि (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "map") {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                   adtypep->findStringDType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        यदि (newp) {
            newp->protect(असत्य);
            newp->did_विस्तारः(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य विधानम्_Call_Assoc(Ast_विधानम्_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = शून्यसूचकः;
        यदि (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        यदि (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ASSOC_आकार};  // So don't need num()
            newp->dtype_समुच्चयः_Int();
        } अन्यथा यदि (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", विधानम्_Arg(nodep, 0), adtypep->कुञ्चिका_DTypep(), BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Assoc_सूचकांक_व्यञ्जकम्(nodep, adtypep);
            विधानम्_Call_LValue_Recurse(nodep, सूचकांक_exprp, VAccess::READWRITE);
            newp
                = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                     VCMethod::array_विधानम्(nodep->name()),  // first/last/next/prev
                                     सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
            newp->dtype_समुच्चयः_Int();
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", विधानम्_Arg(nodep, 0), adtypep->कुञ्चिका_DTypep(), BOTH);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Assoc_सूचकांक_व्यञ्जकम्(nodep, adtypep);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ASSOC_EXISTS, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
            newp->dtype_समुच्चयः_Bit_Unsized(32, 1, VSigning::SIGNED);
        } अन्यथा यदि (nodep->name() == "delete") {  // function void delete([input integer index])
            विधानम्_Ok_Arguments(nodep, 0, 1);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            यदि (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_समुच्चयः_Void();
            } अन्यथा {
                iterateCheckTyped(nodep, "argument", विधानम्_Arg(nodep, 0), adtypep->कुञ्चिका_DTypep(),
                                  BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Assoc_सूचकांक_व्यञ्जकम्(nodep, adtypep);
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::ASSOC_ERASE, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
                newp->dtype_समुच्चयः_Void();
            }
        } अन्यथा यदि (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } अन्यथा यदि (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ध्रुव withp = विधानम्_With_Clause(
                nodep, असत्य, सत्य, शून्यसूचकः, nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            यदि (nodep->name() == "unique_index") {
                newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->कुञ्चिका_DTypep()));
            } अन्यथा {
                newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            }
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ध्रुव withp = विधानम्_With_Clause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ध्रुव withp = विधानम्_With_Clause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->कुञ्चिका_DTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "map") {
            AstWith* ध्रुव withp = विधानम्_With_Clause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                                    adtypep->कुञ्चिका_DTypep(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        यदि (newp) {
            newp->protect(असत्य);
            newp->did_विस्तारः(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* विधानम्_Call_Assoc_सूचकांक_व्यञ्जकम्(Ast_विधानम्_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_ग्रन्थिः* ध्रुव सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", सूचकांक_exprp, CONTEXT_DET, FINAL, adtypep->कुञ्चिका_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        प्रत्यागम nodep->argsp()->exprp();
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* विधानम्_Call_Wildcard_सूचकांक_व्यञ्जकम्(Ast_विधानम्_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ग्रन्थिः* ध्रुव सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", सूचकांक_exprp, CONTEXT_DET, FINAL, adtypep->findStringDType(),
                     EXTEND_EXP);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        प्रत्यागम nodep->argsp()->exprp();
    }
    शून्य विधानम्_Call_LValue_Recurse(Ast_विधानम्_Call* nodep, Ast_ग्रन्थिः* childp, ध्रुव VAccess& access) {
        यदि (AstCMethodHard* ध्रुव ichildp = VN_CAST(childp, CMethodHard)) {
            तार्किक methAt = असत्य;
            तार्किक meth_At_पृष्ठभागः = असत्य;
            तार्किक methAtWrite = असत्य;
            विकल्प (ichildp->विधानम्()) {
            पक्ष VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = सत्य;
                भङ्ग;
            पक्ष VCMethod::ARRAY_AT_पृष्ठभागः:  // FALLTHRU
                meth_At_पृष्ठभागः = सत्य;
                भङ्ग;
            पक्ष VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            पक्ष VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            पक्ष VCMethod::DYN_AT_WRITE_APPEND_पृष्ठभागः:  //
                methAtWrite = सत्य;
                भङ्ग;
            स्वभाव: भङ्ग;
            }
            यदि (methAt || meth_At_पृष्ठभागः || methAtWrite) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                यदि (VN_IS(fromDtypep, QueueDType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    यदि (methAt) {
                        ichildp->विधानम्(VCMethod::ARRAY_AT_WRITE);
                    } अन्यथा यदि (meth_At_पृष्ठभागः) {
                        ichildp->विधानम्(VCMethod::DYN_AT_WRITE_APPEND_पृष्ठभागः);
                    }
                }
                विधानम्_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                प्रत्यागम;
            }
        }
        यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(childp, ग्रन्थिः_Var_Ref)) {
            varrefp->access(access);
        } अन्यथा यदि (ध्रुव AstMemberSel* ध्रुव ichildp = VN_CAST(childp, MemberSel)) {
            विधानम्_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } अन्यथा यदि (ध्रुव AstStructSel* ध्रुव ichildp = VN_CAST(childp, StructSel)) {
            विधानम्_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_Sel* ध्रुव ichildp = VN_CAST(childp, ग्रन्थिः_Sel)) {
            विधानम्_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } अन्यथा यदि (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } अन्यथा {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* विधानम्_Call_Array(Ast_विधानम्_Call* nodep, Ast_ग्रन्थिः_DType* adtypep) {
        AstCMethodHard* newp = शून्यसूचकः;

        यदि (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = शून्यसूचकः;
            यदि (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = विधानम्_With_Clause(nodep, असत्य, सत्य, शून्यसूचकः, nodep->findUInt32DType(),
                                         adtypep->subDTypep());
            }
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ध्रुव withp = विधानम्_With_Clause(
                nodep, असत्य, सत्य, शून्यसूचकः, nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            यदि (nodep->name() == "unique_index") {
                newp->dtypep(newp->find_Queue_सूचकांक_DType());
            } अन्यथा {
                newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            }
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, सत्य, असत्य, nodep->findBitDType(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->find_Queue_सूचकांक_DType());
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, सत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(queue_DType_अनुक्रमितम्_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        }
        प्रत्यागम newp;
    }
    शून्य विधानम्_Call_Dyn(Ast_विधानम्_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = शून्यसूचकः;
        यदि (nodep->name() == "at") {  // Created internally for []
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा यदि (nodep->name() == "atWrite") {  // Created internally for []
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा यदि (nodep->name() == "size") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::DYN_आकार};
            newp->dtype_समुच्चयः_Int();
        } अन्यथा यदि (nodep->name() == "delete") {  // function void delete()
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि ((newp = विधानम्_Call_Array(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        यदि (newp) {
            newp->protect(असत्य);
            newp->did_विस्तारः(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    शून्य विधानम्_Call_Queue(Ast_विधानम्_Call* nodep, AstQueueDType* adtypep) {
        AstCMethodHard* newp = शून्यसूचकः;
        यदि (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा यदि (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा यदि (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::DYN_आकार};
            newp->dtype_समुच्चयः_Int();
        } अन्यथा यदि (nodep->name() == "delete") {  // function void delete([input integer index])
            विधानम्_Ok_Arguments(nodep, 0, 1);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            यदि (!nodep->argsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_समुच्चयः_Void();
            } अन्यथा {
                iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Queue_सूचकांक_व्यञ्जकम्(nodep);
                यदि (सूचकांक_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                              VCMethod::DYN_निष्कासय_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_समुच्चयः_Void();
                } अन्यथा {
                    newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                              VCMethod::DYN_ERASE, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
                    newp->dtype_समुच्चयः_Void();
                }
            }
        } अन्यथा यदि (nodep->name() == "insert") {
            विधानम्_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", विधानम्_Arg(nodep, 0), BOTH);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव सूचकांक_exprp = विधानम्_Call_Queue_सूचकांक_व्यञ्जकम्(nodep);
            AstArg* ध्रुव argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            यदि (सूचकांक_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::DYN_प्रक्षिप_FRONT, argp->exprp()->unlink_Fr_पृष्ठभागः()};
                newp->dtype_समुच्चयः_Void();
            } अन्यथा {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::DYN_निवेशय, सूचकांक_exprp->unlink_Fr_पृष्ठभागः()};
                newp->सङ्कलय_Pinsp(argp->exprp()->unlink_Fr_पृष्ठभागः());
                newp->dtype_समुच्चयः_Void();
            }
        } अन्यथा यदि (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } अन्यथा यदि (nodep->name() == "push_back" || nodep->name() == "push_front") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", विधानम्_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ध्रुव argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्(nodep->name()),
                                      argp->exprp()->unlink_Fr_पृष्ठभागः()};
            newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                      VCMethod::array_विधानम्("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
        } अन्यथा यदि ((newp = विधानम्_Call_Array(nodep, adtypep))) {
        } अन्यथा {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        यदि (newp) {
            newp->protect(असत्य);
            newp->did_विस्तारः(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* विधानम्_Call_Queue_सूचकांक_व्यञ्जकम्(Ast_विधानम्_Call* nodep) {
        Ast_ग्रन्थिः* ध्रुव सूचकांक_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", सूचकांक_exprp, BOTH);
        VL_DANGLING(सूचकांक_exprp);  // May have been edited
        प्रत्यागम nodep->argsp()->exprp();
    }
    शून्य विधानम्_Call_Warn_Timing(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep, ध्रुव std::string& className) {
        यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } अन्यथा यदि (!v3Global.opt.timing().is_समुच्चयः_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    शून्य विधानम्_Call_Iface_Ref(Ast_विधानम्_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ध्रुव ifacep = adtypep->iface_द्वारा_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
            = VN_CAST(m_memberMap.findMember(ifacep, nodep->name()), ग्रन्थिः_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            यदि (!modportAllowsItem(nodep, adtypep, ftaskp)) {
                nodep->dtype_समुच्चयः_Void();
                प्रत्यागम;
            }
            यदि (adtypep->isVirtual()) {
                प्रति (Ast_ग्रन्थिः* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    यदि (AstVar* ध्रुव mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, शून्यसूचकः);
            यदि (ftaskp->isStatic()) {
                AstArg* ध्रुव argsp = nodep->argsp();
                यदि (argsp) argsp->unlink_Fr_पृष्ठभागः_With_अग्र();
                Ast_ग्रन्थिः_FTask_Ref* newp = शून्यसूचकः;
                यदि (VN_IS(ftaskp, Task)) {
                    newp = नूतन AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } अन्यथा {
                    newp = नूतन AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            } अन्यथा {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                यदि (VN_IS(ftaskp, Task)) nodep->dtype_समुच्चयः_Void();
                process_FTask_Ref_तर्काः(nodep);
            }
            प्रत्यागम;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    शून्य handle_Randomize_तर्काः(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep, AstClass* ध्रुव classp) {
        तार्किक has_Non_Null_तर्काः = असत्य;
        AstConst* nullp = शून्यसूचकः;
        प्रति (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = शून्यसूचकः;
            Ast_ग्रन्थिः_व्यञ्जकम्* exprp = argp->exprp();
            यदि (AstConst* ध्रुव constp = VN_CAST(exprp, Const)) {
                यदि (constp->अङ्कः().isNull()) {
                    nullp = constp;
                    अनुवर्त;
                }
            }
            has_Non_Null_तर्काः = सत्य;
            AstVar* fromVarp = शून्यसूचकः;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            यदि (Ast_विधानम्_Call* विधानम्_Callp = VN_CAST(nodep, विधानम्_Call)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* fromp = विधानम्_Callp->fromp();
                यावत् (AstMemberSel* ध्रुव memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ध्रुव varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            यदि (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
                अनुवर्त;
            }
            यावत् (exprp) {
                यदि (AstMemberSel* ध्रुव memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } अन्यथा {
                    यदि (AstVarRef* ध्रुव varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } अन्यथा {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
                    }
                    exprp = शून्यसूचकः;
                }
                // All variables in the dot hierarchy must be randomizable
                यदि (randVarp && !randVarp->isRand()) randVarp->यादृच्छिकम्(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            यदि (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } अन्यथा यदि (classp->existsMember([&](ध्रुव AstClass*, ध्रुव AstVar* memberVarp) {
                           प्रत्यागम memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } अन्यथा {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), argp);
            }
        }
        यदि (nullp && has_Non_Null_तर्काः) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    शून्य विधानम्_Call_Class(Ast_विधानम्_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ध्रुव first_classp = adtypep->classp();
        AstWith* withp = शून्यसूचकः;
        यदि (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                     adtypep->findBitDType(), adtypep);
            प्रति (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                यदि (argp->exprp()) userIterate(argp->exprp(), विस्तारः_VP{SELF, BOTH}.p());
            }
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, first_classp);
            handle_Randomize_तर्काः(nodep, first_classp);
        } अन्यथा यदि (nodep->name() == "srandom") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, first_classp);
        }
        UASSERT_OBJ(first_classp, nodep, "Unlinked");
        प्रति (AstClass* classp = first_classp; classp;) {
            यदि (nodep->fileline()->timingOn()) {
                यदि (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ध्रुव packagep = getItemPackage(classp);
                    // Check if it's std
                    यदि (packagep && packagep->name() == "std") {
                        यदि (classp->name() == "process") {
                            विधानम्_Call_Warn_Timing(nodep, "process");
                        } अन्यथा यदि (classp->name() == "semaphore" && nodep->name() == "get") {
                            विधानम्_Call_Warn_Timing(nodep, "semaphore");
                        } अन्यथा यदि (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            विधानम्_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                = VN_CAST(m_memberMap.findMember(classp, nodep->name()), ग्रन्थिः_FTask)) {
                userIterate(ftaskp, शून्यसूचकः);
                यदि (ftaskp->isStatic()) {
                    AstArg* ध्रुव argsp = nodep->argsp();
                    यदि (argsp) argsp->unlink_Fr_पृष्ठभागः_With_अग्र();
                    Ast_ग्रन्थिः_FTask_Ref* newp = शून्यसूचकः;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    यदि (!m_vup) {  // Called as task
                        newp = नूतन AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } अन्यथा {
                        newp = नूतन AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                } अन्यथा {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    यदि (VN_IS(ftaskp, Task)) {
                        यदि (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_समुच्चयः_Void();
                    }
                    यदि (withp) nodep->withp(withp);
                    process_FTask_Ref_तर्काः(nodep);
                }
                प्रत्यागम;
            } अन्यथा यदि (nodep->name() == "get_randstate") {
                विधानम्_Ok_Arguments(nodep, 0, 0);
                first_classp->आधारः_Most_Classp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                         VCMethod::RNG_GET_RANDSTATE, शून्यसूचकः};
                newp->usePtr(सत्य);
                newp->dtype_समुच्चयः_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा यदि (nodep->name() == "set_randstate") {
                विधानम्_Ok_Arguments(nodep, 1, 1);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->argsp()->exprp();
                first_classp->आधारः_Most_Classp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                         VCMethod::RNG_समुच्चयः_RANDSTATE, exprp->unlink_Fr_पृष्ठभागः()};
                newp->usePtr(सत्य);
                newp->dtype_समुच्चयः_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा यदि (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(सत्य);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः;
        }
        {
            VSpellCheck speller;
            प्रति (AstClass* classp = first_classp; classp;) {
                प्रति (Ast_ग्रन्थिः* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    यदि (VN_IS(itemp, ग्रन्थिः_FTask)) speller.प्रक्षिप_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : शून्यसूचकः;
            }
            ध्रुव string suggest = speller.best_Candidate_सन्देशः(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << first_classp->verilogKwd() << " " << first_classp->prettyNameQ()
                           << "\n"
                           << (suggest.रिक्तानि() ? "" : nodep->fileline()->warn_अधिकम्() + suggest));
        }
        nodep->dtype_समुच्चयः_Int();  // Guess on error
    }
    शून्य विधानम्_Call_Constraint(Ast_विधानम्_Call* nodep, AstConstraintRefDType*) {
        यदि (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            विधानम्_Ok_Arguments(nodep, 0, 1);
            यदि (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            } अन्यथा {
                nodep->dtype_समुच्चयः_Void();
            }
            v3Global.useRandomizeMethods(सत्य);
        } अन्यथा {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य विधानम्_Call_Rand_बहुलकम्(Ast_विधानम्_Call* nodep) {
        विधानम्_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        यदि (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            nodep->dtype_समुच्चयः_Void();
        } अन्यथा {
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(सत्य);
    }
    शून्य विधानम्_Call_Unpack(Ast_विधानम्_Call* nodep, AstUnpackArrayDType* adtypep) {
        गणना : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_योग,
            ARRAY_गुणनफलम्
        } विधानम्_Id;

        विधानम्_Id = UNKNOWN;
        यदि (nodep->name() == "or") {
            विधानम्_Id = ARRAY_OR;
        } अन्यथा यदि (nodep->name() == "and") {
            विधानम्_Id = ARRAY_AND;
        } अन्यथा यदि (nodep->name() == "xor") {
            विधानम्_Id = ARRAY_XOR;
        } अन्यथा यदि (nodep->name() == "sum") {
            विधानम्_Id = ARRAY_योग;
        } अन्यथा यदि (nodep->name() == "product") {
            विधानम्_Id = ARRAY_गुणनफलम्;
        }

        यदि (विधानम्_Id) {
            AstWith* ध्रुव withp
                = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->subDTypep(),
                                   nodep->findUInt32DType(), adtypep->subDTypep());
            विधानम्_Ok_Arguments(nodep, 0, 0);
            यदि (withp) {
                विधानम्_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(),
                                         VCMethod::array_विधानम्("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                यदि (!nodep->firstAbovep()) newp->dtype_समुच्चयः_Void();
                newp->protect(असत्य);
                newp->did_विस्तारः(सत्य);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
                प्रत्यागम;
            } अन्यथा {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
                Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
                प्रति (पूर्णाङ्क i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव arrayRef = nodep->fromp()->clone_वृक्षः_विशुद्धम्(असत्य);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव selector = नूतन AstArraySel{fl, arrayRef, i};
                    यदि (!newp) {
                        newp = selector;
                    } अन्यथा {
                        विकल्प (विधानम्_Id) {
                        पक्ष ARRAY_OR: newp = नूतन AstOr{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_AND: newp = नूतन AstAnd{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_XOR: newp = नूतन AstXor{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_योग: newp = नूतन Ast_सङ्कलय{fl, newp, selector}; भङ्ग;
                        पक्ष ARRAY_गुणनफलम्: newp = नूतन AstMul{fl, newp, selector}; भङ्ग;
                        स्वभाव: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            }
        } अन्यथा यदि (AstCMethodHard* newp = विधानम्_Call_Array(nodep, adtypep)) {
            newp->protect(असत्य);
            newp->did_विस्तारः(सत्य);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    शून्य विधानम्_Call_Event(Ast_विधानम्_Call* nodep, AstBasicDType*) {
        // Method call on event
        यदि (nodep->name() == "triggered") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ध्रुव callp = नूतन AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_समुच्चयः_Bit();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    शून्य विधानम्_Call_String(Ast_विधानम्_Call* nodep, AstBasicDType*) {
        // Method call on string
        यदि (nodep->name() == "len") {
            // Constant value
            विधानम्_Ok_Arguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        } अन्यथा यदि (nodep->name() == "itoa") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } अन्यथा यदि (nodep->name() == "hextoa") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } अन्यथा यदि (nodep->name() == "octtoa") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } अन्यथा यदि (nodep->name() == "bintoa") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } अन्यथा यदि (nodep->name() == "realtoa") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } अन्यथा यदि (nodep->name() == "tolower") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन Ast_गन्तव्यम्_अधस्तनम्_N{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->name() == "toupper") {
            विधानम्_Ok_Arguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन Ast_गन्तव्यम्_ऊर्ध्वस्थम्_N{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->name() == "compare" || nodep->name() == "icompare") {
            ध्रुव तार्किक ignoreCase = nodep->name()[0] == 'i';
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            AstArg* ध्रुव argp = nodep->argsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhs = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhs = argp->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->name() == "putc") {
            विधानम्_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", विधानम्_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", विधानम्_Arg(nodep, 1), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            AstArg* ध्रुव arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ग्रन्थिः_Var_Ref* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), VarRef);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thsp = arg1p->exprp()->unlink_Fr_पृष्ठभागः();
            AstVarRef* ध्रुव varrefp
                = नूतन AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ग्रन्थिः* ध्रुव newp = नूतन Ast_नियोजय{
                nodep->fileline(), fromp, नूतन AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            प्रक्षिप_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } अन्यथा यदि (nodep->name() == "getc") {
            विधानम्_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", विधानम्_Arg(nodep, 0), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->name() == "substr") {
            विधानम्_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", विधानम्_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", विधानम्_Arg(nodep, 1), BOTH);
            AstArg* ध्रुव arg0p = nodep->argsp();
            AstArg* ध्रुव arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->fromp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = arg0p->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव thsp = arg1p->exprp()->unlink_Fr_पृष्ठभागः();
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा यदि (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            यदि (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } अन्यथा यदि (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } अन्यथा यदि (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } अन्यथा यदि (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } अन्यथा यदि (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } अन्यथा {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            विधानम्_Ok_Arguments(nodep, 0, 0);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_पृष्ठभागः(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    AstQueueDType* queue_DType_अनुक्रमितम्_By(Ast_ग्रन्थिः_DType* सूचकांक_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        यदि (AstQueueDType* ध्रुव queuep = m_queue_DType_अनुक्रमितम्[सूचकांक_DTypep]) {
            प्रत्यागम queuep;
        } अन्यथा {
            AstQueueDType* ध्रुव newp
                = नूतन AstQueueDType{सूचकांक_DTypep->fileline(), सूचकांक_DTypep, शून्यसूचकः};
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newp);
            m_queue_DType_अनुक्रमितम्[सूचकांक_DTypep] = newp;
            प्रत्यागम newp;
        }
    }

    शून्य दर्शय(AstNew* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        AstClass* classp = शून्यसूचकः;
        तार्किक नियोजय_2 = असत्य;
        यदि (VN_IS(nodep->backp(), नियोजय)) {  // assignment case
            नियोजय_2 = सत्य;
            Ast_ग्रन्थिः* warnp = शून्यसूचकः;
            AstClassRefDType* refp = शून्यसूचकः;

            यदि (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                यदि (AstClass* ध्रुव clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ध्रुव adtypep
                        = नूतन AstClassRefDType{nodep->fileline(), clsp, शून्यसूचकः};
                    v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(adtypep);
                    refp = adtypep;
                }
            } अन्यथा {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : शून्यसूचकः;
            }
            यदि (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ग्रन्थिः_DType)
                           ? ("data type "s + VN_AS(warnp, ग्रन्थिः_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                प्रत्यागम;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                = VN_CAST(m_memberMap.findMember(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } अन्यथा {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } अन्यथा {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_तर्काः(nodep);
        यदि (!नियोजय_2) nodep->dtypeFrom(nodep->taskp());
    }
    शून्य दर्शय(AstNewCopy* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        AstClassRefDType* ध्रुव refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        यदि (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            प्रत्यागम;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        यदि (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    शून्य दर्शय(AstNewDynamic* nodep) override {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        AstDynArrayDType* ध्रुव adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        यदि (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            प्रत्यागम;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        यदि (!VN_IS(nodep->backp(), ग्रन्थिः_नियोजय)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            प्रत्यागम;
        }
        nodep->dtypep(adtypep);
        यदि (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        यदि (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ध्रुव consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            यदि (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlink_Fr_पृष्ठभागः());
                nodep->replaceWith(consp->unlink_Fr_पृष्ठभागः());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }

    शून्य दर्शय(Ast_Tagged_व्यञ्जकम्* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_समुच्चयः_Bit();
        userIterateChildren(nodep, m_vup);
    }
    शून्य दर्शय(Ast_Tagged_आदर्शः* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_समुच्चयः_Bit();
        userIterateChildren(nodep, m_vup);
    }
    शून्य दर्शय(Ast_आदर्शः_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_समुच्चयः_Bit();
    }
    शून्य दर्शय(Ast_आदर्शः_तारकम्* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_समुच्चयः_Bit();
    }
    शून्य दर्शय(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_समुच्चयः_Bit();
        userIterateChildren(nodep, m_vup);
    }

    शून्य दर्शय(Ast_आदर्शः* nodep) override {
        यदि (m_अन्तर्_Param_Override && !nodep->childDTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_अन्तर्_Param_Override);
            प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_अन्तर्_Param_Override = असत्य;
                user_Iterate_And_अग्र(patp->repp(), विस्तारः_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                यदि (!VN_IS(patp->keyp(), Text) && !VN_IS(patp->keyp(), ग्रन्थिः_DType)) {
                    user_Iterate_And_अग्र(patp->keyp(), विस्तारः_VP{SELF, BOTH}.p());
                }
                प्रति (Ast_ग्रन्थिः *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_अन्तर्_Param_Override = VN_IS(valuep, आदर्शः);
                    userIterate(valuep, विस्तारः_VP{SELF, BOTH}.p());
                }
            }
            प्रत्यागम;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_अन्तर्_Param_Override);
        m_अन्तर्_Param_Override = असत्य;
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        UINFO(9, "PATTERN " << nodep);
        यदि (nodep->childDTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        यदि (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ग्रन्थिः_DType* dtypep = nodep->dtypep();
        यदि (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            यदि (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                प्रत्यागम;
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            यदि (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                प्रत्यागम;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ध्रुव पूर्णाङ्क times = दर्शय_Pat_Member_Rep(patp);
                प्रति (पूर्णाङ्क i = 1; i < times; i++) {
                    AstPatMember* ध्रुव newp = patp->clone_वृक्षः(असत्य);
                    patp->सङ्कलय_अग्र_अत्र(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                यदि (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_पृष्ठभागः(&relinkHandle);
                    यावत् (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव movep = VN_AS(patp->lhssp()->nextp(), ग्रन्थिः_व्यञ्जकम्)) {
                        movep->unlink_Fr_पृष्ठभागः();  // Not unlinkFrBackWithNext, just one
                        Ast_ग्रन्थिः* newkeyp = शून्यसूचकः;
                        यदि (patp->keyp()) newkeyp = patp->keyp()->clone_वृक्षः(सत्य);
                        AstPatMember* ध्रुव newp
                            = नूतन AstPatMember{patp->fileline(), movep, newkeyp, शून्यसूचकः};
                        patp->सङ्कलय_अग्र(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = शून्यसूचकः;
            प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ध्रुव nextp = VN_AS(patp->nextp(), PatMember);
                यदि (patp->isDefault()) {
                    यदि (defaultp) {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_पृष्ठभागः();
                }
                patp = nextp;
            }
            यावत् (ध्रुव AstConstDType* ध्रुव vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, विस्तारः_VP{SELF, BOTH}.p());

            यदि (स्वचालित* ध्रुव vdtypep = VN_CAST(dtypep, StructDType)) {
                आदर्शः_Struct(nodep, vdtypep, defaultp);
            } अन्यथा यदि (स्वचालित* ध्रुव vdtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                आदर्शः_Array(nodep, vdtypep, defaultp);
            } अन्यथा यदि (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ध्रुव consp = नूतन AstConsAssoc{nodep->fileline()};
                आदर्शः_Keyed<AstConsAssocItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा यदि (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ध्रुव consp = नूतन AstConsWildcard{nodep->fileline()};
                आदर्शः_Keyed<AstConsWildcardItem>(nodep, consp, dtypep, defaultp);
            } अन्यथा यदि (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, QueueDType)) {
                आदर्शः_Dyn_Array_Or_Queue(nodep, dtypep, defaultp);
            } अन्यथा यदि (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                आदर्शः_Basic(nodep, dtypep, defaultp);
            } अन्यथा {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalse{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            यदि (defaultp) VL_DO_DANGLING(प्रक्षिप_Deletep(defaultp), defaultp);
        }
    }
    शून्य आदर्शः_Struct(Ast_आदर्शः* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        उपयुज्य PatMap = std::map<ध्रुव AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ध्रुव AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            यावत् (patp) {
                कुरु {
                    यदि (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        यदि (ध्रुव AstText* textp = VN_CAST(patp->keyp(), Text)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.findMember(vdtypep, textp->text()),
                                           MemberDType);
                            यदि (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->text() << "' not found as member");
                                भङ्ग;
                            } अन्यथा {
                                ध्रुव std::pair<PatMap::iterator, तार्किक> ret
                                    = patmap.emplace(memp, patp);
                                यदि (!ret.second) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), Text)->text());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } अन्यथा यदि (Ast_ग्रन्थिः_DType* nodedtypep = VN_CAST(patp->keyp(), ग्रन्थिः_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, विस्तारः_VP{SELF, BOTH}.p());
                            ध्रुव string dtype = nodedtypep->dtypep()->prettyDTypeName(सत्य);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } अन्यथा {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } अन्यथा यदि (memp) {
                        // constant expr
                        ध्रुव std::pair<PatMap::iterator, तार्किक> ret = patmap.emplace(memp, patp);
                        यदि (!ret.second) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), Text)->text());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } अन्यथा {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } यावत् (असत्य);

                //अग्र
                यदि (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        यदि (vdtypep->packed()) {
            प्रति (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ध्रुव स्वचालित it = patmap.find(memp);
                यदि (it == patmap.अन्तः_2()) {  // default or default_type assignment
                    यदि (AstStructDType* ध्रुव memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_आदर्शः_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } अन्यथा {
                        AstPatMember* ध्रुव patp
                            = default_Patp_आदर्शः_Struct(nodep, memp, defaultp, dtypemap);
                        newp = मानम्_Concat_आदर्शः_Struct(patp, newp, memp, nodep);
                        यदि (patp) VL_DO_DANGLING(प्रक्षिप_Deletep(patp), patp);
                    }
                } अन्यथा {  // member assignment
                    newp = मानम्_Concat_आदर्शः_Struct(it->second, newp, memp, nodep);
                }
            }
        } अन्यथा {  // Unpacked
            AstConsStruct* ध्रुव consp = नूतन AstConsStruct{nodep->fileline(), vdtypep};
            प्रति (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ध्रुव स्वचालित it = patmap.find(memp);
                AstPatMember* patp = शून्यसूचकः;
                यदि (it == patmap.अन्तः_2()) {  // Default or default_type assignment
                    patp = default_Patp_आदर्शः_Struct(nodep, memp, defaultp, dtypemap);
                    प्रक्षिप_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    प्रक्षिप_Deletep(patp);  // patp used below
                } अन्यथा {
                    patp = it->second;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = आदर्शः_Member_मानम्_Iterate(patp);
                consp->सङ्कलय_Itemsp(नूतन AstConsStructItem{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        यदि (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* nestedvalue_Concat_आदर्शः_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ग्रन्थिः_व्यञ्जकम्* newp,
                                                 Ast_आदर्शः* nodep, ध्रुव DTypeMap& dtypemap) {
        प्रति (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            यदि (AstStructDType* ध्रुव memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_आदर्शः_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } अन्यथा {
                AstPatMember* ध्रुव patp
                    = default_Patp_आदर्शः_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = मानम्_Concat_आदर्शः_Struct(patp, newp, memp_nested, nodep);
                यदि (patp) VL_DO_DANGLING(प्रक्षिप_Deletep(patp), patp);
            }
        }
        प्रत्यागम newp;
    }

    AstPatMember* default_Patp_आदर्शः_Struct(Ast_आदर्शः* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ध्रुव DTypeMap& dtypemap) {
        ध्रुव string memp_DType = memp->virtRefDTypep()->prettyDTypeName(सत्य);
        ध्रुव स्वचालित it = dtypemap.find(memp_DType);
        यदि (it != dtypemap.अन्तः_2()) {
            // default_value for data_type
            प्रत्यागम it->second->clone_वृक्षः(असत्य);
        }
        यदि (defaultp) {
            // default_value for any unmatched member yet
            प्रत्यागम defaultp->clone_वृक्षः(असत्य);
        }
        यदि (memp->valuep()) {
            प्रत्यागम नूतन AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_वृक्षः(असत्य), ग्रन्थिः_व्यञ्जकम्),
                                    नूतन AstText{nodep->fileline(), memp->name()}, शून्यसूचकः};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        प्रत्यागम शून्यसूचकः;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* मानम्_Concat_आदर्शः_Struct(AstPatMember* patp, Ast_ग्रन्थिः_व्यञ्जकम्* newp,
                                           AstMemberDType* memp, Ast_आदर्शः* nodep) {
        यदि (patp) {
            patp->dtypep(memp);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = आदर्शः_Member_मानम्_Iterate(patp);
            यदि (!newp) {
                newp = valuep;
            } अन्यथा {
                AstConcat* ध्रुव concatp = नूतन AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_संहिता::WIDTHCONCAT, सत्य);
                valuep->fileline()->modifyWarnOff(V3_Error_संहिता::WIDTHCONCAT, सत्य);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        प्रत्यागम newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ग्रन्थिः_DType* elemDTypep,
                                       ध्रुव DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = defaultp->lhssp()->clone_वृक्षः(असत्य);
        Ast_ग्रन्थिः_DType* ध्रुव elemDTypeSkipRefp = elemDTypep->skipRefp();
        ध्रुव AstStructDType* ध्रुव structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ध्रुव तार्किक unpackedStruct = structp && !structp->packed();

        यदि (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            प्रत्यागम defaultp->clone_वृक्षः(असत्य);
        }
        यदि (VN_IS(valuep, आदर्शः)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            प्रत्यागम defaultp->clone_वृक्षः(असत्य);
        }
        यदि (!valuep->dtypep()) userIterate(valuep, विस्तारः_VP{SELF, BOTH}.p());
        तार्किक सम्पूर्णम्_Element = unpackedStruct;
        यदि (Ast_ग्रन्थिः_DType* ध्रुव मानम्_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            सम्पूर्णम्_Element = unpackedStruct ? !मानम्_DTypep->skipRefp()->isIntegralOrPacked()
                                          : मानम्_DTypep->similarDType(elemDTypeSkipRefp);
        }
        यदि (सम्पूर्णम्_Element) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            प्रत्यागम defaultp->clone_वृक्षः(असत्य);
        }

        AstPatMember* ध्रुव nestedDefaultp
            = नूतन AstPatMember{defaultp->fileline(), valuep, शून्यसूचकः, शून्यसूचकः};
        nestedDefaultp->isDefault(सत्य);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        प्रति (ध्रुव स्वचालित& प्रविष्टिः : dtypemap) nestedDefaultp->सङ्कलय_अग्र(प्रविष्टिः.second->clone_वृक्षः(असत्य));
        Ast_आदर्शः* ध्रुव recursivePatternp = नूतन Ast_आदर्शः{defaultp->fileline(), nestedDefaultp};
        प्रत्यागम नूतन AstPatMember{defaultp->fileline(), recursivePatternp, शून्यसूचकः, शून्यसूचकः};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ग्रन्थिः_व्यञ्जकम्* आदर्शः_Default_Valuep(AstPatMember* defaultp, Ast_ग्रन्थिः_DType* elemDTypep) {
        यदि (!defaultp) प्रत्यागम शून्यसूचकः;
        AstPatMember* ध्रुव elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = आदर्शः_Member_मानम्_Iterate(elemDefaultp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(elemDefaultp), elemDefaultp);
        प्रत्यागम valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    शून्य आदर्शः_Concat_Array(Ast_आदर्शः* nodep, AstUnpackArrayDType* arrayDtp) {
        ध्रुव VNumRange range = arrayDtp->declRange();
        ध्रुव पूर्णाङ्क elements = range.elements();
        AstConsArray* ध्रुव newp = नूतन AstConsArray{nodep->fileline(), arrayDtp};
        पूर्णाङ्क entn = 0;  // Number of elements given so far, from the left
        तार्किक error = असत्य;  // Reported an error, so the value is not complete
        प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = आदर्शः_Member_मानम्_Iterate(patp);
            ध्रुव uint64_t सूचकांक = range.ascending() ? entn : elements - 1 - entn;
            ध्रुव AstUnpackArrayDType* ध्रुव spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            यदि (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_मानानि(सूचकांक, valuep);
                अनुवर्त;
            }
            ध्रुव VCastable castable = Ast_ग्रन्थिः::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            यदि (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = सत्य;
            }
            newp->valuep(सूचकांक, valuep);
            ++entn;
        }
        यदि (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = सत्य;
        }
        यदि (error) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}});
        } अन्यथा {
            nodep->replaceWith(newp);
        }
    }
    शून्य आदर्शः_Array(Ast_आदर्शः* nodep, Ast_ग्रन्थिः_Array_DType* arrayDtp, AstPatMember* defaultp) {
        यदि (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            आदर्शः_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            प्रत्यागम;
        }
        ध्रुव VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_सदिशम्_Map(nodep, range);
        UINFO(9, "ent " << range.वामतः() << " to " << range.दक्षिणतः());
        Ast_ग्रन्थिः* newp = शून्यसूचकः;
        तार्किक missed = असत्य;  // Some element missed, so not a complete value
        प्रति (पूर्णाङ्क entn = 0, ent = range.वामतः(); entn < range.elements();
             ++entn, ent += range.वामः_गन्तव्यम्_दक्षिणः_Inc()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;
            ध्रुव स्वचालित it = patmap.find(ent);
            यदि (it == patmap.अन्तः_2()) {
                यदि (defaultp) {
                    valuep = आदर्शः_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = सत्य;
                }
            } अन्यथा {
                AstPatMember* ध्रुव patp = it->second;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = आदर्शः_Member_मानम्_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ध्रुव AstUnpackArrayDType* ध्रुव memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                यदि (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_अधिकम्()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                    valuep = नूतन AstConst{patp->fileline(), AstConst::BitFalseErroring{}};
                }
            }

            यदि (valuep) {
                यदि (VN_IS(arrayDtp, UnpackArrayDType)) {
                    यदि (!newp) newp = नूतन AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } अन्यथा {  // Packed. Convert to concat for now.
                    यदि (!newp) {
                        newp = valuep;
                    } अन्यथा {
                        AstConcat* ध्रुव concatp
                            = नूतन AstConcat{valuep->fileline(), VN_AS(newp, ग्रन्थिः_व्यञ्जकम्), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        यदि (!patmap.रिक्तानि()) nodep->v3error("Assignment pattern with too many elements");
        यदि (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(newp), newp);
            newp = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
        }
        यदि (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* आदर्शः_Const_Keyp(AstPatMember* patp) {
        यदि (VN_IS(patp->keyp(), ग्रन्थिः_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            प्रत्यागम शून्यसूचकः;
        }
        यदि (patp->कुञ्चिका_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->कुञ्चिका_Exprp());  // May change
        AstConst* ध्रुव constp = VN_CAST(patp->कुञ्चिका_Exprp(), Const);
        यदि (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        प्रत्यागम constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* आदर्शः_Scalar_Keyp(AstPatMember* patp, Ast_ग्रन्थिः_DType* कुञ्चिका_DTypep) {
        यदि (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            प्रत्यागम शून्यसूचकः;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        यदि (कुञ्चिका_DTypep->skipRefp()->isIntegralOrPacked() && patp->कुञ्चिका_Exprp()) {
            userIterate(patp->कुञ्चिका_Exprp(), विस्तारः_VP{कुञ्चिका_DTypep, PRELIM}.p());
            ध्रुव पूर्णाङ्क विस्तारः = std::max(patp->कुञ्चिका_Exprp()->विस्तारः(), कुञ्चिका_DTypep->विस्तारः());
            Ast_ग्रन्थिः_DType* ध्रुव contextDTypep
                = कुञ्चिका_DTypep->findBitOrLogicDType(विस्तारः, विस्तारः, कुञ्चिका_DTypep->numeric(), सत्य);
            iterateCheck(patp, "key", patp->कुञ्चिका_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, असत्य);
        }
        AstConst* ध्रुव constp = आदर्शः_Const_Keyp(patp);
        यदि (!constp) प्रत्यागम शून्यसूचकः;
        यदि (constp->अङ्कः().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            प्रत्यागम शून्यसूचकः;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, कुञ्चिका_DTypep, EXTEND_EXP, असत्य);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव convp = patp->कुञ्चिका_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        प्रत्यागम VN_AS(keyp->unlink_Fr_पृष्ठभागः(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ग्रन्थिः_व्यञ्जकम्* आदर्शः_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        Ast_ग्रन्थिः_DType* ध्रुव कुञ्चिका_DTypep = VN_AS(arrayDtp, AssocArrayDType)->कुञ्चिका_DTypep();
        यदि (!कुञ्चिका_DTypep->skipRefp()->isAggregateType() || !patp->कुञ्चिका_Exprp()) {
            प्रत्यागम आदर्शः_Scalar_Keyp(patp, कुञ्चिका_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->कुञ्चिका_Exprp(), कुञ्चिका_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->कुञ्चिका_Exprp());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव foldedp = patp->कुञ्चिका_Exprp();
        यदि (!foldedp->is_Constant_वृक्षः()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            प्रत्यागम शून्यसूचकः;
        }
        यदि (foldedp->exists([](ध्रुव AstConst* constp) { प्रत्यागम constp->अङ्कः().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम foldedp->unlink_Fr_पृष्ठभागः();
    }
    AstConst* आदर्शः_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        प्रत्यागम आदर्शः_Scalar_Keyp(patp, arrayDtp->findStringDType());
    }
    AstConst* आदर्शः_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_ग्रन्थिः_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        प्रत्यागम आदर्शः_Scalar_Keyp(patp, arrayDtp->findBasicDType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    प्रतिरूपम् <प्रकारनाम T_Item, प्रकारनाम T_Cons>
    शून्य क्रमय_Cons_वस्तूनि(T_Cons* nodep) {
        std::सदिशम्<T_Item*> itemps;
        यावत् (T_Item* ध्रुव itemp = nodep->itemsp()) {
            itemp->unlink_Fr_पृष्ठभागः();
            itemps.प्रक्षिप_पृष्ठभागः(itemp);
        }
        ध्रुव स्वचालित कुञ्चिका_सम्बन्धि = [](ध्रुव T_Item* itemp) -> ध्रुव V3_सङ्ख्या& {
            ध्रुव Ast_ग्रन्थिः* ध्रुव keyp = itemp->keyp();
            प्रत्यागम VN_AS(keyp, Const)->अङ्कः();
        };
        std::stable_क्रमय(itemps.begin(), itemps.अन्तः_2(), [&](ध्रुव T_Item* ap, ध्रुव T_Item* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            प्रत्यागम कुञ्चिका_सम्बन्धि(ap).isLtXZ(कुञ्चिका_सम्बन्धि(bp));
        });
        ध्रुव T_Item* firstp = शून्यसूचकः;  // First item with the current key
        प्रति (T_Item* ध्रुव itemp : itemps) {
            यदि (firstp && कुञ्चिका_सम्बन्धि(firstp).isCaseEq(कुञ्चिका_सम्बन्धि(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } अन्यथा {
                firstp = itemp;
            }
            nodep->सङ्कलय_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    प्रतिरूपम् <प्रकारनाम T_Item, प्रकारनाम T_Cons>
    शून्य आदर्शः_Keyed(Ast_आदर्शः* nodep, T_Cons* consp, Ast_ग्रन्थिः_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(आदर्शः_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ध्रुव स्वचालित keyp = आदर्शः_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = आदर्शः_Member_मानम्_Iterate(patp);
            यदि (keyp) {
                consp->सङ्कलय_Itemsp(नूतन T_Item{patp->fileline(), keyp, valuep});
            } अन्यथा {
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ध्रुव AstAssocArrayDType* ध्रुव assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        यदि (!assocDtp || !assocDtp->कुञ्चिका_DTypep()->skipRefp()->isAggregateType()) {
            क्रमय_Cons_वस्तूनि<T_Item>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* newConsDynArrayOrQueue(File_पङ्क्तिः* fileline, ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        यदि (VN_IS(dtypep, DynArrayDType)) प्रत्यागम नूतन AstConsDynArray{fileline};
        यदि (VN_IS(dtypep, QueueDType)) प्रत्यागम नूतन AstConsQueue{fileline};
        प्रत्यागम शून्यसूचकः;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    प्रतिरूपम् <प्रकारनाम T_Item, प्रकारनाम T_Cons>
    शून्य सङ्कलय_Cons_Item(T_Cons* consp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        Ast_ग्रन्थिः_DType* ध्रुव arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ध्रुव तार्किक isElement
            = Ast_ग्रन्थिः::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), शून्यसूचकः)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ध्रुव AstQueueDType* ध्रुव subQueueDtp = VN_CAST(valuep->dtypep()->skipRefp(), QueueDType);
        यदि (!isElement && !(subQueueDtp && subQueueDtp->boundp())) {
            T_Cons* ध्रुव subp = Ast_ग्रन्थिः::cast<T_Cons>(valuep);
            यदि (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                यदि (subp->itemsp()) consp->सङ्कलय_Itemsp(subp->itemsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                VL_DO_DANGLING(प्रक्षिप_Deletep(subp), subp);
                प्रत्यागम;
            }
            यदि (AstConsQueue* ध्रुव queuep = VN_CAST(valuep, ConsQueue)) {
                सङ्कलय_Cons_वस्तूनि(consp, queuep);
                प्रत्यागम;
            }
            यदि (AstConsDynArray* ध्रुव dynp = VN_CAST(valuep, ConsDynArray)) {
                सङ्कलय_Cons_वस्तूनि(consp, dynp);
                प्रत्यागम;
            }
        }
        T_Item* ध्रुव itemp = नूतन T_Item{valuep->fileline(), valuep};
        consp->सङ्कलय_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        यदि (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } अन्यथा {
            user_Iterate_And_अग्र(valuep, विस्तारः_VP{arrayDtp, FINAL}.p());
        }
    }
    शून्य सङ्कलय_Cons_Item(Ast_ग्रन्थिः_व्यञ्जकम्* consp, Ast_ग्रन्थिः_व्यञ्जकम्* valuep) {
        यदि (AstConsDynArray* ध्रुव dynp = VN_CAST(consp, ConsDynArray)) {
            सङ्कलय_Cons_Item<AstConsDynArrayItem>(dynp, valuep);
        } अन्यथा {
            सङ्कलय_Cons_Item<AstConsQueueItem>(VN_AS(consp, ConsQueue), valuep);
        }
    }
    प्रतिरूपम् <प्रकारनाम T_Sub>
    शून्य सङ्कलय_Cons_वस्तूनि(Ast_ग्रन्थिः_व्यञ्जकम्* consp, T_Sub* subp) {
        यावत् (स्वचालित* ध्रुव itemp = subp->itemsp()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = itemp->valuep()->unlink_Fr_पृष्ठभागः();
            VL_DO_DANGLING(प्रक्षिप_Deletep(itemp->unlink_Fr_पृष्ठभागः()), itemp);
            सङ्कलय_Cons_Item(consp, valuep);
        }
        VL_DO_DANGLING(प्रक्षिप_Deletep(subp), subp);
    }
    शून्य आदर्शः_Dyn_Array_Or_Queue(Ast_आदर्शः* nodep, Ast_ग्रन्थिः_DType* arrayp, AstPatMember* defaultp) {
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = newConsDynArrayOrQueue(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ध्रुव AstPatMember* ध्रुव firstp = VN_AS(nodep->itemsp(), PatMember);
        ध्रुव तार्किक keyed = defaultp || (firstp && firstp->keyp());
        यदि (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            आदर्शः_Keyed<AstConsDynArrayItem>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            प्रत्यागम;
        }
        ध्रुव वर्ण* ध्रुव kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        यदि (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            यदि (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            सङ्कलय_Cons_Item(newp, आदर्शः_Member_मानम्_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    शून्य आदर्शः_Basic(Ast_आदर्शः* nodep, Ast_ग्रन्थिः_DType* vdtypep, AstPatMember* defaultp) {
        ध्रुव AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ध्रुव VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_सदिशम्_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ग्रन्थिः_व्यञ्जकम्* newp = शून्यसूचकः;
        प्रति (ध्रुव पूर्णाङ्क ent : range.seq_Hi_गन्तव्यम्_Lo()) {
            Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;
            ध्रुव स्वचालित it = patmap.find(ent);
            यदि (it == patmap.अन्तः_2()) {
                यदि (defaultp) {
                    valuep = आदर्शः_Default_Valuep(defaultp, nodep->findBitDType());
                } अन्यथा {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } अन्यथा {
                AstPatMember* ध्रुव patp = it->second;
                patmap.erase(it);
                patp->dtypep(nodep->findBitDType());
                valuep = आदर्शः_Member_मानम्_Iterate(patp);
            }
            यदि (valuep) {  // Packed. Convert to concat for now.
                यदि (!newp) {
                    newp = valuep;
                } अन्यथा {
                    AstConcat* ध्रुव concatp = नूतन AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        यदि (!patmap.रिक्तानि()) nodep->v3error("Assignment pattern with too many elements");
        यदि (newp) {
            nodep->replaceWith(newp);
        } अन्यथा {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* आदर्शः_Member_मानम्_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, विस्तारः_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = patp->lhssp()->unlink_Fr_पृष्ठभागः();
        यदि (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            यदि (AstConst* ध्रुव newp = V3_विस्तारः_Commit::new_If_Const_Commit_आकार(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        प्रत्यागम valuep;
    }

    स्थिर शून्य checkEventAssignment(ध्रुव Ast_ग्रन्थिः_नियोजय* ध्रुव asgnp) {
        string unsupEvtAsgn;
        यदि (!उपयुङ्क्ते_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        यदि (VN_IS(asgnp->rhsp(), CReset)) प्रत्यागम;
        यदि (asgnp->rhsp()->dtypep()->isEvent() && !उपयुङ्क्ते_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.रिक्तानि() ? "from" : " and from");
        }
        यदि (!unsupEvtAsgn.रिक्तानि()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_अधिकम्()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_अधिकम्()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    स्थिर तार्किक उपयुङ्क्ते_Dynamic_Scheduler(Ast_ग्रन्थिः* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        यावत् (सत्य) {
            AstVarRef* ध्रुव vrefp = VN_CAST(nodep, VarRef);
            यदि (vrefp) प्रत्यागम उपयुङ्क्ते_Dynamic_Scheduler(vrefp);
            यदि (VN_IS(nodep, MemberSel)) {
                प्रत्यागम सत्य;
            } अन्यथा यदि (Ast_ग्रन्थिः_Sel* selp = VN_CAST(nodep, ग्रन्थिः_Sel)) {
                nodep = selp->fromp();
            } अन्यथा {
                प्रत्यागम असत्य;
            }
        }
    }

    स्थिर तार्किक उपयुङ्क्ते_Dynamic_Scheduler(AstVarRef* vrefp) {
        प्रत्यागम VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    शून्य दर्शय(AstPatMember* nodep) override {
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), नियोजय_3, FINAL, vdtypep, EXTEND_LHS);
    }
    पूर्णाङ्क दर्शय_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        यदि (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep->repp(), Const);
            यदि (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } अन्यथा {
                times = constp->गन्तव्यम्_UInt();
            }
            यदि (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_पृष्ठभागः_With_अग्र()
                ->delete_वृक्षः();  // Done with replicate before cloning
        }
        प्रत्यागम times;
    }

    शून्य दर्शय(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ग्रन्थिः* propp = nodep->propp();
            यावत् (AstLogNot* ध्रुव notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ध्रुव untilp = VN_CAST(propp, Until);
            यदि (untilp && VN_IS(nodep->backp(), ग्रन्थिः_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_समुच्चयः_False() || !v3Global.opt.timing().is_समुच्चयः_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(नूतन AstConst{untilp->fileline(), AstConst::BitFalse{}});
                VL_DO_DANGLING(प्रक्षिप_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_अग्र(nodep->sensesp(), शून्यसूचकः);
            यदि (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_समुच्चयः_Bit();
        }
    }

    तार्किक firstNewStatementOkRecurse(Ast_ग्रन्थिः* nodep) {
        यदि (AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
            यदि (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) प्रत्यागम सत्य;
        }
        यदि (Ast_नियोजय* ध्रुव aitemp = VN_CAST(nodep, नियोजय)) {
            यदि (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) प्रत्यागम सत्य;
        }
        यदि (AstInitialStaticStmt* ध्रुव aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            प्रति (Ast_ग्रन्थिः* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (!firstNewStatementOkRecurse(stmtp)) प्रत्यागम असत्य;
            }
            प्रत्यागम सत्य;
        }
        यदि (AstInitialAutomaticStmt* ध्रुव aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            प्रति (Ast_ग्रन्थिः* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                यदि (!firstNewStatementOkRecurse(stmtp)) प्रत्यागम असत्य;
            }
            प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    //--------------------
    // Top levels

    प्रतिरूपम् <प्रकारनाम CaseItem>
    शून्य handleCaseType(Ast_ग्रन्थिः* casep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, CaseItem* itemsp) {
        Ast_Attr_सम्बन्धि* ध्रुव exprap = VN_CAST(exprp, Attr_सम्बन्धि);
        यदि (!exprap) प्रत्यागम;
        यदि (exprap->attrType() != VAttrType::TYPEID) प्रत्यागम;

        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव व्यञ्जकम्_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << व्यञ्जकम्_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_पङ्क्तिः* ध्रुव newfl = casep->fileline();
        newfl->warnOff(V3_Error_संहिता::CASEINCOMPLETE, सत्य);  // Side effect of transform
        newfl->warnOff(V3_Error_संहिता::CASEOVERLAP, सत्य);  // Side effect of transform
        casep->fileline(newfl);
        प्रति (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            यदि (itemp->isDefault()) अनुवर्त;
            तार्किक लब्धम् = असत्य;
            प्रति (Ast_ग्रन्थिः* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ध्रुव Ast_Attr_सम्बन्धि* ध्रुव condAttrp = VN_CAST(condp, Attr_सम्बन्धि);
                यदि (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } अन्यथा {
                    Ast_ग्रन्थिः_DType* ध्रुव condDtp = condAttrp->dtypep();
                    यदि (Ast_ग्रन्थिः::computeCastable(व्यञ्जकम्_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        लब्धम् = सत्य;
                        भङ्ग;
                    }
                }
            }
            प्रक्षिप_Deletep(itemp->condsp()->unlink_Fr_पृष्ठभागः_With_अग्र());
            // Item condition becomes constant 1 if hits else 0
            itemp->सङ्कलय_Condsp(नूतन AstConst{newfl, AstConst::BitTrue{}, लब्धम्});
        }
        exprap->replaceWith(नूतन AstConst{newfl, AstConst::BitTrue{}});
        VL_DO_DANGLING(प्रक्षिप_Deletep(exprap), exprap);
    }

    प्रतिरूपम् <प्रकारनाम CaseItem>
    शून्य handleCase(Ast_ग्रन्थिः* casep, Ast_ग्रन्थिः_व्यञ्जकम्* exprp, CaseItem* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ग्रन्थिः_DType* subDTypep = exprp->dtypep();
        प्रति (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            प्रति (Ast_ग्रन्थिः* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                यदि (condp->dtypep() == subDTypep) अनुवर्त;

                यदि (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->findDoubleDType();
                } अन्यथा यदि (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->findStringDType();
                } अन्यथा {
                    ध्रुव पूर्णाङ्क विस्तारः = std::max(subDTypep->विस्तारः(), condp->विस्तारः());
                    ध्रुव पूर्णाङ्क mwidth = std::max(subDTypep->विस्तारः_Min(), condp->विस्तारः_Min());
                    ध्रुव तार्किक issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->findBitOrLogicDType(
                        विस्तारः, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        प्रति (CaseItem* itemp = itemsp; itemp; itemp = Ast_ग्रन्थिः::as<CaseItem>(itemp->nextp())) {
            प्रति (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    शून्य दर्शय(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
        प्रति (AstGenCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), GenCaseItem);
            प्रति (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, विस्तारः_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य दर्शय(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->initsp(), शून्यसूचकः);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_अग्र(nodep->incsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    शून्य दर्शय(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
        प्रति (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_अग्र(itemp->stmtsp(), शून्यसूचकः);
            प्रति (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, विस्तारः_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    शून्य दर्शय(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ग्रन्थिः_DType* ध्रुव itemDTypep = nodep->findUInt32DType();
        प्रति (AstCaseItem *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), CaseItem);  // Prelim may cause the node to get replaced
            user_Iterate_And_अग्र(itemp->stmtsp(), शून्यसूचकः);
            प्रति (Ast_ग्रन्थिः *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, itemDTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    शून्य दर्शय(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->stmtsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    शून्य दर्शय(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    शून्य दर्शय(Ast_ग्रन्थिः_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_अग्र(nodep->thensp(), शून्यसूचकः);
        user_Iterate_And_अग्र(nodep->elsesp(), शून्यसूचकः);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    शून्य दर्शय(Ast_व्यञ्जकम्_Stmt* nodep) override {
        user_Iterate_And_अग्र(nodep->stmtsp(), शून्यसूचकः);
        // expected result is same as parent's expected result
        user_Iterate_And_अग्र(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Foreach* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        nodep->did_विस्तारः(सत्य);
        ध्रुव Ast_Foreach_शीर्षकम्* ध्रुव headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_अग्र(headerp->fromp(), विस्तारः_VP{SELF, BOTH}.p());
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ग्रन्थिः_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        प्रति (Ast_ग्रन्थिः *argsp = headerp->elementsp(), *अग्र_argsp; argsp; argsp = अग्र_argsp) {
            अग्र_argsp = argsp->nextp();
            ध्रुव तार्किक रिक्तानि = VN_IS(argsp, रिक्तानि_2);
            AstVar* ध्रुव varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || रिक्तानि, argsp, "Missing foreach loop variable");
            यदि (varp) varp->प्रयुक्तम्_Loop_Idx(सत्य);
            यदि (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
                प्रत्यागम;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            यदि (VN_IS(fromDtp, ग्रन्थिः_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, QueueDType)) {
                // Nothing special here
            } अन्यथा यदि (AstBasicDType* ध्रुव adtypep = VN_CAST(fromDtp, BasicDType)) {
                यदि (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
                    प्रत्यागम;
                }
            } अन्यथा यदि (ध्रुव AstAssocArrayDType* ध्रुव adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->कुञ्चिका_DTypep());
            } अन्यथा {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
                प्रत्यागम;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ग्रन्थिः* ध्रुव bodyp = nodep->bodyp();
        user_Iterate_And_अग्र(bodyp, शून्यसूचकः);
        यदि (AstForeach* ध्रुव loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_गन्तव्यम्_While(loopp), loopp, nodep);
            प्रत्यागम;
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः_नियोजय* nodep) override {
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
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->विस्तारः_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            यदि (ध्रुव AstIfaceRefDType* ध्रुव irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                यदि (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                    प्रत्यागम;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            प्रवाहः_नियोजय_अधस्तनम्(nodep);
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            यदि (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsDTypep->skipRefp();
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ध्रुव तार्किक lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ध्रुव तार्किक rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                यदि (lhsIsUnpackArray && rhsIsUnpackArray) {
                    यदि (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ग्रन्थिः_Var_Ref, Ast_ग्रन्थिः_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ग्रन्थिः_Var_Ref), VN_CAST(nodep->rhsp(), ग्रन्थिः_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                यदि (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ग्रन्थिः_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_नियोजय(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        यदि (VN_IS(nodep, नियोजय_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        यदि (Ast_ग्रन्थिः* ध्रुव controlp = nodep->timingControlp()) {
            यदि (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः(), controlp);
            } अन्यथा यदि (nodep->fileline()->timingOn() && v3Global.opt.timing().is_समुच्चयः_True()) {
                iterateNull(controlp);
            } अन्यथा {
                यदि (nodep->fileline()->timingOn()) {
                    यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } अन्यथा {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_पृष्ठभागः_With_अग्र()->delete_वृक्षः(), controlp);
            }
        }
        यदि (VN_IS(nodep->rhsp(), रिक्तानि_Queue)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            यदि (!VN_IS(lhsDtp, QueueDType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
                प्रत्यागम;
            }
            Ast_विधानम्_Call* ध्रुव newp
                = नूतन Ast_विधानम्_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(), "delete"};
            newp->dtype_समुच्चयः_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            // Need to now convert it
            दर्शय(newp);
            प्रत्यागम;
        }
        यदि (ध्रुव AstNewDynamic* ध्रुव dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            यदि (!dynp->rhsp()) {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_पृष्ठभागः()};
            } अन्यथा {
                newp = नूतन AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_पृष्ठभागः()};
                newp->सङ्कलय_Pinsp(dynp->rhsp()->unlink_Fr_पृष्ठभागः());
            }
            newp->did_विस्तारः(सत्य);
            newp->protect(असत्य);
            newp->dtype_समुच्चयः_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        यदि (AstCMethodHard* ध्रुव slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod नियोजय_विधानम्;
            यदि (slicep->विधानम्() == VCMethod::DYN_खण्डिका) {
                नियोजय_विधानम् = VCMethod::DYN_खण्डिका_नियोजय;
            } अन्यथा यदि (slicep->विधानम्() == VCMethod::DYN_खण्डिका_पृष्ठभागः_पृष्ठभागः) {
                नियोजय_विधानम् = VCMethod::DYN_खण्डिका_नियोजय_पृष्ठभागः_पृष्ठभागः;
            } अन्यथा यदि (slicep->विधानम्() == VCMethod::DYN_खण्डिका_FRONT_पृष्ठभागः) {
                नियोजय_विधानम् = VCMethod::DYN_खण्डिका_नियोजय_FRONT_पृष्ठभागः;
            } अन्यथा {
                नियोजय_विधानम् = VCMethod::_ENUM_MAX;  // not a slice
            }
            यदि (नियोजय_विधानम् != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = slicep->fromp()->unlink_Fr_पृष्ठभागः();
                // Collect existing slice index pins (lsb, msb)
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lsbp = slicep->pinsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव msbp = slicep->pinsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                AstCMethodHard* ध्रुव newp
                    = नूतन AstCMethodHard{nodep->fileline(), fromp, नियोजय_विधानम्};
                newp->सङ्कलय_Pinsp(lsbp);
                newp->सङ्कलय_Pinsp(msbp);
                newp->सङ्कलय_Pinsp(rhsp);
                newp->did_विस्तारः(सत्य);
                newp->protect(असत्य);
                newp->dtype_समुच्चयः_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            }
        }

        यदि (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.समुच्चयः_Assigns_Events();
        }
    }

    शून्य दर्शय(AstRSRule* nodep) override {
        यदि (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_अग्र(nodep->prodlistsp(), शून्यसूचकः);
        user_Iterate_And_अग्र(nodep->weightStmtsp(), शून्यसूचकः);
    }

    शून्य दर्शय(AstRelease* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->विस्तारः_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    शून्य दर्शय(AstDeassign* nodep) override {
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->विस्तारः_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    स्थिर तार्किक isFormatNonNumericArg(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        प्रत्यागम dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, QueueDType)  //
               || (VN_IS(dtypep, ग्रन्थिः_UOr_Struct_DType)
                   && !VN_AS(dtypep, ग्रन्थिः_UOr_Struct_DType)->packed());
    }

    शून्य checkFormatNumericArg(Ast_ग्रन्थिः* argp, वर्ण ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        यदि (std::tolower(ch) == 'd') प्रत्यागम;
        // In contrast, %x of a class reference, causes errors on several simulators.
        यदि (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_अधिकम्() << "... Suggest use '%s'");
    }

    शून्य दर्शय(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    शून्य दर्शय(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->fmtp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(Ast_गन्तव्यम्_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_समुच्चयः_String();
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    शून्य दर्शय(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        यदि (m_vup && !m_vup->prelim()) प्रत्यागम;  // Can be called as statement or function
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;
        //
        UINFO(9, "  Display in " << nodep->text());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        यदि (!nodep->व्यञ्जकम्_Format_2()) {  // Non-constant format, can't check format string types
            दर्शय_sformatf_format(nodep);
        }

        Ast_ग्रन्थिः_व्यञ्जकम्* oldExprsp = nodep->exprsp();
        यदि (nodep->व्यञ्जकम्_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ग्रन्थिः_व्यञ्जकम्);
        }
        यदि (oldExprsp) oldExprsp->unlink_Fr_पृष्ठभागः_With_अग्र();
        यावत् (Ast_ग्रन्थिः_व्यञ्जकम्* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ग्रन्थिः_व्यञ्जकम्);
            यदि (oldExprsp) oldExprsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            यदि (VN_IS(argp, SFormatArg)) {
                nodep->सङ्कलय_Exprsp(argp);
                अनुवर्त;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ग्रन्थिः_DType* ध्रुव dtypep = argp->dtypep()->skipRefp();
            यदि (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } अन्यथा यदि (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } अन्यथा यदि (isFormatNonNumericArg(dtypep)) {
                यदि (AstClassRefDType* ध्रुव classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    यदि (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(सत्य);
                    }
                } अन्यथा {
                    // Class handles inside containers print type names, not their members.
                    Ast_ग्रन्थिः_DType* ग्रन्थिः_Dtypep = dtypep;
                    यावत् (ग्रन्थिः_Dtypep) {
                        ग्रन्थिः_Dtypep = ग्रन्थिः_Dtypep->skipRefp();
                        यदि (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव uOrStructDTypep
                            = VN_CAST(ग्रन्थिः_Dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                            uOrStructDTypep->समुच्चयः_Emit_गन्तव्यम्_String();
                            v3Global.hasPrintedObjects(सत्य);
                        }
                        ग्रन्थिः_Dtypep = ग्रन्थिः_Dtypep->subDTypep();
                    }
                }
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन Ast_गन्तव्यम्_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } अन्यथा यदि (nodep->व्यञ्जकम्_Format_2()) {
                यदि (AstEnumDType* ध्रुव enumDtp = formatEnumDType(argp)) {
                    nodep->सङ्कलय_Exprsp(newFormatEnumArg(argp, enumDtp));
                    अनुवर्त;
                }
            }
            यदि (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            यदि (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->सङ्कलय_Exprsp(argp);
            } अन्यथा {
                nodep->सङ्कलय_Exprsp(नूतन AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->text());
    }
    शून्य दर्शय(AstCReset* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    शून्य दर्शय(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    शून्य दर्शय(AstConstraintRef* nodep) override { userIterateChildren(nodep, शून्यसूचकः); }
    शून्य दर्शय(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        यदि (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        यदि (!m_paramsOnly) {
            nodep->foreach([एतत्](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    नूतन AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string text = VString::dequotePercent(nodep->fmtp()->text());
            यदि (text.रिक्तानि()) text = "Elaboration system task message (IEEE 1800-2023 20.11)";
            विकल्प (nodep->displayType()) {
            पक्ष VDisplayType::DT_INFO: nodep->v3warn(USERINFO, text); भङ्ग;
            पक्ष VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, text); भङ्ग;
            पक्ष VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, text); भङ्ग;
            पक्ष VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, text); भङ्ग;
            स्वभाव: UASSERT_OBJ(असत्य, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
        }
    }
    शून्य दर्शय(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        यदि (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    शून्य दर्शय(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    शून्य दर्शय(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    शून्य दर्शय(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य दर्शय(AstFError* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_अग्र(nodep->strp(), विस्तारः_VP{SELF, BOTH}.p());
        }
    }
    शून्य दर्शय(AstFEof* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    शून्य दर्शय(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        यदि (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य दर्शय(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य दर्शय(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    शून्य दर्शय(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->विस्थापनम्(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    शून्य दर्शय(AstFGetC* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    शून्य दर्शय(AstFGetS* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_अग्र(nodep->strgp(), विस्तारः_VP{SELF, BOTH}.p());
        }
    }
    शून्य दर्शय(AstFUngetC* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    शून्य दर्शय(AstFRead* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->memp(), विस्तारः_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            यदि (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            यदि (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    शून्य दर्शय(AstFScanF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_अग्र(nodep->exprsp(), विस्तारः_VP{SELF, BOTH}.p());
        }
    }
    शून्य दर्शय(AstSScanF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->fromp(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->exprsp(), विस्तारः_VP{SELF, BOTH}.p());
        }
    }
    शून्य दर्शय(Ast_राशिः_Trace_F* nodep) override { nodep->dtype_समुच्चयः_String(); }
    शून्य दर्शय(AstSysIgnore* nodep) override {
        user_Iterate_And_अग्र(nodep->exprsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstSystemF* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_अग्र(nodep->memp(), विस्तारः_VP{SELF, BOTH}.p());
        ध्रुव Ast_ग्रन्थिः_DType* subp = शून्यसूचकः;
        यदि (ध्रुव AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            यदि (!adtypep->कुञ्चिका_DTypep()->skipRefp()->basicp()
                || !adtypep->कुञ्चिका_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } अन्यथा यदि (ध्रुव AstUnpackArrayDType* ध्रुव adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा यदि (ध्रुव AstDynArrayDType* ध्रुव adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } अन्यथा {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        यदि (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_अग्र(nodep->lsbp(), विस्तारः_VP{SELF, BOTH}.p());
        user_Iterate_And_अग्र(nodep->msbp(), विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(Ast_Test_Plus_तर्काः* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    शून्य दर्शय(Ast_मानम्_Plus_तर्काः* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_अग्र(nodep->outp(), विस्तारः_VP{SELF, BOTH}.p());
        }
    }
    शून्य दर्शय(AstTimeFormat* nodep) override {
        assertAtStatement(nodep);
        यदि (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        यदि (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        यदि (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        यदि (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    शून्य दर्शय(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), शून्यसूचकः);
        user_Iterate_And_अग्र(nodep->failsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
    }
    शून्य दर्शय(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), शून्यसूचकः);
        user_Iterate_And_अग्र(nodep->failsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_अग्र(nodep->passsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    तार्किक pinInstArrayElement(AstPin* nodep, ध्रुव AstRange* rangep, पूर्णाङ्क idx) {
        Ast_ग्रन्थिः_DType* ध्रुव modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ग्रन्थिः_DType* ध्रुव conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        यदि (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) प्रत्यागम सत्य;
        // Types don't match, but all dimensions done, still need the usual checks
        यदि (!rangep) प्रत्यागम असत्य;

        ध्रुव AstRange* ध्रुव subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        पूर्णाङ्क subElems = 1;
        प्रति (ध्रुव AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ध्रुव पूर्णाङ्क elements = rangep->elementsConst();
        ध्रुव पूर्णाङ्क from_वामः = idx / subElems;

        ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ध्रुव uint32_t conDims = conDTypep->dimensions(असत्य).second;
        ध्रुव uint32_t modDims = modDTypep->dimensions(असत्य).second;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        यदि (arrp && conDims != modDims) {
            यदि (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_अग्र(nodep->exprp(),
                                   विस्तारः_VP{conDTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
                प्रत्यागम सत्य;
            }
            // ArraySel index is counted from lo
            ध्रुव पूर्णाङ्क bit = arrp->declRange().ascending() ? from_वामः : elements - 1 - from_वामः;
            // Connection is self-determined
            user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{conDTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->unlink_Fr_पृष्ठभागः();
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemp = newIfaceArrayElement(fromp, bit)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(fromp), fromp);
            } अन्यथा {
                AstArraySel* ध्रुव selp = नूतन AstArraySel{fromp->fileline(), fromp, bit};
                selp->did_विस्तारः(सत्य);
                nodep->exprp(selp);
            }
            प्रत्यागम pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        यदि (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) प्रत्यागम असत्य;

        // Packed connection
        ध्रुव पूर्णाङ्क modwidth = modDTypep->विस्तारः();
        ध्रुव पूर्णाङ्क conwidth = conDTypep->विस्तारः();

        // Connection has the width of the port, connect it to every element
        यदि (conwidth == modwidth) प्रत्यागम असत्य;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        यदि (conwidth == modwidth * elements * subElems) {
            यदि (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->वामः_Const() << ":"
                                                     << rangep->दक्षिणः_Const() << "]");
            }
            ध्रुव पूर्णाङ्क selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{conDTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव fromp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्)->unlink_Fr_पृष्ठभागः();
            AstSel* ध्रुव selp = नूतन AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_वामः), selwidth};
            selp->did_विस्तारः(सत्य);
            nodep->exprp(selp);
            प्रत्यागम pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{conDTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
        प्रत्यागम सत्य;
    }

    शून्य दर्शय(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        यदि (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ध्रुव concatp = VN_CAST(nodep->exprp(), Replicate);
            ध्रुव AstConst* ध्रुव countp = concatp ? VN_CAST(concatp->countp(), Const) : शून्यसूचकः;
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            यदि (countp && countp->गन्तव्यम्_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_आदर्शः* ध्रुव patternp = नूतन Ast_आदर्शः{concatp->fileline(), शून्यसूचकः};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_अन्तर्_Param_Override);
            m_अन्तर्_Param_Override = is_Override_आदर्शः(nodep->exprp());
            userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
        } अन्यथा यदि (!m_paramsOnly) {
            यदि (!nodep->modVarp()->did_विस्तारः()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), शून्यसूचकः);
            }
            यदि (!nodep->exprp()) {  // No-connect
                यदि (nodep->modVarp()->is_Iface_Array_विभज()) expandIfaceArrayPin(nodep);
                प्रत्यागम;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ध्रुव VDirection pinDirection = nodep->modVarp()->direction();
            यदि (VN_IS(nodep->exprp(), ग्रन्थिः_प्रवाहः) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ग्रन्थिः_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ग्रन्थिः_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            यदि (ध्रुव AstIfaceRefDType* ध्रुव modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ध्रुव AstIfaceRefDType* ध्रुव conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                यदि (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ग्रन्थिः_DType* subDTypep = modDTypep;
            ध्रुव पूर्णाङ्क modwidth = modDTypep->विस्तारः();
            पूर्णाङ्क conwidth = conDTypep->विस्तारः();
            यदि (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_अग्र(nodep->exprp(),
                                   विस्तारः_VP{subDTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
            } अन्यथा {
                यदि (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    यदि (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        यदि (nodep->modVarp()->is_Iface_Array_विभज()) expandIfaceArrayPin(nodep);
                        प्रत्यागम;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->विस्तारः();
                }
                यदि (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } अन्यथा यदि (nodep->modVarp()->isTristate()) {
                    यदि (modwidth != conwidth) {
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
                } अन्यथा यदि (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                यदि ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव व्यञ्जकम्_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                यदि (व्यञ्जकम्_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_आकार(conDTypep) != unpacked_Array_आकार(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_आकार(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_आकार(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा यदि ((व्यञ्जकम्_Arrayp && !modArrayp) || (!व्यञ्जकम्_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (व्यञ्जकम्_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } अन्यथा {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ग्रन्थिः_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ग्रन्थिः_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_नियोजय(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            यदि (nodep->modVarp()->is_Iface_Array_विभज()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    शून्य दर्शय(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        यदि (!m_paramsOnly) {
            यदि (VN_IS(nodep->modp(), Not_प्राप्तम्_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_मार्गः_Looked_सन्देशः(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            यदि (nodep->rangep()) user_Iterate_And_अग्र(nodep->rangep(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->pinsp(), शून्यसूचकः);
            // If element of an instance array, pins are now connected, no longer need the ranges
            यदि (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                प्रक्षिप_Deletep(nodep->rangep()->unlink_Fr_पृष्ठभागः_With_अग्र());
            }
        }
        user_Iterate_And_अग्र(nodep->paramsp(), शून्यसूचकः);
    }
    शून्य दर्शय(AstGatePin* nodep) override {
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->rangep(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव pinDTypep = nodep->exprp()->dtypep();
            ध्रुव पूर्णाङ्क अङ्कः_Insts = nodep->rangep()->elementsConst();
            ध्रुव पूर्णाङ्क modwidth = अङ्कः_Insts;
            ध्रुव पूर्णाङ्क pinwidth = pinDTypep->विस्तारः();
            यदि (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findLogicDType(1, 1, pinDTypep->numeric());
                user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{subDTypep, FINAL}.p());
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_पृष्ठभागः(),
                                       static_cast<uint32_t>(अङ्कः_Insts)};
                nodep->replaceWith(newp);
            } अन्यथा {
                यदि (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_अग्र(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_पृष्ठभागः());
            }
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        यदि (nodep->doing_विस्तारः()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(सत्य);
            nodep->did_विस्तारः(सत्य);
            प्रत्यागम;
        }
        यदि (nodep->class_विधानम्() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } अन्यथा यदि (nodep->class_विधानम्() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        यदि (nodep->class_विधानम्() && nodep->name() == "new") {
            यदि (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            यदि (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ग्रन्थिः* firstp = शून्यसूचकः;
            प्रति (Ast_ग्रन्थिः* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                यदि (Ast_Stmt_व्यञ्जकम्* ध्रुव sep = VN_CAST(itemp, Stmt_व्यञ्जकम्)) {
                    यदि (AstNew* ध्रुव newp = VN_CAST(sep->exprp(), New)) {
                        यदि (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            भङ्ग;
                        }
                    }
                    अनुवर्त;
                }
                यदि (firstNewStatementOkRecurse(itemp)) अनुवर्त;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_विस्तारः(सत्य);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        यदि (nodep->fvarp()) userIterate(nodep->fvarp(), शून्यसूचकः);
        यदि (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_समुच्चयः_Void();
        } अन्यथा यदि (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } अन्यथा यदि (VN_IS(nodep, Property)) {
            nodep->dtype_समुच्चयः_Bit();
        }
        विस्तारः_VP* vup = शून्यसूचकः;
        यदि (VN_IS(nodep, Property)) vup = विस्तारः_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_विस्तारः(सत्य);
        nodep->doing_विस्तारः(असत्य);
        यदि (nodep->dpiImport() && !nodep->dpiOpenParent() && markHasOpenArray(nodep)) {
            nodep->dpiOpenParentInc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        यदि (nodep->fvarp() && !nodep->dpiImport() && !nodep->विशुद्धम्_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_अग्र([&](ध्रुव AstVarRef* varrefp) {
                   प्रत्यागम varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::सदिशम्<AstVar*> ports;
        std::सदिशम्<Ast_Attr_सम्बन्धि*> protos;
        Ast_Attr_सम्बन्धि* protop = शून्यसूचकः;  // Base prototype
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_पृष्ठभागः(portp); }
            } अन्यथा यदि (Ast_Attr_सम्बन्धि* ध्रुव attrp = VN_CAST(stmtp, Attr_सम्बन्धि)) {
                यदि (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_पृष्ठभागः(attrp); }
                यदि (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        यदि (protop) {
            प्रति (आकार_t i = 0; i < std::max(ports.आकार(), protos.आकार()); ++i) {
                यदि (i >= ports.आकार() || i >= protos.आकार()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    भङ्ग;
                } अन्यथा {
                    AstVar* ध्रुव portp = ports[i];
                    Ast_Attr_सम्बन्धि* ध्रुव rprotop = protos[i];
                    Ast_ग्रन्थिः_DType* ध्रुव declDtp = portp->dtypep();
                    Ast_ग्रन्थिः_DType* ध्रुव protoDtp = rprotop->fromp()->dtypep();
                    यदि (portp->name() != rprotop->name()) {
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
                    } अन्यथा यदि (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_अधिकम्() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_अधिकम्() << "... Declaration data type: "
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
    शून्य दर्शय(AstConstraint* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, शून्यसूचकः);
        nodep->did_विस्तारः(सत्य);
    }
    शून्य दर्शय(AstProperty* nodep) override {
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        यदि (nodep->doing_विस्तारः()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(सत्य);
            nodep->did_विस्तारः(सत्य);
            प्रत्यागम;
        }
        nodep->doing_विस्तारः(सत्य);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_समुच्चयः_Bit();
        प्रति (Ast_ग्रन्थिः* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            यदि (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, शून्यसूचकः);
            } अन्यथा यदि (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } अन्यथा यदि (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, शून्यसूचकः);
            } अन्यथा {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_विस्तारः(सत्य);
        nodep->doing_विस्तारः(असत्य);
    }
    शून्य दर्शय(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    शून्य दर्शय(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        nodep->dtype_समुच्चयः_Bit();
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (VN_IS(stmtp, Var)) {
                userIterate(stmtp, शून्यसूचकः);
            } अन्यथा यदि (VN_IS(stmtp, ग्रन्थिः_व्यञ्जकम्)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } अन्यथा {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_विस्तारः(सत्य);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* getItemPackage(Ast_ग्रन्थिः* pkgItemp) {
        यावत् (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        प्रत्यागम VN_CAST(pkgItemp->backp(), Package);
    }
    शून्य दर्शय(AstFuncRef* nodep) override {
        दर्शय(static_cast<Ast_ग्रन्थिः_FTask_Ref*>(nodep));
        यदि (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            यदि (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_व्यञ्जकम्))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_समुच्चयः_Void();
        } अन्यथा {
            nodep->dtypeFrom(nodep->taskp());
        }
        यदि (nodep->fileline()->timingOn()) {
            Ast_ग्रन्थिः_Module* ध्रुव classp = nodep->classOrPackagep();
            यदि (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ध्रुव packagep = getItemPackage(classp);
                यदि (packagep && packagep->name() == "std") {
                    विधानम्_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    स्थिर तार्किक are_Same_आकार(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ध्रुव std::सदिशम्<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        यदि (dims0.आकार() != dims1.आकार()) प्रत्यागम असत्य;
        प्रति (आकार_t i = 0; i < dims0.आकार(); ++i) {
            यदि (dims0[i]->elementsConst() != dims1[i]->elementsConst()) प्रत्यागम असत्य;
        }
        प्रत्यागम सत्य;
    }
    // Makes sure that port and pin have same size and same datatype
    शून्य check_Unpacked_Array_तर्काः(AstVar* portp, Ast_ग्रन्थिः* pinp) {
        यदि (AstUnpackArrayDType* ध्रुव पत्तनम्_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            यदि (AstUnpackArrayDType* ध्रुव pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                यदि (!are_Same_आकार(पत्तनम्_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << पत्तनम्_Dtypep->prettyDTypeNameQ() << ")");
                }
                ध्रुव AstBasicDType* ध्रुव पत्तनम्_Basicp = पत्तनम्_Dtypep->basicp();
                ध्रुव AstBasicDType* ध्रुव pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ध्रुव तार्किक sameElemDType
                    = पत्तनम्_Basicp && pinBasicp
                          ? पत्तनम्_Basicp->विस्तारः() == pinBasicp->विस्तारः()
                                && पत्तनम्_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(पत्तनम्_Dtypep->elemDTypep());
                यदि (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, पत्तनम्_Dtypep, "Argument", "Parameter"));
                }
            } अन्यथा {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    शून्य process_FTask_Ref_तर्काः(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        userIterate(nodep->taskp(), शून्यसूचकः);
        //
        // And do the arguments to the task/function too
        कुरु {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ध्रुव V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            यदि (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* pinp = argp->exprp();
                यदि (!pinp) अनुवर्त;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                यदि (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_पृष्ठभागः_With_अग्र(&handle);  // Format + additional args, if any
                    Ast_ग्रन्थिः_व्यञ्जकम्* argsp = शून्यसूचकः;
                    यावत् (AstArg* ध्रुव nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->सङ्कलय_अग्र(nextargp->exprp()->unlink_Fr_पृष्ठभागः_With_अग्र());
                        nextargp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः();  // Remove the call's Arg wrapper
                    }
                    string format;
                    यदि (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->अङ्कः().गन्तव्यम्_String();
                    } अन्यथा {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(argp), argp);
                    AstSFormatF* ध्रुव newp
                        = नूतन AstSFormatF{nodep->fileline(), format, असत्य, argsp};
                    यदि (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(नूतन AstScopeName{newp->fileline(), सत्य});
                    }
                    handle.relink(नूतन AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    गच्छ reloop;  // so exit early; next loop will correct it
                }  //
                अन्यथा यदि (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_पृष्ठभागः(&handle);  // No next, that's the next pin
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                यदि (portp->isWritable()) {
                    V3LinkLValue::link_LValue_समुच्चयः(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, विस्तारः_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } यावत् (असत्य);
        // Stage 2
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
                AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
                यदि (!pinp) अनुवर्त;  // Argument error we'll find later
                // Change data types based on above accept completion
                यदि (nodep->taskp()->dpiImport()) check_Unpacked_Array_तर्काः(portp, pinp);
                यदि (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव pinp = argp->exprp();
                यदि (!pinp) अनुवर्त;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, विस्तारः_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        यदि (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
                ध्रुव AstVar* ध्रुव portp = tconnect.first;
                ध्रुव AstArg* ध्रुव argp = tconnect.second;
                Ast_ग्रन्थिः_व्यञ्जकम्* pinp = argp->exprp();
                यदि (!pinp) अनुवर्त;  // Argument error we'll find later
                Ast_ग्रन्थिः_DType* ध्रुव पत्तनम्_DTypep = portp->dtypep()->skip_Ref_गन्तव्यम्_Enump();
                यदि (VN_IS(pinp, रिक्तानि_Queue)) {
                    Ast_ग्रन्थिः_व्यञ्जकम्* newp = newConsDynArrayOrQueue(pinp->fileline(), पत्तनम्_DTypep);
                    यदि (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = नूतन AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(पत्तनम्_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव pinDTypep = pinp->dtypep()->skip_Ref_गन्तव्यम्_Enump();
                ध्रुव AstIfaceRefDType* ध्रुव पत्तनम्_Ifacep
                    = VN_CAST(पत्तनम्_DTypep->elemDTypep(सत्य), IfaceRefDType);
                ध्रुव VCastable ifaceCastable
                    = पत्तनम्_Ifacep ? Ast_ग्रन्थिः::computeCastable(पत्तनम्_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ सत्य)
                                 : VCastable{VCastable::UNSUPPORTED};
                ध्रुव तार्किक matchingRefDTypes = पत्तनम्_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(पत्तनम्_DTypep, pinDTypep);
                ध्रुव तार्किक pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->अङ्कः().isNull();
                यदि ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    यदि (पत्तनम्_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } अन्यथा {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << पत्तनम्_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } अन्यथा यदि (पत्तनम्_Ifacep && पत्तनम्_Ifacep->isVirtual() && portp->is_निवेशः() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } अन्यथा यदि (portp->isWritable() && pinp->विस्तारः() != portp->विस्तारः()) {
                    pinp->v3widthWarn(portp->विस्तारः(), pinp->विस्तारः(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->विस्तारः()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->विस्तारः() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_पृष्ठभागः(&relinkHandle);
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                यदि (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(पत्तनम्_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && पत्तनम्_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << पत्तनम्_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    अनुवर्त;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                यदि (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } अन्यथा यदि (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_नियोजय(nodep, "Function Argument", pinp, पत्तनम्_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, विस्तारः_VP{पत्तनम्_DTypep, FINAL, प्रवाहः_USE_नियोजय}.p());
                } अन्यथा {
                    iterate_Check_नियोजय(nodep, "Function Argument", pinp, FINAL, पत्तनम्_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    शून्य handle_Std_Randomize_तर्काः(Ast_ग्रन्थिः_FTask_Ref* ध्रुव nodep) {
        AstConst* nullp = शून्यसूचकः;
        प्रति (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = argp->exprp();
            यदि (AstConst* ध्रुव constp = VN_CAST(exprp, Const)) {
                यदि (constp->अङ्कः().isNull()) {
                    nullp = constp;
                    अनुवर्त;
                }
            }
            यदि (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                अनुवर्त;
            }
            यदि (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                अनुवर्त;
            }
            यदि (ध्रुव AstCMethodHard* ध्रुव methodp = VN_CAST(exprp, CMethodHard)) {
                यदि (methodp->विधानम्() == VCMethod::ARRAY_AT
                    || methodp->विधानम्() == VCMethod::ARRAY_AT_WRITE) {
                    अनुवर्त;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        यदि (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    शून्य दर्शय(Ast_ग्रन्थिः_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        यदि (nodep->did_विस्तारः()) प्रत्यागम;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = शून्यसूचकः;
        यदि (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(सत्य);
            nodep->dtypep(nodep->findBasicDType(VBasicDTypeKwd::INT));
            प्रत्यागम;  // Handled in V3Randomize
        } अन्यथा यदि (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ध्रुव classp = VN_CAST(nodep->classOrPackagep(), Class);
            यदि (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(सत्य);
                Ast_ग्रन्थिः_DType* ध्रुव adtypep = nodep->findBitDType();
                withp = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                प्रति (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_अग्र(argp->exprp(), विस्तारः_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_तर्काः(nodep);  // Provided args should be in current scope
                process_FTask_Ref_तर्काः(nodep);
                यदि (withp) nodep->withp(withp);
                nodep->did_विस्तारः(सत्य);
                प्रत्यागम;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            यदि (nodep->name() == "randomize") {
                AstClassRefDType* ध्रुव adtypep
                    = नूतन AstClassRefDType{nodep->fileline(), classp, शून्यसूचकः};
                v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(adtypep);
                withp = विधानम्_With_Clause(nodep, असत्य, असत्य, adtypep->findVoidDType(),
                                         adtypep->findBitDType(), adtypep);
                प्रति (ध्रुव AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_अग्र(argp->exprp(), विस्तारः_VP{SELF, BOTH}.p());
                }
                handle_Randomize_तर्काः(nodep, classp);
            } अन्यथा यदि (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } अन्यथा यदि (nodep->name() == "get_randstate") {
                विधानम्_Ok_Arguments(nodep, 0, 0);
                classp->आधारः_Most_Classp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCExpr* ध्रुव newp
                    = नूतन AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_समुच्चयः_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा यदि (nodep->name() == "set_randstate") {
                विधानम्_Ok_Arguments(nodep, 1, 1);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = nodep->argsp()->exprp();
                classp->आधारः_Most_Classp()->needRNG(सत्य);
                v3Global.useRandomizeMethods(सत्य);
                AstCExpr* ध्रुव newp
                    = नूतन AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->सङ्कलय(exprp->unlink_Fr_पृष्ठभागः());
                newp->सङ्कलय(")");
                newp->dtype_समुच्चयः_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा {
                UASSERT_OBJ(असत्य, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        यदि (nodep->did_विस्तारः()) {
            यदि (withp) nodep->withp(withp);
            प्रत्यागम;
        }
        यदि ((nodep->taskp()->class_विधानम्() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            तार्किक allow = असत्य;
            यदि (m_ftaskp && m_ftaskp->class_विधानम्() && !m_ftaskp->isStatic()) {
                यदि (ध्रुव AstFuncRef* ध्रुव funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } अन्यथा यदि (ध्रुव AstTaskRef* ध्रुव taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                यदि (!allow) {
                    ध्रुव AstClass* callerClassp = m_containingClassFinder.find(m_ftaskp);
                    यदि (!callerClassp) {
                        callerClassp = m_containingClassFinder.find(m_ftaskp->classOrPackagep());
                    }
                    ध्रुव AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    यदि (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            यदि (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ध्रुव AstClass* ध्रुव cgClassp = m_containingClassFinder.find(nodep);
                यदि (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ध्रुव AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    यदि (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.find(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            यदि (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        यदि (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(नूतन AstScopeName{nodep->fileline(), असत्य});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_तर्काः(nodep);
        यदि (withp) nodep->withp(withp);
        nodep->did_विस्तारः(सत्य);
        // See steps that follow in visit(AstFuncRef*)
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, शून्यसूचकः);
    }
    शून्य दर्शय(AstSenItem* nodep) override {
        यदि (nodep->is_Combo_तारकम्()) प्रत्यागम;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        यदि (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            स्वचालित* ध्रुव concatOrReplp = VN_CAST(nodep->sensp(), ग्रन्थिः_Biop);
            स्वचालित* ध्रुव rhsp = concatOrReplp->rhsp()->unlink_Fr_पृष्ठभागः();
            यदि (nodep->edgeType() == VEdgeType::ET_परिवर्तितम्) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                स्वचालित* ध्रुव lhsp = concatOrReplp->lhsp()->unlink_Fr_पृष्ठभागः();
                nodep->सङ्कलय_अग्र_अत्र(नूतन AstSenItem{lhsp->fileline(), nodep->edgeType(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(नूतन AstSenItem{rhsp->fileline(), nodep->edgeType(), rhsp});
            VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
        } अन्यथा {
            userIterateChildren(nodep, विस्तारः_VP{SELF, BOTH}.p());
            यदि (nodep->edgeType().anEdge()) {
                Ast_ग्रन्थिः_DType* ध्रुव sensDtp = nodep->sensp()->dtypep()->skipRefp();
                यदि (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } अन्यथा यदि (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    शून्य दर्शय(AstClockingItem* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ध्रुव refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, विस्तारः_VP{SELF, PRELIM}.p());
    }
    शून्य दर्शय(AstWait* nodep) override {
        यदि (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), nodep);
            प्रत्यागम;
        }
        यदि (nodep->fileline()->timingOn()) {
            यदि (v3Global.opt.timing().is_समुच्चयः_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव varrefp = VN_CAST(nodep->condp(), ग्रन्थिः_Var_Ref)) {
                    यदि (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_अधिकम्() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                प्रत्यागम;
            } अन्यथा यदि (v3Global.opt.timing().is_समुच्चयः_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } अन्यथा {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        यदि (Ast_ग्रन्थिः* ध्रुव stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_पृष्ठभागः_With_अग्र();
            nodep->replaceWith(stmtsp);
        } अन्यथा {
            nodep->unlink_Fr_पृष्ठभागः();
        }
        VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
    }
    शून्य दर्शय(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->सूचकांक_Arg_Refp(), शून्यसूचकः);
        userIterateChildren(nodep->मानम्_Arg_Refp(), शून्यसूचकः);
        यदि (!nodep->exprp()) {
            nodep->dtype_समुच्चयः_Void();
        } अन्यथा {
            यदि (!nodep->exprp()->hasDType()) {
                user_Iterate_And_अग्र(nodep->exprp(), शून्यसूचकः);
            } अन्यथा {
                यदि (vdtypep) {
                    user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{nodep->dtypep(), PRELIM}.p());
                } अन्यथा {  // 'sort with' allows arbitrary type
                    user_Iterate_And_अग्र(nodep->exprp(), विस्तारः_VP{SELF, PRELIM}.p());
                }
            }

            यदि (!nodep->exprp()->hasDType()) {
                nodep->dtype_समुच्चयः_Void();
            } अन्यथा {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_नियोजय(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    शून्य दर्शय(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        यदि (nodep->सूचकांक()) {
            nodep->dtypeFrom(m_withp->सूचकांक_Arg_Refp());
        } अन्यथा {
            nodep->dtypeFrom(m_withp->मानम्_Arg_Refp());
        }
    }
    शून्य दर्शय(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, शून्यसूचकः);
    }

    //--------------------
    // Default
    शून्य दर्शय(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) override {
        यदि (!nodep->did_विस्तारः()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, शून्यसूचकः);
    }
    शून्य दर्शय(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << पङ्क्त्यन्तः);
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        यदि (Ast_ग्रन्थिः_DType* ध्रुव vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } अन्यथा यदि (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->विस्तारः() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->विस्तारः() << " bits -> " << nodep->विस्तारः()
                                    << " bits" << पङ्क्त्यन्तः);

        // Final processing
        user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, FINAL}.p());
        nodep->did_विस्तारः(सत्य);
    }
    शून्य दर्शय_Class(AstClass* nodep) {
        यदि (nodep->did_विस्तारः_And_समुच्चयः()) प्रत्यागम;

        // If the class is std::process
        यदि (nodep->name() == "process") {
            AstPackage* ध्रुव packagep = getItemPackage(nodep);
            यदि (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                यदि (AstVar* ध्रुव varp
                    = VN_CAST(m_memberMap.findMember(nodep, "m_process"), Var)) {
                    Ast_ग्रन्थिः_DType* ध्रुव dtypep = varp->getChildDTypep();
                    यदि (!varp->dtypep()) {
                        VL_DO_DANGLING(प्रक्षिप_Deletep(dtypep->unlink_Fr_पृष्ठभागः()), dtypep);
                    }
                    AstBasicDType* ध्रुव newdtypep = नूतन AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                यदि (Ast_ग्रन्थिः_FTask* ध्रुव ftaskp
                    = VN_CAST(m_memberMap.findMember(nodep, "self"), ग्रन्थिः_FTask)) {
                    ftaskp->समुच्चयः_Need_Process();
                    v3Global.समुच्चयः_उपयुङ्क्ते_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_अग्र(nodep->extendsp(), शून्यसूचकः);
        userIterateChildren(nodep, शून्यसूचकः);  // First size all members
    }
    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_अस्थायी_नामानि);
        VL_RESTORER_COPY(m_inside_अस्थायी_नामानि);
        यदि (AstClass* ध्रुव classp = VN_CAST(nodep, Class)) {
            दर्शय_Class(classp);
        } अन्यथा {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([एतत्](ध्रुव AstVar* varp) { m_curModVars.निवेशय(varp); });
            userIterateChildren(nodep, शून्यसूचकः);
        }
    }
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, शून्यसूचकः);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    शून्य दर्शय_Or_Lu64(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_समुच्चयः_Double();
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = nodep->findBitDType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    शून्य दर्शय(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_समुच्चयः_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM}.p());
            यदि (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    नूतन AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_पृष्ठभागः()});
                VL_DO_DANGLING(nodep->delete_वृक्षः(), nodep);
            }
        }
    }
    शून्य दर्शय(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_समुच्चयः_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM}.p());
        }
    }
    शून्य दर्शय_Os32_Lr(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_Integer2_State();
        }
    }
    शून्य दर्शय_Ou64_Lr(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_UInt64();
        }
    }

    शून्य दर्शय_log_not(AstLogNot* nodep) {
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
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_समुच्चयः_Bit_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            यदि (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ध्रुव newp
                    = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            }
        }
    }
    शून्य दर्शय_log_and_or(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit_Or_Logic(
                Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    शून्य दर्शय_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय_red_and_or(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_समुच्चयः_Bit_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    शून्य दर्शय_red_unknown(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            nodep->dtype_समुच्चयः_Bit();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    तार्किक isEquivalentDType(ध्रुव Ast_ग्रन्थिः_DType* lhs, ध्रुव Ast_ग्रन्थिः_DType* rhs) {
        // a) If two types match, they are equivalent.
        यदि (!lhs || !rhs) प्रत्यागम असत्य;
        lhs = lhs->skip_Ref_गन्तव्यम्_Enump();
        rhs = rhs->skip_Ref_गन्तव्यम्_Enump();
        यदि (lhs == rhs) प्रत्यागम सत्य;
        // If both are basic types, check if they are the same type
        यदि (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ध्रुव स्वचालित* lb = VN_CAST(lhs, BasicDType);
            ध्रुव स्वचालित* rb = VN_CAST(rhs, BasicDType);
            यदि (lb->isString() != rb->isString()) प्रत्यागम असत्य;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ध्रुव तार्किक lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ध्रुव तार्किक rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        यदि (lhsIsUnpackArray || rhsIsUnpackArray) {
            यदि (lhsIsUnpackArray && rhsIsUnpackArray) {
                ध्रुव AstUnpackArrayDType* ध्रुव lhsp = VN_CAST(lhs, UnpackArrayDType);
                ध्रुव AstUnpackArrayDType* ध्रुव rhsp = VN_CAST(rhs, UnpackArrayDType);
                ध्रुव पूर्णाङ्क lsz = lhsp->elementsConst();
                ध्रुव पूर्णाङ्क rsz = rhsp->elementsConst();
                यदि (lsz >= 0 && rsz >= 0 && lsz != rsz) प्रत्यागम असत्य;
                प्रत्यागम isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            प्रत्यागम असत्य;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ध्रुव तार्किक lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ध्रुव तार्किक rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ध्रुव तार्किक lhsIsQueue = VN_IS(lhs, QueueDType);
        ध्रुव तार्किक rhsIsQueue = VN_IS(rhs, QueueDType);
        ध्रुव तार्किक lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ध्रुव तार्किक rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        यदि (lhsIsDynArray || rhsIsDynArray || lhsIsQueue || rhsIsQueue || lhsIsAssocArray
            || rhsIsAssocArray) {
            यदि (ध्रुव AstDynArrayDType* ध्रुव lhsp = VN_CAST(lhs, DynArrayDType)) {
                यदि (ध्रुव AstDynArrayDType* ध्रुव rhsp = VN_CAST(rhs, DynArrayDType)) {
                    प्रत्यागम isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            यदि (ध्रुव AstQueueDType* ध्रुव lhsp = VN_CAST(lhs, QueueDType)) {
                यदि (ध्रुव AstQueueDType* ध्रुव rhsp = VN_CAST(rhs, QueueDType)) {
                    प्रत्यागम isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            यदि (ध्रुव AstAssocArrayDType* ध्रुव lhsp = VN_CAST(lhs, AssocArrayDType)) {
                यदि (ध्रुव AstAssocArrayDType* ध्रुव rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    प्रत्यागम isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->कुञ्चिका_DTypep(), rhsp->कुञ्चिका_DTypep());
                }
            }

            प्रत्यागम असत्य;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        यदि (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            यदि (lhs->विस्तारः() != rhs->विस्तारः()) प्रत्यागम असत्य;
            यदि (lhs->isFourstate() != rhs->isFourstate()) प्रत्यागम असत्य;
            यदि (lhs->isSigned() != rhs->isSigned()) प्रत्यागम असत्य;
            प्रत्यागम सत्य;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        यदि (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) प्रत्यागम असत्य;
        प्रत्यागम !(VN_IS(lhs, ग्रन्थिः_UOr_Struct_DType) || VN_IS(rhs, ग्रन्थिः_UOr_Struct_DType));
    }

    स्थिर तार्किक isAggregateType(ध्रुव Ast_ग्रन्थिः* nodep) {
        यदि (!nodep) प्रत्यागम असत्य;
        ध्रुव Ast_ग्रन्थिः_DType* dtypep = nodep->dtypep();
        यदि (!dtypep) प्रत्यागम असत्य;
        dtypep = dtypep->skipRefp();
        यदि (!dtypep) प्रत्यागम असत्य;
        प्रत्यागम dtypep->isAggregateType();
    }

    शून्य दर्शय_cmp_eq_gt(Ast_ग्रन्थिः_Biop* nodep, ध्रुव ComparisonType cmpType) {
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
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ध्रुव तार्किक lhsIsPat = VN_IS(nodep->lhsp(), आदर्शः);
            ध्रुव तार्किक rhsIsPat = VN_IS(nodep->rhsp(), आदर्शः);
            यदि (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } अन्यथा यदि (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } अन्यथा {
                user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            }

            ध्रुव तार्किक isAggrLhs = isAggregateType(nodep->lhsp());
            ध्रुव तार्किक isAggrRhs = isAggregateType(nodep->rhsp());

            यदि ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::प्रसामान्यम्_EQ || cmpType == ComparisonType::CASE)) {
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDType = nodep->lhsp()->dtypep();
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDType = nodep->rhsp()->dtypep();

                यदि (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_क्रमः(nodep->rhsp(), nodep->lhsp()->dtypep());
            } अन्यथा यदि (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                यदि (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ग्रन्थिः* ध्रुव newp
                        = नूतन AstConst{nodep->fileline(), AstConst::BitFalseErroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                    प्रत्यागम;
                }
                यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा यदि (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } अन्यथा {
                ध्रुव तार्किक signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ध्रुव पूर्णाङ्क विस्तारः = std::max(nodep->lhsp()->विस्तारः(), nodep->rhsp()->विस्तारः());
                ध्रुव पूर्णाङ्क ewidth = std::max(nodep->lhsp()->विस्तारः_Min(), nodep->rhsp()->विस्तारः_Min());
                तार्किक warnOn = सत्य;
                यदि (!signedFl && विस्तारः == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    यदि ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->विस्तारः() >= nodep->rhsp()->विस्तारः_Min())) {
                        warnOn = असत्य;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    यदि ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->विस्तारः_Min() >= nodep->rhsp()->विस्तारः())) {
                        warnOn = असत्य;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(विस्तारः, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->findBitOrLogicDType(विस्तारः, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_समुच्चयः_Bit_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::प्रसामान्यम्_EQ
                                               || cmpType == ComparisonType::प्रसामान्यम्_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    शून्य दर्शय_cmp_real(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय_cmp_string(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Bit();
        }
    }
    शून्य दर्शय_cmp_type(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, BOTH}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{SELF, BOTH}.p());
            ध्रुव Ast_Attr_सम्बन्धि* ध्रुव lhsap = VN_AS(nodep->lhsp(), Attr_सम्बन्धि);
            ध्रुव Ast_Attr_सम्बन्धि* ध्रुव rhsap = VN_AS(nodep->rhsp(), Attr_सम्बन्धि);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsap->dtypep();
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ध्रुव तार्किक invert = VN_IS(nodep, NeqT);
            ध्रुव तार्किक identical
                = Ast_ग्रन्थिः::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ध्रुव तार्किक eq = invert ^ identical;
            Ast_ग्रन्थिः* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::BitTrue{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        }
    }

    शून्य दर्शय_negate_not(Ast_ग्रन्थिः_Uniop* nodep, तार्किक real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            यदि (!real_ok) checkCvtUS(nodep->lhsp(), असत्य);
        }
        यदि (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            यदि (Ast_ग्रन्थिः_Uniop* ध्रुव newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_समुच्चयः_Double();
                प्रत्यागम;
            }
        } अन्यथा {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        यदि (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            // Some warning suppressions
            तार्किक lhsWarn = सत्य;
            यदि (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                यदि (subDTypep->विस्तारः_Min() == (nodep->lhsp()->विस्तारः_Min() + 1)) lhsWarn = असत्य;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    शून्य दर्शय_signed_unsigned(Ast_ग्रन्थिः_Uniop* nodep, VSigning rs_बहिर्गमनम्) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), सत्य);
            ध्रुव पूर्णाङ्क विस्तारः = nodep->lhsp()->विस्तारः();
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = nodep->findBitOrLogicDType(
                विस्तारः, विस्तारः, rs_बहिर्गमनम्, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य दर्शय_सरणम्_2(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_सरणम्_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ध्रुव Ast_ग्रन्थिः_Biop* ध्रुव newp = iterate_सरणम्_final(nodep);
        VL_DANGLING(nodep);
        (शून्य)newp;  // Ununused
    }
    शून्य iterate_सरणम्_prelim(Ast_ग्रन्थिः_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), असत्य);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            यदि (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_समुच्चयः_Logic_Unsized(nodep->विस्तारः(), nodep->विस्तारः_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ग्रन्थिः_Biop* iterate_सरणम्_final(Ast_ग्रन्थिः_Biop* nodep) {
        // Nodep maybe edited
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            यदि (VN_IS(nodep, सरणम्_RS)) {
                यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            तार्किक warnOn = सत्य;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            यदि (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ग्रन्थिः_नियोजय)) warnOn = असत्य;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ध्रुव shiftp = VN_CAST(nodep->rhsp(), Const);
            यदि (shiftp && !shiftp->अङ्कः().isFourState() && shiftp->विस्तारः() <= 32) {
                ध्रुव int64_t सरणम्_Val = shiftp->अङ्कः().गन्तव्यम्_SQuad();
                यदि (VN_IS(nodep, सरणम्_L)) {
                    यदि (सरणम्_Val > 0 && nodep->विस्तारः() == nodep->lhsp()->विस्तारः() + सरणम्_Val)
                        warnOn = असत्य;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            यदि (nodep->rhsp()->विस्तारः() > 32) {
                यदि (shiftp && shiftp->अङ्कः().most_समुच्चयः_Bit_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_सङ्ख्या अङ्कः{shiftp, 32, shiftp->अङ्कः()};
                    Ast_ग्रन्थिः* ध्रुव shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(नूतन AstConst{shiftrhsp->fileline(), अङ्कः});
                    VL_DO_DANGLING(shiftrhsp->delete_वृक्षः(), shiftrhsp);
                }
            }
        }
        प्रत्यागम nodep;  // May edit
    }

    शून्य दर्शय_boolexpr_and_or(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), असत्य);
            checkCvtUS(nodep->rhsp(), असत्य);
            ध्रुव पूर्णाङ्क विस्तारः = std::max(nodep->lhsp()->विस्तारः(), nodep->rhsp()->विस्तारः());
            ध्रुव पूर्णाङ्क mwidth = std::max(nodep->lhsp()->विस्तारः_Min(), nodep->rhsp()->विस्तारः_Min());
            ध्रुव तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                विस्तारः, mwidth, VSigning::fromBool(expSigned),
                Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        यदि (m_vup->final()) {
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    शून्य दर्शय_सङ्कलय_sub_replace(Ast_ग्रन्थिः_Biop* nodep, तार्किक real_ok, ध्रुव तार्किक modOrDiv) {
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
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_अग्र(nodep->lhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_अग्र(nodep->rhsp(), विस्तारः_VP{CONTEXT_DET, PRELIM}.p());
            यदि (!real_ok) {
                checkCvtUS(nodep->lhsp(), असत्य);
                checkCvtUS(nodep->rhsp(), असत्य);
            }
            यदि (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_समुच्चयः_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                प्रत्यागम;
            } अन्यथा यदि (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, सङ्कलय_2)
                            ? (nodep->warn_अधिकम्()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम;
            } अन्यथा {
                ध्रुव पूर्णाङ्क विस्तारः = std::max(nodep->lhsp()->विस्तारः(), nodep->rhsp()->विस्तारः());
                ध्रुव पूर्णाङ्क mwidth = std::max(nodep->lhsp()->विस्तारः_Min(), nodep->rhsp()->विस्तारः_Min());
                ध्रुव तार्किक expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                    विस्तारः, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        यदि (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ग्रन्थिः_DType* ध्रुव expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ग्रन्थिः_DType* ध्रुव subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            यदि (Ast_ग्रन्थिः_Biop* ध्रुव newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            तार्किक lhsWarn = सत्य;
            तार्किक rhsWarn = सत्य;
            यदि (VN_IS(nodep, सङ्कलय_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                यदि (subDTypep->विस्तारः_Min() == (nodep->lhsp()->विस्तारः_Min() + 1)) lhsWarn = असत्य;
                यदि (subDTypep->विस्तारः_Min() == (nodep->rhsp()->विस्तारः_Min() + 1)) rhsWarn = असत्य;
                यदि (VN_IS(nodep, सङ्कलय_2) && nodep->lhsp()->विस्तारः() == 1
                    && nodep->rhsp()->विस्तारः() != 1)
                    lhsWarn = असत्य;  // do_increment + ...
                यदि (nodep->rhsp()->विस्तारः() == 1 && nodep->lhsp()->विस्तारः() != 1)
                    rhsWarn = असत्य;  // ... + do_increment
            } अन्यथा यदि (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                यदि (subDTypep->विस्तारः_Min() >= (nodep->lhsp()->विस्तारः_Min())) lhsWarn = असत्य;
                यदि (subDTypep->विस्तारः_Min() >= (nodep->rhsp()->विस्तारः_Min())) rhsWarn = असत्य;
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
    शून्य दर्शय_real_सङ्कलय_sub(Ast_ग्रन्थिः_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_समुच्चयः_Double();
        }
    }
    शून्य दर्शय_real_neg_ceil(Ast_ग्रन्थिः_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_व्यञ्जकम्(nodep);
        यदि (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_समुच्चयः_Double();
        }
    }

    शून्य दर्शय_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->व्यञ्जकम्_Format_2(), nodep, "Assumes constant format");
        तार्किक अन्तर्_Pct = असत्य;
        Ast_ग्रन्थिः_व्यञ्जकम्* argp = nodep->exprsp();
        string newFormat;
        प्रति (वर्ण ch : nodep->text()) {
            यदि (!अन्तर्_Pct && ch == '%') {
                अन्तर्_Pct = सत्य;
                newFormat += ch;
            } अन्यथा यदि (अन्तर्_Pct && (std::अङ्कः_वा(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } अन्यथा यदि (!अन्तर्_Pct) {  // Normal text
                newFormat += ch;
            } अन्यथा {
                अन्तर्_Pct = असत्य;
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव nextp = argp ? VN_AS(argp->nextp(), ग्रन्थिः_व्यञ्जकम्) : शून्यसूचकः;
                AstSFormatArg* ध्रुव fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ग्रन्थिः_व्यञ्जकम्* subargp = fargp ? fargp->exprp() : argp;
                ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : शून्यसूचकः;
                ch = std::tolower(ch);
                विकल्प (ch) {
                पक्ष '?':  // Unspecified by user, guess
                    यदि (dtypep->isDouble()) {
                        ch = 'g';
                    } अन्यथा यदि (dtypep->isString()) {
                        ch = 's';
                    } अन्यथा यदि (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } अन्यथा {
                        ch = 'p';
                    }
                    argp = nextp;
                    भङ्ग;
                पक्ष '%': भङ्ग;  // %% - just output a %
                पक्ष 'm': भङ्ग;  // %m - auto insert "name"
                पक्ष 'l': भङ्ग;  // %m - auto insert "library"
                पक्ष 'd':
                    यदि (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        यदि (subargp->isDouble()) spliceCvtS(subargp, सत्य, 64);
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'b':  // FALLTHRU
                पक्ष 'o':  // FALLTHRU
                पक्ष 'x':
                    यदि (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 't':  // Decimal/float time
                    यदि (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'e':  // FALLTHRU
                पक्ष 'f':  // FALLTHRU
                पक्ष 'g':
                    यदि (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        यदि (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    भङ्ग;
                पक्ष 'p':  // FALLTHRU
                पक्ष 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    यदि (subargp && !fargp) {
                        यदि (AstEnumDType* ध्रुव enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_पृष्ठभागः(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    भङ्ग;
                स्वभाव:  // Most operators, just move to next argument
                    argp = nextp;
                    भङ्ग;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->text(newFormat);
    }

    स्थिर AstEnumDType* formatEnumDType(Ast_ग्रन्थिः_व्यञ्जकम्* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
        यदि (!enumDtp) {
            यदि (ध्रुव AstVarRef* ध्रुव varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_गन्तव्यम्_Enump(), EnumDType);
            }
        }
        प्रत्यागम enumDtp;
    }

    Ast_व्यञ्जकम्_Stmt* newEnumCapture(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, AstEnumDType* dtypep) {
        File_पङ्क्तिः* ध्रुव flp = valuep->fileline();
        AstVar* ध्रुव varp = नूतन AstVar{flp, VVarType::XTEMP, m_enum_अस्थायी_नामानि.get(valuep), dtypep};
        varp->noSubst(सत्य);
        यदि (m_ftaskp) {
            varp->funcLocal(सत्य);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } अन्यथा {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->सङ्कलय_Stmtsp(varp);
        }
        iterate(varp);
        Ast_व्यञ्जकम्_Stmt* ध्रुव exprp = नूतन Ast_व्यञ्जकम्_Stmt{
            flp, नूतन Ast_नियोजय{flp, नूतन AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            नूतन AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        प्रत्यागम exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ग्रन्थिः_व्यञ्जकम्* valuep, AstEnumDType* dtypep) {
        यदि (valuep->is_विशुद्धम्()) valuep = V3Const::constifyEdit(valuep);
        Ast_ग्रन्थिः_व्यञ्जकम्* keyp;
        यदि (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_वृक्षः(असत्य);
        } अन्यथा {
            Ast_व्यञ्जकम्_Stmt* ध्रुव capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_वृक्षः_विशुद्धम्(असत्य);
        }
        ध्रुव VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ध्रुव argp = नूतन AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        प्रत्यागम argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    तार्किक विस्तारः_Bad(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः_DType* expDTypep) {
        ध्रुव पूर्णाङ्क exp_विस्तारः = expDTypep->विस्तारः();
        पूर्णाङ्क exp_विस्तारः_Min = expDTypep->विस्तारः_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        यदि (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) प्रत्यागम असत्य;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->विस्तारः() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_विस्तारः != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        यदि (exp_विस्तारः_Min == 0) exp_विस्तारः_Min = exp_विस्तारः;
        यदि (nodep->dtypep()->विस्तारः() == exp_विस्तारः) प्रत्यागम असत्य;
        यदि (nodep->dtypep()->विस्तारः_Sized() && nodep->विस्तारः() != exp_विस्तारः_Min) प्रत्यागम सत्य;
        यदि (!nodep->dtypep()->विस्तारः_Sized() && nodep->विस्तारः_Min() > exp_विस्तारः_Min) प्रत्यागम सत्य;
        प्रत्यागम असत्य;
    }

    शून्य fix_विस्तारः_Extend(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, Ast_ग्रन्थिः_DType* expDTypep, Extend_नियमः extend_नियमः) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<पूर्णाङ्क>(extend_नियमः) << ") old: " << nodep);
        यदि (extend_नियमः == EXTEND_OFF) प्रत्यागम;
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        ध्रुव पूर्णाङ्क exp_विस्तारः = expDTypep->विस्तारः();
        यदि (constp && !constp->अङ्कः().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_सङ्ख्या अङ्कः{nodep, exp_विस्तारः, constp->अङ्कः()};
            अङ्कः.isSigned(असत्य);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), अङ्कः};
            constp->replaceWith(newp);
            VL_DO_DANGLING(प्रक्षिप_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा यदि (exp_विस्तारः < nodep->विस्तारः()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstSel{nodep->fileline(), nodep, 0, exp_विस्तारः};
            newp->did_विस्तारः(सत्य);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } अन्यथा {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            तार्किक doSigned = असत्य;
            विकल्प (extend_नियमः) {
            पक्ष EXTEND_ZERO: doSigned = असत्य; भङ्ग;
            पक्ष EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); भङ्ग;
            पक्ष EXTEND_LHS: doSigned = nodep->isSigned(); भङ्ग;
            स्वभाव: nodep->v3fatalSrc("bad case");
            }
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp
                = (doSigned ? static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ग्रन्थिः_व्यञ्जकम्*>(नूतन AstExtend{nodep->fileline(), nodep}));
            newp->did_विस्तारः(सत्य);
            linker.relink(newp);
            nodep = newp;
        }
        यदि (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = spliceCvtD(nodep);
            nodep = newp;
        }
        यदि (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_विस्तारः_Signed(exp_विस्तारः, expDTypep->विस्तारः_Min(), expDTypep->numeric());
        } अन्यथा {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    शून्य fix_विस्तारः_Reduce(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ध्रुव पूर्णाङ्क exp_विस्तारः = 1;
        ध्रुव पूर्णाङ्क expSigned = असत्य;
        यदि (nodep->विस्तारः() == exp_विस्तारः) प्रत्यागम;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        यदि (constp) {
            V3_सङ्ख्या अङ्कः(nodep, exp_विस्तारः);
            अङ्कः.opRedOr(constp->अङ्कः());
            अङ्कः.isSigned(expSigned);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), अङ्कः};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } अन्यथा {
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            Ast_ग्रन्थिः_DType* ध्रुव dtypeSkipp = nodep->dtypep()->skipRefp();
            यदि (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = नूतन AstNeq{nodep->fileline(),
                                  नूतन AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } अन्यथा {
                // But, AstRedOr can be faster on wide
                newp = नूतन AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_विस्तारः_Signed(exp_विस्तारः, exp_विस्तारः, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    तार्किक fixAutoExtend(Ast_ग्रन्थिः_व्यञ्जकम्*& nodepr, पूर्णाङ्क exp_विस्तारः) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        यदि (AstConst* ध्रुव constp = VN_CAST(nodepr, Const)) {
            यदि (constp->अङ्कः().autoExtend() && !constp->अङ्कः().sized() && constp->विस्तारः() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_सङ्ख्या अङ्कः(constp, exp_विस्तारः);
                अङ्कः.opRepl(constp->अङ्कः(), exp_विस्तारः);  // {width{'1}}
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{constp->fileline(), अङ्कः};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                प्रत्यागम सत्य;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            अन्यथा यदि (!constp->अङ्कः().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_विस्तारः > 32 && constp->अङ्कः().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_विस्तारः
                                                  << " bits: " << constp->prettyName());
                V3_सङ्ख्या अङ्कः(constp, exp_विस्तारः);
                अङ्कः.opExtendXZ(constp->अङ्कः(), constp->विस्तारः());
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{constp->fileline(), अङ्कः};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_वृक्षः(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;  // No change
    }
    तार्किक is_आधारः_Class_Recurse(ध्रुव AstClass* ध्रुव cls1p, ध्रुव AstClass* ध्रुव cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        यदि (cls1p == cls2p) प्रत्यागम सत्य;
        प्रति (ध्रुव AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            यदि (is_आधारः_Class_Recurse(cls1p, cextp->classp())) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    // Lines to end a message on nodep with the data types of its operands
    स्थिर string warnDTypes(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव Ast_ग्रन्थिः_DType* lhsDtp,
                             ध्रुव Ast_ग्रन्थिः_DType* rhsDtp, ध्रुव string& lhsName = "Left-hand",
                             ध्रुव string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        यदि (lhsState == rhsState) lhsState = rhsState = "";
        प्रत्यागम "\n" + nodep->warn_अधिकम्() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_अधिकम्()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    स्थिर तार्किक unpacked_Array_Sizes_मेलनम्(ध्रुव Ast_ग्रन्थिः_DType* lhsDtp, ध्रुव Ast_ग्रन्थिः_DType* rhsDtp) {
        यावत् (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ध्रुव AstUnpackArrayDType* ध्रुव lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ध्रुव AstUnpackArrayDType* ध्रुव rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            यदि (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                प्रत्यागम असत्य;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        प्रत्यागम सत्य;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    प्रतिरूपम् <प्रकारनाम T, प्रकारनाम N>
    शून्य checkUnpackedArrayAssignmentCompatible(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव T* ध्रुव lhsRefp,
                                                ध्रुव N* ध्रुव rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::मानम् || std::is_same<T, Ast_ग्रन्थिः_Var_Ref>::मानम्)
                && (std::is_same<N, AstVar>::मानम् || std::is_same<N, Ast_ग्रन्थिः_Var_Ref>::मानम्),
            "Unsupported types provided.");
        यदि (!lhsRefp || !rhsRefp) प्रत्यागम;
        std::string lhsName;
        std::string rhsName;
        यदि (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } अन्यथा {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsDtp = lhsRefp->dtypep()->skipRefp();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDtp = rhsRefp->dtypep()->skipRefp();
        ध्रुव तार्किक isLhsAggregate = lhsDtp->isAggregateType();
        ध्रुव तार्किक isRhsAggregate = rhsDtp->isAggregateType();
        यदि (!isLhsAggregate && !isRhsAggregate) प्रत्यागम;
        यदि (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            प्रत्यागम;
        } अन्यथा यदि (VN_IS(lhsDtp, QueueDType) && VN_IS(rhsDtp, रिक्तानि_Queue_DType)) {
            प्रत्यागम;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(असत्य),
                                      rhsDim = rhsDtp->dimensions(असत्य);
        // Check if unpacked array dimensions are matching
        यदि (lhsDim.second != rhsDim.second) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            प्रत्यागम;
        }

        ध्रुव Ast_ग्रन्थिः_DType* lhsDtpIterp = lhsDtp;
        ध्रुव Ast_ग्रन्थिः_DType* rhsDtpIterp = rhsDtp;
        प्रति (uint32_t dim = 0; dim < rhsDim.second; dim++) {
            // Associative arrays are compatible only with each other
            यदि (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                प्रत्यागम;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        यदि (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    शून्य check_Class_नियोजय(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* side, Ast_ग्रन्थिः* rhsp,
                          Ast_ग्रन्थिः_DType* ध्रुव lhsDTypep, तार्किक isOutputArg = असत्य) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव lhsRawDTypep = lhsDTypep->skipRefp();
        ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsRawDTypep = rhsp->dtypep()->skipRefp();
        यदि (ध्रुव AstClassRefDType* ध्रुव lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            यदि (ध्रुव AstClassRefDType* ध्रुव rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ध्रुव AstClass* ध्रुव dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ध्रुव AstClass* ध्रुव srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                यदि (is_आधारः_Class_Recurse(dstp, srcp)) प्रत्यागम;
            } अन्यथा यदि (rhsp->isNull()) {
                प्रत्यागम;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } अन्यथा यदि (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        यदि (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_पृष्ठभागः(&relinker);
            relinker.relink(
                नूतन Ast_Cvt_Unpack_गन्तव्यम्_Queue{rhsp->fileline(), VN_AS(rhsp, ग्रन्थिः_व्यञ्जकम्), lhsDTypep});
        }
    }
    स्थिर तार्किक similarDTypeRecurse(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव node1p,
                                    ध्रुव Ast_ग्रन्थिः_DType* ध्रुव node2p) {
        प्रत्यागम node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    शून्य iterateCheckFileDesc(Ast_ग्रन्थिः* parentp, Ast_ग्रन्थिः* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, PRELIM}.p());
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->findUInt32DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckReal(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findDoubleDType(), stage);
    }
    शून्य iterateCheckSigned8(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findSigned8DType(), stage);
    }
    शून्य iterateCheckSigned32(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findIntDType(), stage);
    }
    शून्य iterateCheckUInt32(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->findUInt32DType(), stage);
    }
    शून्य iterateCheckDelay(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        यदि (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, PRELIM}.p());
        }
        यदि (stage & FINAL) {
            Ast_ग्रन्थिः_DType* expDTypep;
            यदि (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->findDoubleDType();
            } अन्यथा {
                File_पङ्क्तिः* ध्रुव newFl = नूतन File_पङ्क्तिः{underp->fileline()};
                newFl->warnOff(V3_Error_संहिता::WIDTHEXPAND, सत्य);
                underp->fileline(newFl);
                expDTypep = parentp->findBitDType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, असत्य);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckString(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->findStringDType(), stage);
    }
    शून्य iterateCheckTyped(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                           Ast_ग्रन्थिः_DType* expDTypep, Stage stage) {
        यदि (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{expDTypep, PRELIM}.p());
        }
        यदि (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckTypedSelfPrelim(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                                     Ast_ग्रन्थिः_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        यदि (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, PRELIM}.p());
        }
        यदि (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckIntegralSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, सत्य);
    }
    शून्य iterateCheckSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                          Stage stage, तार्किक integralOnly = असत्य) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        यदि (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ग्रन्थिः_व्यञ्जकम्) ? checkCvtUS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), integralOnly) : underp;
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (शून्य)underp;  // cppcheck
    }
    शून्य iterateCheckSizedSelf(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                               Stage stage, तार्किक deferUntyped = असत्य) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        यदि (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ग्रन्थिः_व्यञ्जकम्) ? checkCvtUS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), असत्य) : underp;
        Ast_ग्रन्थिः_DType* ध्रुव expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ग्रन्थिः_DType* ध्रुव checkDtp = expDTypep->skip_Ref_गन्तव्यम्_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        यदि (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (शून्य)underp;  // cppcheck
    }
    शून्य iterate_Check_नियोजय(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* rhsp, Stage stage,
                            Ast_ग्रन्थिः_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        यदि (AstConst* constp = VN_CAST(rhsp, Const)) {
            यदि (ध्रुव AstUnpackArrayDType* ध्रुव arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                यदि (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    यदि (basicp->विस्तारः() == 8 && constp->अङ्कः().isFromString()) {
                        AstConsArray* newp = नूतन AstConsArray{
                            constp->fileline(), lhsDTypep,
                            नूतन AstConst{constp->fileline(), AstConst::Widthed_मानम्{}, 8, 0}};
                        प्रति (ध्रुव पूर्णाङ्क aindex : arrayp->declRange().seq_Lo_गन्तव्यम्_Hi()) {
                            पूर्णाङ्क cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_सङ्ख्या selected{constp, 8};
                            selected.opSel(constp->अङ्कः(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            यदि (!selected.isFourState()) {
                                यदि (ध्रुव uint32_t c = selected.गन्तव्यम्_UInt()) {
                                    newp->valuep(aindex,
                                                 नूतन AstConst{constp->fileline(),
                                                              AstConst::Widthed_मानम्{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(प्रक्षिप_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_नियोजय(parentp, side, rhsp, lhsDTypep);
        ध्रुव तार्किक lhs_प्रवाहः = (VN_IS(parentp, ग्रन्थिः_नियोजय)
                                && VN_IS(VN_AS(parentp, ग्रन्थिः_नियोजय)->lhsp(), ग्रन्थिः_प्रवाहः));
        rhsp = iterateCheck(parentp, side, rhsp, नियोजय_3, FINAL, lhsDTypep,
                            lhs_प्रवाहः ? EXTEND_OFF : EXTEND_LHS, सत्य, प्रवाहः_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (शून्य)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ग्रन्थिः_व्यञ्जकम्* convert_Unpacked_क्रमः(Ast_ग्रन्थिः_व्यञ्जकम्* exprp, Ast_ग्रन्थिः_DType* dtypep) {
        ध्रुव AstUnpackArrayDType* ध्रुव arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        यदि (!arrayp || !arrayp->is_Reverse_सम्बन्धि(exprp->dtypep())) प्रत्यागम exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_पृष्ठभागः(&relinker);
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन Ast_Cvt_Unpack_गन्तव्यम्_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        प्रत्यागम newp;
    }

    स्थिर तार्किक isLogicalOperator(ध्रुव Ast_ग्रन्थिः* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        प्रत्यागम VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    शून्य iterateCheckBool(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, विस्तारः_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_ग्रन्थिः_DType* ध्रुव underVDTypep = underp ? underp->dtypep()->skipRefp() : शून्यसूचकः;
        यदि (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstNeqD{parentp->fileline(), VN_AS(underp, ग्रन्थिः_व्यञ्जकम्),
                              नूतन AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } अन्यथा यदि (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः* ध्रुव newp
                = नूतन AstNeqN{parentp->fileline(), VN_AS(underp, ग्रन्थिः_व्यञ्जकम्),
                              नूतन AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } अन्यथा यदि (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_विस्तारः_Reduce(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्)), underp);  //परिवर्तितम्
        } अन्यथा यदि (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(नूतन AstConst{parentp->fileline(), AstConst::BitFalseErroring{}});
            VL_DO_DANGLING(प्रक्षिप_Deletep(underp), underp);
        } अन्यथा {
            ध्रुव तार्किक bad = विस्तारः_Bad(underp, parentp->findBitDType());
            यदि (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->विस्तारः(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->विस्तारः()
                                             << (underp->विस्तारः() != underp->विस्तारः_Min()
                                                     ? " or " + cvt_गन्तव्यम्_Str(underp->विस्तारः_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            यदि (Ast_व्यञ्जकम्_Stmt* ध्रुव व्यञ्जकम्_Stmtp = VN_CAST(underp, व्यञ्जकम्_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_विस्तारः_Reduce(व्यञ्जकम्_Stmtp->resultp());
                व्यञ्जकम्_Stmtp->dtypeFrom(व्यञ्जकम्_Stmtp->resultp());
            } अन्यथा यदि (underp->विस्तारः() != 1) {
                VL_DO_DANGLING(fix_विस्तारः_Reduce(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्)), underp);  //परिवर्तितम्
            }
        }
    }

    स्थिर शून्य checkEnum(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* parentp, ध्रुव Ast_ग्रन्थिः_DType* targetDTypep,
                          ध्रुव Ast_ग्रन्थिः_DType* sourceDTypep, ध्रुव Ast_ग्रन्थिः_व्यञ्जकम्* sourceExprp) {
        यदि (ध्रुव AstEnumDType* ध्रुव targetEnump
            = VN_CAST(targetDTypep->skip_Ref_गन्तव्यम्_Enump(), EnumDType)) {
            ध्रुव VCastable castable
                = Ast_ग्रन्थिः::computeCastable(targetEnump, sourceDTypep, sourceExprp);
            यदि (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << targetDTypep->prettyDTypeNameQ() << " from "
                                             << sourceDTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_अधिकम्()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_ग्रन्थिः* iterateCheck(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side, Ast_ग्रन्थिः* underp, Determ determ,
                          Stage stage, Ast_ग्रन्थिः_DType* expDTypep, Extend_नियमः extend_नियमः,
                          तार्किक warnOn = सत्य, प्रवाहः_Use प्रवाहः_Use_2 = प्रवाहः_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ध्रुव प्रवाहः_Use child_प्रवाहः_Use
            = प्रवाहः_Use_2 != प्रवाहः_USE_NONE ? प्रवाहः_Use_2 : प्रवाहः_Use_For_Determ(determ);
        यदि (rhsAccess.isReadOnly()) {
            यदि (Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_CAST(underp, ग्रन्थिः_व्यञ्जकम्)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_क्रमः(exprp, expDTypep);
            }
        }
        यदि (VN_IS(underp, ग्रन्थिः_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } अन्यथा यदि (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   विस्तारः_VP{expDTypep, FINAL, child_प्रवाहः_Use}.p());
        } अन्यथा यदि (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   विस्तारः_VP{expDTypep, FINAL, child_प्रवाहः_Use}.p());
        } अन्यथा यदि (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
            underp = userIterateSubtreeReturnEdits(oldp, विस्तारः_VP{SELF, FINAL, child_प्रवाहः_Use}.p());
        } अन्यथा यदि (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), सत्य, expDTypep->विस्तारः());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, विस्तारः_VP{SELF, FINAL, child_प्रवाहः_Use}.p());
        } अन्यथा यदि (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ग्रन्थिः* ध्रुव oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
            underp = userIterateSubtreeReturnEdits(oldp, विस्तारः_VP{SELF, FINAL, child_प्रवाहः_Use}.p());
        } अन्यथा {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव underDtp = underp->dtypep()->skipRefp();
            यदि (determ != नियोजय_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ग्रन्थिः* ध्रुव newp
                    = नूतन AstConst{underp->fileline(), AstConst::BitFalseErroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(प्रक्षिप_Deletep(underp), underp);
                underp = newp;
            }
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव expDtp = expDTypep->skipRefp();
            यदि (determ == नियोजय_3 && VN_IS(expDtp, ग्रन्थिः_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            यदि (determ == नियोजय_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_मेलनम्(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ध्रुव AstBasicDType* ध्रुव expBasicp = expDTypep->basicp();
            ध्रुव AstBasicDType* ध्रुव underBasicp = underp->dtypep()->basicp();
            यदि (expBasicp && underBasicp) {
                यदि (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_संहिता::ENUMVALUE)
                    && warnOn) {
                    यदि (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ग्रन्थिः_व्यञ्जकम्));
                    }
                    यदि (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, शून्यसूचकः);
                    }
                }
                Ast_ग्रन्थिः_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                यदि (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, विस्तारः_VP{SELF, FINAL, child_प्रवाहः_Use}.p());
                } अन्यथा यदि (determ == नियोजय_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    यदि (underp->isSigned() != subDTypep->isSigned()
                        || underp->विस्तारः() != subDTypep->विस्तारः()) {
                        subDTypep = parentp->findBitOrLogicDType(
                            std::max(subDTypep->विस्तारः(), underp->विस्तारः()),
                            std::max(subDTypep->विस्तारः_Min(), underp->विस्तारः_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, विस्तारः_VP{subDTypep, FINAL, child_प्रवाहः_Use}.p());
                } अन्यथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, विस्तारः_VP{subDTypep, FINAL, child_प्रवाहः_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                विस्तारः_Check_Sized(parentp, side, VN_AS(underp, ग्रन्थिः_व्यञ्जकम्), expDTypep, extend_नियमः,
                                warnOn);
            } अन्यथा यदि (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } अन्यथा यदि (ध्रुव AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ध्रुव AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                यदि (VN_IS(underp, Const) && VN_AS(underp, Const)->अङ्कः().isNull()) {
                    // '= null' is ok
                } अन्यथा यदि (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_द्वारा_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } अन्यथा यदि (expIfaceRefp->iface_द्वारा_Cellp() != underIfaceRefp->iface_द्वारा_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_द्वारा_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_द्वारा_Cellp()->prettyNameQ() << ").");
                } अन्यथा यदि (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } अन्यथा {
                    underp = userIterateSubtreeReturnEdits(
                        underp, विस्तारः_VP{expDTypep, FINAL, child_प्रवाहः_Use}.p());
                }
            } अन्यथा {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, विस्तारः_VP{expDTypep, FINAL, child_प्रवाहः_Use}.p());
            }
        }
        प्रत्यागम underp;
    }

    शून्य
    विस्तारः_Check_Sized(Ast_ग्रन्थिः* parentp, ध्रुव वर्ण* side,
                    Ast_ग्रन्थिः_व्यञ्जकम्* underp,  // Node to be checked or have typecast added in front of
                    Ast_ग्रन्थिः_DType* expDTypep, Extend_नियमः extend_नियमः, तार्किक warnOn = सत्य) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ध्रुव AstBasicDType* ध्रुव expBasicp = expDTypep->basicp();
        ध्रुव AstBasicDType* ध्रुव underBasicp = underp->dtypep()->basicp();
        यदि (expDTypep == underp->dtypep()) {
            प्रत्यागम;  // Same type must match
        } अन्यथा यदि (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#यदि VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#यदि_अन्तः
            प्रत्यागम;
        } अन्यथा {
            ध्रुव पूर्णाङ्क exp_विस्तारः = expDTypep->विस्तारः();
            पूर्णाङ्क exp_विस्तारः_Min = expDTypep->विस्तारः_Min();
            यदि (exp_विस्तारः_Min == 0) exp_विस्तारः_Min = exp_विस्तारः;
            ध्रुव तार्किक bad = विस्तारः_Bad(underp, expDTypep);
            यदि ((bad || underp->विस्तारः() != exp_विस्तारः) && fixAutoExtend(underp /*ref*/, exp_विस्तारः)) {
                underp = शून्यसूचकः;  // Changes underp
                प्रत्यागम;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            यदि (VN_IS(parentp->backp(), Cast_आकार)) warnOn = असत्य;
            यदि (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = असत्य;
            यदि (VN_IS(underp, Const) && VN_AS(underp, Const)->अङ्कः().isFromString()
                && exp_विस्तारः > underp->विस्तारः()
                && (((exp_विस्तारः - underp->विस्तारः()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = असत्य;
            }
            यदि ((VN_IS(parentp, सङ्कलय_2) && underp->विस्तारः() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->विस्तारः() == 1 && underp->isOne()
                    && 0 == std::वाक्यं_तुलय(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = असत्य;
            }
            यदि (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_विस्तारः, underp->विस्तारः(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_विस्तारः
                        << (exp_विस्तारः != exp_विस्तारः_Min ? " or " + cvt_गन्तव्यम्_Str(exp_विस्तारः_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->विस्तारः()
                        << (underp->विस्तारः() != underp->विस्तारः_Min()
                                ? " or " + cvt_गन्तव्यम्_Str(underp->विस्तारः_Min())
                                : "")
                        << " bits.");
            }
            यदि (bad || underp->विस्तारः() != exp_विस्तारः) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ध्रुव Ast_ग्रन्थिः_नियोजय* assignp = VN_CAST(parentp, ग्रन्थिः_नियोजय);
                ध्रुव AstPin* pinp = VN_CAST(parentp, Pin);
                यदि (assignp && VN_IS(assignp->lhsp(), ग्रन्थिः_प्रवाहः)) {
                } अन्यथा यदि (pinp && pinp->modVarp()->direction() != VDirection::निवेशः) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } अन्यथा यदि (assignp && VN_IS(underp, ग्रन्थिः_प्रवाहः) && exp_विस्तारः > underp->विस्तारः()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ध्रुव पूर्णाङ्क सरणम्_2 = exp_विस्तारः - underp->विस्तारः();
                    underp->unlink_Fr_पृष्ठभागः(&linker);
                    AstExtend* ध्रुव widenedp = नूतन AstExtend{underp->fileline(), underp};
                    widenedp->did_विस्तारः(सत्य);
                    // Shift left so zeros fill on the right
                    Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव shiftedp = नूतन Ast_सरणम्_L{
                        underp->fileline(), widenedp,
                        नूतन AstConst{underp->fileline(), static_cast<uint32_t>(सरणम्_2)}, exp_विस्तारः};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } अन्यथा {
                    VL_DO_DANGLING(fix_विस्तारः_Extend(underp, expDTypep, extend_नियमः), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ग्रन्थिः_व्यञ्जकम्* checkCvtUS(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, तार्किक fatal) {
        यदि (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            यदि (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } अन्यथा {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, असत्य, 32);
        }
        प्रत्यागम nodep;
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtD(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        यदि (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* newp;
            यदि (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = नूतन AstISToRD{nodep->fileline(), nodep};
            } अन्यथा {
                newp = नूतन AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            प्रत्यागम newp;
        } अन्यथा {
            प्रत्यागम nodep;
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtS(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, तार्किक warnOn, पूर्णाङ्क विस्तारः) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        यदि (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                यदि (constp->isDouble() && V3_सङ्ख्या::epsilonIntegral(constp->अङ्कः().गन्तव्यम्_Double())) {
                    warnOn = असत्य;
                }
            }
            यदि (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_समुच्चयः_Bit_Sized(विस्तारः, VSigning::SIGNED);
            प्रत्यागम newp;
        } अन्यथा {
            प्रत्यागम nodep;
        }
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* spliceCvtString(Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        यदि (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_पृष्ठभागः(&linker);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            प्रत्यागम newp;
        } अन्यथा {
            प्रत्यागम nodep;
        }
    }
    Ast_ग्रन्थिः_Biop* replaceWithUOrSVersion(Ast_ग्रन्थिः_Biop* nodep, तार्किक signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        यदि (signedFlavorNeeded == nodep->signedFlavor()) प्रत्यागम शून्यसूचकः;
        यदि (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        विकल्प (nodep->type()) {
        पक्ष VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::सङ्कलय_2: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        पक्ष VNType::सरणम्_L: nodep->dtypeChgSigned(signedFlavorNeeded); प्रत्यागम शून्यसूचकः;
        स्वभाव: भङ्ग;
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* newp = शून्यसूचकः;
        विकल्प (nodep->type()) {
        पक्ष VNType::Gt: newp = नूतन AstGtS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::GtS: newp = नूतन AstGt{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte: newp = नूतन AstGteS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::GteS: newp = नूतन AstGte{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt: newp = नूतन AstLtS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::LtS: newp = नूतन AstLt{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte: newp = नूतन AstLteS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::LteS: newp = नूतन AstLte{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Div: newp = नूतन AstDivS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::DivS: newp = नूतन AstDiv{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ModDiv: newp = नूतन AstModDivS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::ModDivS: newp = नूतन AstModDiv{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Mul: newp = नूतन AstMulS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::MulS: newp = नूतन AstMul{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::सरणम्_R: newp = नूतन Ast_सरणम्_RS{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::सरणम्_RS: newp = नूतन Ast_सरणम्_R{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_Biop* replaceWithDVersion(Ast_ग्रन्थिः_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        यदि (nodep->doubleFlavor()) प्रत्यागम शून्यसूचकः;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* newp = शून्यसूचकः;
        // No width change on output;...                // All below have bool or double outputs
        विकल्प (nodep->type()) {
        पक्ष VNType::सङ्कलय_2: newp = नूतन Ast_सङ्कलय_D{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Sub: newp = नूतन AstSubD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Pow: newp = नूतन AstPowD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Eq:
        पक्ष VNType::EqCase: newp = नूतन AstEqD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Neq:
        पक्ष VNType::NeqCase: newp = नूतन AstNeqD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gt:
        पक्ष VNType::GtS: newp = नूतन AstGtD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte:
        पक्ष VNType::GteS: newp = नूतन AstGteD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt:
        पक्ष VNType::LtS: newp = नूतन AstLtD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte:
        पक्ष VNType::LteS: newp = नूतन AstLteD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Div:
        पक्ष VNType::DivS: newp = नूतन AstDivD{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Mul:
        पक्ष VNType::MulS: newp = नूतन AstMulD{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_Biop* replaceWithNVersion(Ast_ग्रन्थिः_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        यदि (nodep->stringFlavor()) प्रत्यागम शून्यसूचकः;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Biop* newp = शून्यसूचकः;
        // No width change on output;...                // All below have bool or double outputs
        विकल्प (nodep->type()) {
        पक्ष VNType::Eq:
        पक्ष VNType::EqCase:
        पक्ष VNType::EqWild: newp = नूतन AstEqN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Neq:
        पक्ष VNType::NeqCase:
        पक्ष VNType::NeqWild: newp = नूतन AstNeqN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gt:
        पक्ष VNType::GtS: newp = नूतन AstGtN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Gte:
        पक्ष VNType::GteS: newp = नूतन AstGteN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lt:
        पक्ष VNType::LtS: newp = नूतन AstLtN{fl, lhsp, rhsp}; भङ्ग;
        पक्ष VNType::Lte:
        पक्ष VNType::LteS: newp = नूतन AstLteN{fl, lhsp, rhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_Uniop* replaceWithDVersion(Ast_ग्रन्थिः_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        यदि (nodep->doubleFlavor()) प्रत्यागम शून्यसूचकः;
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव lhsp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
        Ast_ग्रन्थिः_Uniop* newp = शून्यसूचकः;
        विकल्प (nodep->type()) {
        पक्ष VNType::Negate: newp = नूतन AstNegateD{fl, lhsp}; भङ्ग;
        स्वभाव:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            भङ्ग;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
        प्रत्यागम newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    शून्य replaceWithSFormat(Ast_विधानम्_Call* nodep, ध्रुव string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ध्रुव argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ग्रन्थिः_Var_Ref* ध्रुव fromp = VN_AS(nodep->fromp()->unlink_Fr_पृष्ठभागः(), VarRef);
        Ast_ग्रन्थिः* ध्रुव newp = नूतन Ast_नियोजय{
            nodep->fileline(), fromp,
            नूतन AstSFormatF{nodep->fileline(), format, असत्य, argp->exprp()->unlink_Fr_पृष्ठभागः()}};
        fromp->access(VAccess::WRITE);
        प्रक्षिप_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ग्रन्थिः_DType* iterateEditMoveDTypep(Ast_ग्रन्थिः* parentp, Ast_ग्रन्थिः_DType* dtnodep) {
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
        ध्रुव तार्किक child1 = (parentp->getChildDTypep() == dtnodep);
        ध्रुव तार्किक child2 = (parentp->getChild2DTypep() == dtnodep);
        यदि (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, शून्यसूचकः), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->getChildDTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_विस्तारः(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_पृष्ठभागः();
            v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(dtnodep);
        }
        यदि (!dtnodep->did_विस्तारः()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, शून्यसूचकः);
            UASSERT_OBJ(dtnodep->did_विस्तारः(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        प्रत्यागम dtnodep;
    }

    AstConst* dimension_मानम्(File_पङ्क्तिः* fileline, Ast_ग्रन्थिः_DType* nodep, VAttrType attrType,
                             पूर्णाङ्क dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ग्रन्थिः_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        प्रति (पूर्णाङ्क i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                declRange = adtypep->declRange();
                यदि (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                अनुवर्त;
            } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep
                       = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                भङ्ग;  // Sub elements don't look like arrays and can't iterate into
            } अन्यथा यदि (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(dtypep, BasicDType)) {
                यदि (adtypep->isRanged()) declRange = adtypep->declRange();
                भङ्ग;
            }
            भङ्ग;  // LCOV_EXCL_LINE
        }
        AstConst* valp = शून्यसूचकः;  // If nullptr, construct from val
        पूर्णाङ्क val = 0;
        विकल्प (attrType) {
        पक्ष VAttrType::DIM_द्वयङ्काः: {
            पूर्णाङ्क द्वयङ्काः = 1;
            यावत् (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव adtypep = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                    द्वयङ्काः *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    अनुवर्त;
                } अन्यथा यदि (ध्रुव Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव adtypep
                           = VN_CAST(dtypep, ग्रन्थिः_UOr_Struct_DType)) {
                    द्वयङ्काः *= adtypep->isStreamableFixedAggregate() ? adtypep->विस्तारः_प्रवाहः()
                                                                  : adtypep->विस्तारः();
                    भङ्ग;
                } अन्यथा यदि (ध्रुव AstBasicDType* ध्रुव adtypep = VN_CAST(dtypep, BasicDType)) {
                    द्वयङ्काः *= adtypep->विस्तारः();
                    भङ्ग;
                }
                भङ्ग;
            }
            यदि (dim == 0) {
                val = 0;
            } अन्यथा यदि (dim == 1 && !declRange.ranged()
                       && द्वयङ्काः == 1) {  // $bits should be sane for non-arrays
                val = nodep->विस्तारः();
            } अन्यथा {
                val = द्वयङ्काः;
            }
            भङ्ग;  // LCOV_EXCL_LINE
        }
        पक्ष VAttrType::DIM_उच्चः: val = !declRange.ranged() ? 0 : declRange.hi(); भङ्ग;
        पक्ष VAttrType::DIM_वामः: val = !declRange.ranged() ? 0 : declRange.वामतः(); भङ्ग;
        पक्ष VAttrType::DIM_नीचः: val = !declRange.ranged() ? 0 : declRange.lo(); भङ्ग;
        पक्ष VAttrType::DIM_दक्षिणः: val = !declRange.ranged() ? 0 : declRange.दक्षिणतः(); भङ्ग;
        पक्ष VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            भङ्ग;
        पक्ष VAttrType::DIM_आकार: val = !declRange.ranged() ? 0 : declRange.elements(); भङ्ग;
        स्वभाव: nodep->v3fatalSrc("Missing DIM ATTR type case"); भङ्ग;
        }
        यदि (!valp) valp = नूतन AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_गन्तव्यम्_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        प्रत्यागम valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ग्रन्थिः_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_सारणी_Map[std::make_pair(nodep, attrType)];
        यदि (cachedpr) प्रत्यागम cachedpr->clone_वृक्षः(असत्य);

        Ast_ग्रन्थिः_Array_DType* ध्रुव vardtypep = नूतन AstUnpackArrayDType{
            nodep->fileline(), nodep->findIntDType(), नूतन AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(vardtypep);
        userIterate(vardtypep, विस्तारः_VP{SELF, BOTH}.p());
        AstConsArray* ध्रुव consp = नूतन AstConsArray{nodep->fileline(), vardtypep};
        प्रति (अचिह्नित i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_मानम्(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(consp);
        VL_DO_DANGLING(consp->delete_वृक्षः(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_विस्तारः(सत्य);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_विस्तारः(सत्य);
        प्रक्षिप_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        प्रत्यागम cachedpr->clone_वृक्षः(असत्य);
    }
    स्थिर uint64_t enum_Max_मानम्(ध्रुव Ast_ग्रन्थिः* errNodep, ध्रुव AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        प्रति (ध्रुव AstEnumItem* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            ध्रुव AstConst* ध्रुव vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            यदि (!vconstp->अङ्कः().isAnyXZ() && vconstp->गन्तव्यम्_UQuad() >= maxval)
                maxval = vconstp->गन्तव्यम्_UQuad();
        }
        यदि (adtypep->itemsp()->विस्तारः() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            प्रत्यागम 64;
        }
        प्रत्यागम maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ध्रुव nodep, VAttrType attrType, तार्किक assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_सारणी_Map[std::make_pair(nodep, attrType)];
        यदि (cachedpr) प्रत्यागम cachedpr->clone_वृक्षः(असत्य);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        Ast_ग्रन्थिः_DType* basep;
        यदि (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->findStringDType();
        } अन्यथा यदि (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->findBitDType();
        } अन्यथा {
            basep = nodep->dtypep();
        }
        Ast_ग्रन्थिः_DType* vardtypep;
        यदि (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = नूतन AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } अन्यथा {
            vardtypep = नूतन AstUnpackArrayDType{flp, basep, नूतन AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->सङ्कलय_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = शून्यसूचकः;
        यदि (attrType == VAttrType::ENUM_NAME) {
            defaultp = नूतन AstConst{flp, AstConst::String{}, ""};
        } अन्यथा यदि (attrType == VAttrType::ENUM_अग्र || attrType == VAttrType::ENUM_पूर्व) {
            defaultp = नूतन AstConst{flp, V3_सङ्ख्या{nodep, nodep->विस्तारः(), 0}};
        } अन्यथा यदि (attrType == VAttrType::ENUM_VALID) {
            defaultp = नूतन AstConst{flp, AstConst::BitFalse{}};
        } अन्यथा {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ग्रन्थिः_व्यञ्जकम्*> मानानि;
        {
            uint64_t ordinal = 0;
            AstEnumItem* ध्रुव firstp = nodep->itemsp();
            ध्रुव AstEnumItem* prevp = firstp;  // Prev must start with last item
            यावत् (prevp->nextp()) prevp = VN_AS(prevp->nextp(), EnumItem);
            प्रति (AstEnumItem* itemp = firstp; itemp;) {
                AstEnumItem* ध्रुव nextp = VN_AS(itemp->nextp(), EnumItem);
                ध्रुव AstConst* ध्रुव vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                यदि (!vconstp->अङ्कः().isAnyXZ()) {  // Can 2-state runtime decode
                    ध्रुव uint64_t i = nodep->isWide() ? ++ordinal : vconstp->गन्तव्यम्_UQuad();
                    यदि (attrType == VAttrType::ENUM_NAME) {
                        मानानि[i] = नूतन AstConst{flp, AstConst::String{},
                                                 V3_सङ्ख्या::displayedEnumName(itemp)};
                    } अन्यथा यदि (attrType == VAttrType::ENUM_अग्र) {
                        मानानि[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_वृक्षः(असत्य);  // A const
                    } अन्यथा यदि (attrType == VAttrType::ENUM_पूर्व) {
                        मानानि[i] = prevp->valuep()->clone_वृक्षः(असत्य);  // A const
                    } अन्यथा यदि (attrType == VAttrType::ENUM_VALID) {
                        मानानि[i] = नूतन AstConst{flp, AstConst::BitTrue{}};
                    } अन्यथा {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = शून्यसूचकः;
        यदि (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ध्रुव consp = नूतन AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            प्रति (ध्रुव स्वचालित& itr : मानानि) {
                V3_सङ्ख्या कुञ्चिका{nodep, nodep->विस्तारः()};
                कुञ्चिका.समुच्चयः_Quad(itr.first);
                consp->सङ्कलय_Itemsp(नूतन AstConsAssocItem{flp, नूतन AstConst{flp, कुञ्चिका}, itr.second});
            }
            valuep = consp;
        } अन्यथा {
            AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, vardtypep, defaultp};
            प्रति (uint64_t i = 0; i < (msbdim + 1); ++i) {
                यदि (मानानि[i]) consp->valuep(i, मानानि[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::find(valuep);
        VL_DO_DANGLING(प्रक्षिप_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_विस्तारः(सत्य);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_विस्तारः(सत्य);
        प्रक्षिप_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        प्रत्यागम cachedpr->clone_वृक्षः(असत्य);
    }

    Ast_ग्रन्थिः_व्यञ्जकम्* enumSelect(Ast_ग्रन्थिः_व्यञ्जकम्* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        यदि (attrType == VAttrType::ENUM_NAME) {
            यदि (nodep->is_विशुद्धम्()) nodep = V3Const::constifyEdit(nodep);
            यदि (ध्रुव AstConst* ध्रुव constp = VN_CAST(nodep, Const)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->अङ्कः().displayedEnumName(adtypep)};
                VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
                प्रत्यागम newp;
            }
        }
        यदि (attrType == VAttrType::ENUM_NAME && !nodep->is_विशुद्धम्() && (m_modep || m_ftaskp)) {
            Ast_व्यञ्जकम्_Stmt* ध्रुव capturep = newEnumCapture(nodep, adtypep);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव valuep = capturep->resultp()->unlink_Fr_पृष्ठभागः();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_समुच्चयः_String();
            प्रत्यागम capturep;
        }
        यदि (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ग्रन्थिः_व्यञ्जकम्* indexp = नूतन AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            प्रति (ध्रुव AstEnumItem* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), EnumItem)) {
                ध्रुव AstConst* ध्रुव constp = VN_AS(itemp->valuep(), Const);
                यदि (constp->अङ्कः().isAnyXZ()) अनुवर्त;
                indexp = नूतन AstCond{nodep->fileline(),
                                     नूतन AstEq{nodep->fileline(), nodep->clone_वृक्षः(असत्य),
                                               itemp->valuep()->clone_वृक्षः(असत्य)},
                                     नूतन AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ध्रुव tabRefp = enumVarRefp(adtypep, attrType, असत्य, ordinal);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव newp = नूतन AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_समुच्चयः_String();
            VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
            प्रत्यागम newp;
        }
        ध्रुव uint64_t msbdim = enum_Max_मानम्(nodep, adtypep);
        ध्रुव तार्किक assoc = msbdim > ENUM_LOOKUP_द्वयङ्काः;
        Ast_ग्रन्थिः_व्यञ्जकम्* newp;
        यदि (assoc) {
            AstVarRef* ध्रुव tabRefp = enumVarRefp(adtypep, attrType, सत्य, 0);
            newp = नूतन AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } अन्यथा {
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ध्रुव tabRefp
                = enumVarRefp(adtypep, attrType, असत्य, (1ULL << selwidth) - 1);
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->विस्तारः() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? नूतन AstGt{nodep->fileline(), nodep->clone_वृक्षः(असत्य),
                                  नूतन AstConst{nodep->fileline(),
                                               V3_सङ्ख्या{nodep, adtypep->विस्तारः(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : शून्यसूचकः;
            newp = नूतन AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                नूतन AstSel{nodep->fileline(), nodep, 0, selwidth}};
            यदि (boundp) {
                newp = नूतन AstCond{nodep->fileline(), boundp,
                                   नूतन AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        यदि (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_समुच्चयः_String();
        } अन्यथा यदि (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_समुच्चयः_Bit();
        } अन्यथा {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        प्रत्यागम newp;
    }
    Ast_ग्रन्थिः_व्यञ्जकम्* enumTestValid(Ast_ग्रन्थिः_व्यञ्जकम्* valp, AstEnumDType* enumDtp) {
        ध्रुव uint64_t maxval = enum_Max_मानम्(valp, enumDtp);
        ध्रुव तार्किक assoc = maxval > ENUM_LOOKUP_द्वयङ्काः;
        Ast_ग्रन्थिः_व्यञ्जकम्* testp = शून्यसूचकः;
        File_पङ्क्तिः* ध्रुव fl_novalue = नूतन File_पङ्क्तिः{valp->fileline()};
        fl_novalue->warnOff(V3_Error_संहिता::ENUMVALUE, सत्य);
        fl_novalue->warnOff(V3_Error_संहिता::CMPCONST, सत्य);
        यदि (assoc) {
            AstVarRef* ध्रुव tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, सत्य, 0);
            testp = नूतन AstAssocSel{fl_novalue, tabRefp, valp->clone_वृक्षः_विशुद्धम्(असत्य)};
        } अन्यथा {
            ध्रुव पूर्णाङ्क selwidth = V3_सङ्ख्या::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ध्रुव tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, असत्य, (1ULL << selwidth) - 1);
            File_पङ्क्तिः* ध्रुव fl_nowidth = नूतन File_पङ्क्तिः{fl_novalue};
            fl_nowidth->warnOff(V3_Error_संहिता::विस्तारः_2, सत्य);
            testp = नूतन AstCond{
                fl_novalue,
                नूतन AstGt{fl_nowidth, valp->clone_वृक्षः_विशुद्धम्(असत्य),
                          नूतन AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                नूतन AstConst{fl_novalue, AstConst::BitFalse{}},
                नूतन AstArraySel{fl_novalue, tabRefp,
                                नूतन AstSel{fl_novalue, valp->clone_वृक्षः_विशुद्धम्(असत्य), 0, selwidth}}};
        }
        प्रत्यागम testp;
    }

    PatVecMap pat_सदिशम्_Map(Ast_आदर्शः* nodep, ध्रुव VNumRange& range) {
        PatVecMap patmap;
        पूर्णाङ्क element = range.वामतः();
        प्रति (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            यदि (patp->keyp()) {
                यदि (ध्रुव AstConst* ध्रुव constp = आदर्शः_Const_Keyp(patp)) {
                    यदि (constp->अङ्कः().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } अन्यथा {
                        element = constp->गन्तव्यम्_SInt();
                    }
                }
            }
            ध्रुव तार्किक new_प्रविष्टिः = patmap.emplace(element, patp).second;
            यदि (!new_प्रविष्टिः) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.वामः_गन्तव्यम्_दक्षिणः_Inc();
        }
        प्रत्यागम patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    शून्य patConcatConvertRecurse(Ast_आदर्शः* patternp, Ast_ग्रन्थिः_व्यञ्जकम्* nodep) {
        यदि (AstConcat* ध्रुव concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } अन्यथा {
            AstPatMember* ध्रुव newp
                = नूतन AstPatMember{nodep->fileline(), nodep->unlink_Fr_पृष्ठभागः(), शून्यसूचकः, शून्यसूचकः};
            newp->isConcat(सत्य);
            patternp->सङ्कलय_Itemsp(newp);
        }
    }

    शून्य makeOpenArrayShell(Ast_ग्रन्थिः_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ग्रन्थिः_FTask* ध्रुव oldTaskp = nodep->taskp();
        oldTaskp->dpiOpenParentInc();
        UASSERT_OBJ(!oldTaskp->dpiOpenChild(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ग्रन्थिः_FTask* ध्रुव newTaskp = oldTaskp->clone_वृक्षः(असत्य);
        newTaskp->dpiOpenChild(सत्य);
        newTaskp->dpiOpenParentClear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_गन्तव्यम्_Str(oldTaskp->dpiOpenParent()));
        oldTaskp->सङ्कलय_अग्र_अत्र(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ध्रुव V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        प्रति (ध्रुव स्वचालित& tconnect : tconnects) {
            AstVar* ध्रुव portp = tconnect.first;
            ध्रुव AstArg* ध्रुव argp = tconnect.second;
            ध्रुव Ast_ग्रन्थिः* ध्रुव pinp = argp->exprp();
            यदि (!pinp) अनुवर्त;  // Argument error we'll find later
            यदि (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ग्रन्थिः_DType* ध्रुव oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ग्रन्थिः* nodep) {
                    यदि (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    तार्किक markHasOpenArray(Ast_ग्रन्थिः_FTask* nodep) {
        तार्किक hasOpen = असत्य;
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstVar* ध्रुव portp = VN_CAST(stmtp, Var)) {
                यदि (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(सत्य);
                    hasOpen = सत्य;
                }
            }
        }
        प्रत्यागम hasOpen;
    }
    तार्किक hasOpenArrayDTypeRecurse(Ast_ग्रन्थिः_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        यदि (VN_IS(nodep, UnsizedArrayDType)) प्रत्यागम सत्य;
        यदि (nodep->subDTypep()) प्रत्यागम hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        प्रत्यागम असत्य;
    }
    Ast_ग्रन्थिः_DType* dtypeNotIntAtomOrVecRecurse(Ast_ग्रन्थिः_DType* nodep, तार्किक ranged = असत्य) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_गन्तव्यम्_Enump();
        यदि (AstBasicDType* ध्रुव dtp = VN_CAST(nodep, BasicDType)) {
            यदि (ranged && (!dtp->isBitLogic() || dtp->isRanged()))
                प्रत्यागम dtp;  // Packed when already packed
            यदि (dtp->keyword().isIntNumeric()) प्रत्यागम शून्यसूचकः;
            प्रत्यागम dtp;
        } अन्यथा यदि (AstPackArrayDType* ध्रुव dtp = VN_CAST(nodep, PackArrayDType)) {
            यदि (ranged) प्रत्यागम dtp;  // Packed when already packed
            प्रत्यागम dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), सत्य);
        }
        प्रत्यागम nodep;
    }
    Ast_ग्रन्थिः_DType* dtypeNot4StateIntegralRecurse(Ast_ग्रन्थिः_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        यदि (AstBasicDType* ध्रुव dtp = VN_CAST(nodep, BasicDType)) {
            यदि (!dtp->keyword().isFourstate()) प्रत्यागम dtp;
            प्रत्यागम शून्यसूचकः;
        } अन्यथा यदि (Ast_ग्रन्थिः_Array_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_Array_DType)) {
            प्रत्यागम dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } अन्यथा यदि (Ast_ग्रन्थिः_UOr_Struct_DType* ध्रुव dtp = VN_CAST(nodep, ग्रन्थिः_UOr_Struct_DType)) {
            प्रति (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ग्रन्थिः_DType* ध्रुव badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                यदि (badDtp) प्रत्यागम badDtp;
            }
            प्रत्यागम शून्यसूचकः;
        }
        प्रत्यागम nodep;
    }
    शून्य checkForceReleaseLhs(Ast_ग्रन्थिः* nodep, Ast_ग्रन्थिः* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        यदि (Ast_ग्रन्थिः* ध्रुव selNodep = V3_विस्तारः::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    शून्य assert_At_व्यञ्जकम्(Ast_ग्रन्थिः* nodep) {
        यदि (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    शून्य assertAtStatement(Ast_ग्रन्थिः* nodep) {
        यदि (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    शून्य checkConstantOrReplace(Ast_ग्रन्थिः* nodep, तार्किक noFourState, ध्रुव string& सन्देशः) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ध्रुव constp = VN_CAST(nodep, Const);
        यदि (!constp || (noFourState && constp->अङ्कः().isFourState())) {
            nodep->v3error(सन्देशः);
            nodep->replaceWith(नूतन AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(प्रक्षिप_Deletep(nodep), nodep, constp);
            प्रत्यागम;
        }
    }
    Ast_ग्रन्थिः* ग्रन्थिः_For_Unsized_Warning(Ast_ग्रन्थिः* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        यदि (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->विस्तारः_Sized()) {
            प्रत्यागम nodep->op1p();
        } अन्यथा यदि (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->विस्तारः_Sized()) {
            प्रत्यागम nodep->op2p();
        }
        प्रत्यागम nodep;  // By default return this
    }
    AstRefDType* check_Ref_गन्तव्यम्_Typedef_Recurse(Ast_ग्रन्थिः* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        यदि (VL_LIKELY(!nodep)) प्रत्यागम शून्यसूचकः;
        यदि (स्वचालित* ध्रुव refp = VN_CAST(nodep, RefDType)) {
            यदि (refp->typedefp() == typedefp) प्रत्यागम refp;
        }
        यदि (स्वचालित* ध्रुव refp = check_Ref_गन्तव्यम्_Typedef_Recurse(nodep->op1p(), typedefp)) प्रत्यागम refp;
        यदि (स्वचालित* ध्रुव refp = check_Ref_गन्तव्यम्_Typedef_Recurse(nodep->op2p(), typedefp)) प्रत्यागम refp;
        यदि (स्वचालित* ध्रुव refp = check_Ref_गन्तव्यम्_Typedef_Recurse(nodep->op3p(), typedefp)) प्रत्यागम refp;
        यदि (स्वचालित* ध्रुव refp = check_Ref_गन्तव्यम्_Typedef_Recurse(nodep->op4p(), typedefp)) प्रत्यागम refp;
        प्रत्यागम शून्यसूचकः;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    स्थिर तार्किक isIfaceArrayDType(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        यदि (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) प्रत्यागम असत्य;
        ध्रुव AstIfaceRefDType* ध्रुव irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        प्रत्यागम irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    स्थिर std::string unpacked_Array_आकार(ध्रुव Ast_ग्रन्थिः_DType* dtypep) {
        std::string आकार;
        dtypep = dtypep->skipRefp();
        यावत् (ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            यदि (!आकार.रिक्तानि()) आकार += "x";
            आकार += std::गन्तव्यम्_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        प्रत्यागम आकार;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    स्थिर Ast_ग्रन्थिः_व्यञ्जकम्* newIfaceArrayElement(Ast_ग्रन्थिः_व्यञ्जकम्* fromp, पूर्णाङ्क bit) {
        AstConsArray* ध्रुव consp = VN_CAST(fromp, ConsArray);
        यदि (!consp || !isIfaceArrayDType(consp->dtypep())) प्रत्यागम शून्यसूचकः;
        ध्रुव AstUnpackArrayDType* ध्रुव arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        यदि (bit < 0 || bit >= arrp->elementsConst()) प्रत्यागम शून्यसूचकः;
        प्रत्यागम consp->valuep(bit)->clone_वृक्षः(असत्य);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ग्रन्थिः_व्यञ्जकम्* new_Iface_Array_मानम्(ध्रुव Ast_ग्रन्थिः_Var_Ref* refp, Ast_ग्रन्थिः_DType* dtypep,
                                    ध्रुव std::string& name, ध्रुव std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_पङ्क्तिः* ध्रुव flp = refp->fileline();
        AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        यदि (!arrp) {
            AstVar* ध्रुव varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ग्रन्थिः_Var_Ref* newp;
            यदि (ध्रुव AstVarXRef* ध्रुव xrefp = VN_CAST(refp, VarXRef)) {
                newp = नूतन AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } अन्यथा {
                newp = नूतन AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, विस्तारः_VP{SELF, BOTH}.p());
            प्रत्यागम newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ध्रुव consp = नूतन AstConsArray{flp, arrp};
        ध्रुव VNumRange range = arrp->declRange();
        प्रति (ध्रुव पूर्णाङ्क i : range.seq_वामः_गन्तव्यम्_दक्षिणः()) {
            ध्रुव std::string s = name + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_मानम्(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        प्रत्यागम consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    शून्य replace_विभज_Iface_Array_Ref(Ast_ग्रन्थिः_Var_Ref* nodep) {
        AstVar* ध्रुव varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_विभज(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        यदि (varp->isIfaceParent()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_मानम्(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    शून्य expandIfaceArrayPinDimensions(AstPin* nodep, ध्रुव Ast_ग्रन्थिः_DType* dtypep,
                                       Ast_ग्रन्थिः_व्यञ्जकम्* conp, ध्रुव std::string& suffix,
                                       AstVar*& elemVarpr) {
        ध्रुव AstUnpackArrayDType* ध्रुव arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        यदि (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ध्रुव newp = nodep->clone_वृक्षः(असत्य);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            यदि (conp) newp->exprp(conp->clone_वृक्षः(असत्य));
            nodep->सङ्कलय_अग्र_अत्र(newp);
            प्रत्यागम;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ध्रुव VNumRange range = arrp->declRange();
        प्रति (पूर्णाङ्क n = range.elements() - 1; n >= 0; --n) {
            ध्रुव पूर्णाङ्क i = range.वामतः() + n * range.वामः_गन्तव्यम्_दक्षिणः_Inc();
            ध्रुव std::string s = suffix + "__BRA__" + Ast_ग्रन्थिः::encode_सङ्ख्या(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव elemConnp = [&]() -> Ast_ग्रन्थिः_व्यञ्जकम्* {
                यदि (!conp) प्रत्यागम शून्यसूचकः;
                ध्रुव AstUnpackArrayDType* ध्रुव conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ध्रुव पूर्णाङ्क elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                प्रत्यागम resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    शून्य expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ध्रुव portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_विभज(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        प्रति (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव exprp = VN_AS(nodep->exprp(), ग्रन्थिः_व्यञ्जकम्);
        यदि (exprp) प्रक्षिप_Deletep(exprp->unlink_Fr_पृष्ठभागः());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ध्रुव तार्किक connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_आकार(exprp->dtypep()) == unpacked_Array_आकार(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : शून्यसूचकः, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(प्रक्षिप_Deletep(nodep->unlink_Fr_पृष्ठभागः()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ग्रन्थिः* userIterateSubtreeReturnEdits(Ast_ग्रन्थिः* nodep, विस्तारः_VP* vup) {
        यदि (!nodep) प्रत्यागम शून्यसूचकः;
        Ast_ग्रन्थिः* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        प्रत्यागम ret;
    }
    शून्य userIterate(Ast_ग्रन्थिः* nodep, विस्तारः_VP* vup) {
        यदि (!nodep) प्रत्यागम;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    शून्य user_Iterate_And_अग्र(Ast_ग्रन्थिः* nodep, विस्तारः_VP* vup) {
        यदि (!nodep) प्रत्यागम;
        यदि (nodep->did_विस्तारः()) प्रत्यागम;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_अग्र_Null(nodep);
    }
    शून्य userIterateChildren(Ast_ग्रन्थिः* nodep, विस्तारः_VP* vup) {
        यदि (!nodep) प्रत्यागम;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    शून्य userIterateChildrenBackwardsConst(Ast_ग्रन्थिः* nodep, विस्तारः_VP* vup) {
        यदि (!nodep) प्रत्यागम;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

सार्वजनिक:
    // CONSTRUCTORS
    विस्तारः_Visitor(तार्किक paramsOnly,  // [in] TRUE if we are considering parameters only.
                 तार्किक doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_अस्थायी_नामानि{"__Venum"}
        , m_inside_अस्थायी_नामानि{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ग्रन्थिः* mainAcceptEdit(Ast_ग्रन्थिः* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, विस्तारः_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        प्रति (AstVar* ध्रुव varp : m_विभज_Iface_Array_Varps) {
            VL_DO_DANGLING(प्रक्षिप_Deletep(varp->unlink_Fr_पृष्ठभागः()), varp);
        }
        m_विभज_Iface_Array_Varps.clear();
        प्रत्यागम nodep;
    }
    स्थिर तार्किक अधस्तनम्_As_Fixed_Aggregate(ध्रुव Ast_ग्रन्थिः_DType* ध्रुव dtypep) {
        प्रत्यागम dtypep->isStreamableFixedAggregate() && dtypep->अन्तर्भवति_Unpacked_Struct();
    }
    शून्य प्रवाहः_नियोजय_अधस्तनम्(Ast_ग्रन्थिः_नियोजय* nodep) {
        Ast_ग्रन्थिः_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        यदि (Ast_ग्रन्थिः_प्रवाहः* ध्रुव streamp = VN_CAST(nodep->rhsp(), ग्रन्थिः_प्रवाहः)) {
            Ast_ग्रन्थिः_DType* ध्रुव lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ध्रुव पूर्णाङ्क lwidth = lhsDTypeSkippedRefp->विस्तारः_प्रवाहः();
            ध्रुव पूर्णाङ्क rwidth = streamp->lhsp()->dtypep()->skipRefp()->विस्तारः_प्रवाहः();
            यदि (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            यदि (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), QueueDType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), QueueDType)) {
                ध्रुव पूर्णाङ्क queue_Element_आकार = streamp->lhsp()->dtypep()->subDTypep()->विस्तारः();
                UASSERT_OBJ(queue_Element_आकार <= lwidth, nodep, "LHS < RHS");
            }
            यदि (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || अधस्तनम्_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_पृष्ठभागः();
                nodep->rhsp(
                    नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        यदि (Ast_ग्रन्थिः_प्रवाहः* ध्रुव streamp = VN_CAST(nodep->lhsp(), ग्रन्थिः_प्रवाहः)) {
            ध्रुव Ast_ग्रन्थिः_DType* ध्रुव rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ग्रन्थिः_DType* ध्रुव lhs_प्रवाहः_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ध्रुव पूर्णाङ्क lwidth = lhs_प्रवाहः_DTypep->विस्तारः_प्रवाहः();
            ध्रुव पूर्णाङ्क rwidth = rhsDTypep->विस्तारः_प्रवाहः();
            यदि (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            यदि (अधस्तनम्_As_Fixed_Aggregate(lhs_प्रवाहः_DTypep)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव प्रवाहः_Exprp = nodep->lhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव dstp = streamp->lhsp()->unlink_Fr_पृष्ठभागः();
                Ast_ग्रन्थिः_व्यञ्जकम्* srcp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                यदि (VN_IS(streamp, प्रवाहः_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_समुच्चयः_Bit_Or_Logic_Unsized(
                        srcp->विस्तारः(), srcp->विस्तारः_Min(), VSigning::UNSIGNED,
                        Ast_ग्रन्थिः_व्यञ्जकम्::isFourstateOrNoDtype(प्रवाहः_Exprp, srcp));
                    srcp = प्रवाहः_Exprp;
                } अन्यथा {
                    यदि (srcp->विस्तारः() > lwidth) {
                        srcp = नूतन AstSel{streamp->fileline(), srcp, srcp->विस्तारः() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(प्रक्षिप_Deletep(प्रवाहः_Exprp), प्रवाहः_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(नूतन Ast_Cvt_Packed_गन्तव्यम्_Array{srcp->fileline(), srcp, lhs_प्रवाहः_DTypep});
                nodep->dtypeFrom(dstp);
            } अन्यथा यदि (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ग्रन्थिः_व्यञ्जकम्* ध्रुव rhsp = nodep->rhsp()->unlink_Fr_पृष्ठभागः();
                nodep->rhsp(नूतन Ast_Cvt_Array_गन्तव्यम्_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~विस्तारः_Visitor() override = स्वभाव;
};

//######################################################################
// Width class functions

शून्य V3_विस्तारः::विस्तारः(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ध्रुव विस्तारः_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ध्रुव VDTypeNameScopes scopes{nodep};
        विस्तारः_Visitor visitor{असत्य, असत्य};
        (शून्य)visitor.mainAcceptEdit(nodep);
        विस्तारः_Remove_Visitor rvisitor;
        (शून्य)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("width", 0, dump_वृक्षः_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ग्रन्थिः* V3_विस्तारः::विस्तारः_Params_Edit(Ast_ग्रन्थिः* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    विस्तारः_Visitor visitor{सत्य, असत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    प्रत्यागम nodep;
}

शून्य V3_विस्तारः::प्रवाहः_नियोजय_अधस्तनम्_Edit(Ast_ग्रन्थिः_नियोजय* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    विस्तारः_Visitor visitor{असत्य, असत्य};
    visitor.प्रवाहः_नियोजय_अधस्तनम्(nodep);
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
Ast_ग्रन्थिः* V3_विस्तारः::विस्तारः_Generate_Params_Edit(
    Ast_ग्रन्थिः* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    विस्तारः_Visitor visitor{सत्य, सत्य};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    प्रत्यागम nodep;
}

Ast_ग्रन्थिः* V3_विस्तारः::selectNonConstantRecurse(Ast_ग्रन्थिः* nodep, तार्किक अन्तर्_Sel) {
    // If node has a non-constant select, return that select
    Ast_ग्रन्थिः* resultp = शून्यसूचकः;
    यदि (Ast_ग्रन्थिः_Sel* ध्रुव anodep = VN_CAST(nodep, ग्रन्थिः_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), अन्तर्_Sel);
        यदि (resultp) प्रत्यागम resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), सत्य);
    } अन्यथा यदि (AstSel* ध्रुव anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), अन्तर्_Sel);
        यदि (resultp) प्रत्यागम resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), सत्य);
    } अन्यथा यदि (Ast_ग्रन्थिः_Var_Ref* ध्रुव anodep = VN_CAST(nodep, ग्रन्थिः_Var_Ref)) {
        यदि (अन्तर्_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) प्रत्यागम anodep;
    } अन्यथा {
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, अन्तर्_Sel);
        यदि (resultp) प्रत्यागम resultp;
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, अन्तर्_Sel);
        यदि (resultp) प्रत्यागम resultp;
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, अन्तर्_Sel);
        यदि (resultp) प्रत्यागम resultp;
        यदि (Ast_ग्रन्थिः* ध्रुव refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, अन्तर्_Sel);
    }
    प्रत्यागम resultp;
}
