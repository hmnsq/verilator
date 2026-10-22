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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Width.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Assert.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Begin.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3ConstPool.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Global.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3LinkLValue.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Number.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Randomize.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3UniqueNames.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3WidthCommit.h"

// More code; this file was getting too large; see actions there
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3_ਚੌੜਾਈ_CPP
#ਅਨ੍ਤਰ੍ਭਾਵ "V3WidthRemove.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

ਵਰਗ ContainingClassFinder final {
    // The cache stores the class containing each node, excluding the node itself when it is a
    // class. This lets recursion through preceding siblings reuse the same ownership result.
    // Outside of V3Width, where the whole netlist is iterated, instead of this technique,
    // parents should be remembered during the top-down visit recursion into classes.
    std::unordered_map<ਅਚਲ Ast_ਗੰਢ*, AstClass*> m_cache;

    AstClass* ਲੱਭੋ_Cached(Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = m_cache.ਲੱਭੋ(nodep);
        ਜੇ (it != m_cache.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->backp() && nodep->backp()->nextp() == nodep) {
            classp = ਲੱਭੋ_Cached(nodep->backp());
        } ਨਹੀਂਤਾਂ ਜੇ (AstClass* ਅਚਲ parentp = VN_CAST(nodep->backp(), Class)) {
            classp = parentp;
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassPackage* ਅਚਲ packagep = VN_CAST(nodep->backp(), ClassPackage)) {
            classp = packagep->classp();
        } ਨਹੀਂਤਾਂ {
            classp = ਲੱਭੋ_Cached(nodep->backp());
        }
        m_cache.emplace(nodep, classp);
        ਵਾਪਸ classp;
    }

ਜਨਤਕ:
    AstClass* ਲੱਭੋ(Ast_ਗੰਢ* nodep) {
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) ਵਾਪਸ classp;
        ਜੇ (AstClassPackage* ਅਚਲ packagep = VN_CAST(nodep, ClassPackage)) {
            ਵਾਪਸ packagep->classp();
        }
        ਵਾਪਸ ਲੱਭੋ_Cached(nodep);
    }
};

//######################################################################

ਗਿਣਤੀ Stage : uint8_t {
    PRELIM = 1,
    FINAL = 2,
    BOTH = 3
};  // Numbers are a bitmask <0>=prelim, <1>=final
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& str, ਅਚਲ Stage& rhs) {
    ਵਾਪਸ str << ("-PFB"[static_cast<ਪੂਰਨ>(rhs)]);
}

ਗਿਣਤੀ Determ : uint8_t {
    SELF,  // Self-determined
    CONTEXT_DET,  // Context-determined
    ਸੌਂਪੋ_3  // Assignment-like where sign comes from RHS only
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& str, ਅਚਲ Determ& rhs) {
    ਸਥਿਰ constexpr ਅਚਲ ਅੱਖਰ* ਅਚਲ s_det[] = {"SELF", "CNTX", "ASSN"};
    ਵਾਪਸ str << s_det[rhs];
}

ਗਿਣਤੀ ਧਾਰਾ_Use : uint8_t {
    ਧਾਰਾ_USE_NONE,  // Stream concatenation is not allowed in this context
    ਧਾਰਾ_USE_ਸੌਂਪੋ,  // Assignment-like context permits stream concatenation
    ਧਾਰਾ_USE_CAST,  // Explicit cast context permits stream concatenation
};

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ v3widthWarn(lhs, rhs, ਸੁਨੇਹਾ_2) \
    v3warn_ਕੋਡ(((lhs) < (rhs)   ? V3_Error_ਕੋਡ::WIDTHTRUNC \
                : (lhs) > (rhs) ? V3_Error_ਕੋਡ::WIDTHEXPAND \
                                : V3_Error_ਕੋਡ::ਚੌੜਾਈ_2), \
               ਸੁਨੇਹਾ_2)

//######################################################################
// Width state, as a visitor of each AstNode

ਵਰਗ ਚੌੜਾਈ_VP final {
    // Parameters to pass down hierarchy with visit functions.
    Ast_ਗੰਢ_DType* ਅਚਲ m_dtypep;  // Parent's data type to resolve to
    ਅਚਲ Stage m_stage;  // If true, report errors
    ਅਚਲ ਧਾਰਾ_Use m_ਧਾਰਾ_Use;  // Current expression may be a stream concat

ਜਨਤਕ:
    ਚੌੜਾਈ_VP(Ast_ਗੰਢ_DType* dtypep, Stage stage, ਧਾਰਾ_Use ਧਾਰਾ_Use_2 = ਧਾਰਾ_USE_NONE)
        : m_dtypep{dtypep}
        , m_stage{stage}
        , m_ਧਾਰਾ_Use{ਧਾਰਾ_Use_2} {
        // Prelim doesn't look at assignments, so shouldn't need a dtype,
        // however AstPattern uses them
    }
    ਚੌੜਾਈ_VP(Determ determ, Stage stage, ਧਾਰਾ_Use ਧਾਰਾ_Use_2 = ਧਾਰਾ_USE_NONE)
        : m_dtypep{ਖਾਲੀ_ਸੂਚਕ}
        , m_stage{stage}
        , m_ਧਾਰਾ_Use{ਧਾਰਾ_Use_2} {
        ਜੇ (determ != SELF && stage != PRELIM)
            v3fatalSrc("Context-determined width request only allowed as prelim step");
    }
    ਚੌੜਾਈ_VP* p() { ਵਾਪਸ ਇਹ; }
    ਤਾਰਕਿਕ selfDtm() ਅਚਲ { ਵਾਪਸ m_dtypep == ਖਾਲੀ_ਸੂਚਕ; }
    ਧਾਰਾ_Use ਧਾਰਾ_Use_2() ਅਚਲ { ਵਾਪਸ m_ਧਾਰਾ_Use; }
    Ast_ਗੰਢ_DType* dtypep() ਅਚਲ {
        // Detect where overrideDType is probably the intended call
        UASSERT(m_dtypep, "Width dtype request on self-determined or preliminary VUP");
        ਵਾਪਸ m_dtypep;
    }
    Ast_ਗੰਢ_DType* dtypeNullp() ਅਚਲ { ਵਾਪਸ m_dtypep; }
    Ast_ਗੰਢ_DType* dtypeNullSkipRefp() ਅਚਲ {
        Ast_ਗੰਢ_DType* dtp = dtypeNullp();
        ਜੇ (dtp) dtp = dtp->skipRefp();
        ਵਾਪਸ dtp;
    }
    Ast_ਗੰਢ_DType* dtypeOverridep(Ast_ਗੰਢ_DType* ਅਚਲ defaultp) ਅਚਲ {
        UASSERT(m_stage != PRELIM, "Parent dtype should be a final-stage action");
        ਜੇ (!m_dtypep) ਵਾਪਸ defaultp;
        ਜੇ (m_dtypep->skipRefp()->isIntegralOrPacked()
            && defaultp->skipRefp()->isIntegralOrPacked()
            && m_dtypep->isFourstate() != defaultp->isFourstate()) {
            UASSERT_OBJ(m_dtypep->skipRefp()->basicp() && defaultp->skipRefp()->basicp(), m_dtypep,
                        "Unexpected type override from: " << defaultp << " to: " << m_dtypep);
            ਵਾਪਸ defaultp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(m_dtypep->ਚੌੜਾਈ(), m_dtypep->ਚੌੜਾਈ_Min(),
                                                 m_dtypep->numeric(), defaultp->isFourstate());
        }
        ਵਾਪਸ m_dtypep;
    }
    ਪੂਰਨ ਚੌੜਾਈ() ਅਚਲ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ਵਾਪਸ m_dtypep->ਚੌੜਾਈ();
    }
    ਪੂਰਨ ਚੌੜਾਈ_Min() ਅਚਲ {
        UASSERT(m_dtypep, "Width request on self-determined or preliminary VUP");
        ਵਾਪਸ m_dtypep->ਚੌੜਾਈ_Min();
    }
    ਤਾਰਕਿਕ prelim() ਅਚਲ { ਵਾਪਸ m_stage & PRELIM; }
    ਤਾਰਕਿਕ final() ਅਚਲ { ਵਾਪਸ m_stage & FINAL; }
    ਖਾਲੀ dump(std::ostream& str) ਅਚਲ {
        ਜੇ (!m_dtypep) {
            str << "  VUP(s=" << m_stage << ",self)";
        } ਨਹੀਂਤਾਂ {
            str << "  VUP(s=" << m_stage << ",dt=" << cvt_ਮੰਜ਼ਿਲ_Hex(dtypep());
            dtypep()->dumpSmall(str);
            str << ")";
        }
    }
};
std::ostream& ਸੰਚਾਲਕ<<(std::ostream& str, ਅਚਲ ਚੌੜਾਈ_VP* vup) {
    ਜੇ (vup) vup->dump(str);
    ਵਾਪਸ str;
}

//######################################################################

ਵਰਗ ਚੌੜਾਈ_Clear_Visitor final {
    // Rather than a VNVisitor, can just quickly touch every node
    ਖਾਲੀ clear_ਚੌੜਾਈ_Recurse(Ast_ਗੰਢ* nodep) {
        ਲਈ (; nodep; nodep = nodep->nextp()) {
            nodep->did_ਚੌੜਾਈ(ਝੂਠ);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) clear_ਚੌੜਾਈ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) clear_ਚੌੜਾਈ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) clear_ਚੌੜਾਈ_Recurse(refp);
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) clear_ਚੌੜਾਈ_Recurse(refp);
        }
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਚੌੜਾਈ_Clear_Visitor(AstNetlist* nodep) { clear_ਚੌੜਾਈ_Recurse(nodep); }
    ਆਭਾਸੀ ~ਚੌੜਾਈ_Clear_Visitor() = ਮੂਲ;
};

//######################################################################

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ accept ਵਿੱਚ_ਚੌੜਾਈ_Visitor_use_Ast_ਗੰਢ_iterate_instead_ਦਾ_Ast_ਗੰਢ_accept

//######################################################################

ਵਰਗ ਚੌੜਾਈ_Visitor final : ਜਨਤਕ VNVisitor {
    // TYPES
    ਵਰਤੋਂ PatVecMap = std::map<ਪੂਰਨ, AstPatMember*>;
    ਵਰਤੋਂ DTypeMap = std::map<ਅਚਲ std::string, AstPatMember*>;

    // STATE
    V3_Unique_ਨਾਂ m_enum_ਆਰਜ਼ੀ_ਨਾਂ;  // Captured enum expressions
    V3_Unique_ਨਾਂ m_inside_ਆਰਜ਼ੀ_ਨਾਂ;  // For generating unique temporary variable names for
                                      // `inside` expressions
    VMemberMap m_memberMap;  // Member names cached for fast lookup
    std::unordered_map<ਅਚਲ AstModport*, std::unordered_ਸੈੱਟ<string>>
        m_modport_ਨਾਂ;  // Names listed by each modport
    V3TaskConnectState m_taskConnectState;  // State to cache V3Task::taskConnects
    ਚੌੜਾਈ_VP* m_vup = ਖਾਲੀ_ਸੂਚਕ;  // Current node state
    ਤਾਰਕਿਕ m_underFork = ਝੂਠ;  // Visiting under a fork
    ਤਾਰਕਿਕ m_underSExpr = ਝੂਠ;  // Visiting under a sequence expression
    ਤਾਰਕਿਕ m_underPackedArray = ਝੂਠ;  // Visiting under a AstPackArrayDType
    ਤਾਰਕਿਕ m_underMemberSel = ਝੂਠ;  // Viting under a MemberSel
    Ast_ਗੰਢ* m_seqUnsupp = ਖਾਲੀ_ਸੂਚਕ;  // Property has unsupported node
    ਤਾਰਕਿਕ m_hasSExpr = ਝੂਠ;  // Property has a sequence expression
    ਅਚਲ AstCell* m_cellp = ਖਾਲੀ_ਸੂਚਕ;  // Current cell for arrayed instantiations
    std::ਸਦਿਸ਼<AstVar*> m_ਵੰਡੋ_Iface_Array_Varps;  // Interface arrays split by V3Param
    ਅਚਲ Ast_Enum_ਵਸਤੂ* m_enumItemp = ਖਾਲੀ_ਸੂਚਕ;  // Current enum item
    Ast_ਗੰਢ_FTask* m_ftaskp = ਖਾਲੀ_ਸੂਚਕ;  // Current function/task
    Ast_ਗੰਢ_Module* m_modep = ਖਾਲੀ_ਸੂਚਕ;  // Current module
    ਅਚਲ AstConstraint* m_constraintp = ਖਾਲੀ_ਸੂਚਕ;  // Current constraint
    Ast_ਗੰਢ_Procedure* m_procedurep = ਖਾਲੀ_ਸੂਚਕ;  // Current final/always
    ਅਚਲ AstWith* m_withp = ਖਾਲੀ_ਸੂਚਕ;  // Current 'with' statement
    ਅਚਲ AstFunc* m_funcp = ਖਾਲੀ_ਸੂਚਕ;  // Current function
    ਅਚਲ Ast_Attr_ਦਾ* m_attrp = ਖਾਲੀ_ਸੂਚਕ;  // Current attribute
    ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* m_randomizeFromp = ਖਾਲੀ_ਸੂਚਕ;  // Current randomize method call fromp
    ਅਚਲ ਤਾਰਕਿਕ m_paramsOnly;  // Computing parameter value; limit operation
    ਅਚਲ ਤਾਰਕਿਕ m_doGenerate;  // Do errors later inside generate statement
    ਤਾਰਕਿਕ m_ਧਾਰਾ_Concat = ਝੂਠ;  // True if visiting arguments of stream concatenation
    ਤਾਰਕਿਕ m_ਵਿੱਚ_Param_Override = ਝੂਠ;  // True if visiting the value of a parameter override pin
    // Created dimension and enum tables, mapped to a reference reading the table (always cloned)
    std::map<std::pair<ਅਚਲ Ast_ਗੰਢ_DType*, VAttrType>, AstVarRef*> m_ਸਾਰਣੀ_Map;
    // Queues with given index type
    std::map<ਅਚਲ Ast_ਗੰਢ_DType*, Ast_ਕਤਾਰ_DType*> m_ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ;
    ContainingClassFinder m_containingClassFinder;
    std::unordered_ਸੈੱਟ<AstVar*> m_aliasedVars;  // Variables referenced in alias
    std::unordered_ਸੈੱਟ<ਅਚਲ AstVar*> m_curModVars;  // Variables declared in current module

    ਸਥਿਰ constexpr ਪੂਰਨ ENUM_LOOKUP_ਬਿੱਟ = 16;  // Maximum # bits to make enum lookup table

    // ENUMS
    ਗਿਣਤੀ Extend_ਨਿਯਮ : uint8_t {
        EXTEND_EXP,  // Extend if expect sign and node signed, e.g. node=y in ADD(x,y), "x + y"
        EXTEND_ZERO,  // Extend with zeros. e.g. node=y in EQ(x,y), "x == y"
        EXTEND_LHS,  // Extend with sign if node signed. e.g. node=y in ASSIGN(y,x), "x = y"
        EXTEND_OFF  // No extension
    };
    ਗਿਣਤੀ ਵਰਗ ComparisonType : uint8_t {
        ਸਾਧਾਰਨ_EQ,  // Conventional comparison equal/unequal operator: ==, !=
        ਸਾਧਾਰਨ_GL,  // Conventional comparison grater/less than: <, >, <=, >=
        CASE,  // Four-state aware comparison: ===, !==
        WILD,  // Wildcard equality operators: ?== ?=!
    };

    ਸਥਿਰ ਧਾਰਾ_Use ਧਾਰਾ_Use_For_Determ(Determ determ) {
        ਵਾਪਸ determ == ਸੌਂਪੋ_3 ? ਧਾਰਾ_USE_ਸੌਂਪੋ : ਧਾਰਾ_USE_NONE;
    }

    ਧਾਰਾ_Use current_ਧਾਰਾ_Use() ਅਚਲ { ਵਾਪਸ m_vup ? m_vup->ਧਾਰਾ_Use_2() : ਧਾਰਾ_USE_NONE; }

    ਸਥਿਰ ਖਾਲੀ packIfUnpacked(Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nodep) {
        ਜੇ (AstUnpackArrayDType* ਅਚਲ unpackDTypep = VN_CAST(nodep->dtypep(), UnpackArrayDType)) {
            ਅਚਲ ਪੂਰਨ elements_ਅੰਕ = unpackDTypep->arrayUnpackedElements();
            ਅਚਲ ਪੂਰਨ unpack_Min_ਬਿੱਟ = elements_ਅੰਕ * unpackDTypep->subDTypep()->ਚੌੜਾਈ_Min();
            ਅਚਲ ਪੂਰਨ unpack_ਬਿੱਟ = elements_ਅੰਕ * unpackDTypep->subDTypep()->ਚੌੜਾਈ();
            VNRelinker relinker;
            nodep->unlink_Fr_ਪਿੱਠ(&relinker);
            relinker.relink(ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed{
                nodep->fileline(), nodep,
                nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(unpack_ਬਿੱਟ, unpack_Min_ਬਿੱਟ, VSigning::UNSIGNED,
                                           nodep->dtypep()->isFourstate())});
        }
    }
    // When fromp() is a DType (e.g. unlinked RefDType), resolve through
    // the ref chain; when it's an expression, dtypep() is already resolved.
    ਸਥਿਰ Ast_ਗੰਢ_DType* fromDTypep(Ast_ਗੰਢ* fromp) {
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtypep = VN_CAST(fromp, ਗੰਢ_DType))
            ਵਾਪਸ dtypep->skipRefOrNullp();
        ਵਾਪਸ fromp ? fromp->dtypep() : ਖਾਲੀ_ਸੂਚਕ;
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
    ਖਾਲੀ ਦੇਖੋ(AstLogNot* nodep) override { ਦੇਖੋ_log_not(nodep); }
    // Widths: 1 bit out, lhs 1 bit, rhs 1 bit; Real: converts via compare with 0
    ਖਾਲੀ ਦੇਖੋ(AstLogAnd* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogOr* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    // Sequence and/or/intersect (IEEE 1800-2023 16.9.2), same width rules as LogAnd/LogOr
    ਖਾਲੀ ਦੇਖੋ(AstSAnd* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSIntersect* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSOr* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSWithin* nodep) override { ਦੇਖੋ_log_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLogEq* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        ਦੇਖੋ_log_and_or(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLogIf* nodep) override {
        // Conversion from real not in IEEE, but a fallout
        ਦੇਖੋ_log_and_or(nodep);
    }

    // Widths: 1 bit out, Any width lhs
    ਖਾਲੀ ਦੇਖੋ(AstRedAnd* nodep) override { ਦੇਖੋ_red_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRedOr* nodep) override { ਦੇਖੋ_red_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRedXor* nodep) override { ਦੇਖੋ_red_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstOneHot* nodep) override { ਦੇਖੋ_red_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstOneHot0* nodep) override { ਦੇਖੋ_red_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstIsUnknown* nodep) override {
        ਦੇਖੋ_red_unknown(nodep);  // Allow real
    }

    // These have different node types, as they operate differently
    // Must add to case statement below,
    // Widths: 1 bit out, lhs width == rhs width.  real if lhs|rhs real
    ਖਾਲੀ ਦੇਖੋ(AstEq* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_EQ); }
    ਖਾਲੀ ਦੇਖੋ(AstNeq* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_EQ); }
    ਖਾਲੀ ਦੇਖੋ(AstGt* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstGte* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstLt* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstLte* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstGtS* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstGteS* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstLtS* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstLteS* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::ਸਾਧਾਰਨ_GL); }
    ਖਾਲੀ ਦੇਖੋ(AstEqCase* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqCase* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::CASE); }
    // ...    These comparisons don't allow reals
    ਖਾਲੀ ਦੇਖੋ(AstEqWild* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqWild* nodep) override { ਦੇਖੋ_cmp_eq_gt(nodep, ComparisonType::WILD); }
    // ...    Real compares
    ਖਾਲੀ ਦੇਖੋ(AstEqD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLtD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLteD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstGtD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstGteD* nodep) override { ਦੇਖੋ_cmp_real(nodep); }
    // ...    String compares
    ਖਾਲੀ ਦੇਖੋ(AstEqN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLtN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstLteN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstGtN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstGteN* nodep) override { ਦੇਖੋ_cmp_string(nodep); }
    // ...    Data type compares
    ਖਾਲੀ ਦੇਖੋ(AstEqT* nodep) override { ਦੇਖੋ_cmp_type(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstNeqT* nodep) override { ਦੇਖੋ_cmp_type(nodep); }

    // Widths: out width = lhs width = rhs width
    // Signed: Output signed iff LHS & RHS signed.
    // Real: Not allowed
    ਖਾਲੀ ਦੇਖੋ(AstAnd* nodep) override { ਦੇਖੋ_boolexpr_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstOr* nodep) override { ਦੇਖੋ_boolexpr_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstXor* nodep) override { ਦੇਖੋ_boolexpr_and_or(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstBufIf1* nodep) override {
        ਦੇਖੋ_boolexpr_and_or(nodep);
    }  // Signed behavior changing in 3.814
    // Width: Max(Lhs,Rhs) sort of.
    // Real: If either side real
    // Signed: If both sides real
    ਖਾਲੀ ਦੇਖੋ(Ast_ਜੋੜੋ* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਸੱਚ, ਝੂਠ); }
    ਖਾਲੀ ਦੇਖੋ(AstSub* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਸੱਚ, ਝੂਠ); }
    ਖਾਲੀ ਦੇਖੋ(AstDiv* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਸੱਚ, ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstMul* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਸੱਚ, ਝੂਠ); }
    // These can't promote to real
    ਖਾਲੀ ਦੇਖੋ(AstModDiv* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਝੂਠ, ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstModDivS* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਝੂਠ, ਸੱਚ); }
    ਖਾਲੀ ਦੇਖੋ(AstMulS* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਝੂਠ, ਝੂਠ); }
    ਖਾਲੀ ਦੇਖੋ(AstDivS* nodep) override { ਦੇਖੋ_ਜੋੜੋ_sub_replace(nodep, ਝੂਠ, ਸੱਚ); }
    // Widths: out width = lhs width, but upper matters
    // Signed: Output signed iff LHS signed; unary operator
    // Unary promote to real
    ਖਾਲੀ ਦੇਖੋ(AstNegate* nodep) override { ਦੇਖੋ_negate_not(nodep, ਸੱਚ); }
    // Unary never real
    ਖਾਲੀ ਦੇਖੋ(AstNot* nodep) override { ਦੇਖੋ_negate_not(nodep, ਝੂਠ); }

    // Real: inputs and output real
    ਖਾਲੀ ਦੇਖੋ(Ast_ਜੋੜੋ_D* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSubD* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstDivD* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstMulD* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstPowD* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_System_Biop_D* nodep) override { ਦੇਖੋ_real_ਜੋੜੋ_sub(nodep); }
    // Real: Output real
    ਖਾਲੀ ਦੇਖੋ(AstNegateD* nodep) override { ਦੇਖੋ_real_neg_ceil(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_System_Uniop_D* nodep) override { ਦੇਖੋ_real_neg_ceil(nodep); }

    // Widths: out signed/unsigned width = lhs width, input un|signed
    ਖਾਲੀ ਦੇਖੋ(AstSigned* nodep) override { ਦੇਖੋ_signed_unsigned(nodep, VSigning::SIGNED); }
    ਖਾਲੀ ਦੇਖੋ(AstUnsigned* nodep) override { ਦੇਖੋ_signed_unsigned(nodep, VSigning::UNSIGNED); }

    // Widths: Output width from lhs, rhs<33 bits
    // Signed: If lhs signed
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_L* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ_2(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_R* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ_2(nodep); }
    // ShiftRS converts to ShiftR, but not vice-versa
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਿਸਕਾਓ_RS* nodep) override { ਦੇਖੋ_ਖਿਸਕਾਓ_2(nodep); }

    //========
    // Widths: Output real, input integer signed
    ਖਾਲੀ ਦੇਖੋ(Ast_ਬਿੱਟ_ਮੰਜ਼ਿਲ_Real_D* nodep) override { ਦੇਖੋ_Or_Lu64(nodep); }

    // Widths: Output integer signed, input real
    ਖਾਲੀ ਦੇਖੋ(AstRToIS* nodep) override { ਦੇਖੋ_Os32_Lr(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstRToIRoundS* nodep) override {
        // Only created here, size comes from upper expression
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
        }
        UASSERT_OBJ(nodep->dtypep()->ਚੌੜਾਈ_Sized(), nodep, "RToIRoundS should be presized");
    }

    // Widths: Output integer unsigned, input real
    ਖਾਲੀ ਦੇਖੋ(Ast_Real_ਮੰਜ਼ਿਲ_ਬਿੱਟ* nodep) override { ਦੇਖੋ_Ou64_Lr(nodep); }

    // Output integer, input string
    ਖਾਲੀ ਦੇਖੋ(AstLenN* nodep) override {
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->lhsp(), nodep, "For unary ops only!");
        ਜੇ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_Int();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPutcN* nodep) override {
        // CALLER: str.putc()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ਜੇ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned8(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_String();  // AstPutcN returns the new string to be assigned by
                                      // AstAssign
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstGetcN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ਜੇ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(8, VSigning::UNSIGNED);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstGetcRefN* nodep) override {
        // CALLER: str.getc()
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        ਜੇ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(8, VSigning::UNSIGNED);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSubstrN* nodep) override {
        // CALLER: str.substr()
        UASSERT_OBJ(nodep->rhsp() && nodep->thsp(), nodep, "For ternary ops only!");
        ਜੇ (m_vup && m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_String();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCompareNN* nodep) override {
        // CALLER: str.compare(), str.icompare()
        // Widths: 32 bit out
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_Int();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAtoN* nodep) override {
        // CALLER: str.atobin(), atoi(), atohex(), atooct(), atoreal()
        // Width: 64bit floating point for atoreal(), 32bit out for the others
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            ਜੇ (nodep->fmtType() == AstAtoN::ATOREAL) {
                nodep->dtype_ਸੈੱਟ_Double();
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_ਸੈੱਟ_Integer();
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstNToI* nodep) override {
        // Created here, should be already sized
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            UASSERT_OBJ(nodep->dtypep(), nodep, "NToI should be sized when created");
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
        }
    }

    // Widths: Constant, terminal
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ* nodep) override { nodep->dtype_ਸੈੱਟ_Time2_State(); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_D* nodep) override { nodep->dtype_ਸੈੱਟ_Double(); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Precision* nodep) override { nodep->dtype_ਸੈੱਟ_Integer2_State(); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Get_Initial_Random_ਬੀਜ* nodep) override { nodep->dtype_ਸੈੱਟ_Int(); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Unit* nodep) override {
        nodep->replaceWith(
            ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Signed32{}, nodep->timeunit().power_ਦਾ_Ten()});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstScopeName* nodep) override {
        nodep->dtype_ਸੈੱਟ_UInt64();  // A pointer, but not that it matters
    }

    // Whether nodep, an override's value, is an untyped pattern, or a conditional of such
    ਸਥਿਰ ਤਾਰਕਿਕ is_Override_ਨਮੂਨਾ(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (ਅਚਲ Ast_ਨਮੂਨਾ* ਅਚਲ patternp = VN_CAST(nodep, ਨਮੂਨਾ)) {
            ਵਾਪਸ !patternp->ਬੱਚਾ_DTypep();
        }
        ਅਚਲ AstCond* ਅਚਲ condp = VN_CAST(nodep, Cond);
        ਵਾਪਸ condp && is_Override_ਨਮੂਨਾ(condp->thenp()) && is_Override_ਨਮੂਨਾ(condp->elsep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCond* nodep) override {
        // op = cond ? expr1 : expr2
        // See IEEE-2012 11.4.11 and Table 11-21.
        //   LHS is self-determined
        //   Width: max(RHS, THS)
        //   Signed: Output signed iff RHS & THS signed  (presumed, not in IEEE)
        //   Real: Output real if either expression is real, non-real argument gets converted
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_ਵਿੱਚ_Param_Override) {
            // Choosing between override patterns, so type and fold only the condition, then
            // keep the pattern it picks, which the specialized module types as a lone one
            ਅਚਲ ਪੂਰਨ errors = V3Error::error_ਗਿਣਤੀ();
            {
                VL_RESTORER(m_ਵਿੱਚ_Param_Override);
                m_ਵਿੱਚ_Param_Override = ਝੂਠ;
                iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
                V3Const::constifyParamsEdit(nodep->condp());  // Reports a non-constant
            }
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->condp(), Const);
            ਅਚਲ ਤਾਰਕਿਕ ਗਿਆਤ = constp && !constp->ਅੰਕ().isFourState();
            // Folding reports a non-constant, but leaves x or z, even inside a conditional
            ਜੇ (!ਗਿਆਤ && V3Error::error_ਗਿਣਤੀ() == errors) {
                nodep->condp()->v3warn(E_UNSUPPORTED,
                                       "Unsupported: Parameter override '?:' with assignment"
                                       " patterns and a condition without a known value.");
            }
            // After an error, keep the first pattern, so that no conditional is left untyped
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ keepp
                = (ਗਿਆਤ && constp->isZero() ? nodep->elsep() : nodep->thenp())->unlink_Fr_ਪਿੱਠ();
            nodep->replaceWith(keepp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            userIterate(keepp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਵਾਪਸ;
        }
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // Just once, do the conditional, expect one bit out.
            iterateCheckBool(nodep, "Conditional Test", nodep->condp(), BOTH);
            // Determine sub expression widths only relying on what's in the subops
            //  CONTEXT_DET determined, but need data type for pattern assignments
            user_Iterate_And_ਅਗਲਾ(nodep->thenp(), ਚੌੜਾਈ_VP{m_vup->dtypeNullp(), PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->elsep(), ਚੌੜਾਈ_VP{m_vup->dtypeNullp(), PRELIM}.p());
            // Calculate width of this expression.
            // First call (prelim()) m_vup->width() is probably zero, so we'll return
            //  the size of this subexpression only.
            // Second call (final()) m_vup->width() is probably the expression size, so
            //  the expression includes the size of the output too.
            Ast_ਗੰਢ_DType* ਅਚਲ thenDTypep = nodep->thenp()->dtypep();
            Ast_ਗੰਢ_DType* ਅਚਲ elseDTypep = nodep->elsep()->dtypep();
            Ast_ਗੰਢ_DType* ਅਚਲ thenDTypeSkippedp = thenDTypep->skipRefp();
            Ast_ਗੰਢ_DType* ਅਚਲ elseDTypeSkippedp = elseDTypep->skipRefp();
            ਜੇ (nodep->thenp()->isNull() && nodep->elsep()->isNull()) {
                nodep->dtypep(m_vup->dtypeNullp() ? m_vup->dtypeNullp() : nodep->ਲੱਭੋ_Null_DType());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->thenp()->is_Class_Handle_ਮੁੱਲ()
                       || nodep->elsep()->is_Class_Handle_ਮੁੱਲ()) {
                Ast_ਗੰਢ_DType* commonClassTypep = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (nodep->thenp()->is_Class_Handle_ਮੁੱਲ() && nodep->elsep()->is_Class_Handle_ਮੁੱਲ()) {
                    // Get the most-deriving class type that both arguments can be casted to.
                    commonClassTypep
                        = Ast_ਗੰਢ::getCommonClassTypep(nodep->thenp(), nodep->elsep());
                }
                ਜੇ (commonClassTypep) {
                    nodep->dtypep(commonClassTypep);
                } ਨਹੀਂਤਾਂ {
                    nodep->v3error("Incompatible types of operands of condition operator: "
                                   << thenDTypep->prettyTypeName() << " and "
                                   << elseDTypep->prettyTypeName());
                    nodep->dtypeFrom(thenDTypep);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (!((thenDTypeSkippedp->isIntegralOrPacked()
                          || VN_IS(thenDTypeSkippedp, BasicDType))
                         && (elseDTypeSkippedp->isIntegralOrPacked()
                             || VN_IS(elseDTypeSkippedp, BasicDType)))) {
                ਜੇ (Ast_ਗੰਢ::computeCastable(elseDTypep, thenDTypep, ਖਾਲੀ_ਸੂਚਕ).isAssignable()) {
                    nodep->dtypep(elseDTypep);
                } ਨਹੀਂਤਾਂ {
                    // Set any type even if not castable
                    nodep->dtypep(thenDTypep);
                    ਜੇ (!Ast_ਗੰਢ::computeCastable(thenDTypep, elseDTypep, ਖਾਲੀ_ਸੂਚਕ)
                             .isAssignable()) {
                        nodep->v3error("Neither " << thenDTypep->prettyDTypeNameQ() << " or "
                                                  << elseDTypep->prettyDTypeNameQ()
                                                  << " is castable to the other one");
                    }
                }
            } ਨਹੀਂਤਾਂ ਜੇ (thenDTypep->isDouble() || elseDTypep->isDouble()) {
                nodep->dtype_ਸੈੱਟ_Double();
            } ਨਹੀਂਤਾਂ ਜੇ (thenDTypep->isString() || elseDTypep->isString()) {
                // There is a specific case:
                // strings and how they should be handled when condp() is 'x
                // ('x ? "Option1" : "Option2") === "Option0"
                // ('x ? "Option1" : "Option2") ===
                // 56'b010011110111000001110100011010010110111101101110001100xx
                // Issue is that the type of this is still string but the value contains 'x - types
                // and domains are not the same thing in SystemVerilog but in Verilator they are
                nodep->dtype_ਸੈੱਟ_String();
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਤਾਰਕਿਕ is_Any_ਨਤੀਜਾ_Fourstate
                    = thenDTypeSkippedp->isFourstate() || elseDTypeSkippedp->isFourstate();
                ਤਾਰਕਿਕ handled = ਝੂਠ;
                ਅਚਲ ਤਾਰਕਿਕ isCondpFourstate = nodep->condp()->isFourstateOrNoDtype();
                Ast_ਗੰਢ_DType* ਅਚਲ thenEnumDTypep = thenDTypep->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                ਜੇ (VN_IS(thenDTypeSkippedp, ਗੰਢ_UOr_Struct_DType)
                    && thenDTypeSkippedp->same_ਰੁੱਖ(elseDTypeSkippedp)) {
                    nodep->dtypep(thenDTypep);
                    handled = ਸੱਚ;
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(thenEnumDTypep, EnumDType)
                           && thenEnumDTypep->same_ਰੁੱਖ(elseDTypep->skip_Ref_ਮੰਜ਼ਿਲ_Enump())) {
                    // if enum is two-state we have to check condp() - because if it is
                    // four-state the result must be four-state as well
                    ਜੇ (is_Any_ਨਤੀਜਾ_Fourstate || !isCondpFourstate) {
                        nodep->dtypep(thenEnumDTypep);
                        handled = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ {
                        // This is similar case to what happens with strings
                        // So, to avoid cases where:
                        // enum int {A, B} type_t;
                        // logic'(foo) ? A : B
                        // throws an ENUMVALUE error for this error is disabled for this expression
                        // This is a result of how Verilator's type system works - to avoid such
                        // situations Verilator would need to either treat everything as four-state
                        // or separate idea of type and values domain - since four-state values are
                        // casted to two-state only with explicit casts or with assignments
                        nodep->fileline(ਨਵਾਂ File_ਸਤਰ{nodep->fileline()});
                        nodep->fileline()->warnOff(V3_Error_ਕੋਡ::ENUMVALUE, ਸੱਚ);
                    }
                }
                ਜੇ (!handled) {
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(thenDTypep->ਚੌੜਾਈ(), elseDTypep->ਚੌੜਾਈ());
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = std::max(thenDTypep->ਚੌੜਾਈ_Min(), elseDTypep->ਚੌੜਾਈ_Min());
                    ਅਚਲ VSigning numeric
                        = VSigning::fromBool(thenDTypep->isSigned() && elseDTypep->isSigned());
                    ਅਚਲ ਤਾਰਕਿਕ isFourstate = is_Any_ਨਤੀਜਾ_Fourstate || isCondpFourstate;
                    // Try to preserve a dtype
                    ਜੇ ((ਚੌੜਾਈ == elseDTypeSkippedp->ਚੌੜਾਈ()
                         && ਚੌੜਾਈ_Min == elseDTypeSkippedp->ਚੌੜਾਈ_Min())
                        && elseDTypeSkippedp->numeric() == numeric
                        && isFourstate == elseDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(elseDTypep);
                    } ਨਹੀਂਤਾਂ ਜੇ ((ਚੌੜਾਈ == thenDTypeSkippedp->ਚੌੜਾਈ()
                                && ਚੌੜਾਈ_Min == thenDTypeSkippedp->ਚੌੜਾਈ_Min())
                               && thenDTypeSkippedp->numeric() == numeric
                               && isFourstate == thenDTypeSkippedp->isFourstate()) {
                        nodep->dtypep(thenDTypep);
                    } ਨਹੀਂਤਾਂ {
                        // None of the children dtypes match generate one
                        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(ਚੌੜਾਈ, ਚੌੜਾਈ_Min, numeric, isFourstate);
                    }
                }
            }
        }
        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "Conditional True", nodep->thenp(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
            iterateCheck(nodep, "Conditional False", nodep->elsep(), CONTEXT_DET, FINAL, subDTypep,
                         EXTEND_EXP);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConcat* nodep) override {
        // Real: Not allowed (assumed)
        // Signed: unsigned output, input either (assumed)
        // IEEE-2012 Table 11-21, and 11.8.1:
        //   LHS, RHS is self-determined
        //   signed: Unsigned  (11.8.1)
        //   width: LHS + RHS
        Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypeNullSkipRefp();
        userIterate(vdtypep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // Conversions
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = new_Cons_Dyn_Array_Or_ਕਤਾਰ(nodep->fileline(), vdtypep)) {
            // IEEE 1800-2023 10.10 requires looking at argument data type
            // to determine if value or push
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{vdtypep, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{vdtypep, PRELIM}.p());
            newp->dtypeFrom(vdtypep);
            ਜੋੜੋ_Cons_ਵਸਤੂ(newp, nodep->lhsp()->unlink_Fr_ਪਿੱਠ());
            ਜੋੜੋ_Cons_ਵਸਤੂ(newp, nodep->rhsp()->unlink_Fr_ਪਿੱਠ());
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (VN_IS(vdtypep, UnpackArrayDType)) {
            Ast_ਨਮੂਨਾ* ਅਚਲ newp = ਨਵਾਂ Ast_ਨਮੂਨਾ{nodep->fileline(), ਖਾਲੀ_ਸੂਚਕ};
            patConcatConvertRecurse(newp, nodep);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ਵਾਪਸ;
        }

        // Concat handling
        ਜੇ (m_vup->prelim()) {
            ਜੇ (VN_IS(vdtypep, AssocArrayDType)  //
                || VN_IS(vdtypep, DynArrayDType)  //
                || VN_IS(vdtypep, ਕਤਾਰ_DType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Concatenation to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }

            ਜੇ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
                iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
                nodep->dtype_ਸੈੱਟ_String();
            } ਨਹੀਂਤਾਂ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
                iterateCheckIntegralSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);

                ਜੇ (m_ਧਾਰਾ_Concat) {
                    packIfUnpacked(nodep->lhsp());
                    packIfUnpacked(nodep->rhsp());
                }
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                    nodep->lhsp()->ਚੌੜਾਈ() + nodep->rhsp()->ਚੌੜਾਈ(),
                    nodep->lhsp()->ਚੌੜਾਈ_Min() + nodep->rhsp()->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED,
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
            // Cleanup zero width Verilog2001 {x,{0{foo}}} now,
            // otherwise having width(0) will cause later assertions to fire
            ਜੇ (ਅਚਲ AstReplicate* ਅਚਲ repp = VN_CAST(nodep->lhsp(), Replicate)) {
                ਜੇ (repp->ਚੌੜਾਈ() == 0) {  // Keep rhs
                    nodep->replaceWith(nodep->rhsp()->unlink_Fr_ਪਿੱਠ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (ਅਚਲ AstReplicate* ਅਚਲ repp = VN_CAST(nodep->rhsp(), Replicate)) {
                ਜੇ (repp->ਚੌੜਾਈ() == 0) {  // Keep lhs
                    nodep->replaceWith(nodep->lhsp()->unlink_Fr_ਪਿੱਠ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
        }
        ਜੇ (m_vup->final()) {
            ਜੇ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                ਜੇ (!lhsp->isString()) lhsp = ਨਵਾਂ AstCvtPackString{lhsp->fileline(), lhsp};
                ਜੇ (!rhsp->isString()) rhsp = ਨਵਾਂ AstCvtPackString{rhsp->fileline(), rhsp};
                Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConcatN{nodep->fileline(), lhsp, rhsp};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
            ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized()) {
                // See also error in V3Number
                ਗੰਢ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConcatN* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        // String concatenate.
        // Already did AstConcat simplifications
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_String();
        }
        ਜੇ (m_vup->final()) {
            nodep->did_ਚੌੜਾਈ(ਸੱਚ);
            ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized()) {
                // See also error in V3Number
                ਗੰਢ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in concatenations.");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefaultDisable* nodep) override {
        assertAtStatement(nodep);
        // it's like an if() condition.
        iterateCheckBool(nodep, "default disable iff condition", nodep->condp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDelay* nodep) override {
        ਜੇ (nodep->is_ਚੱਕਰ_Delay() && m_underSExpr) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਜੇ (nodep->rhsp() && !nodep->isUnbounded()) {
                user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            }
            // A delay with a formal argument of a sequence or property is checked when that is
            // instanced, with the actual argument (IEEE 1800-2023 16.8)
            ਜੇ (V3AssertCommon::refsSvaFormal(nodep->lhsp())
                || (nodep->rhsp() && V3AssertCommon::refsSvaFormal(nodep->rhsp()))) {
                ਵਾਪਸ;
            }
            // Fold parameterized SVA cycle-delay bounds
            V3AssertCommon::check_Sva_ਚੱਕਰ_Delay(nodep, nodep);
            ਵਾਪਸ;
        }
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fallDelayp = nodep->fallDelay()) {
            iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
            iterateCheckDelay(nodep, "delay", fallDelayp, BOTH);
            ਵਾਪਸ;
        }
        ਜੇ (VN_IS(m_procedurep, Final)) {
            nodep->v3error("Delays are not legal in final blocks (IEEE 1800-2023 9.2.3)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Delays are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->fileline()->timingOn()) {
            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                iterateCheckDelay(nodep, "delay", nodep->lhsp(), BOTH);
                iterate_And_ਅਗਲਾ_Null(nodep->stmtsp());
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                nodep->v3warn(STMTDLY, "Ignoring delay on this statement due to --no-timing");
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(
                    E_NEEDTIMINGOPT,
                    "Use --timing or --no-timing to specify how delays should be handled");
            }
        }
        ਜੇ (nodep->stmtsp()) nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        assertAtStatement(nodep);
        iterateCheckSelf(nodep, "LHS", nodep->operandp(), SELF, BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (!m_underFork && VN_IS(m_ftaskp, Func) && !nodep->joinType().joinNone()) {
            nodep->v3error("Only fork .. join_none is legal in functions. "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (!nodep->fileline()->timingOn()
            // With no statements, begin is identical
            || !nodep->forksp()
            || (!v3Global.opt.timing().is_ਸੈੱਟ_True()  // If no --timing
                && (v3Global.opt.bboxUnsup()
                    // With one statement and no timing, a begin block does as good as a
                    // fork/join or join_any
                    || (!nodep->forksp()->nextp() && !nodep->joinType().joinNone())))) {
            AstBegin* ਅਚਲ newp = ਨਵਾਂ AstBegin{nodep->fileline(), nodep->name(), ਖਾਲੀ_ਸੂਚਕ, ਝੂਠ};
            nodep->replaceWith(newp);
            ਜੇ (nodep->declsp()) newp->ਜੋੜੋ_Declsp(nodep->declsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            ਜੇ (nodep->stmtsp()) newp->ਜੋੜੋ_Stmtsp(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            ਜੇ (nodep->forksp()) newp->ਜੋੜੋ_Stmtsp(nodep->forksp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
            VL_RESTORER(m_underFork);
            m_underFork = ਸੱਚ;
            iterateChildren(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
            nodep->v3warn(E_NOTIMING, "Fork statements require --timing");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_NEEDTIMINGOPT,
                          "Use --timing or --no-timing to specify how forks should be handled");
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Wait_Or_Disable_Fork(Ast_ਗੰਢ* nodep) {
        ਜੇ (nodep->fileline()->timingOn()) {
            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                nodep->v3warn(E_NOTIMING, "Support for '" << nodep->verilogKwd()
                                                          << "' statement requires --timing");
                VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                                   << "disable fork should be handled");
            }
        }
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisableFork* nodep) override { ਦੇਖੋ_Wait_Or_Disable_Fork(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstWaitFork* nodep) override { ਦੇਖੋ_Wait_Or_Disable_Fork(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_String();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_String();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstReplicate* nodep) override {
        // IEEE-2012 Table 11-21:
        //   LHS, RHS is self-determined
        //   width: value(LHS) * width(RHS)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "RHS", nodep->countp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->countp());  // rhsp may change

            int32_t times = 1;  // IEEE replicate value is integral

            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->countp(), Const);
            ਜੇ (constp) {
                ਜੇ (constp->ਅੰਕ().isFourState()
                    || (constp->dtypep()->isSigned() && constp->ਅੰਕ().isNegative())) {
                    nodep->v3error("Replication value of < 0 or X/Z not legal"
                                   " (IEEE 1800-2023 11.4.12.1): "
                                   << constp->prettyNameQ());
                } ਨਹੀਂਤਾਂ {
                    times = constp->ਮੰਜ਼ਿਲ_SInt();
                }
            }

            Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypeNullSkipRefp();
            ਜੇ (VN_IS(vdtypep, ਕਤਾਰ_DType) || VN_IS(vdtypep, DynArrayDType)
                || VN_IS(vdtypep, UnpackArrayDType)) {
                ਜੇ (times != 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-1 replication to form "
                                                     << vdtypep->prettyDTypeNameQ()
                                                     << " data type");
                }
                ਜੇ (VN_IS(nodep->srcp(), Concat)) {
                    // Convert to concat directly, and visit(AstConst) will convert.
                    // Don't iterate lhsp as SELF, the potential Concat below needs
                    // the adtypep passed down to recognize the QueueDType
                    user_Iterate_And_ਅਗਲਾ(nodep->srcp(), ਚੌੜਾਈ_VP{vdtypep, BOTH}.p());
                    nodep->replaceWith(nodep->srcp()->unlink_Fr_ਪਿੱਠ());
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ {  // int a[] = {lhs} -> same as '{lhs}
                    Ast_ਨਮੂਨਾ* ਅਚਲ newp = ਨਵਾਂ Ast_ਨਮੂਨਾ{
                        nodep->fileline(),
                        ਨਵਾਂ AstPatMember{nodep->srcp()->fileline(), nodep->srcp()->unlink_Fr_ਪਿੱਠ(),
                                         ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ਵਾਪਸ;
                }
            }
            ਜੇ (VN_IS(vdtypep, AssocArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: Replication to form "
                                                 << vdtypep->prettyDTypeNameQ() << " data type");
            }
            ਜੇ (vdtypep && vdtypep->isString()) {
                iterateCheckString(nodep, "LHS", nodep->srcp(), BOTH);
            } ਨਹੀਂਤਾਂ {
                iterateCheckIntegralSelf(nodep, "LHS", nodep->srcp(), SELF, BOTH);
            }

            ਜੇ ((vdtypep && vdtypep->isString()) || nodep->srcp()->isString()) {
                Ast_ਗੰਢ* ਅਚਲ newp
                    = ਨਵਾਂ AstReplicateN{nodep->fileline(), nodep->srcp()->unlink_Fr_ਪਿੱਠ(),
                                        nodep->countp()->unlink_Fr_ਪਿੱਠ()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                ਜੇ (!constp) nodep->v3error("Replication value isn't a constant.");
                ਜੇ (times == 0
                    && !VN_IS(nodep->backp(), Concat)) {  // Concat Visitor will clean it up.
                    nodep->v3warn(ZEROREPL,
                                  "Replication value of 0 is only legal under a concatenation"
                                  " (IEEE 1800-2023 11.4.12.1)");
                    times = 1;  // Set to 1, so we can continue looking for errors
                }
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                    (nodep->srcp()->ਚੌੜਾਈ() * times), (nodep->srcp()->ਚੌੜਾਈ_Min() * times),
                    VSigning::UNSIGNED, nodep->srcp()->isFourstateOrNoDtype());
            }
        }
        ਜੇ (m_vup->final()) {
            ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized()) {
                // See also error in V3Number
                ਗੰਢ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstReplicateN* nodep) override {
        // Replicate with string
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsNoWarnEdit(nodep->rhsp());  // rhsp may change
            nodep->dtype_ਸੈੱਟ_String();
        }
        ਜੇ (m_vup->final()) {
            ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized()) {
                // See also error in V3Number
                ਗੰਢ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in replications.");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Biop* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Dist_Triop* nodep) override {
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckSigned32(nodep, "seed", nodep->lhsp(), BOTH);
            iterateCheckSigned32(nodep, "RHS", nodep->rhsp(), BOTH);
            iterateCheckSigned32(nodep, "THS", nodep->thsp(), BOTH);
        }
    }
    ਤਾਰਕਿਕ ਧਾਰਾ_Implicit_Use_Allowed(ਅਚਲ Ast_ਗੰਢ_ਧਾਰਾ* nodep) ਅਚਲ {
        ਜੇ (m_ਧਾਰਾ_Concat) ਵਾਪਸ ਸੱਚ;
        ਜੇ (m_vup && m_vup->ਧਾਰਾ_Use_2() != ਧਾਰਾ_USE_NONE) ਵਾਪਸ ਸੱਚ;

        ਅਚਲ Ast_ਗੰਢ* ਅਚਲ backp = nodep->backp();
        ਜੇ (!backp) ਵਾਪਸ ਸੱਚ;  // Error elsewhere

        ਵਾਪਸ VN_IS(backp, ਗੰਢ_ਸੌਂਪੋ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਧਾਰਾ* nodep) override {
        VL_RESTORER(m_ਧਾਰਾ_Concat);
        // UINFOTREE(1, nodep, "stream-in vup" << m_vup, "stream-in ");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            m_ਧਾਰਾ_Concat = ਸੱਚ;
            iterateCheckSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            m_ਧਾਰਾ_Concat = ਝੂਠ;
            iterateCheckSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->rhsp());  // rhsp may change
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->rhsp(), Const)) {
                ਜੇ (constp->ਮੰਜ਼ਿਲ_UInt() == 0) nodep->v3error("Slice size cannot be zero.");
            } ਨਹੀਂਤਾਂ {
                nodep->v3error("Slice size isn't a constant or basic data type.");
            }
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDtypep = nodep->lhsp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
            ਜੇ (VN_IS(lhsDtypep, DynArrayDType) || VN_IS(lhsDtypep, ਕਤਾਰ_DType)
                || (VN_IS(lhsDtypep, UnpackArrayDType) && lhsDtypep->isStreamableFixedAggregate())
                || (VN_IS(lhsDtypep, ਗੰਢ_UOr_Struct_DType)
                    && !VN_AS(lhsDtypep, ਗੰਢ_UOr_Struct_DType)->packed()
                    && lhsDtypep->isStreamableFixedAggregate())) {
                nodep->dtype_ਸੈੱਟ_ਧਾਰਾ();
            } ਨਹੀਂਤਾਂ ਜੇ (lhsDtypep->isCompound()) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: Stream operation on a variable of a type "
                                  << lhsDtypep->prettyDTypeNameQ());
                nodep->dtype_ਸੈੱਟ_ਧਾਰਾ();
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                    nodep->lhsp()->ਚੌੜਾਈ(), nodep->lhsp()->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED,
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ਜੇ (m_vup->final()) {
            ਜੇ (!ਧਾਰਾ_Implicit_Use_Allowed(nodep)) {
                nodep->v3error(
                    "Streaming concatenation cannot be used in an implicitly cast context "
                    "(IEEE 1800-2023 11.4.14)\n"
                    << nodep->warn_ਹੋਰ() << "... Suggest use a cast");
            }
            ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized()) {
                // See also error in V3Number
                ਗੰਢ_For_Unsized_Warning(nodep)->v3warn(
                    WIDTHCONCAT, "Unsized numbers/parameters not allowed in streams.");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRange* nodep) override {
        // Real: Not allowed
        // Signed: unsigned output, input either
        // Convert all range values to constants
        UINFO(6, "RANGE " << nodep);
        V3Const::constifyParamsEdit(nodep->leftp());  // May relink pointed to node
        V3Const::constifyParamsEdit(nodep->rightp());  // May relink pointed to node
        checkConstantOrReplace(nodep->leftp(), ਸੱਚ,
                               "left side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        checkConstantOrReplace(nodep->rightp(), ਸੱਚ,
                               "right side of bit range isn't a two-state constant"
                               " (IEEE 1800-2023 6.9.1)");
        ਜੇ (m_vup->prelim()) {
            // Don't need to iterate because V3Const already constified
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->elementsConst();
            ਜੇ (nodep->fromBracket() && nodep->ਖੱਬਾ_Const() > nodep->ਸੱਜਾ_Const()) {
                // From a C-Style '[size]' declaration, due to range handling a '[-1]'
                // will get transformed to look like a '[0:1]', instead report as error
                // now we know the datatype is used
                nodep->v3error("Size of range is '["
                               << (-ਚੌੜਾਈ + 2)
                               << "]', must be positive integer (IEEE 1800-2023 7.4.2)");
            } ਨਹੀਂਤਾਂ ਜੇ (ਚੌੜਾਈ > (1 << 28)) {
                nodep->v3error("Width of bit range is huge; vector of over 1 billion bits: 0x"
                               << std::hex << ਚੌੜਾਈ << std::dec);
            }
            // Note width() not set on range; use elementsConst()
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Dead_Module = m_modep && m_modep->dead();
            // Suppress ASCRANGE in parameterized template modules whose parameter-dependent
            // ranges haven't been resolved yet, or when the type has no owning module
            // (e.g. moved to TypeTable during DepGraph resolution).
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Type_ਸਾਰਣੀ = !m_modep;
            ਅਚਲ Ast_ਗੰਢ* basep = nodep->backp();
            ਜਦੋਂ (VN_IS(basep, Range)) basep = basep->backp();
            ਜੇ (nodep->ascending() && !VN_IS(basep, UnpackArrayDType)
                && !VN_IS(basep, Cell)  // For cells we warn when connecting pins
                && !m_paramsOnly  // Skip during parameter evaluation
                && !ਵਿੱਚ_Dead_Module && !ਵਿੱਚ_Parameterized_Template && !ਵਿੱਚ_Type_ਸਾਰਣੀ) {
                nodep->v3warn(ASCRANGE, "Ascending bit range vector: left < right of bit range: ["
                                            << nodep->ਖੱਬਾ_Const() << ":" << nodep->ਸੱਜਾ_Const()
                                            << "]");
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        // Signed: always unsigned; Real: Not allowed
        // LSB is self-determined (IEEE 2012 11.5.1)
        // We also use SELs to shorten a signed constant etc, in this case they are signed.
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "selWidth");
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->lsbp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->fromp(), ਝੂਠ);
            iterateCheckSizedSelf(nodep, "Select LHS", nodep->fromp(), SELF, BOTH);
            ਪੂਰਨ ਚੌੜਾਈ = nodep->ਚੌੜਾਈ_Const();
            ਜੇ (ਚੌੜਾਈ <= 0) {
                nodep->v3error("Width of bit extract must be positive (IEEE 1800-2023 11.5.1)");
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                ਵਾਪਸ;
            }
            UASSERT_OBJ(nodep->dtypep(), nodep, "dtype wasn't set");  // by V3WidthSel

            ਅਚਲ ਤਾਰਕਿਕ isWriteSelect = nodep->fromp()->isLValue();
            // Suppress SELRANGE in parameterized template modules where
            // parameter-dependent widths haven't been resolved yet.
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Parameterized_Template
                = m_modep && (m_modep->dead() || m_modep->parameterizedTemplate());

            ਜੇ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() < nodep->lsbConst()) {
                // Likely impossible given above width check
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: left < right of bit extract: "  // LCOV_EXCL_LINE
                                  << nodep->msbConst() << "<" << nodep->lsbConst());
                ਚੌੜਾਈ = (nodep->lsbConst() - nodep->msbConst() + 1);
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Sized(ਚੌੜਾਈ, VSigning::UNSIGNED,
                                               nodep->fromp()->isFourstateOrNoDtype());
                nodep->ਚੌੜਾਈ_Const(ਚੌੜਾਈ);
                ਧੱਕੋ_Deletep(nodep->lsbp());
                nodep->lsbp()->replaceWith(ਨਵਾਂ AstConst{nodep->lsbp()->fileline(), 0});
            }
            // We're extracting, so just make sure the expression is at least wide enough.
            ਜੇ (nodep->fromp()->ਚੌੜਾਈ() < ਚੌੜਾਈ && !ਵਿੱਚ_Parameterized_Template) {
                // If it is not a lvalue, extend it
                ਜੇ (!isWriteSelect) {
                    nodep->v3warn(SELRANGE, "Extracting " << ਚੌੜਾਈ << " bits from only "
                                                          << nodep->fromp()->ਚੌੜਾਈ()
                                                          << " bit number");
                    Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                        ਚੌੜਾਈ, ਚੌੜਾਈ, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    ਚੌੜਾਈ_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ਝੂਠ /*noerror*/);
                } ਨਹੀਂਤਾਂ {
                    // Partial assignment
                    nodep->v3warn(SELRANGE, "Assigning " << ਚੌੜਾਈ << " bits to only "
                                                         << nodep->fromp()->ਚੌੜਾਈ()
                                                         << " bit number");
                }
            }
            // Check bit indexes.
            // What is the MSB?  We want the true MSB, not one starting at
            // 0, because a 4 bit index is required to look at a one-bit
            // variable[15:15] and 5 bits for [15:-2]
            ਪੂਰਨ frommsb = nodep->fromp()->ਚੌੜਾਈ() - 1;
            ਪੂਰਨ fromlsb = 0;
            ਅਚਲ ਪੂਰਨ elw = nodep->decl_El_ਚੌੜਾਈ();  // Must adjust to tell user bit ranges
            ਜੇ (nodep->declRange().ranged()) {
                frommsb = nodep->declRange().hiMaxSelect() * elw
                          + (elw - 1);  // Corrected for negative lsb
                fromlsb = nodep->declRange().lo() * elw;
            } ਨਹੀਂਤਾਂ {
                // nodep->v3fatalSrc("Should have been declRanged in V3WidthSel");
            }
            ਅਚਲ ਪੂਰਨ selwidth = V3_ਸੰਖਿਆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ਗੰਢ_DType* ਅਚਲ selwidthDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                selwidth, selwidth, nodep->lsbp()->dtypep()->numeric(),
                nodep->lsbp()->dtypep()->isFourstate());
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, FINAL}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->lsbp(), ਚੌੜਾਈ_VP{SELF, FINAL}.p());
            ਜੇ (ਚੌੜਾਈ_Bad(nodep->lsbp(), selwidthDTypep) && nodep->lsbp()->ਚੌੜਾਈ() != 32) {
                ਜੇ (!nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::ਚੌੜਾਈ_2)) {
                    nodep->v3widthWarn(
                        (selwidth / elw), (nodep->lsbp()->ਚੌੜਾਈ() / elw),
                        "Bit extraction of var["
                            << (frommsb / elw) << ":" << (fromlsb / elw) << "] requires "
                            << (selwidth / elw) << " bit index, not "
                            << (nodep->lsbp()->ਚੌੜਾਈ() / elw)
                            << (nodep->lsbp()->ਚੌੜਾਈ() != nodep->lsbp()->ਚੌੜਾਈ_Min()
                                    ? " or " + cvt_ਮੰਜ਼ਿਲ_Str(nodep->lsbp()->ਚੌੜਾਈ_Min() / elw)
                                    : "")
                            << " bits.");
                    UINFO(1, "    Related node: " << nodep);
                }
            }
            ਜੇ (VN_IS(nodep->lsbp(), Const) && nodep->msbConst() > frommsb) {
                // See also warning in V3Const
                // We need to check here, because the widthCheckSized may silently
                // add another SEL which will lose the out-of-range check
                //
                // We don't want to trigger an error here if we are just
                // evaluating type sizes for a generate block condition. We
                // should only trigger the error if the out-of-range access is
                // actually generated.
                ਜੇ (m_doGenerate) {
                    UINFO(5, "Selection index out of range inside generate");
                } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Parameterized_Template) {
                    ਜੇ (nodep->declRange().ranged()) {
                        nodep->v3warn(SELRANGE, "Selection index out of range: "
                                                    << nodep->msbConst() << ":"
                                                    << nodep->lsbConst() << " outside " << frommsb
                                                    << ":" << fromlsb);
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3warn(SELRANGE,
                                      "Selection "
                                          << nodep->msbConst() << ":" << nodep->lsbConst()
                                          << " performed on an object without declared range");
                    }
                    UINFO(1, "    Related node: " << nodep);
                }
                // Extend unless it's a lvalue,
                // because extending lvalue would lose write access.
                ਜੇ (!isWriteSelect) {
                    // Extend it
                    ਅਚਲ ਪੂਰਨ extend_ਮੰਜ਼ਿਲ = nodep->msbConst() + 1;
                    Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                        extend_ਮੰਜ਼ਿਲ, extend_ਮੰਜ਼ਿਲ, nodep->fromp()->dtypep()->numeric(),
                        nodep->fromp()->dtypep()->isFourstate());
                    ਚੌੜਾਈ_Check_Sized(nodep, "errorless...", nodep->fromp(), subDTypep, EXTEND_EXP,
                                    ਝੂਠ /*noerror*/);
                }
            }
            // iterate FINAL is two blocks above
            //
            // If we have a width problem with GENERATE etc, this will reduce
            // it down and mask it, so we have no chance of finding a real
            // error in the future. So don't do this for them.
            ਜੇ (!m_doGenerate) {
                // lsbp() must be self-determined, however for performance
                // we want the select to be truncated to fit within the
                // maximum select range, e.g. turn Xs outside of the select
                // into something fast which pulls from within the array.
                ਚੌੜਾਈ_Check_Sized(nodep, "Extract Range", nodep->lsbp(), selwidthDTypep, EXTEND_EXP,
                                ਝੂਠ /*NOWARN*/);
            }
            // UINFOTREE(9, nodep, "", "seldone");
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override {
        // Signed/Real: Output signed iff LHS signed/real; binary operator
        // Note by contrast, bit extract selects are unsigned
        // LSB is self-determined (IEEE 2012 11.5.1)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "Bit select", nodep->bitp(), SELF, BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            // Element of a whole array of interfaces
            ਜੇ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                V3Const::constifyParamsEdit(nodep->bitp());  // May relink pointed to node
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->bitp(), Const)) {
                    ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = constp->ਮੰਜ਼ਿਲ_SInt();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = newIfaceArrayElement(nodep->fromp(), ਦੋਆਧਾਰੀ_ਅੰਕ_2);
                    ਜੇ (!newp) {
                        // No such instance, so an error, unlike other out of range selects
                        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp
                            = VN_AS(nodep->fromp()->dtypep()->skipRefp(), UnpackArrayDType);
                        nodep->v3error("Selection index out of range of interface array: "
                                       << ਦੋਆਧਾਰੀ_ਅੰਕ_2 + arrp->lo() << " outside " << arrp->hi() << ":"
                                       << arrp->lo() << " (IEEE 1800-2023 23.6)");
                        newp = newIfaceArrayElement(nodep->fromp(), 0);
                    }
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
            }
            //
            ਪੂਰਨ frommsb;
            ਪੂਰਨ fromlsb;
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
                frommsb = adtypep->hi();
                fromlsb = adtypep->lo();
                ਜੇ (fromlsb > frommsb) std::ਅਦਲਾ_ਬਦਲੀ(frommsb, fromlsb);
                // However, if the lsb<0 we may go negative, so need more bits!
                ਜੇ (fromlsb < 0) frommsb += -fromlsb;
                nodep->dtypeFrom(adtypep->subDTypep());  // Need to strip off array reference
            } ਨਹੀਂਤਾਂ {
                // Note PackArrayDType doesn't use an ArraySel but a normal Sel.
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Array reference exceeds dimension of array");
                frommsb = fromlsb = 0;
            }
            ਅਚਲ ਪੂਰਨ selwidth = V3_ਸੰਖਿਆ::log2b(frommsb + 1 - 1) + 1;  // Width to address a bit
            Ast_ਗੰਢ_DType* ਅਚਲ selwidthDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                selwidth, selwidth, nodep->bitp()->dtypep()->numeric(),
                nodep->bitp()->isFourstateOrNoDtype());
            ਜੇ (ਚੌੜਾਈ_Bad(nodep->bitp(), selwidthDTypep) && nodep->bitp()->ਚੌੜਾਈ() != 32) {
                nodep->v3widthWarn(selwidth, nodep->bitp()->ਚੌੜਾਈ(),
                                   "Bit extraction of array["
                                       << frommsb << ":" << fromlsb << "] requires " << selwidth
                                       << " bit index, not " << nodep->bitp()->ਚੌੜਾਈ()
                                       << (nodep->bitp()->ਚੌੜਾਈ() != nodep->bitp()->ਚੌੜਾਈ_Min()
                                               ? " or " + cvt_ਮੰਜ਼ਿਲ_Str(nodep->bitp()->ਚੌੜਾਈ_Min())
                                               : "")
                                       << " bits.");
                ਜੇ (!nodep->fileline()->warnIsOff(V3_Error_ਕੋਡ::ਚੌੜਾਈ_2)) {
                    UINFO(1, "    Related node: " << nodep);
                    UINFO(1, "    Related dtype: " << nodep->dtypep());
                }
            }
            ਜੇ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ਜੇ (VN_IS(nodep->bitp(), Const)
                    && (VN_AS(nodep->bitp(), Const)->ਮੰਜ਼ਿਲ_SInt() > (frommsb - fromlsb)
                        || VN_AS(nodep->bitp(), Const)->ਮੰਜ਼ਿਲ_SInt() < 0)) {
                    // Suppress in dead/parameterized template modules
                    ਜੇ (!(m_modep && (m_modep->dead() || m_modep->parameterizedTemplate()))) {
                        nodep->v3warn(SELRANGE,
                                      "Selection index out of range: "
                                          << (VN_AS(nodep->bitp(), Const)->ਮੰਜ਼ਿਲ_SInt() + fromlsb)
                                          << " outside " << frommsb << ":" << fromlsb);
                    }
                }
                ਚੌੜਾਈ_Check_Sized(nodep, "Extract Range", nodep->bitp(), selwidthDTypep, EXTEND_EXP,
                                ਝੂਠ /*NOWARN*/);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstAssocSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ਅਚਲ AstAssocArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, AssocArrayDType);
            ਜੇ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Associative array reference is not to associative array");
            }
            iterateCheckTyped(nodep, "Associative select", nodep->bitp(), adtypep->ਕੁੰਜੀ_DTypep(),
                              BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstAlias* nodep) override {
        ਜੇ (!nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) {
            user_Iterate_And_ਅਗਲਾ(nodep->itemsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }

        ਅਚਲ ਸਵੈਚਾਲਿਤ check_If_ਵਿਅੰਜਕ_Ok = [ਇਹ](ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp) {
            ਜੇ (VN_IS(exprp, VarXRef)) {
                exprp->v3error("Hierarchical reference used for net alias (IEEE 1800-2023 10.11)");
                ਵਾਪਸ ਝੂਠ;
            }

            ਅਚਲ AstVarRef* ਅਚਲ varRefp = VN_CAST(exprp, VarRef);
            ਜੇ (!varRefp) {
                exprp->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Operand of alias statement is not a variable reference");
                ਵਾਪਸ ਝੂਠ;
            }
            AstVar* ਅਚਲ varp = varRefp->varp();
            ਜੇ (!varp->isNet()) {
                exprp->v3error("Only nets are allowed in alias (IEEE 1800-2023 10.11): "
                               << varp->prettyNameQ());
                ਵਾਪਸ ਝੂਠ;
            }
            ਜੇ (m_aliasedVars.ਲੱਭੋ(varp) != m_aliasedVars.ਸਿਰਾ()) {
                varRefp->v3error("Alias is specified more than once (IEEE 1800-2023 10.11): "
                                 << varp->prettyNameQ());
                ਵਾਪਸ ਝੂਠ;
            } ਨਹੀਂਤਾਂ {
                m_aliasedVars.ਪਾਓ(varp);
            }
            ਵਾਪਸ ਸੱਚ;
        };

        check_If_ਵਿਅੰਜਕ_Ok(nodep->itemsp());
        Ast_ਗੰਢ_DType* ਅਚਲ ਪਹਿਲਾ_ਵਸਤੂ_Dtypep = nodep->itemsp()->dtypep();
        ਲਈ (Ast_ਗੰਢ* itemp = nodep->itemsp()->nextp(); itemp; itemp = itemp->nextp()) {
            check_If_ਵਿਅੰਜਕ_Ok(VN_AS(itemp, ਗੰਢ_ਵਿਅੰਜਕ));
            ਜੇ (!ਪਹਿਲਾ_ਵਸਤੂ_Dtypep->similarDType(itemp->dtypep())) {
                itemp->v3error("Incompatible data types of nets used for net alias. First operand "
                               "has the type "
                               << ਪਹਿਲਾ_ਵਸਤੂ_Dtypep->prettyDTypeNameQ() << ", other has "
                               << itemp->dtypep()->prettyDTypeNameQ());
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstWildcardSel* nodep) override {
        // Signed/Real: Output type based on array-declared type; binary operator
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ਅਚਲ AstWildcardArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, WildcardArrayDType);
            ਜੇ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Wildcard array reference is not to wildcard array");
            }
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = nodep->bitp()->dtypep()->skipRefp()->basicp();
            ਜੇ (!basicp
                || (basicp->keyword() != VBasicDTypeKwd::STRING
                    && !basicp->keyword().isIntNumeric())) {
                nodep->v3error("Wildcard index must be integral (IEEE 1800-2023 7.8.1)");
            }
            iterateCheckTyped(nodep, "Wildcard associative select", nodep->bitp(),
                              adtypep->ਲੱਭੋ_String_DType(), BOTH);
            nodep->dtypeFrom(adtypep->subDTypep());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਟੁਕੜਾ_Sel* nodep) override {
        // Always creates as output an unpacked array
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            //
            // Array indices are always constant
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skipRefp();
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, UnpackArrayDType);
            ਜੇ (!adtypep) {
                UINFO(1, "    Related dtype: " << fromDtp);
                nodep->v3fatalSrc("Packed array reference exceeds dimension of array");
            }
            // Build new array Dtype based on the original's base type, but with new bounds
            Ast_ਗੰਢ_DType* ਅਚਲ newDtp
                = ਨਵਾਂ AstUnpackArrayDType{nodep->fileline(), adtypep->subDTypep(),
                                          ਨਵਾਂ AstRange{nodep->fileline(), nodep->declRange()}};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newDtp);
            nodep->dtypeFrom(newDtp);

            ਜੇ (!m_doGenerate) {
                // Must check bounds before adding a select that truncates the bound
                // Note we've already subtracted off LSB
                ਅਚਲ ਪੂਰਨ subtracted = adtypep->declRange().lo();
                // Add subtracted value to get the original range
                ਅਚਲ VNumRange declRange{nodep->declRange().hi() + subtracted,
                                          nodep->declRange().lo() + subtracted,
                                          nodep->declRange().ascending()};
                ਜੇ ((declRange.hi() > adtypep->declRange().hi())
                    || declRange.lo() < adtypep->declRange().lo()) {
                    // Other simulators warn too
                    nodep->v3error("Slice selection index '" << declRange << "'"
                                                             << " outside data type's '"
                                                             << adtypep->declRange() << "'");
                    // Replaced by a placeholder value, so no element is out of range later
                    nodep->replaceWith(ਨਵਾਂ AstConsArray{
                        nodep->fileline(), newDtp,
                        ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}}});
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ ਜੇ ((declRange.ascending() != adtypep->declRange().ascending())
                           && declRange.hi() != declRange.lo()) {
                    nodep->v3error("Slice selection '"
                                   << declRange << "'"
                                   << " has reversed range order versus data type's '"
                                   << adtypep->declRange() << "'");
                }
            }
            // A slice of a whole array of interfaces is the array of the selected elements
            ਜੇ (VN_IS(nodep->fromp(), ConsArray) && isIfaceArrayDType(nodep->fromp()->dtypep())) {
                AstConsArray* ਅਚਲ fromp = VN_AS(nodep->fromp(), ConsArray);
                AstConsArray* ਅਚਲ newp = ਨਵਾਂ AstConsArray{nodep->fileline(), newDtp};
                ਅਚਲ VNumRange range = nodep->declRange();
                ਲਈ (ਪੂਰਨ n = 0; n < range.elements(); ++n) {
                    newp->valuep(n, fromp->valuep(range.lo() + n)->clone_ਰੁੱਖ(ਝੂਠ));
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Sel_ਦੋਆਧਾਰੀ_ਅੰਕ* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->bitp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->thsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->attrp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ* ਅਚਲ selp = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Sel_No_Iter_Edit(nodep);
        ਜੇ (selp != nodep) {
            VL_DANGLING(nodep);
            userIterate(selp, m_vup);
            ਵਾਪਸ;
        }
        nodep->v3fatalSrc("AstSelBit should disappear after widthSel");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sel_ਕੱਢੋ* nodep) override {
        // Just a quick check as after V3Param these nodes instead are AstSel's
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->leftp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->rightp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->attrp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ* ਅਚਲ selp = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Sel_No_Iter_Edit(nodep);
        ਜੇ (selp != nodep) {
            nodep = ਖਾਲੀ_ਸੂਚਕ;
            userIterate(selp, m_vup);
            ਵਾਪਸ;
        }
        nodep->v3fatalSrc("AstSelExtract should disappear after widthSel");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSelPlus* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->bitp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->widthp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->attrp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ* ਅਚਲ selp = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Sel_No_Iter_Edit(nodep);
        ਜੇ (selp != nodep) {
            nodep = ਖਾਲੀ_ਸੂਚਕ;
            userIterate(selp, m_vup);
            ਵਾਪਸ;
        }
        nodep->v3fatalSrc("AstSelPlus should disappear after widthSel");
    }
    ਖਾਲੀ ਦੇਖੋ(AstSelMinus* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->bitp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->widthp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());  // FINAL in AstSel
        user_Iterate_And_ਅਗਲਾ(nodep->attrp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ* ਅਚਲ selp = V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Sel_No_Iter_Edit(nodep);
        ਜੇ (selp != nodep) {
            nodep = ਖਾਲੀ_ਸੂਚਕ;
            userIterate(selp, m_vup);
            ਵਾਪਸ;
        }
        nodep->v3fatalSrc("AstSelMinus should disappear after widthSel");
    }

    ਖਾਲੀ ਦੇਖੋ(AstExtend* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstExtendS* nodep) override {
        // Typically created by this process, so we know width from here down is correct.
        // Exception is extend added by V3WidthSel - those need iteration
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        // The node got setup with the signed/real state of the node.
        // However a later operation may have changed the node->signed w/o changing
        // the number's sign.  So we don't: nodep->dtypeChgSigned(nodep->num().isSigned());
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        ਜੇ (m_vup && m_vup->prelim()) {
            ਜੇ (VN_IS(nodep->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType)
                || VN_IS(nodep->dtypep()->skipRefp(), StructDType)
                || VN_IS(nodep->dtypep()->skipRefp(), PackArrayDType)) {
                // Assume this constant was properly casted earlier
                // (Otherwise it couldn't have an enum data type)
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਅੰਕ().isNull()) {
                nodep->dtypep(nodep->ਲੱਭੋ_Null_DType());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਅੰਕ().isString()) {
                nodep->dtype_ਸੈੱਟ_String();
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਅੰਕ().isDouble()) {
                nodep->dtype_Chg_ਚੌੜਾਈ(nodep->ਅੰਕ().ਚੌੜਾਈ(), nodep->ਅੰਕ().sized()
                                                               ? nodep->ਅੰਕ().ਚੌੜਾਈ()
                                                               : nodep->ਅੰਕ().ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit());
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                    nodep->ਅੰਕ().ਚੌੜਾਈ(),
                    nodep->ਅੰਕ().sized() ? nodep->ਅੰਕ().ਚੌੜਾਈ() : nodep->ਅੰਕ().ਚੌੜਾਈ_ਮੰਜ਼ਿਲ_Fit(),
                    nodep->dtypep()->numeric(), nodep->ਅੰਕ().isAnyXZ());
            }
        }
        // We don't size the constant until we commit the widths, as need parameters
        // to remain unsized, and numbers to remain unsized to avoid backp() warnings
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ* nodep) override {
        nodep->dtype_ਸੈੱਟ_ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ();
        ਜੇ (!VN_IS(nodep->backp(), ਸੌਂਪੋ) && !VN_IS(nodep->backp(), Var)
            && !VN_IS(nodep->backp(), Arg)) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/Illegal: empty queue ('{}') in this context");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFell* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            userIterate(nodep->sentreep(), ਖਾਲੀ_ਸੂਚਕ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFalling* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuture* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            userIterate(nodep->sentreep(), ਖਾਲੀ_ਸੂਚਕ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPast* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            nodep->dtypeFrom(nodep->exprp());
            ਜੇ (nodep->ticksp()) {
                iterateCheckSizedSelf(nodep, "Ticks", nodep->ticksp(), SELF, BOTH,
                                      /* deferUntyped */ ਸੱਚ);
                V3Const::constifyParamsEdit(nodep->ticksp());  // ticksp may change
                ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->ticksp(), Const);
                ਜੇ (!constp) {
                    nodep->v3error("$past tick value must be constant (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                } ਨਹੀਂਤਾਂ ਜੇ (constp->ਮੰਜ਼ਿਲ_SInt() < 1) {
                    constp->v3error("$past tick value must be >= 1 (IEEE 1800-2023 16.9.3)");
                    nodep->ticksp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                } ਨਹੀਂਤਾਂ ਜੇ (V3AssertCommon::check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(nodep, constp, "$past tick value")) {
                    ਜੇ (constp->ਮੰਜ਼ਿਲ_SInt() > 10) {
                        constp->v3warn(TICKCOUNT, "$past tick value of "
                                                      << constp->ਮੰਜ਼ਿਲ_SInt()
                                                      << " may have a large performance cost");
                    }
                }
            }
            userIterate(nodep->sentreep(), ਖਾਲੀ_ਸੂਚਕ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSConsRep* nodep) override {
        // IEEE 1800-2023 16.9.2 -- consecutive repetition [*N], [*N:M], [*N:$], [+], [*]
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->countp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            V3Const::constifyParamsEdit(nodep->countp());
            ਅਚਲ AstConst* ਅਚਲ minConstp = VN_CAST(nodep->countp(), Const);
            ਜੇ (!minConstp) {
                nodep->v3error("Consecutive repetition count must be constant expression"
                               " (IEEE 1800-2023 16.9.2)");
            } ਨਹੀਂਤਾਂ ਜੇ (minConstp->ਮੰਜ਼ਿਲ_SInt() < 0) {
                nodep->v3error("Consecutive repetition count must be non-negative"
                               " (IEEE 1800-2023 16.9.2)");
            } ਨਹੀਂਤਾਂ ਜੇ (!nodep->unbounded() && !nodep->maxCountp() && minConstp->ਮੰਜ਼ਿਲ_SInt() < 1) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*0] consecutive repetition");
            }
            ਜੇ (nodep->maxCountp()) {
                user_Iterate_And_ਅਗਲਾ(nodep->maxCountp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->maxCountp());
                ਅਚਲ AstConst* ਅਚਲ maxConstp = VN_CAST(nodep->maxCountp(), Const);
                ਜੇ (!maxConstp) {
                    nodep->v3error("Consecutive repetition max count must be constant"
                                   " expression (IEEE 1800-2023 16.9.2)");
                } ਨਹੀਂਤਾਂ ਜੇ (minConstp && maxConstp->ਮੰਜ਼ਿਲ_SInt() < minConstp->ਮੰਜ਼ਿਲ_SInt()) {
                    nodep->v3error("Consecutive repetition max count must be >= min count"
                                   " (IEEE 1800-2023 16.9.2)");
                } ਨਹੀਂਤਾਂ ਜੇ (maxConstp->ਮੰਜ਼ਿਲ_SInt() < 1) {
                    nodep->v3warn(E_UNSUPPORTED, "Unsupported: [*N:0] consecutive repetition"
                                                 " with zero max count");
                }
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPropAlways* nodep) override {
        // IEEE 1800-2023 16.12.11
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->propp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* propp = nodep->propp();
            ਜੇ (!VN_IS(nodep->loBoundp(), Unbounded)) {
                user_Iterate_And_ਅਗਲਾ(nodep->loBoundp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->loBoundp());
            }
            ਜੇ (!VN_IS(nodep->hiBoundp(), Unbounded)) {
                user_Iterate_And_ਅਗਲਾ(nodep->hiBoundp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                V3Const::constifyParamsEdit(nodep->hiBoundp());
            }
            ਅਚਲ ਤਾਰਕਿਕ loUnbounded = VN_IS(nodep->loBoundp(), Unbounded);
            ਅਚਲ ਤਾਰਕਿਕ hiUnbounded = VN_IS(nodep->hiBoundp(), Unbounded);
            // Strong always must be bounded (IEEE 1800-2023 16.12.11: "the range
            // for a strong always shall be bounded"). Weak always [m:$] is legal:
            // an unbounded upper bound imposes no end-of-trace obligation.
            ਜੇ (nodep->isStrong() && (loUnbounded || hiUnbounded)) {
                Ast_ਗੰਢ* ਅਚਲ boundp = loUnbounded ? nodep->loBoundp() : nodep->hiBoundp();
                boundp->v3error("s_always range must be bounded (IEEE 1800-2023 16.12.11)");
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                ਵਾਪਸ;
            }
            ਜੇ (loUnbounded) {
                // Only the high bound may be $ (cycle_delay_const_range_expression).
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
                ਵਾਪਸ;
            }
            ਅਚਲ AstConst* ਅਚਲ loConstp = VN_CAST(nodep->loBoundp(), Const);
            ਅਚਲ AstConst* ਅਚਲ hiConstp = VN_CAST(nodep->hiBoundp(), Const);
            ਜੇ (!loConstp) {
                nodep->loBoundp()->v3error("always range low bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ਜੇ (!hiUnbounded && !hiConstp) {
                nodep->hiBoundp()->v3error("always range high bound must be a constant expression"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ਜੇ (loConstp && loConstp->ਮੰਜ਼ਿਲ_SInt() < 0) {
                nodep->loBoundp()->v3error("always range low bound must be non-negative"
                                           " (IEEE 1800-2023 16.12.11)");
            } ਨਹੀਂਤਾਂ ਜੇ (loConstp) {
                V3AssertCommon::check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(nodep->loBoundp(), loConstp,
                                                   "always range bound");
            }
            ਜੇ (!hiUnbounded && hiConstp) {
                V3AssertCommon::check_Sva_ਮੁੱਲ_ਹੱਦਬੰਦੀ(nodep->hiBoundp(), hiConstp,
                                                   "always range bound");
            }
            ਜੇ (!hiUnbounded && loConstp && hiConstp && hiConstp->ਮੰਜ਼ਿਲ_SInt() < loConstp->ਮੰਜ਼ਿਲ_SInt()) {
                nodep->hiBoundp()->v3error("always range high bound must be >= low bound"
                                           " (IEEE 1800-2023 16.12.11)");
            }
            ਤਾਰਕਿਕ hasPropertyOp = propp->is_Multi_ਚੱਕਰ_Sva();
            ਜੇ (!hasPropertyOp) {
                propp->foreach([&](ਅਚਲ Ast_ਗੰਢ* np) {
                    ਜੇ (VN_IS(np, Implication) || VN_IS(np, Until) || VN_IS(np, PropSpec)) {
                        hasPropertyOp = ਸੱਚ;
                    }
                });
            }
            ਜੇ (hasPropertyOp) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported: property/sequence operator inside bounded"
                              " always (IEEE 1800-2023 16.12.11)");
            }
            VL_DO_DANGLING(fix_ਚੌੜਾਈ_Reduce(propp), propp);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRising* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRose* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            userIterate(nodep->sentreep(), ਖਾਲੀ_ਸੂਚਕ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSampled* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            nodep->dtypeFrom(nodep->exprp());
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstSetuphold* nodep) override {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਸੌਂਪੋ_W* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->delrefp()) {
            newp = convert_Setuphold_ਮੰਜ਼ਿਲ_ਸੌਂਪੋ(flp, nodep->refevp(), nodep->delrefp());
        }
        ਜੇ (nodep->deldatap()) {
            ਜੇ (!newp) {
                newp = convert_Setuphold_ਮੰਜ਼ਿਲ_ਸੌਂਪੋ(flp, nodep->dataevp(), nodep->deldatap());
            } ਨਹੀਂਤਾਂ {
                newp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(
                    convert_Setuphold_ਮੰਜ਼ਿਲ_ਸੌਂਪੋ(flp, nodep->dataevp(), nodep->deldatap()));
            }
        }
        ਜੇ (!newp) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        nodep->replaceWith(ਨਵਾਂ AstAlways{newp});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    Ast_ਸੌਂਪੋ_W* convert_Setuphold_ਮੰਜ਼ਿਲ_ਸੌਂਪੋ(File_ਸਤਰ* ਅਚਲ flp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ evp,
                                         Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ delp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = delp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = evp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        UASSERT_OBJ(VN_IS(lhsp, ਗੰਢ_Var_Ref) || VN_IS(lhsp, ਗੰਢ_Pre_Sel), lhsp,
                    "Incorrect reference in a timing check");
        ਜੇ (Ast_ਗੰਢ_Var_Ref* varRefp = VN_CAST(lhsp, ਗੰਢ_Var_Ref)) {
            ਜੇ (varRefp->varp()->direction() == VDirection::ਨਿਵੇਸ਼) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            varRefp->access(VAccess::WRITE);
        }
        ਜੇ (Ast_ਗੰਢ_Pre_Sel* selp = VN_CAST(lhsp, ਗੰਢ_Pre_Sel)) {
            ਜੇ (Ast_ਗੰਢ_Var_Ref* varRefp = VN_CAST(selp->fromp(), ਗੰਢ_Var_Ref)) {
                ਜੇ (varRefp->varp()->direction() == VDirection::ਨਿਵੇਸ਼) {
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                }
                varRefp->access(VAccess::WRITE);
            }
        }
        ਵਾਪਸ ਨਵਾਂ Ast_ਸੌਂਪੋ_W{flp, lhsp, rhsp};
    }

    ਖਾਲੀ ਦੇਖੋ(AstStable* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            userIterate(nodep->sentreep(), ਖਾਲੀ_ਸੂਚਕ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSteady* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->exprp(), SELF, BOTH,
                                  /* deferUntyped */ ਸੱਚ);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Stmt_ਵਿਅੰਜਕ* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }

    ਖਾਲੀ ਦੇਖੋ(AstImplication* nodep) override {
        m_seqUnsupp = nodep;
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstUntil* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            // Coerce unsized constant operands (e.g. literal 0/1) to actual
            // 1-bit width; iterateCheckBool keeps widthMin-fitting unsized
            // constants at their nominal 32-bit width, which trips the
            // downstream NFA lowering's Log* chains in V3AssertNfa.
            ਜੇ (nodep->lhsp()->ਚੌੜਾਈ() != 1) fix_ਚੌੜਾਈ_Reduce(nodep->lhsp());
            ਜੇ (nodep->rhsp()->ਚੌੜਾਈ() != 1) fix_ਚੌੜਾਈ_Reduce(nodep->rhsp());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstAbortOn* nodep) override { ਦੇਖੋ_Abort_Prop(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstRand* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            ਜੇ (nodep->urandom()) {
                nodep->dtype_ਸੈੱਟ_UInt32();  // Says the spec
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_ਸੈੱਟ_Int();  // Says the spec
            }
            ਜੇ (nodep->seedp()) iterateCheckSigned32(nodep, "seed", nodep->seedp(), BOTH);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSEventually* nodep) override {
        ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False() || !v3Global.opt.timing().is_ਸੈੱਟ_True()) {
            nodep->v3warn(E_NOTIMING, "s_eventually requires --timing");
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 1, 0});
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSGotoRep* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->countp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਜੇ (nodep->maxCountp()) {
                ਚੌੜਾਈ_Check_Goto_Rep_Range(nodep, "Goto");
            } ਨਹੀਂਤਾਂ {
                ਚੌੜਾਈ_Check_Rep_ਗਿਣਤੀ(nodep);
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSNonConsRep* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->countp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਜੇ (nodep->maxCountp()) {
                ਚੌੜਾਈ_Check_Goto_Rep_Range(nodep, "Nonconsecutive");
            } ਨਹੀਂਤਾਂ {
                ਚੌੜਾਈ_Check_Rep_ਗਿਣਤੀ(nodep);
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Rep>
    ਖਾਲੀ ਚੌੜਾਈ_Check_Rep_ਗਿਣਤੀ(T_Rep* nodep) {
        V3Const::constifyParamsNoWarnEdit(nodep->countp());
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->countp(), Const);
        ਜੇ (!constp) {
            nodep->v3error("Repetition count is not an elaboration-time constant"
                           " (IEEE 1800-2023 16.9.2)");
        } ਨਹੀਂਤਾਂ ਜੇ (constp->dtypep()->isSigned() && constp->ਅੰਕ().isNegative()) {
            nodep->v3error("Repetition count must be non-negative"
                           " (IEEE 1800-2023 16.9.2)");
        } ਨਹੀਂਤਾਂ ਜੇ (constp->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() > 31) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: SVA repetition count exceeds implementation limit of 2147483647");
        } ਨਹੀਂਤਾਂ ਜੇ (constp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: zero repetition count"
                                         " (IEEE 1800-2023 16.9.2)");
        }
    }
    // IEEE 1800-2023 16.9.2 range-form bound validation for goto/nonconsec.
    // Parent accessors are re-fetched after constifyParamsEdit because that
    // call can replace the node in-tree (Lesson: AstSConsRep visitor pattern).
    // Templated to cover AstSGotoRep and AstSNonConsRep uniformly.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Rep>
    ਖਾਲੀ ਚੌੜਾਈ_Check_Goto_Rep_Range(T_Rep* nodep, ਅਚਲ ਅੱਖਰ* ਕਿਸਮ) {
        user_Iterate_And_ਅਗਲਾ(nodep->maxCountp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        V3Const::constifyParamsEdit(nodep->countp());
        V3Const::constifyParamsEdit(nodep->maxCountp());
        ਅਚਲ AstConst* ਅਚਲ minConstp = VN_CAST(nodep->countp(), Const);
        ਅਚਲ AstConst* ਅਚਲ maxConstp = VN_CAST(nodep->maxCountp(), Const);
        ਜੇ (!minConstp || !maxConstp) {
            nodep->v3error(std::string{ਕਿਸਮ}
                           + " repetition range bounds must be constant expressions"
                             " (IEEE 1800-2023 16.9.2)");
            ਵਾਪਸ;
        }
        ਜੇ (minConstp->ਮੰਜ਼ਿਲ_SInt() < 0) {
            nodep->v3error(std::string{ਕਿਸਮ}
                           + " repetition range min count must be non-negative"
                             " (IEEE 1800-2023 16.9.2)");
            ਵਾਪਸ;
        }
        ਜੇ (maxConstp->ਮੰਜ਼ਿਲ_SInt() < minConstp->ਮੰਜ਼ਿਲ_SInt()) {
            nodep->v3error(std::string{ਕਿਸਮ}
                           + " repetition range max count must be >= min count"
                             " (IEEE 1800-2023 16.9.2)");
            ਵਾਪਸ;
        }
        ਜੇ (minConstp->isZero()) {
            nodep->v3warn(E_UNSUPPORTED, std::string{"Unsupported: zero min count in "} + ਕਿਸਮ
                                             + " repetition range (IEEE 1800-2023 16.9.2)");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSThroughout* nodep) override {
        m_hasSExpr = ਸੱਚ;
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // lhsp is a boolean expression, not a sequence -- clear m_underSExpr temporarily
            VL_RESTORER(m_underSExpr);
            m_underSExpr = ਝੂਠ;
            iterateCheckBool(nodep, "lhsp", nodep->lhsp(), BOTH);
            m_underSExpr = ਸੱਚ;
            iterate(nodep->rhsp());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSExpr* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ਸੱਚ;
        m_hasSExpr = ਸੱਚ;
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            ਜੇ (nodep->preExprp()) {
                iterateCheckBool(nodep, "preExprp", nodep->preExprp(), BOTH);
            }
            iterate(nodep->delayp());
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSClocked* nodep) override {
        VL_RESTORER(m_underSExpr);
        m_underSExpr = ਸੱਚ;
        m_hasSExpr = ਸੱਚ;
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // Width each clock expression directly; the senitem chain is hoisted to
            // the assertion clock by V3AssertNfa, where it is fully processed.
            ਲਈ (Ast_Sen_ਵਸਤੂ* senp = nodep->sensesp(); senp;
                 senp = VN_CAST(senp->nextp(), Sen_ਵਸਤੂ)) {
                user_Iterate_And_ਅਗਲਾ(senp->sensp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            }
            iterateCheckBool(nodep, "exprp", nodep->exprp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstURandomRange* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            nodep->dtype_ਸੈੱਟ_UInt32();  // Says the spec
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = nodep->ਲੱਭੋ_UInt32_DType();
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), SELF, FINAL, expDTypep, EXTEND_EXP);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnbounded* nodep) override {
        nodep->dtype_ਸੈੱਟ_Int();  // Used in int context
        ਜੇ (VN_IS(nodep->backp(), IsUnbounded)) ਵਾਪਸ;  // Ok, leave
        ਜੇ (VN_IS(nodep->backp(), BracketArrayDType)) ਵਾਪਸ;  // Ok, leave
        ਜੇ (VN_IS(nodep->backp(), InsideRange)) ਵਾਪਸ;  // Ok, leave
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ varp = VN_CAST(nodep->backp(), Var)) {
            ਜੇ (varp->isParam()) ਵਾਪਸ;  // Ok, leave
        }
        Ast_ਗੰਢ* backp = nodep->backp();
        ਜੇ (VN_IS(backp, Sub)) backp = backp->backp();
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ selp = VN_CAST(backp, Sel_ਕੱਢੋ)) {
            ਜੇ (VN_IS(selp->fromp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ selp = VN_CAST(backp, Sel_ਦੋਆਧਾਰੀ_ਅੰਕ)) {
            ਜੇ (VN_IS(selp->fromp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) ਵਾਪਸ;
        }
        // queue_slice[#:$] and queue_bitsel[$] etc handled in V3WidthSel
        nodep->v3warn(E_UNSUPPORTED,
                      "Unsupported: Unbounded ('$') outside of queue or string operations");
    }
    ਖਾਲੀ ਦੇਖੋ(AstInferredDisable* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstIsUnbounded* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        // Give it the size the user wants.
        ਜੇ (m_vup && m_vup->prelim()) {
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, 1, VSigning::UNSIGNED);  // We don't care
            // All arguments seek their natural sizes
            userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Assume user knows the rules; go with the flow
            ਜੇ (nodep->ਚੌੜਾਈ() > 64) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: $c can't generate wider than 64 bits");
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCLog2* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
    }
    // Width a 'weight' coverage option as the signed 'int' member it sets (IEEE 1800-2023
    // 19.10); a weight is a non-negative integral value, constant for type_option (19.7)
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Option>
    ਖਾਲੀ ਚੌੜਾਈ_Cover_ਭਾਰ(T_Option* nodep) {
        iterateCheckSigned32(nodep, "weight", nodep->valuep(), BOTH);
        V3Const::constifyEdit(nodep->valuep());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = nodep->valuep();
        ਅਚਲ std::string optName
            = std::string{nodep->typeOption() ? "type_option" : "option"} + ".weight";
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
            ਜੇ (!constp->ਅੰਕ().isNegative()) ਵਾਪਸ;
            valuep->v3error("Coverage option '" << optName << "' is set to negative value '"
                                                << constp->ਮੰਜ਼ਿਲ_SInt()
                                                << "'; weights must be non-negative"
                                                   " (IEEE 1800-2023 19.7)");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->typeOption()) {
            valuep->v3error("Coverage option '" << optName
                                                << "' requires a constant expression"
                                                   " (IEEE 1800-2023 19.7.1)");
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ;
        }
        // Continue with the default weight
        valuep->replaceWith(ਨਵਾਂ AstConst{valuep->fileline(), AstConst::Signed32{}, 1});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
    }
    // Width an integral coverage option V3Covergroup reads, folding a constant expression
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Option>
    ਖਾਲੀ ਚੌੜਾਈ_Cover_Int(T_Option* nodep) {
        user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        V3Const::constifyEdit(nodep->valuep());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cg_Option_ਸੌਂਪੋ* nodep) override {
        // Recursive function widthing can reach a covergroup constructor without first visiting
        // its class, so find the owning covergroup structurally instead of using visit context.
        AstClass* ਅਚਲ cgClassp = m_containingClassFinder.ਲੱਭੋ(nodep);
        UASSERT_OBJ(cgClassp && cgClassp->isCovergroup(), nodep,
                    "Covergroup option is not under a covergroup class");

        // V3Covergroup stores the weight into the covergroup's option or type_option
        ਜੇ (nodep->optType() == VCoverOptionType::ਭਾਰ) {
            ਚੌੜਾਈ_Cover_ਭਾਰ(nodep);
            ਵਾਪਸ;
        }
        // V3Covergroup takes a constant option.at_least as the default for coverpoints/crosses
        ਜੇ (nodep->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ) {
            ਚੌੜਾਈ_Cover_Int(nodep);
            ਵਾਪਸ;
        }

        // Extract covergroup option values and store in AstClass before deleting.
        ਜੇ (nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            // By V3Width time, V3Param has already folded any parameter references.
            // If the value is still not a constant, it is a runtime expression - emit error.
            ਜੇ (AstConst* constp = VN_CAST(nodep->valuep(), Const)) {
                cgClassp->cgAutoBinMax(constp->ਮੰਜ਼ਿਲ_SInt());
                UINFO(6, "  Covergroup " << cgClassp->name()
                                         << " option.auto_bin_max = " << constp->ਮੰਜ਼ਿਲ_SInt() << ਸਤਰ_ਅੰਤ);
            } ਨਹੀਂਤਾਂ {
                nodep->valuep()->v3warn(COVERIGN, "Ignoring unsupported: non-constant "
                                                  "'option.auto_bin_max'; using default value");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->optType() == VCoverOptionType::ਮਿਲਾਓ_INSTANCES) {
            // V3Covergroup stores it into type_option; as a type option it is constant
            // (IEEE 1800-2023 19.7.1)
            iterateCheckBool(nodep, "merge_instances", nodep->valuep(), BOTH);
            V3Const::constifyEdit(nodep->valuep());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = nodep->valuep();
            ਜੇ (!VN_IS(valuep, Const)) {
                valuep->v3error("Coverage option 'type_option.merge_instances' requires a"
                                " constant expression (IEEE 1800-2023 19.7.1)");
                valuep->replaceWith(ਨਵਾਂ AstConst{valuep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            }
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->optType() == VCoverOptionType::GET_INST_COVERAGE) {
            // V3Covergroup stores it into option, as the constructor evaluates it
            iterateCheckBool(nodep, "get_inst_coverage", nodep->valuep(), BOTH);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->optType() == VCoverOptionType::DISTRIBUTE_ਪਹਿਲਾ) {
            // A bins 'with' filter applies before the values are distributed to the bins
            // (IEEE 1800-2023 19.5.1.1)
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->valuep(), Const);
            ਜੇ (!constp || !constp->ਅੰਕ().isEqZero()) {
                nodep->v3warn(COVERIGN, "Ignoring unsupported: 'type_option.distribute_first';"
                                        " 'with' filters apply before values are distributed"
                                        " to bins");
            }
        }
        // Add more options here as needed (goal, per_instance, comment)

        // Delete the assignment node (we've extracted the value)
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverOption* nodep) override {
        ਜੇ (nodep->optType() == VCoverOptionType::ਭਾਰ) {
            ਚੌੜਾਈ_Cover_ਭਾਰ(nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->optType() == VCoverOptionType::AT_ਘੱਟ_ਤੋਂ_ਘੱਟ
            || nodep->optType() == VCoverOptionType::AUTO_BIN_MAX) {
            ਚੌੜਾਈ_Cover_Int(nodep);
            ਵਾਪਸ;
        }
        user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCross* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->itemsp(), ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_ਅਗਲਾ(nodep->optionsp(), ਖਾਲੀ_ਸੂਚਕ);
        user_Iterate_And_ਅਗਲਾ(nodep->binsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossBin* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->selectp(), ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverpoint* nodep) override {
        // The coverpoint expression is self-determined (IEEE 1800-2023 19.5).  Width it
        // with a context so a bit/part-select (AstSel) is sized here; otherwise it would
        // reach assertAtExpr() with m_vup==null and fail as an internal error.
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // Bin values compare against the coverpoint expression
        user_Iterate_And_ਅਗਲਾ(nodep->binsp(), ਚੌੜਾਈ_VP{nodep->exprp()->dtypep(), BOTH}.p());
        ਜੇ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        user_Iterate_And_ਅਗਲਾ(nodep->optionsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਚੌੜਾਈ_Covergroup_Ranges(Ast_ਗੰਢ* rangesp, ਪੂਰਨ fill_ਚੌੜਾਈ) {
        // Bin range/value entries are self-determined expressions (IEEE 1800-2023
        // 19.5).  Width each plain single-value entry self-determined so a referenced
        // parameter acquires a dtype, then constify so the reference folds to the AstConst
        // value that V3Covergroup requires.  AstInsideRange entries fold their own bounds in
        // visit(AstInsideRange).
        // '0/'1 entries then fill to fillWidth, the coverpoint width (IEEE 1800-2023 19.5.7).
        ਅਚਲ ਸਵੈਚਾਲਿਤ fill = [&](Ast_ਗੰਢ* nodep) {
            ਜੇ (!fill_ਚੌੜਾਈ) ਵਾਪਸ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = VN_AS(nodep, ਗੰਢ_ਵਿਅੰਜਕ);
            fixAutoExtend(exprp /*ref*/, fill_ਚੌੜਾਈ);
        };
        ਲਈ (Ast_ਗੰਢ *nextp, *itemp = rangesp; itemp; itemp = nextp) {
            nextp = itemp->nextp();
            ਜੇ (AstInsideRange* ਅਚਲ rangep = VN_CAST(itemp, InsideRange)) {
                userIterate(rangep, ਖਾਲੀ_ਸੂਚਕ);
                fill(rangep->lhsp());
                fill(rangep->rhsp());
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(itemp, CoverWith)) {
                userIterate(itemp, m_vup);
            } ਨਹੀਂਤਾਂ {
                itemp = userIterateSubtreeReturnEdits(itemp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                fill(V3Const::constifyEdit(itemp));
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBinsof* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->pointp(), ਖਾਲੀ_ਸੂਚਕ);
        ਚੌੜਾਈ_Covergroup_Ranges(nodep->rangesp(), 0);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverBin* nodep) override {
        // No m_vup for a bin directly in a covergroup body (unsupported, already warned)
        ਚੌੜਾਈ_Covergroup_Ranges(nodep->rangesp(), m_vup ? m_vup->dtypep()->ਚੌੜਾਈ() : 0);
        ਜੇ (nodep->iffp()) iterateCheckBool(nodep, "iff condition", nodep->iffp(), BOTH);
        ਜੇ (nodep->arraySizep()) {
            // The size of 'bins b[N]' or 'bins auto[N]' is a self-determined expression, which
            // V3Covergroup checks once folded
            user_Iterate_And_ਅਗਲਾ(nodep->arraySizep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->arraySizep());  // arraySizep may change
        }
        user_Iterate_And_ਅਗਲਾ(nodep->transp(), m_vup);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverWith* nodep) override {
        // The candidate value 'item' has the coverpoint's type (IEEE 1800-2023 19.5.1.1)
        user_Iterate_And_ਅਗਲਾ(nodep->itemp(), ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (!nodep->itemp()->dtypeSkipRefp()->isIntegralOrPacked()) {
            nodep->v3error("Bin 'with' filters are not allowed on a coverpoint of a "
                           "non-integral expression (IEEE 1800-2023 19.5.1.1)");
        }
        ਜੇ (!VN_IS(nodep->subp(), CoverpointRef)) {
            ਚੌੜਾਈ_Covergroup_Ranges(nodep->subp(), m_vup->dtypep()->ਚੌੜਾਈ());
        }
        ਚੌੜਾਈ_Cover_Filter(nodep->filterp(), "Bin");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverCrossWith* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->selectp(), ਖਾਲੀ_ਸੂਚਕ);
        // Each cross item has the type of its coverpoint (IEEE 1800-2023 19.6.1.3)
        user_Iterate_And_ਅਗਲਾ(nodep->varsp(), ਖਾਲੀ_ਸੂਚਕ);
        ਚੌੜਾਈ_Cover_Filter(nodep->filterp(), "Cross bin");
        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ countp = nodep->countp()) {
            ਜੇ (VN_IS(countp, Unbounded)) {
                countp->dtype_ਸੈੱਟ_Int();  // 'matches $'
            } ਨਹੀਂਤਾਂ {
                // The count of 'matches' is self-determined, which V3Covergroup checks once folded
                user_Iterate_And_ਅਗਲਾ(countp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                V3Const::constifyEdit(nodep->countp());  // countp may change
            }
        }
    }
    // A 'with' filter is true for a nonzero value (IEEE 1800-2023 12.4), of a type assignment
    // compatible with an integral type (19.5.1.1); 'whatp' names its bins
    ਖਾਲੀ ਚੌੜਾਈ_Cover_Filter(Ast_ਗੰਢ_ਵਿਅੰਜਕ* filterp, ਅਚਲ ਅੱਖਰ* whatp) {
        filterp = VN_AS(userIterateSubtreeReturnEdits(filterp, ਚੌੜਾਈ_VP{SELF, BOTH}.p()), ਗੰਢ_ਵਿਅੰਜਕ);
        File_ਸਤਰ* ਅਚਲ fl = filterp->fileline();
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ resultp = filterp->dtypep()->skipRefp();
        ਜੇ (resultp->isDouble()) {
            VNRelinker relinker;
            filterp->unlink_Fr_ਪਿੱਠ(&relinker);
            relinker.relink(
                ਨਵਾਂ AstNeqD{fl, filterp, ਨਵਾਂ AstConst{fl, AstConst::RealDouble{}, 0.0}});
        } ਨਹੀਂਤਾਂ ਜੇ (!resultp->isIntegralOrPacked()) {
            filterp->v3error(whatp << " 'with' filter must be assignment compatible with an "
                                      "integral type, not "
                                   << resultp->prettyDTypeNameQ() << " (IEEE 1800-2023 19.5.1.1)");
            filterp->replaceWith(ਨਵਾਂ AstConst{fl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(filterp), filterp);
        } ਨਹੀਂਤਾਂ {
            fix_ਚੌੜਾਈ_Reduce(filterp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cover_Trans_ਸੈੱਟ* nodep) override { user_Iterate_And_ਅਗਲਾ(nodep->itemsp(), m_vup); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cover_Trans_ਵਸਤੂ* nodep) override {
        ਚੌੜਾਈ_Covergroup_Ranges(nodep->valuesp(), m_vup ? m_vup->dtypep()->ਚੌੜਾਈ() : 0);
    }
    ਖਾਲੀ ਦੇਖੋ(AstPow* nodep) override {
        // Pow is special, output sign only depends on LHS sign, but
        // function result depends on both signs
        // RHS is self-determined (IEEE)
        // Real if either side is real (as with AstAdd)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            ਜੇ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                VL_DO_DANGLING(replaceWithDVersion(nodep), nodep);
                ਵਾਪਸ;
            }
            checkCvtUS(nodep->lhsp(), ਝੂਠ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ਜੇ (!nodep->isFourstateOrNoDtype()
                && (nodep->rhsp()->isFourstateOrNoDtype()
                    || (nodep->rhsp()->isSigned() && !nodep->lhsp()->isNeqZero()
                        && !(VN_IS(nodep->rhsp(), Const)
                             && !VN_AS(nodep->rhsp(), Const)->ਅੰਕ().isNegative())))) {
                // Change dtype to four-state if it may be 0 ** negative
                nodep->dtype_ਸੈੱਟ_Logic_Unsized(nodep->ਚੌੜਾਈ(), nodep->ਚੌੜਾਈ_Min(),
                                            nodep->dtypep()->numeric());
            }
        }

        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ਗੰਢ_DType* ਅਚਲ lhsDTypep = nodep->lhsp()->dtypep()->skipRefp();
            expDTypep
                = expDTypep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(expDTypep->ਚੌੜਾਈ(), expDTypep->ਚੌੜਾਈ_Min(),
                                                 lhsDTypep->numeric(), expDTypep->isFourstate());
            nodep->dtypep(expDTypep);
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep, EXTEND_EXP);
            Ast_ਗੰਢ* newp = ਖਾਲੀ_ਸੂਚਕ;  // No change
            ਜੇ (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ਨਵਾਂ AstPowSS{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                    nodep->rhsp()->unlink_Fr_ਪਿੱਠ()};
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->lhsp()->isSigned() && !nodep->rhsp()->isSigned()) {
                newp = ਨਵਾਂ AstPowSU{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                    nodep->rhsp()->unlink_Fr_ਪਿੱਠ()};
            } ਨਹੀਂਤਾਂ ਜੇ (!nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned()) {
                newp = ਨਵਾਂ AstPowUS{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                    nodep->rhsp()->unlink_Fr_ਪਿੱਠ()};
            }
            ਜੇ (newp) {
                newp->dtypeFrom(nodep);
                UINFO(9, "powOld " << nodep);
                UINFO(9, "powNew " << newp);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPowSU* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstPowSS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstPowUS* nodep) override {
        // POWSU/SS/US only created here, dtype already determined, so
        // nothing to do in this function
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_ਬਿੱਟ* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "THS", nodep->thsp(), SELF, BOTH);
            iterateCheckSizedSelf(nodep, "FHS", nodep->fhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = V3_ਸੰਖਿਆ::log2b(nodep->lhsp()->ਚੌੜਾਈ()) + 1;
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, ਚੌੜਾਈ_Min, VSigning::SIGNED);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗਿਣਤੀ_Ones* nodep) override {
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            // For widthMin, if a 32 bit number, we need a 6 bit number as we need to return '32'.
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ_Min = V3_ਸੰਖਿਆ::log2b(nodep->lhsp()->ਚੌੜਾਈ()) + 1;
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, ਚੌੜਾਈ_Min, VSigning::SIGNED);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCvtPackString* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH, current_ਧਾਰਾ_Use()}.p());
        // Type set in constructor
        // A class handle is not a legal implicit conversion to string
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dt = nodep->lhsp()->dtypep()) {
            ਜੇ (VN_IS(dt->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), ClassRefDType)) {
                nodep->lhsp()->v3error(
                    "Cannot convert 'class' handle to a string:" << dt->prettyDTypeNameQ());
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH, current_ਧਾਰਾ_Use()}.p());
        // Type set in constructor
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH, current_ਧਾਰਾ_Use()}.p());
        // Type set in constructor
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH, current_ਧਾਰਾ_Use()}.p());
        // Type set in constructor
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // Opaque returns, so arbitrary
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // Type set in constructor
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack* nodep) override {
        // Type set when created, the value converted is of its own type
        ਜੇ (m_vup->final()) {
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{nodep->fromp()->dtypep(), FINAL}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Import* nodep) override {
        // LHS is a real number in seconds
        // Need to round to time units and precision
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->lhsp(), Const);
        UASSERT_OBJ(constp && constp->isDouble(), nodep, "Times should be doubles");
        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep, "$time import no units");
        ਅਚਲ ਦੁੱਗਣਾ ਸਮਾਂ_Prescale = constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Double();
        UASSERT_OBJ(!v3Global.rootp()->timeprecision().isNone(), nodep, "Never set precision?");
        ਅਚਲ ਦੁੱਗਣਾ ਸਮਾਂ_2 = ਸਮਾਂ_Prescale / nodep->timeunit().multiplier();
        // IEEE claims you should round to time precision here, but no simulator seems to do this
        AstConst* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::RealDouble{}, ਸਮਾਂ_2};
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstEventControl* nodep) override {
        ਜੇ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Event controls are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->fileline()->timingOn()) {
            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                iterateChildren(nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                nodep->v3warn(E_NOTIMING,
                              "Event control statement in this location requires --timing\n"
                                  << nodep->warn_ਹੋਰ()
                                  << "... With --no-timing, suggest have one event control "
                                  << "statement per procedure, at the top of the procedure");
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               "event controls should be handled");
            }
        }
        ਜੇ (nodep->stmtsp()) nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(nodep->stmtsp()->unlink_Fr_ਪਿੱਠ());
        VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
    }
    // Name of the class a class extends, for $typename, as other simulators give it. Only the
    // class it directly extends, as $typename of that class names the classes that one extends.
    ਸਥਿਰ string typenameExtends(ਅਚਲ AstClass* classp) {
        // An interface class may extend several, so those are not named
        ਜੇ (!classp || classp->isInterfaceClass()) ਵਾਪਸ "";
        ਲਈ (ਅਚਲ AstClassExtends* extendsp = classp->extendsp(); extendsp;
             extendsp = VN_AS(extendsp->nextp(), ClassExtends)) {
            ਜੇ (extendsp->isImplements()) ਜਾਰੀ;
            ਅਚਲ AstClass* ਅਚਲ basep = extendsp->classOrNullp();
            ਵਾਪਸ basep ? " extends class " + basep->dtypeName(ਸੱਚ) : "";
        }
        ਵਾਪਸ "";
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        VL_RESTORER(m_attrp);
        m_attrp = nodep;
        user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (nodep->dimp()) user_Iterate_And_ਅਗਲਾ(nodep->dimp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // Don't iterate children, don't want to lose VarRef.
        ਚੋਣ (nodep->attrType()) {
        ਹਾਲਤ VAttrType::DIM_DIMENSIONS:
        ਹਾਲਤ VAttrType::DIM_UNPK_DIMENSIONS: {
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            ਅਚਲ std::pair<uint32_t, uint32_t> dim = dtypep->dimensions(ਸੱਚ);
            ਅਚਲ ਪੂਰਨ val
                = (nodep->attrType() == VAttrType::DIM_UNPK_DIMENSIONS ? dim.ਦੂਜਾ
                                                                       : (dim.ਪਹਿਲਾ + dim.ਦੂਜਾ));
            nodep->replaceWith(ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{}, val));
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਰੋਕੋ;
        }
        ਹਾਲਤ VAttrType::DIM_ਬਿੱਟ_OR_ਸੰਖਿਆ: {
            // If dtype, compute DIM_BITS, else take expression as a number to use
            ਜੇ (VN_IS(nodep->fromp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                nodep->replaceWith(nodep->fromp()->unlink_Fr_ਪਿੱਠ());
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            } ਨਹੀਂਤਾਂ {
                Ast_ਗੰਢ* newp = ਨਵਾਂ Ast_Attr_ਦਾ{nodep->fileline(), VAttrType::DIM_ਬਿੱਟ,
                                              nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                user_Iterate_And_ਅਗਲਾ(newp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());  // Convert AttrOf
            }
            ਵਾਪਸ;
        }
        ਹਾਲਤ VAttrType::DIM_ਬਿੱਟ:
        ਹਾਲਤ VAttrType::DIM_ਉੱਚਾ:
        ਹਾਲਤ VAttrType::DIM_INCREMENT:
        ਹਾਲਤ VAttrType::DIM_ਖੱਬਾ:
        ਹਾਲਤ VAttrType::DIM_ਨੀਵਾਂ:
        ਹਾਲਤ VAttrType::DIM_ਸੱਜਾ:
        ਹਾਲਤ VAttrType::DIM_ਮਾਪ: {
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = fromDTypep(nodep->fromp());
            UASSERT_OBJ(dtypep, nodep, "Unsized expression");
            // Only worth asking while parameters are still being worked out.
            ਜੇ (m_paramsOnly) {
                // A module that is still being copied does not have its final sizes.
                ਅਚਲ Ast_ਗੰਢ_Module* ਅਚਲ ownModp = v3Global.rootp()->containingModule(dtypep);
                ਜੇ (ownModp && ownModp->parameterizedTemplate() && !ownModp->dead()) {
                    UINFO(9, "size deferred, type still on template " << ownModp->name());
                    // These queries always give an int, so set that now and let the
                    // value be worked out once the copy exists.
                    nodep->dtype_ਸੈੱਟ_Int();
                    ਵਾਪਸ;
                }
            }

            ਜੇ (VN_IS(dtypep, ਕਤਾਰ_DType) || VN_IS(dtypep, DynArrayDType)) {
                ਚੋਣ (nodep->attrType()) {
                ਹਾਲਤ VAttrType::DIM_ਮਾਪ: {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                    Ast_ਗੰਢ* ਅਚਲ newp
                        = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_ਮਾਪ};
                    newp->dtype_ਸੈੱਟ_Int();
                    newp->did_ਚੌੜਾਈ(ਸੱਚ);
                    newp->protect(ਝੂਠ);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ VAttrType::DIM_ਖੱਬਾ:
                ਹਾਲਤ VAttrType::DIM_ਨੀਵਾਂ: {
                    Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{}, 0);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ VAttrType::DIM_ਸੱਜਾ:
                ਹਾਲਤ VAttrType::DIM_ਉੱਚਾ: {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sizep
                        = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, VCMethod::DYN_ਮਾਪ};
                    sizep->dtype_ਸੈੱਟ_Int();
                    sizep->did_ਚੌੜਾਈ(ਸੱਚ);
                    sizep->protect(ਝੂਠ);
                    Ast_ਗੰਢ* ਅਚਲ newp
                        = ਨਵਾਂ AstSub{nodep->fileline(), sizep,
                                     ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{}, 1)};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ VAttrType::DIM_INCREMENT: {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                        = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{}, -1);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਰੋਕੋ;
                }
                ਹਾਲਤ VAttrType::DIM_ਬਿੱਟ: {
                    ਜੇ (VN_IS(dtypep, DynArrayDType)) {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for dynamic array");
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3warn(E_UNSUPPORTED, "Unsupported: $bits for queue");
                    }
                    ਰੋਕੋ;
                }
                ਮੂਲ: nodep->v3fatalSrc("Unhandled attribute type");
                }
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ std::pair<uint32_t, uint32_t> dimpair = dtypep->skipRefp()->dimensions(ਸੱਚ);
                ਅਚਲ uint32_t msbdim = dimpair.ਪਹਿਲਾ + dimpair.ਦੂਜਾ;
                ਜੇ (!nodep->dimp() || msbdim < 1) {
                    ਜੇ (VN_IS(dtypep, BasicDType) && dtypep->basicp()->isString()) {
                        // IEEE undocumented but $bits(string) must give length(string) * 8
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਗੰਢ_ਵਿਅੰਜਕ);
                        Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                            nodep->fileline(), ਨਵਾਂ AstLenN{nodep->fileline(), fromp},
                            ਨਵਾਂ AstConst{nodep->fileline(), 3},  // * 8
                            32};
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    } ਨਹੀਂਤਾਂ {
                        Ast_ਗੰਢ_DType* ਅਚਲ ਅਧਾਰ_DTypep = dtypep->skipRefp();
                        UASSERT_OBJ(ਅਧਾਰ_DTypep, nodep, "Unsized expression");
                        ਅਚਲ ਪੂਰਨ dim = 1;
                        AstConst* ਅਚਲ newp = dimension_ਮੁੱਲ(nodep->fileline(), ਅਧਾਰ_DTypep,
                                                              nodep->attrType(), dim);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep->dimp(), Const)) {
                    ਅਚਲ ਪੂਰਨ dim = VN_AS(nodep->dimp(), Const)->ਮੰਜ਼ਿਲ_SInt();
                    AstConst* ਅਚਲ newp
                        = dimension_ਮੁੱਲ(nodep->fileline(), dtypep, nodep->attrType(), dim);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                } ਨਹੀਂਤਾਂ {  // Need a runtime lookup table.  Yuk.
                    UASSERT_OBJ(nodep->fromp() && dtypep, nodep, "Unsized expression");
                    AstVarRef* ਅਚਲ tabRefp = dimensionVarRefp(dtypep, nodep->attrType(), msbdim);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dimp = nodep->dimp()->unlink_Fr_ਪਿੱਠ();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstArraySel{nodep->fileline(), tabRefp, dimp};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                }
            }
            ਰੋਕੋ;
        }
        ਹਾਲਤ VAttrType::FUNC_ARG_PROTO:
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            // Checked at bottom of visit(AstNodeFTask)
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ਰੋਕੋ;
        ਹਾਲਤ VAttrType::FUNC_RETURN_PROTO: {
            // Created by V3LinkDot only to check that the prototype was correct when we got here
            UASSERT_OBJ(m_ftaskp, nodep, "FUNC attr not under function");
            Ast_ਗੰਢ_DType* ਅਚਲ protoDtp = nodep->fromp()->dtypep();
            Ast_ਗੰਢ_DType* ਅਚਲ declDtp = [&]() {
                ਜੇ (m_ftaskp->fvarp()) ਵਾਪਸ m_ftaskp->fvarp()->dtypep();
                Ast_ਗੰਢ_DType* ਅਚਲ voidp = ਨਵਾਂ AstVoidDType{m_ftaskp->fileline()};
                ਧੱਕੋ_Deletep(voidp);  // Note voidp used past here
                ਵਾਪਸ voidp;
            }();
            ਜੇ (!similarDTypeRecurse(protoDtp, declDtp)) {
                protoDtp->v3warn(
                    PROTOTYPEMIS,
                    "In prototype for "
                        << m_ftaskp->prettyNameQ()
                        << ", return data type does not match out-of-block"
                           " declaration data-type (IEEE 1800-2023 8.24)\n"
                        << protoDtp->warn_ਹੋਰ()
                        << "... Prototype data type: " << protoDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warn_ਹੋਰ()
                        << "... Declaration data type: " << declDtp->prettyDTypeNameQ() << '\n'
                        << protoDtp->warnContextPrimary() << '\n'
                        << declDtp->warnOther() << "... Location of out-of-block declaration\n"
                        << declDtp->warnContextSecondary());
            }
            // V3WidthCommit::visit(AstAttrOf) will delete nodep
            ਰੋਕੋ;
        }
        ਹਾਲਤ VAttrType::TYPENAME: {
            UASSERT_OBJ(nodep->fromp(), nodep, "Unprovided expression");
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->fromp()->dtypep();
            string ਨਤੀਜਾ = dtypep->prettyDTypeName(ਸੱਚ);
            ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ classRefp
                = VN_CAST(dtypep->skipRefOrNullp(), ClassRefDType)) {
                ਨਤੀਜਾ += typenameExtends(classRefp->classp());
            }
            UINFO(9, "typename '" << ਨਤੀਜਾ << "' from " << dtypep);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, ਨਤੀਜਾ};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            ਰੋਕੋ;
        }
        ਹਾਲਤ VAttrType::TYPEID:
            ਜੇ (Ast_ਗੰਢ_DType* dtypep = VN_CAST(nodep->fromp(), ਗੰਢ_DType)) {
                nodep->dtypep(dtypep);
            } ਨਹੀਂਤਾਂ {
                nodep->dtypep(VN_AS(nodep->fromp(), ਗੰਢ_ਵਿਅੰਜਕ)->dtypep());
            }
            ਰੋਕੋ;
        ਹਾਲਤ VAttrType::VAR_ਅਧਾਰ:
            // Soon to be handled in V3LinkWidth SEL generation, under attrp() and newSubLsbOf
            ਰੋਕੋ;
        ਮੂਲ: {
            // Everything else resolved earlier
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, 1, VSigning::UNSIGNED);  // Approximation, unsized 32
            UINFO(1, "Missing ATTR type case node: " << nodep);
            nodep->v3fatalSrc("Missing ATTR type case");
            ਰੋਕੋ;
        }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstPull* nodep) override {
        // May have select underneath, let seek natural size
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ* nodep) override {
        // Only used in CStmts which don't care....
    }

    ਖਾਲੀ ਦੇਖੋ(AstCellArrayRef* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        user_Iterate_And_ਅਗਲਾ(nodep->selp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->selp(), ਚੌੜਾਈ_VP{SELF, FINAL}.p());
        nodep->dtype_ਸੈੱਟ_Void();  // placeholder; this node shouldnt survive beyond linking
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCellRef* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;

        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cellExprp = VN_CAST(nodep->cellp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            user_Iterate_And_ਅਗਲਾ(cellExprp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(cellExprp, ਚੌੜਾਈ_VP{SELF, FINAL}.p());
        }

        ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
            user_Iterate_And_ਅਗਲਾ(exprp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            nodep->dtypeFrom(exprp);
            user_Iterate_And_ਅਗਲਾ(exprp, ਚੌੜਾਈ_VP{SELF, FINAL}.p());
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("CellRef exprp is not a NodeExpr: "  // LCOV_EXCL_LINE
                              << nodep->exprp()->prettyTypeName());
        }

        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
    }

    // DTYPES
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Array_DType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed

        ਜੇ (nodep->subDTypep() == nodep->basicp()) {  // Innermost dimension
            AstBasicDType* ਅਚਲ basicp = nodep->basicp();
            // If basic dtype is LOGIC_IMPLICIT, it is actually 1 bit LOGIC
            ਜੇ (basicp->implicit()) {
                UASSERT_OBJ(basicp->ਚੌੜਾਈ() <= 1, basicp,
                            "must be 1 bit but actually " << basicp->ਚੌੜਾਈ() << " bits");
                AstBasicDType* ਅਚਲ newp = ਨਵਾਂ AstBasicDType{
                    basicp->fileline(), VBasicDTypeKwd::LOGIC, basicp->numeric()};
                newp->ਚੌੜਾਈ_Force(1, 1);
                basicp->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(basicp), basicp);
            }
        }
        VL_RESTORER(m_underPackedArray);
        ਜੇ (VN_IS(nodep, PackArrayDType)) m_underPackedArray = ਸੱਚ;
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));

        // Cleanup array size
        user_Iterate_And_ਅਗਲਾ(nodep->rangep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        nodep->dtypep(nodep);  // The array itself, not subDtype
        ਜੇ (AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(nodep, UnpackArrayDType)) {
            // Historically array elements have width of the ref type not the full array
            nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
            ਜੇ (nodep->subDTypep()->skipRefp()->isCompound()) adtypep->isCompound(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ elemDTypep = nodep->subDTypep()->skipRefp();
            ਜੇ (!elemDTypep->isIntegralOrPacked()) {
                nodep->v3error("Unpacked data type " << elemDTypep->prettyDTypeNameQ()
                                                     << " in packed array (IEEE 1800-2023 7.4.1)");
            }
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->subDTypep()->ਚੌੜਾਈ() * nodep->rangep()->elementsConst();
            nodep->ਚੌੜਾਈ_Force(ਚੌੜਾਈ, ਚੌੜਾਈ);
            ਜੇ (!VL_RESTORER_PREV(m_underPackedArray)) {  // Outermost dimension
                // IEEE 1800-2023 7.4.1 "Packed arrays" says
                //   If a packed array is declared as signed,
                //   then the array viewed as a single vector shall be signed.
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਧਾਰ_DTypep = nodep->subDTypep();
                ਜਦੋਂ (!VN_IS(ਅਧਾਰ_DTypep, RefDType) && ਅਧਾਰ_DTypep->subDTypep()) {
                    ਅਧਾਰ_DTypep = ਅਧਾਰ_DTypep->subDTypep();
                }
                ਅਚਲ AstBasicDType* ਅਚਲ basicp = nodep->basicp();
                ਜੇ (!VN_IS(ਅਧਾਰ_DTypep, RefDType) && basicp && basicp->isSigned()) {
                    nodep->numeric(VSigning::fromBool(ਸੱਚ));
                }
            }
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocArrayDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ਕੁੰਜੀ_DTypep(iterateEditMoveDTypep(nodep, nodep->ਕੁੰਜੀ_DTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBracketArrayDType* nodep) override {
        // Type inserted only because parser didn't know elementsp() type
        // Resolve elementsp's type
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // We must edit when dtype still under normal nodes and before type table
        // See notes in iterateEditMoveDTypep
        Ast_ਗੰਢ_DType* ਅਚਲ childp = nodep->ਬੱਚਾ_DTypep();
        childp->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ* ਅਚਲ elementsp = nodep->elementsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ* newp;
        ਜੇ (VN_IS(elementsp, Unbounded)) {
            newp = ਨਵਾਂ Ast_ਕਤਾਰ_DType{nodep->fileline(), VFlag_ਬੱਚਾ_DType{}, childp, ਖਾਲੀ_ਸੂਚਕ};
            VL_DO_DANGLING(elementsp->delete_ਰੁੱਖ(), elementsp);
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ keyp = VN_CAST(elementsp, ਗੰਢ_DType)) {
            newp = ਨਵਾਂ AstAssocArrayDType{nodep->fileline(), VFlag_ਬੱਚਾ_DType{}, childp, keyp};
        } ਨਹੀਂਤਾਂ {
            // The subtract in the range may confuse users; as the array
            // size is self determined there's no reason to warn about widths
            File_ਸਤਰ* ਅਚਲ elementsNewFl = elementsp->fileline();
            elementsNewFl->warnOff(V3_Error_ਕੋਡ::WIDTHEXPAND, ਸੱਚ);
            // Must be expression that is constant, but we'll determine that later
            newp = ਨਵਾਂ AstUnpackArrayDType{
                nodep->fileline(), VFlag_ਬੱਚਾ_DType{}, childp,
                ਨਵਾਂ AstRange{nodep->fileline(), ਨਵਾਂ AstConst(elementsp->fileline(), 0),
                             ਨਵਾਂ AstSub{elementsNewFl, VN_AS(elementsp, ਗੰਢ_ਵਿਅੰਜਕ),
                                        ਨਵਾਂ AstConst(elementsNewFl, 1)},
                             ਸੱਚ}};
        }
        nodep->replaceWith(newp);
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        // Normally parent's iteration would cover this, but we might have entered by a specific
        // visit
        VL_DO_DANGLING(userIterate(newp, ਖਾਲੀ_ਸੂਚਕ), newp);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintRefDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDynArrayDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਕਤਾਰ_DType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);  // The array itself, not subDtype
        user_Iterate_And_ਅਗਲਾ(nodep->boundp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (VN_IS(nodep->boundp(), Unbounded)) {
            nodep->boundp()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();  // nullptr will represent unbounded
        }
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstVoidDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->dtypep(nodep);
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnsizedArrayDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        ਜੇ (nodep->ਬੱਚਾ_DTypep() && !(m_ftaskp && m_ftaskp->dpiImport())) {
            nodep->v3error("Unsized packed dimension '[]' is only legal in a DPI import"
                           " (IEEE 1800-2023 A.2.2.1)");
        }
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWildcardArrayDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        // Cleanup array size
        nodep->dtypep(nodep);  // The array itself, not subDtype
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        ਜੇ (nodep->generic()) ਵਾਪਸ;  // Already perfect
        ਜੇ (nodep->rangep()) {
            user_Iterate_And_ਅਗਲਾ(nodep->rangep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            // Because this DType has a unique child range, we know it's not
            // pointed at by other nodes unless they are referencing this type.
            // Furthermore the width() calculation would return identical
            // values.  Therefore we can directly replace the width
            nodep->ਚੌੜਾਈ_Force(nodep->rangep()->elementsConst(), nodep->rangep()->elementsConst());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->isRanged()) {
            nodep->ਚੌੜਾਈ_Force(nodep->nrange().elements(), nodep->nrange().elements());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->implicit()) {
            // Parameters may notice implicitness and change to different dtype
            nodep->ਚੌੜਾਈ_Force(1, 1);
        }
        // else width in node is correct; it was set based on keyword().width()
        // at construction time.  Ditto signed, so "unsigned byte" etc works right.
        nodep->cvtRangeConst();
        // TODO: If BasicDType now looks like a generic type, we can convert to a real generic
        // dtype Instead for now doing this in V3WidthCommit
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
        nodep->dtypep(nodep);  // Should already be set, but be clear it's not the subDType
        nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_ਚੌੜਾਈ(ਸੱਚ);
        ਜੇ (nodep->typeofp()) {  // type(typeofp_expression)
            ਜੇ (Ast_ਗੰਢ_DType* typeofDtp = VN_CAST(nodep->typeofp(), ਗੰਢ_DType)) {
                // It's directly a type, e.g. "type(int)"
                typeofDtp = iterateEditMoveDTypep(nodep, typeofDtp);  // Changes typeofp
                nodep->refDTypep(typeofDtp);
            } ਨਹੀਂਤਾਂ {
                // Type comes from expression's type, e.g. "type(variable)"
                user_Iterate_And_ਅਗਲਾ(nodep->typeofp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                Ast_ਗੰਢ* ਅਚਲ typeofp = nodep->typeofp();
                nodep->refDTypep(typeofp->dtypep());
                VL_DO_DANGLING(typeofp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), typeofp);
            }
            // We had to use AstRefDType for this construct as pointers to this type
            // in type table are still correct (which they wouldn't be if we replaced the node)
        }
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (nodep->subDTypep()) {
            // Normally iterateEditMoveDTypep iterate would work, but the refs are under
            // the TypeDef which will upset iterateEditMoveDTypep as it can't find it under
            // this node's childDTypep
            userIterate(nodep->subDTypep(), ਖਾਲੀ_ਸੂਚਕ);
            nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
            // Widths are resolved, but special iterate to check for recursion
            userIterate(nodep->subDTypep(), ਖਾਲੀ_ਸੂਚਕ);
        }
        // Effectively nodep->dtypeFrom(nodep->dtypeSkipRefp());
        // But might be recursive, so instead manually recurse into the referenced type
        ਜੇ (!nodep->subDTypep()) {
            // Defer unlinked RefDTypes in parameterized template modules (or types
            // with no owning module, e.g. moved to TypeTable) until specialization
            // resolves them.
            ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Template_Module = !m_modep || m_modep->parameterizedTemplate();
            ਜੇ (ਵਿੱਚ_Template_Module) {
                nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
                ਵਾਪਸ;
            }
        }
        UASSERT_OBJ(nodep->subDTypep(), nodep, "Unlinked");
        nodep->dtypeFrom(nodep->subDTypep());
        nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
        UINFO(4, "dtWidthed " << nodep);
        // No nodep->typedefp(nullptr) for now; V3WidthCommit needs to check accesses
        nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
    }
    // Width the parameters of the scopes declaring typedef 'nodep', as the name of the type it
    // declares holds their values, though one may follow it, in a module without a parameter
    // port list
    ਖਾਲੀ ਚੌੜਾਈ_DType_Name_Params(ਅਚਲ AstTypedef* nodep) {
        ਅਚਲ VDTypeNameScopes* ਅਚਲ scopesp = VDTypeNameScopes::currentp();
        ਜੇ (!scopesp) ਵਾਪਸ;  // Computing parameters, as by V3Param, so naming as found
        Ast_ਗੰਢ* scopep = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* innerp = nodep; scopesp->outerp(innerp, scopep) && scopep;
             innerp = scopep) {
            ਜੇ (ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ*>* ਅਚਲ paramsp = scopesp->paramsp(scopep)) {
                ਲਈ (Ast_ਗੰਢ* ਅਚਲ paramp : *paramsp) {
                    ਜੇ (!paramp->did_ਚੌੜਾਈ()) userIterate(paramp, ਖਾਲੀ_ਸੂਚਕ);
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(nodep, nodep)) {
            nodep->v3error("Typedef has self-reference: " << nodep->prettyNameQ() << '\n'
                                                          << nodep->warnContextPrimary() << '\n'
                                                          << refp->warnOther()
                                                          << "... Location of reference\n"
                                                          << refp->warnContextSecondary());
            // May cause internal error but avoids infinite loop on dump
            refp->typedefp(ਖਾਲੀ_ਸੂਚਕ);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
            ਵਾਪਸ;
        }
        // As it moves to the type table, a structure, union, or enumeration it declares keeps the
        // name it has from the typedef, with the scope of the typedef, so with the parameters of
        // the scope, widthed first, as one may follow the typedef
        ਅਚਲ ਤਾਰਕਿਕ declares = nodep->ਬੱਚਾ_DTypep() != ਖਾਲੀ_ਸੂਚਕ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ਜੇ (declares) {
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->dtypep();
            ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ sdtypep = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
                ਚੌੜਾਈ_DType_Name_Params(nodep);
                sdtypep->typedefName(nodep->dtypeName());
            } ਨਹੀਂਤਾਂ ਜੇ (AstEnumDType* ਅਚਲ edtypep = VN_CAST(dtypep, EnumDType)) {
                ਚੌੜਾਈ_DType_Name_Params(nodep);
                edtypep->typedefName(nodep->dtypeName());
            }
        }
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed

        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
        nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
        // Clear childDTypep after dtypep is set to satisfy V3Broken invariant.
        // The child dtype has been moved to the type table by iterateEditMoveDTypep.
        ਜੇ (nodep->dtypep() && nodep->ਬੱਚਾ_DTypep()) { nodep->ਬੱਚਾ_DTypep(ਖਾਲੀ_ਸੂਚਕ); }
    }
    ਖਾਲੀ ਦੇਖੋ(AstRequireDType* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ dtp = VN_CAST(nodep->lhsp(), ਗੰਢ_DType)) {
            nodep->replaceWith(dtp->unlink_Fr_ਪਿੱਠ());
        } ਨਹੀਂਤਾਂ {
            ਜੇ (nodep->lhsp())
                nodep->lhsp()->v3error("Expected data type, not a "
                                       << nodep->lhsp()->prettyTypeName());
            nodep->replaceWith(ਨਵਾਂ AstVoidDType{nodep->fileline()});
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastDynamic* nodep) override {
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ_DType* ਅਚਲ ਮੰਜ਼ਿਲ_Dtp = nodep->ਸਿਖਰ()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        ਅਚਲ ਸਵੈਚਾਲਿਤ castable = Ast_ਗੰਢ::computeCastable(ਮੰਜ਼ਿਲ_Dtp, fromDtp, nodep->fromp());
        Ast_ਗੰਢ* newp;
        ਜੇ (castable == VCastable::DYNAMIC_CLASS) {
            // Keep in place, will compute at runtime
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (castable == VCastable::ENUM_EXPLICIT || castable == VCastable::ENUM_IMPLICIT) {
            // TODO is from is a constant we could simplify, though normal constant
            // elimination should do much the same
            // Form: "( ((v > size) ? false : enum_valid[v[N:0]])
            //          ? ExprStmt(ExprAssign(out, Cast(v, type)), 1) : 0)"
            AstEnumDType* ਅਚਲ enumDtp = VN_AS(ਮੰਜ਼ਿਲ_Dtp, EnumDType);
            UASSERT_OBJ(enumDtp, nodep, "$cast determined as enum, but not enum type");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ testp = enumTestValid(nodep->fromp(), enumDtp);
            File_ਸਤਰ* ਅਚਲ fl_novalue = ਨਵਾਂ File_ਸਤਰ{fl};
            fl_novalue->warnOff(V3_Error_ਕੋਡ::ENUMVALUE, ਸੱਚ);
            newp = ਨਵਾਂ AstCond{
                fl, testp,
                ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{fl,
                                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl_novalue, nodep->ਸਿਖਰ()->unlink_Fr_ਪਿੱਠ(),
                                              nodep->fromp()->unlink_Fr_ਪਿੱਠ()},
                                ਨਵਾਂ AstConst{fl, AstConst::Signed32{}, 1}},
                ਨਵਾਂ AstConst{fl, AstConst::Signed32{}, 0}};
        } ਨਹੀਂਤਾਂ ਜੇ ((castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE)
                   // Unsupported: the static cast below has no lowering for unpacked arrays
                   && !VN_IS(ਮੰਜ਼ਿਲ_Dtp, UnpackArrayDType)) {
            nodep->v3warn(CASTCONST, "$cast will always return one as "
                                         << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ()
                                         << " is always castable from "
                                         << fromDtp->prettyDTypeNameQ() << '\n'
                                         << nodep->warn_ਹੋਰ() << "... Suggest static cast");
            newp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
                fl,
                ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, nodep->ਸਿਖਰ()->unlink_Fr_ਪਿੱਠ(),
                              ਨਵਾਂ AstCast{fl, nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਮੰਜ਼ਿਲ_Dtp}},
                ਨਵਾਂ AstConst{fl, AstConst::Signed32{}, 1}};
        } ਨਹੀਂਤਾਂ ਜੇ (castable == VCastable::INCOMPATIBLE) {
            newp = ਨਵਾਂ AstConst{fl, 0};
            nodep->v3warn(CASTCONST, "$cast will always return zero as "
                                         << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ() << " is not castable from "
                                         << fromDtp->prettyDTypeNameQ());
        } ਨਹੀਂਤਾਂ {
            newp = ਨਵਾਂ AstConst{fl, 0};
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: $cast to "
                                             << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ() << " from "
                                             << fromDtp->prettyDTypeNameQ() << '\n'
                                             << nodep->warn_ਹੋਰ()
                                             << "... Suggest try static cast");
        }
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        userIterate(newp, m_vup);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastParse* nodep) override {
        // nodep->dtp could be data type, or a primary_constant
        // Don't iterate lhsp, will deal with that once convert the type
        V3Const::constifyParamsEdit(nodep->dtp());  // itemp may change
        ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(nodep->dtp(), Const)) {
            constp->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ Ast_Cast_ਮਾਪ{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), constp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ refp = VN_CAST(nodep->dtp(), ਗੰਢ_DType)) {
            refp->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstCast{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                              VFlag_ਬੱਚਾ_DType{}, refp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Cast to " << nodep->dtp()->prettyTypeName());
            nodep->replaceWith(nodep->lhsp()->unlink_Fr_ਪਿੱਠ());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstCast* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        UINFO(9, "CAST " << nodep);
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        ਜੇ (m_vup->prelim()) {
            UINFOTREE(9, nodep, "", "CastPre");
            // UINFOTREE(1, nodep->backp(), "", "CastPreUpUp");
            ਜੇ (AstSigned* ਅਚਲ fromp = VN_CAST(nodep->fromp(), Signed)) {
                ਜੇ (VN_IS(fromp->lhsp(), ਗੰਢ_ਧਾਰਾ)) {
                    Ast_ਗੰਢ* ਅਚਲ lhsp = fromp->lhsp()->unlink_Fr_ਪਿੱਠ();
                    fromp->replaceWith(lhsp);
                    VL_DO_DANGLING(fromp->delete_ਰੁੱਖ(), fromp);
                }
            }
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, PRELIM, ਧਾਰਾ_USE_CAST}.p());
            UINFOTREE(9, nodep, "", "CastDit");
            Ast_ਗੰਢ_DType* ਅਚਲ ਮੰਜ਼ਿਲ_Dtp = nodep->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
            Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
            ਅਚਲ ਸਵੈਚਾਲਿਤ castable = Ast_ਗੰਢ::computeCastable(ਮੰਜ਼ਿਲ_Dtp, fromDtp, nodep->fromp());
            ਤਾਰਕਿਕ bad = ਝੂਠ;
            ਜੇ (castable == VCastable::UNSUPPORTED) {
                nodep->v3warn(E_UNSUPPORTED, "Unsupported: static cast to "
                                                 << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ() << " from "
                                                 << fromDtp->prettyDTypeNameQ());
                bad = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (castable == VCastable::SAMEISH || castable == VCastable::COMPATIBLE
                       || castable == VCastable::ENUM_IMPLICIT
                       || castable == VCastable::ENUM_EXPLICIT) {
                ;  // Continue
            } ਨਹੀਂਤਾਂ ਜੇ (castable == VCastable::DYNAMIC_CLASS) {
                nodep->v3error("Dynamic, not static cast, required to cast "
                               << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n'
                               << nodep->warn_ਹੋਰ() << "... Suggest dynamic $cast");
                bad = ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (castable == VCastable::INCOMPATIBLE) {
                nodep->v3error("Incompatible types to static cast to "
                               << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ() << " from "
                               << fromDtp->prettyDTypeNameQ() << '\n');
                bad = ਸੱਚ;
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("bad casting case");
            }
            // For now, replace it ASAP, so widthing can propagate easily
            // The cast may change signing, but we don't know the sign yet.  Make it so.
            // Note we don't sign fromp() that would make the algorithm O(n^2) if lots of casting.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (bad) {
            } ਨਹੀਂਤਾਂ ਜੇ (AstBasicDType* ਅਚਲ basicp = ਮੰਜ਼ਿਲ_Dtp->basicp()) {
                ਜੇ (!basicp->isString() && fromDtp->isString()) {
                    newp = ਨਵਾਂ AstNToI{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਮੰਜ਼ਿਲ_Dtp};
                } ਨਹੀਂਤਾਂ ਜੇ (!basicp->isDouble() && !fromDtp->isDouble()) {
                    Ast_ਗੰਢ_DType* ਅਚਲ origDTypep = nodep->dtypep();
                    ਜੇ (!VN_IS(fromDtp, ਧਾਰਾ_DType)) {
                        ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = ਮੰਜ਼ਿਲ_Dtp->ਚੌੜਾਈ();
                        castSized(nodep, nodep->fromp(), ਚੌੜਾਈ);
                    }
                    nodep->dtypeFrom(origDTypep);  // If was enum, need dtype to preserve as enum
                    // Note castSized might modify nodep->fromp()
                } ਨਹੀਂਤਾਂ {
                    iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, fromDtp, EXTEND_EXP,
                                 ਝੂਠ);
                }
                ਜੇ (newp) {
                } ਨਹੀਂਤਾਂ ਜੇ (basicp->isDouble() && !nodep->fromp()->isDouble()) {
                    ਜੇ (nodep->fromp()->isSigned()) {
                        newp = ਨਵਾਂ AstISToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                    } ਨਹੀਂਤਾਂ {
                        newp = ਨਵਾਂ AstIToRD{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (!basicp->isDouble() && nodep->fromp()->isDouble()) {
                    newp = ਨਵਾਂ AstRToIRoundS{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                    newp->dtypep(basicp);
                } ਨਹੀਂਤਾਂ ਜੇ (basicp->isSigned() && !nodep->fromp()->isSigned()) {
                    newp = ਨਵਾਂ AstSigned{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                } ਨਹੀਂਤਾਂ ਜੇ (!basicp->isSigned() && nodep->fromp()->isSigned()) {
                    newp = ਨਵਾਂ AstUnsigned{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
                } ਨਹੀਂਤਾਂ {
                    // Can just remove cast, but need extend placeholder
                    // so we can avoid warning message
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_Dtp, ਕਤਾਰ_DType)) {
                ਜੇ (VN_IS(fromDtp, BasicDType)) {
                    newp = ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{nodep->fileline(),
                                                   nodep->fromp()->unlink_Fr_ਪਿੱਠ(), ਮੰਜ਼ਿਲ_Dtp};
                } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromDtp, ਕਤਾਰ_DType) || VN_IS(fromDtp, ਧਾਰਾ_DType)) {
                    ਪੂਰਨ src_Element_ਬਿੱਟ = 1;
                    ਪੂਰਨ dst_Element_ਬਿੱਟ = 1;
                    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = fromDtp->subDTypep()) {
                        src_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ dst_ਕਤਾਰ_Dtp = VN_AS(ਮੰਜ਼ਿਲ_Dtp, ਕਤਾਰ_DType);
                    ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ elemDtp = dst_ਕਤਾਰ_Dtp->subDTypep()) {
                        dst_Element_ਬਿੱਟ = elemDtp->ਚੌੜਾਈ();
                    }
                    newp = ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Array{nodep->fileline(),
                                                  nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                                  ਮੰਜ਼ਿਲ_Dtp,
                                                  ਝੂਠ,
                                                  1,
                                                  dst_Element_ਬਿੱਟ,
                                                  src_Element_ਬਿੱਟ};
                }
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਮੰਜ਼ਿਲ_Dtp, ClassRefDType)) {
                // Can just remove cast
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Unimplemented: Casting non-simple data type "
                                  << ਮੰਜ਼ਿਲ_Dtp->prettyDTypeNameQ());
            }
            ਜੇ (!newp) newp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            nodep->fromp(newp);
            UINFOTREE(9, nodep, "", "CastOut");
            // UINFOTREE(1, nodep->backp(), "", "CastOutUpUp");
        }
        ਜੇ (m_vup->final()) {
            UINFOTREE(9, nodep, "", "CastFPit");
            iterateCheck(nodep, "value", nodep->fromp(), SELF, FINAL, nodep->fromp()->dtypep(),
                         EXTEND_EXP, ਝੂਠ, ਧਾਰਾ_USE_CAST);
            UINFOTREE(9, nodep, "", "CastFin");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ underp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            underp->dtypeFrom(nodep);
            underp->did_ਚੌੜਾਈ(ਸੱਚ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstCastWrap{nodep->fileline(), underp};
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            UINFOTREE(9, newp, "", "CastRep");
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Cast_ਮਾਪ* nodep) override {
        // IEEE: Signedness of result is same as self-determined signedness
        // However, the result is same as BITSEL, so we do not sign extend the LHS
        // UINFOTREE(1, nodep, "", "CastSizePre");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            ਪੂਰਨ ਚੌੜਾਈ = nodep->rhsp()->ਮੰਜ਼ਿਲ_SInt();
            ਜੇ (ਚੌੜਾਈ < 1) {
                nodep->v3error("Size-changing cast to zero or negative size: " << ਚੌੜਾਈ);
                ਚੌੜਾਈ = 1;
            }
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM, ਧਾਰਾ_USE_CAST}.p());
            castSized(nodep, nodep->lhsp(), ਚੌੜਾਈ);  // lhsp may change
        }
        ਜੇ (m_vup->final()) {
            // CastSize not needed once sizes determined
            Ast_ਗੰਢ* ਅਚਲ underp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
            nodep->replaceWithKeepDType(underp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
        // UINFOTREE(1, nodep, "", "CastSizeOut");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCastWrap* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{nodep->dtypep(), BOTH, ਧਾਰਾ_USE_CAST}.p());
    }
    ਖਾਲੀ castSized(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* underp, ਪੂਰਨ ਚੌੜਾਈ) {
        ਅਚਲ AstBasicDType* underDtp = VN_CAST(underp->dtypep(), BasicDType);
        ਜੇ (!underDtp) underDtp = underp->dtypep()->basicp();
        ਜੇ (!underDtp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Size-changing cast on non-basic data type");
            underDtp = VN_AS(nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(), BasicDType);
        }
        UASSERT_OBJ(underp == nodep->op1p(), nodep, "Assuming op1 is cast value");
        // A cast propagates its size to the lower expression and is included in the maximum
        // width, so 23'(1'b1 + 1'b1) uses 23-bit math, but 1'(2'h2 * 2'h1) uses two-bit math.
        // However the output width is exactly that requested.
        // So two steps, first do the calculation's width (max of the two widths)
        {
            ਅਚਲ ਪੂਰਨ calc_ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, underDtp->ਚੌੜਾਈ());
            Ast_ਗੰਢ_DType* ਅਚਲ calcDtp = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                calc_ਚੌੜਾਈ, calc_ਚੌੜਾਈ, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(calcDtp);
            // We ignore warnings as that is sort of the point of a cast
            iterateCheck(nodep, "Cast expr", underp, CONTEXT_DET, FINAL, calcDtp, EXTEND_EXP,
                         ਝੂਠ, ਧਾਰਾ_USE_CAST);
            VL_DANGLING(underp);
            underp = nodep->op1p();  // Above asserts that op1 was underp pre-relink
        }
        // UINFOTREE(1, nodep, "", "CastSizeClc");
        // Next step, make the proper output width
        {
            Ast_ਗੰਢ_DType* ਅਚਲ ਬਾਹਰਲਾ_Dtp = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                ਚੌੜਾਈ, ਚੌੜਾਈ, underDtp->numeric(), underDtp->isFourstate());
            nodep->dtypep(ਬਾਹਰਲਾ_Dtp);
            // We ignore warnings as that is sort of the point of a cast
            ਚੌੜਾਈ_Check_Sized(nodep, "Cast expr", VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ), ਬਾਹਰਲਾ_Dtp, EXTEND_EXP,
                            ਝੂਠ);
            VL_DANGLING(underp);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintBefore* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->lhssp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->rhssp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Constraint_ਵਿਅੰਜਕ* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintUnique* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->rangesp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Dist_ਵਸਤੂ* nodep) override {
        userIterate(nodep->rangep(), m_vup);
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            userIterate(nodep->weightp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
        nodep->dtypep(nodep->rangep()->dtypep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        // UINFOTREE(1, nodep, "", "InitPre");
        // Must have deterministic constant width
        // We can't skip this step when width()!=0, as creating a AstVar
        // with non-constant range gets size 1, not size 0.  So use didWidth().
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        ਜੇ (nodep->doing_ਚੌੜਾਈ()) {  // Early exit if have circular parameter definition
            UASSERT_OBJ(nodep->valuep(), nodep, "circular, but without value");
            nodep->v3error("Variable's initial value is circular: " << nodep->prettyNameQ());
            ਧੱਕੋ_Deletep(nodep->valuep()->unlink_Fr_ਪਿੱਠ());
            nodep->valuep(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
            nodep->dtypeFrom(nodep->valuep());
            nodep->did_ਚੌੜਾਈ(ਸੱਚ);
            ਵਾਪਸ;
        }
        nodep->doing_ਚੌੜਾਈ(ਸੱਚ);
        // Make sure dtype is sized
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        UASSERT_OBJ(nodep->dtypep(), nodep, "No dtype determined for var");
        // Removed at the end, see mainAcceptEdit
        ਜੇ (!m_paramsOnly && nodep->is_Iface_Array_ਵੰਡੋ()) m_ਵੰਡੋ_Iface_Array_Varps.ਧੱਕੋ_ਪਿੱਠ(nodep);
        ਜੇ (nodep->attrsp()) {
            nodep->attrsp()->foreach([ਇਹ, nodep](Ast_Attr_ਦਾ* attrp) {
                ਜੇ (attrp->attrType() == VAttrType::VAR_ਪੋਰਟ_DTYPE) {
                    V3Const::constifyParamsEdit(attrp->fromp());  // fromp may change
                    ਜੇ (!similarDTypeRecurse(nodep->dtypep(), VN_AS(attrp->fromp(), ਗੰਢ_DType))) {
                        nodep->dtypep()->v3error("Non-ANSI I/O declaration of signal "
                                                 "conflicts with type declaration: "
                                                 << nodep->prettyNameQ() << '\n'
                                                 << nodep->dtypep()->warnContextPrimary() << '\n'
                                                 << attrp->warnOther()
                                                 << "... Location of other declaration\n"
                                                 << attrp->warnContextSecondary());
                    }
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(attrp->unlink_Fr_ਪਿੱਠ()), attrp);
                }
            });
        }
        ਜੇ (m_ftaskp && m_ftaskp->dpiImport()) {
            Ast_ਗੰਢ_DType* dtp = nodep->dtypep();
            Ast_ਗੰਢ_DType* np = ਖਾਲੀ_ਸੂਚਕ;
            ਜਦੋਂ (VN_IS(dtp->skipRefp(), DynArrayDType)
                   || VN_IS(dtp->skipRefp(), UnpackArrayDType)) {
                ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ unsizedp
                    = VN_CAST(dtp->skipRefp(), DynArrayDType)) {
                    ਜੇ (!np) {
                        UINFO(9, "Dynamic becomes unsized array (var itself) " << nodep);
                    } ਨਹੀਂਤਾਂ {
                        UINFO(9, "Dynamic becomes unsized array (subDType) " << dtp);
                    }
                    AstUnsizedArrayDType* ਅਚਲ newp
                        = ਨਵਾਂ AstUnsizedArrayDType{unsizedp->fileline(), unsizedp->subDTypep()};
                    newp->dtypep(newp);
                    ਜੇ (!np) {  // for Var itself
                        nodep->dtypep(newp);
                    } ਨਹੀਂਤਾਂ {  // for subDType
                        np->virtRefDTypep(newp);
                    }
                    v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newp);
                    np = newp;
                } ਨਹੀਂਤਾਂ {
                    np = dtp->skipRefp();
                }
                dtp = np->virtRefDTypep();
            }
        }
        ਜੇ (AstWildcardArrayDType* ਅਚਲ wildp
            = VN_CAST(nodep->dtypeSkipRefp(), WildcardArrayDType)) {
            nodep->dtypep(wildp);  // Skip RefDType like for other dynamic array types
        }
        ਜੇ (VN_IS(nodep->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Constp(), ConstDType)) nodep->isConst(ਸੱਚ);
        // Parameters if implicit untyped inherit from what they are assigned to
        ਅਚਲ AstBasicDType* ਅਚਲ bdtypep = VN_CAST(nodep->dtypep(), BasicDType);
        ਤਾਰਕਿਕ didchk = ਝੂਠ;
        ਅਚਲ ਤਾਰਕਿਕ implicitParam = nodep->isParam() && bdtypep && bdtypep->implicit();
        ਜੇ (implicitParam) {
            ਜੇ (nodep->valuep()) {
                // Remove blanket deferral. We must attempt to visit the value to determine
                // type/deps. If it remains unresolved, specific node visitors (like AttrOf) should
                // handle deferral by setting a placeholder type to prevent "No dtype" errors
                // later.
                user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
                UINFO(9, "implicitParamPRELIMIV " << nodep->valuep());
                // Although nodep will get a different width for parameters
                // just below, we want the init numbers to retain their
                // width/minwidth until parameters are replaced.
                // This prevents width warnings at the location the parameter is substituted in
                ਜੇ (nodep->valuep()->isDouble()) {
                    nodep->dtype_ਸੈੱਟ_Double();
                    VL_DANGLING(bdtypep);
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->valuep()->isString()) {
                    nodep->dtype_ਸੈੱਟ_String();
                    VL_DANGLING(bdtypep);
                } ਨਹੀਂਤਾਂ {
                    ਪੂਰਨ ਚੌੜਾਈ = 0;
                    Ast_ਗੰਢ_DType* ਅਚਲ ਮੁੱਲ_DTypep = nodep->valuep()->dtypep();
                    UASSERT_OBJ(ਮੁੱਲ_DTypep, nodep->valuep(),
                                "Null dtype on implicit param value");
                    ਅਚਲ AstBasicDType* ਅਚਲ ਮੁੱਲ_Bdtypep = ਮੁੱਲ_DTypep->basicp();
                    ਤਾਰਕਿਕ issigned = ਝੂਠ;
                    ਜੇ (bdtypep->isNosign()) {
                        ਜੇ (ਮੁੱਲ_Bdtypep && ਮੁੱਲ_Bdtypep->isSigned()) issigned = ਸੱਚ;
                    } ਨਹੀਂਤਾਂ {
                        issigned = bdtypep->isSigned();
                    }
                    ਜੇ (ਮੁੱਲ_Bdtypep->isString()) {
                        // parameter X = "str", per IEEE is a number, not a string
                        ਜੇ (ਅਚਲ ਸਵੈਚਾਲਿਤ* ਅਚਲ constp = VN_CAST(nodep->valuep(), Const)) {
                            ਜੇ (constp->ਅੰਕ().isString()) {
                                ਚੌੜਾਈ = constp->ਅੰਕ().ਮੰਜ਼ਿਲ_String().ਲੰਮਾਈ() * 8;
                            }
                        }
                        ਜੇ (ਚੌੜਾਈ < 8) ਚੌੜਾਈ = 8;
                    } ਨਹੀਂਤਾਂ ਜੇ (nodep->valuep()->dtypep()->ਚੌੜਾਈ_Sized()) {
                        ਚੌੜਾਈ = nodep->valuep()->ਚੌੜਾਈ();
                    } ਨਹੀਂਤਾਂ {
                        ਜੇ (nodep->valuep()->ਚੌੜਾਈ() > 32) {
                            nodep->valuep()->v3warn(
                                ਚੌੜਾਈ_2,
                                "Assigning >32 bit to unranged parameter (defaults to 32 bits)");
                        }
                        ਚੌੜਾਈ = 32;
                    }
                    // Can't just inherit valuep()->dtypep() as mwidth might not equal width
                    ਅਚਲ ਤਾਰਕਿਕ is_ਮੁੱਲ_Fourstate
                        = nodep->valuep()->dtypep()->skipRefp()->isFourstate();
                    ਜੇ (ਚੌੜਾਈ == 1) {
                        // one bit parameter is same as "parameter [0] foo",
                        // not "parameter logic foo" as you can extract
                        // "foo[0]" from a parameter but not a wire
                        nodep->dtypep(nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Range_DType(
                            VNumRange{0, 0}, nodep->valuep()->ਚੌੜਾਈ_Min(),
                            VSigning::fromBool(issigned), is_ਮੁੱਲ_Fourstate));
                    } ਨਹੀਂਤਾਂ {
                        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(ਚੌੜਾਈ, nodep->valuep()->ਚੌੜਾਈ_Min(),
                                                         VSigning::fromBool(issigned),
                                                         is_ਮੁੱਲ_Fourstate);
                    }
                    didchk = ਸੱਚ;
                }
                iterate_Check_ਸੌਂਪੋ(nodep, "Initial value", nodep->valuep(), FINAL,
                                   nodep->dtypep());
                UINFO(9, "implicitParamFromIV " << nodep->valuep());
                // UINFO below will print variable nodep
            } ਨਹੀਂਤਾਂ {
                // Or, if nothing assigned, they're integral
                nodep->dtype_ਸੈੱਟ_Integer();
                VL_DANGLING(bdtypep);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (bdtypep && bdtypep->implicit()) {  // Implicits get converted to size 1
            nodep->dtype_ਸੈੱਟ_Logic_Sized(1, bdtypep->numeric());
            VL_DANGLING(bdtypep);
        }
        ਜੇ (nodep->isNet()) {
            Ast_ਗੰਢ_DType* ਅਚਲ badDtp = dtypeNot4StateIntegralRecurse(nodep->dtypep());
            ਜੇ (badDtp)
                nodep->v3error(
                    "Net " << nodep->prettyNameQ()
                           << " data type must be 4-state integral or array/union/struct of such"
                           << " (IEEE 1800-2023 6.7.1)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << badDtp->warnOther() << "... Location of failing data type "
                           << badDtp->prettyDTypeNameQ() << '\n'
                           << badDtp->warnContextSecondary());
        }
        ਜੇ (nodep->valuep() && !didchk) {
            // UINFOTREE(1, nodep, "", "final");
            // AstPattern requires assignments to pass datatype on PRELIM
            user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ਸੌਂਪੋ(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
        user_Iterate_And_ਅਗਲਾ(nodep->delayp(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
        UINFO(4, "varWidthed " << nodep);
        // UINFOTREE(1, nodep, "", "InitOut");
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Var_Ref* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        ਜੇ (!nodep->varp()) {
            ਜੇ (m_paramsOnly && VN_IS(nodep, VarXRef)) {
                checkConstantOrReplace(
                    nodep, ਝੂਠ,
                    "Parameter-resolved constants must not use dotted references: "
                        + nodep->prettyNameQ());
                VL_DANGLING(nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Unlinked varref");
            }
        }
        ਜੇ (!nodep->varp()->did_ਚੌੜਾਈ()) {
            // Var hasn't been widthed, so make it so.
            userIterate(nodep->varp(), ਖਾਲੀ_ਸੂਚਕ);
        }
        ਜੇ (!m_paramsOnly && nodep->varp()->is_Iface_Array_ਵੰਡੋ()) {
            replace_ਵੰਡੋ_Iface_Array_Ref(nodep);
            ਵਾਪਸ;
        }
        // UINFOTREE(9, nodep, "", "VRin");
        // UINFOTREE(9, nodep->varp(), "", "forvar");
        // Note genvar's are also entered as integers
        nodep->dtypeFrom(nodep->varp());
        ਜੇ (VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ) && nodep->access().isWriteOrRW()) {  // On LHS
            UASSERT_OBJ(nodep->dtypep(), nodep, "LHS var should be dtype completed");
        }
        // UINFOTREE(9, nodep, "", "VRout");
        ਜੇ (nodep->access().isWriteOrRW()) nodep->varp()->icoMaybeWritten(ਸੱਚ);
        ਜੇ (nodep->access().isWriteOrRW() && nodep->varp()->direction() == VDirection::CONSTREF) {
            nodep->v3error("Assigning to const ref variable: " << nodep->prettyNameQ());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->access().isWriteOrRW() && nodep->varp()->is_ਨਿਵੇਸ਼()
                   && !nodep->varp()->isFuncLocal() && nodep->varp()->isReadOnly()
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !VN_IS(nodep->abovep(), ਸੌਂਪੋ_Force) && !VN_IS(nodep->abovep(), Release)) {
            // Skip ASSIGNIN for continuous assignments to net-type input ports
            // via hierarchical reference. Net ports allow multiple continuous
            // drivers (IEEE 1800-2023 23.3.3.3). Input ports default to net
            // when port kind is omitted (23.2.2.3, PORT type).
            ਅਚਲ ਤਾਰਕਿਕ hierRef = !m_curModVars.ਗਿਣਤੀ_2(nodep->varp());
            ਅਚਲ ਤਾਰਕਿਕ net_ਪੋਰਟ
                = nodep->varp()->isNet() || nodep->varp()->varType() == VVarType::ਪੋਰਟ;
            ਅਚਲ ਤਾਰਕਿਕ cont_ਸੌਂਪੋ = VN_IS(nodep->abovep(), ਸੌਂਪੋ_W);
            ਅਚਲ ਤਾਰਕਿਕ pinConn = VN_IS(nodep->abovep(), Pin);
            ਜੇ (!(hierRef && net_ਪੋਰਟ && (cont_ਸੌਂਪੋ || pinConn))) {
                nodep->v3warn(ASSIGNIN,
                              "Assigning to input/const variable: " << nodep->prettyNameQ());
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->access().isWriteOrRW() && nodep->varp()->isConst() && !m_paramsOnly
                   && (!m_ftaskp || !m_ftaskp->isConstructor())
                   && !VN_IS(m_procedurep, InitialAutomatic) && !VN_IS(m_procedurep, InitialStatic)
                   && !m_underMemberSel) {
            // Too loose, but need to allow our generated first assignment
            // Move this to a property of the AstInitial block
            nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                              << nodep->prettyNameQ()
                                              << " (IEEE 1800-2023 6.20.6)");
        }
        ਜੇ (nodep->varp()->isClassMember() && !nodep->varp()->isFuncLocal()
            && !nodep->varp()->lifetime().isStatic() && m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot access non-static member variable "
                           << nodep->prettyNameQ() << " from a static method "
                           << m_ftaskp->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
        }
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
    }

    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "  ENUMDTYPE " << nodep);
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->dtypep(nodep);
        Ast_ਗੰਢ_DType* basicp = nodep->dtypep()->skipRefp()->basicp();
        Ast_ਗੰਢ_DType* ਅਚਲ badDtp = dtypeNotIntAtomOrVecRecurse(nodep->subDTypep());
        ਜੇ (badDtp) {
            nodep->v3error(
                "Enum data type must be an integer atom or vector type (IEEE 1800-2023 6.19)\n"
                << nodep->warnContextPrimary() << '\n'
                << badDtp->warnOther() << "... Location of failing data type "
                << badDtp->prettyDTypeNameQ() << '\n'
                << badDtp->warnContextSecondary());
            basicp = nodep->ਲੱਭੋ_Int_DType()->basicp();
            nodep->refDTypep(basicp);
        }
        nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
        // Assign widths
        user_Iterate_And_ਅਗਲਾ(nodep->itemsp(), ਚੌੜਾਈ_VP{nodep->dtypep(), BOTH}.p());
        // Assign missing values
        V3_ਸੰਖਿਆ ਅੰਕ(nodep, nodep->ਚੌੜਾਈ(), 0);
        ਅਚਲ V3_ਸੰਖਿਆ one{nodep, nodep->ਚੌੜਾਈ(), 1};
        ਤਾਰਕਿਕ wrapAround = ਝੂਠ;
        std::map<ਅਚਲ V3_ਸੰਖਿਆ, Ast_Enum_ਵਸਤੂ*> inits;
        ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            ਜੇ (itemp->valuep()) {
                ਜੇ (debug() >= 9) {
                    UINFO(0, "EnumInit " << itemp);
                    itemp->valuep()->dump_ਰੁੱਖ("-  EnumInit: ");
                }
                V3Const::constifyParamsEdit(itemp->valuep());  // itemp may change
                ਜੇ (!VN_IS(itemp->valuep(), Const)) {
                    itemp->valuep()->v3error("Enum value isn't a constant");
                    itemp->valuep()->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();
                    ਜਾਰੀ;
                }
                // TODO IEEE says assigning sized number that is not same size as enum is illegal
                wrapAround = ਝੂਠ;  // Explicit value resets wrap-around tracking
            }
            ਜੇ (!itemp->valuep()) {
                ਜੇ (wrapAround) {
                    itemp->v3error("Enum value illegally wrapped around (IEEE 1800-2023 6.19)");
                }
                ਜੇ (ਅੰਕ.isFourState()) {
                    itemp->v3error("Enum value that is unassigned cannot follow value with X/Zs "
                                   "(IEEE 1800-2023 6.19)");
                }
                itemp->valuep(ਨਵਾਂ AstConst{itemp->fileline(), ਅੰਕ});
            }

            ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(itemp->valuep(), Const);
            ਜੇ (constp->ਅੰਕ().isFourState() && basicp->basicp() && !basicp->isFourstate()) {
                itemp->v3error("Enum value with X/Zs cannot be assigned to non-fourstate type "
                               "(IEEE 1800-2023 6.19)");
            }
            ਅੰਕ.op_ਸੌਂਪੋ(constp->ਅੰਕ());
            // Look for duplicates
            ਅਚਲ ਸਵੈਚਾਲਿਤ pair = inits.emplace(ਅੰਕ, itemp);
            ਜੇ (!pair.ਦੂਜਾ) {  // IEEE says illegal
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ otherp = pair.ਪਹਿਲਾ->ਦੂਜਾ;
                itemp->v3error("Overlapping enumeration value: "
                               << itemp->prettyNameQ() << '\n'
                               << itemp->warnContextPrimary() << '\n'
                               << otherp->warnOther() << "... Location of original declaration\n"
                               << otherp->warnContextSecondary());
            }
            // Detect wrap-around after increment for the next auto-valued item.
            // For signed types, overflow is positive-to-negative (e.g., INT_MAX+1).
            // For unsigned types, overflow wraps to zero (e.g., UINT_MAX+1).
            ਅਚਲ ਤਾਰਕਿਕ prevNeg = constp->ਅੰਕ().isNegative();
            ਅੰਕ.op_ਜੋੜੋ(one, constp->ਅੰਕ());
            ਜੇ (basicp->isSigned()) {
                wrapAround = !prevNeg && ਅੰਕ.isNegative();
            } ਨਹੀਂਤਾਂ {
                wrapAround = ਅੰਕ.isEqZero();
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ* nodep) override {
        UINFO(5, "   ENUMITEM " << nodep);
        VL_RESTORER(m_enumItemp);
        m_enumItemp = nodep;
        Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypep();
        UASSERT_OBJ(vdtypep, nodep, "ENUMITEM not under ENUM");
        nodep->dtypep(vdtypep);
        ਜੇ (nodep->valuep()) {  // else the value will be assigned sequentially
            // Default type is int, but common to assign narrower values, so minwidth from value
            user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            ਤਾਰਕਿਕ warnOn = ਸੱਚ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = nodep->valuep();
            ਜੇ (ਅਚਲ Ast_ਜੋੜੋ* ਅਚਲ anodep = VN_CAST(valuep, ਜੋੜੋ_2)) {
                // If constructed by V3LinkParse due to "enumitem[N_REPEATS] value"
                ਜੇ (anodep->fileline()->equal_ਪਹਿਲਾ_ਸਤਰ_ਖਾਨਾ(*(nodep->fileline())))
                    valuep = anodep->lhsp();
            }
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(valuep, Const)) {
                ਜੇ (static_cast<ਪੂਰਨ>(constp->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1()) > nodep->ਚੌੜਾਈ()) {
                    constp->v3warn(ENUMITEMWIDTH,
                                   "Enum value exceeds width of enum type (IEEE 1800-2023 6.19)");
                    warnOn = ਝੂਠ;  // Prevent normal WIDTHTRUNC
                }
            }
            // Minwidth does not come from value, as spec says set based on parent
            // and if we keep minwidth we'll consider it unsized which is incorrect
            iterateCheck(nodep, "Enum value", nodep->valuep(), CONTEXT_DET, FINAL, nodep->dtypep(),
                         EXTEND_EXP, warnOn);
            // Always create a cast, to avoid later ENUMVALUE warnings
            nodep->valuep(ਨਵਾਂ AstCast{nodep->valuep()->fileline(), nodep->valuep()->unlink_Fr_ਪਿੱਠ(),
                                      nodep->dtypep()});
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ_Ref* nodep) override {
        UASSERT_OBJ(nodep->itemp(), nodep, "Unlinked");
        ਜੇ (!nodep->itemp()->did_ਚੌੜਾਈ()) {
            // We need to do the whole enum en masse
            Ast_ਗੰਢ* enump = nodep->itemp();
            UASSERT_OBJ(enump, nodep, "EnumItemRef not linked");
            ਲਈ (; enump; enump = enump->backp()) {
                ਜੇ (VN_IS(enump, EnumDType)) ਰੋਕੋ;
            }
            UASSERT_OBJ(enump, nodep, "EnumItemRef can't deref back to an Enum");
            VL_DO_DANGLING(userIterate(enump, m_vup), enump);  // Parent enump maybe relinked
        }
        nodep->dtypeFrom(nodep->itemp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons* nodep) override {}  // Widthed when created
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons_ਵਸਤੂ* nodep) override {}  // Widthed when created
    ਖਾਲੀ ਦੇਖੋ(AstDist* nodep) override {
        //  x dist {a :/ p, b :/ q} --> (p > 0 && x == a) || (q > 0 && x == b)
        // (only outside constraints; inside constraints V3Randomize handles weighted selection)
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
        ਲਈ (Ast_ਗੰਢ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ਗੰਢ_DType* subDTypep = ਖਾਲੀ_ਸੂਚਕ;
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();

        ਜੇ (dtype && dtype->isString()) {
            nodep->dtype_ਸੈੱਟ_String();
            subDTypep = nodep->ਲੱਭੋ_String_DType();
        } ਨਹੀਂਤਾਂ ਜੇ (dtype && dtype->isDouble()) {
            nodep->dtype_ਸੈੱਟ_Double();
            subDTypep = nodep->ਲੱਭੋ_Double_DType();
        } ਨਹੀਂਤਾਂ {
            // Take width as maximum across all items
            ਪੂਰਨ ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();
            ਪੂਰਨ mwidth = nodep->exprp()->ਚੌੜਾਈ_Min();
            ਤਾਰਕਿਕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, itemp->ਚੌੜਾਈ());
                mwidth = std::max(mwidth, itemp->ਚੌੜਾਈ_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            subDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                ਚੌੜਾਈ, mwidth, nodep->exprp()->dtypep()->numeric(), isFourstate);
        }

        iterateCheck(nodep, "Dist expression", nodep->exprp(), CONTEXT_DET, FINAL, subDTypep,
                     EXTEND_EXP);
        ਲਈ (Ast_ਗੰਢ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();
            itemp = VN_AS(itemp, Dist_ਵਸਤੂ)->rangep();
            ਜੇ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, ਚੌੜਾਈ_VP{subDTypep, FINAL}.p());
            } ਨਹੀਂਤਾਂ {
                iterateCheck(nodep, "Dist Item", itemp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }

        // Inside a constraint, V3Randomize handles dist lowering with proper weights,
        // but only for simple scalar/range items. Container-type items (queues, arrays)
        // must be lowered here via insideItem() which knows how to expand them.
        ਜੇ (m_constraintp) {
            ਤਾਰਕਿਕ can_ਹੇਠਲਾ = ਸੱਚ;
            ਲਈ (Ast_Dist_ਵਸਤੂ* ditemp = nodep->itemsp(); ditemp;
                 ditemp = VN_AS(ditemp->nextp(), Dist_ਵਸਤੂ)) {
                ਜੇ (VN_IS(ditemp->rangep(), InsideRange)) ਜਾਰੀ;
                // An integral item is a plain equality on either path, so it may be
                // non-constant.  Testing for AstConst here instead would silently drop
                // the weights of a dist whose items are state variables.  Compound
                // items (containers, strings) still need insideItem(), and an impure
                // item must not be cloned into the bucket chain.
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਵਸਤੂ_Dtp = ditemp->rangep()->dtypep()->skipRefp();
                ਜੇ (!ਵਸਤੂ_Dtp->isIntegralOrPacked() || !ditemp->rangep()->is_ਸ਼ੁੱਧ()) {
                    can_ਹੇਠਲਾ = ਝੂਠ;
                    ਰੋਕੋ;
                }
            }
            ਜੇ (can_ਹੇਠਲਾ) ਵਾਪਸ;
        }

        // Outside constraint: lower to inside (ignores weights)
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_Dist_ਵਸਤੂ* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Dist_ਵਸਤੂ)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* inewp
                = inside_ਵਸਤੂ(nodep, nodep->exprp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), itemp->rangep());
            ਜੇ (!inewp) ਜਾਰੀ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ cmpp
                = ਨਵਾਂ AstGt{itemp->fileline(), itemp->weightp()->unlink_Fr_ਪਿੱਠ(),
                            ਨਵਾਂ AstConst{itemp->fileline(), 0}};
            cmpp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::UNSIGNED, ਸੱਚ);
            cmpp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::CMPCONST, ਸੱਚ);
            inewp = ਨਵਾਂ AstLogAnd{itemp->fileline(), cmpp, inewp};
            newp = newp ? ਨਵਾਂ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ਜੇ (!newp) newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};

        UINFOTREE(9, nodep, "", "dist-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstInside* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
        ਲਈ (Ast_ਗੰਢ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            VL_DO_DANGLING(userIterate(itemp, ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p()), itemp);
        }

        AstBasicDType* dtype = VN_CAST(nodep->exprp()->dtypep(), BasicDType);
        Ast_ਗੰਢ_DType* expDTypep = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (dtype && dtype->isString()) {
            nodep->dtype_ਸੈੱਟ_String();
            expDTypep = nodep->ਲੱਭੋ_String_DType();
        } ਨਹੀਂਤਾਂ ਜੇ (dtype && dtype->isDouble()) {
            nodep->dtype_ਸੈੱਟ_Double();
            expDTypep = nodep->ਲੱਭੋ_Double_DType();
        } ਨਹੀਂਤਾਂ {
            // Take width as maximum across all items
            ਪੂਰਨ ਚੌੜਾਈ = nodep->exprp()->ਚੌੜਾਈ();
            ਪੂਰਨ mwidth = nodep->exprp()->ਚੌੜਾਈ_Min();
            ਤਾਰਕਿਕ isFourstate = nodep->exprp()->dtypep()->isFourstate();
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* itemp = nodep->itemsp(); itemp; itemp = itemp->nextp()) {
                ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, itemp->ਚੌੜਾਈ());
                mwidth = std::max(mwidth, itemp->ਚੌੜਾਈ_Min());
                isFourstate |= itemp->dtypep()->isFourstate();
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            ਅਚਲ VSigning numeric = nodep->exprp()->dtypep()->numeric();
            expDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, mwidth, numeric, isFourstate);
        }

        iterateCheck(nodep, "Inside expression", nodep->exprp(), CONTEXT_DET, FINAL, expDTypep,
                     EXTEND_EXP);
        ਲਈ (Ast_ਗੰਢ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = itemp->nextp();  // iterate may cause the node to get replaced
            ਜੇ (VN_IS(itemp, InsideRange)) {
                userIterate(itemp, ਚੌੜਾਈ_VP{expDTypep, FINAL}.p());
            } ਨਹੀਂਤਾਂ ਜੇ (!itemp->dtypep()->skipRefp()->isNonPackedArray()) {
                iterateCheck(nodep, "Inside Item", itemp, CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP);
            }
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp;
        Ast_ਵਿਅੰਜਕ_Stmt* ਵਿਅੰਜਕ_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
        // Skip constraints since those are declarative expressions and they are never actually
        // executed so, there is no need for purification since they cannot generate sideeffects.
        ਜੇ (!m_constraintp && !nodep->exprp()->is_ਸ਼ੁੱਧ()) {
            File_ਸਤਰ* ਅਚਲ fl = nodep->exprp()->fileline();
            // Ensure sized dtype for temp variable
            Ast_ਗੰਢ_DType* ਅਚਲ ਵਿਅੰਜਕ_Dtp = nodep->exprp()->dtypep();
            ਅਚਲ ਪੂਰਨ w = ਵਿਅੰਜਕ_Dtp->ਚੌੜਾਈ();
            Ast_ਗੰਢ_DType* ਅਚਲ ਆਰਜ਼ੀ_DTypep
                = ਵਿਅੰਜਕ_Dtp->ਚੌੜਾਈ_Sized() ? ਵਿਅੰਜਕ_Dtp
                                        : nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(w, w, ਵਿਅੰਜਕ_Dtp->numeric(),
                                                                     ਵਿਅੰਜਕ_Dtp->isFourstate());
            AstVar* ਅਚਲ varp
                = ਨਵਾਂ AstVar{fl, VVarType::XTEMP, m_inside_ਆਰਜ਼ੀ_ਨਾਂ.get(nodep), ਆਰਜ਼ੀ_DTypep};
            exprp = ਨਵਾਂ AstVarRef{fl, varp, VAccess::READ};
            ਵਿਅੰਜਕ_Stmtp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{fl,
                                        ਨਵਾਂ Ast_ਸੌਂਪੋ{fl, ਨਵਾਂ AstVarRef{fl, varp, VAccess::WRITE},
                                                      nodep->exprp()->unlink_Fr_ਪਿੱਠ()},
                                        exprp};
            ਜੇ (m_ftaskp) {
                varp->funcLocal(ਸੱਚ);
                varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                m_ftaskp->prependStmtsp(varp);
            } ਨਹੀਂਤਾਂ {
                m_modep->prependStmtsp(varp);
            }
            iterate(varp);
            iterate(ਵਿਅੰਜਕ_Stmtp);
        } ਨਹੀਂਤਾਂ {
            exprp = nodep->exprp();
        }
        UINFOTREE(9, nodep, "", "inside-in");
        // Now rip out the inside and replace with simple math
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);  // Will be unlinking
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* inewp;
            ਜੇ (ਵਿਅੰਜਕ_Stmtp) {
                inewp = inside_ਵਸਤੂ(nodep, ਵਿਅੰਜਕ_Stmtp, itemp);
                ਵਿਅੰਜਕ_Stmtp = ਖਾਲੀ_ਸੂਚਕ;
            } ਨਹੀਂਤਾਂ {
                inewp = inside_ਵਸਤੂ(nodep, exprp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), itemp);
            }
            ਜੇ (!inewp) ਜਾਰੀ;
            newp = newp ? ਨਵਾਂ AstLogOr{nodep->fileline(), newp, inewp} : inewp;
        }
        ਜੇ (ਵਿਅੰਜਕ_Stmtp) VL_DO_DANGLING(ਵਿਅੰਜਕ_Stmtp->delete_ਰੁੱਖ(), ਵਿਅੰਜਕ_Stmtp);
        ਜੇ (!newp) newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        UINFOTREE(9, newp, "", "inside-out");
        nodep->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* inside_ਵਸਤੂ(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* itemp) {
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਵਸਤੂ_Dtp = itemp->dtypep()->skipRefp();
        ਜੇ (AstInsideRange* ਅਚਲ irangep = VN_CAST(itemp, InsideRange)) {
            // Similar logic in V3Case
            ਵਾਪਸ irangep->newAndFromInside(exprp, irangep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                             irangep->rhsp()->unlink_Fr_ਪਿੱਠ());
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਵਸਤੂ_Dtp, UnpackArrayDType) || VN_IS(ਵਸਤੂ_Dtp, DynArrayDType)
                   || VN_IS(ਵਸਤੂ_Dtp, ਕਤਾਰ_DType)) {
            // Unsupported in parameters
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ inewp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), itemp->unlink_Fr_ਪਿੱਠ(),
                                                          VCMethod::ARRAY_INSIDE, exprp};
            iterateCheckTyped(nodep, "inside value", exprp, ਵਸਤੂ_Dtp->subDTypep(), BOTH);
            inewp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            inewp->did_ਚੌੜਾਈ(ਸੱਚ);
            ਵਾਪਸ inewp;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(ਵਸਤੂ_Dtp, AssocArrayDType)) {
            nodep->v3error("Inside operator not specified on associative arrays "
                           "(IEEE 1800-2023 11.4.13)");
            VL_DO_DANGLING(exprp->delete_ਰੁੱਖ(), exprp);
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ AstEqWild::newTyped(itemp->fileline(), exprp, itemp->unlink_Fr_ਪਿੱਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstInsideRange* nodep) override {
        // Just do each side; AstInside will rip these nodes out later.
        ਜੇ (!m_vup) {
            // This range appears outside a normal expression context (e.g. in a covergroup
            // bin declaration).  Width the bounds self-determined first so each bound (and any
            // referenced parameter) has a dtype, then constify so constant arithmetic is folded
            // (e.g. AstNegate(Const) -> Const, or a parameter reference -> Const).  Folding
            // preserves the now-present dtype, so no bound reaches V3WidthCommit without one.
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            V3Const::constifyEdit(nodep->lhsp());  // lhsp may change
            V3Const::constifyEdit(nodep->rhsp());  // rhsp may change
        } ਨਹੀਂਤਾਂ {
            ਜੇ (m_vup->prelim()) {
                user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), m_vup);
                user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), m_vup);
            }
            ਜੇ (m_vup->final()) {
                Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
                // Warning waivers match visit_cmp_eq_gt on the lowered Gte/Lte
                ਅਚਲ ਪੂਰਨ exp_ਚੌੜਾਈ = expDTypep->ਚੌੜਾਈ();
                ਅਚਲ ਤਾਰਕਿਕ waiveLhs = exp_ਚੌੜਾਈ == 32
                                      && !(expDTypep->isSigned() && nodep->lhsp()->isSigned())
                                      && expDTypep->ਚੌੜਾਈ_Min() >= nodep->lhsp()->ਚੌੜਾਈ();
                ਅਚਲ ਤਾਰਕਿਕ waiveRhs = exp_ਚੌੜਾਈ == 32
                                      && !(expDTypep->isSigned() && nodep->rhsp()->isSigned())
                                      && exp_ਚੌੜਾਈ >= nodep->rhsp()->ਚੌੜਾਈ_Min();
                iterateCheck(nodep, "Range LHS", nodep->lhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveLhs);
                iterateCheck(nodep, "Range RHS", nodep->rhsp(), CONTEXT_DET, FINAL, expDTypep,
                             EXTEND_EXP, !waiveRhs);
            }
        }
        nodep->dtypeFrom(nodep->lhsp());
    }

    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        UINFO(5, "   IFACEREF " << nodep);
        userIterateChildren(nodep, m_vup);
        nodep->dtypep(nodep);
        ਜੇ (nodep->isVirtual()) nodep->iface_ਰਾਹੀਂ_Cellp()->ਸੈੱਟ_Has_Virtual_Ref();
        UINFO(4, "dtWidthed " << nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        ਜੇ (nodep->doing_ਚੌੜਾਈ()) {  // Early exit if have circular parameter definition
            nodep->v3error("Struct's type is circular: " << nodep->prettyName());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        nodep->doing_ਚੌੜਾਈ(ਸੱਚ);
        UINFO(5, "   NODEUORS " << nodep);
        // Check for tagged unions
        ਜੇ (ਅਚਲ AstUnionDType* ਅਚਲ unionp = VN_CAST(nodep, UnionDType)) {
            ਜੇ (unionp->isTagged()) { nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union"); }
        }
        // UINFOTREE(9, nodep, "", "class-in");
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);  // First size all members
        nodep->dtypep(nodep);
        nodep->isFourstate(ਝੂਠ);
        // Error checks
        ਲਈ (AstMemberDType* itemp = nodep->membersp(); itemp;
             itemp = VN_AS(itemp->nextp(), MemberDType)) {
            Ast_ਗੰਢ_DType* ਅਚਲ dtp = itemp->subDTypep()->skipRefp();
            ਜੇ (nodep->packed() && !dtp->isIntegralOrPacked())
                itemp->v3error("Unpacked data type "
                               << dtp->prettyDTypeNameQ()
                               << " in packed struct/union (IEEE 1800-2023 7.2.1)");
            ਜੇ ((VN_IS(nodep, UnionDType) || nodep->packed()) && itemp->valuep()) {
                itemp->v3error("Initial values not allowed in packed struct/union"
                               " (IEEE 1800-2023 7.2.2)");
                ਧੱਕੋ_Deletep(itemp->valuep()->unlink_Fr_ਪਿੱਠ());
            }
        }
        ਅਚਲ ਤਾਰਕਿਕ isHardPackedUnion
            = nodep->packed() && VN_IS(nodep, UnionDType) && !VN_CAST(nodep, UnionDType)->isSoft();

        // Suppress union size errors in parameterized template modules where member
        // widths depend on unresolved parameters.  Also suppress when the type has no
        // owning module (e.g. moved to TypeTable during DepGraph resolution).
        // TODO: Revisit this gate if DepGraph becomes the sole flow and widthing
        // can assume all types are already specialized.
        ਅਚਲ ਤਾਰਕਿਕ ਵਿੱਚ_Template_Module = (m_modep && m_modep->parameterizedTemplate())
                                      || (VN_IS(nodep, UnionDType) && !m_modep);

        // Determine bit assignments and width
        ਜੇ (VN_IS(nodep, UnionDType) || nodep->packed()) {
            ਪੂਰਨ lsb = 0;
            ਪੂਰਨ ਚੌੜਾਈ = 0;
            ਤਾਰਕਿਕ ਪਹਿਲਾ = ਸੱਚ;
            // Report errors on first member first
            AstMemberDType* itemp;
            // MSB is first, so loop backwards
            ਲਈ (itemp = nodep->membersp(); itemp && itemp->nextp();
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {}
            ਲਈ (; itemp; itemp = VN_CAST(itemp->backp(), MemberDType)) {
                ਜੇ (itemp->isFourstate()) nodep->isFourstate(ਸੱਚ);
                itemp->lsb(lsb);
                ਜੇ (VN_IS(nodep, UnionDType)) {
                    ਅਚਲ ਪੂਰਨ ਵਸਤੂ_ਚੌੜਾਈ = itemp->ਚੌੜਾਈ();
                    // Skip union size check for template modules with unresolved parameters
                    ਜੇ (!ਪਹਿਲਾ && isHardPackedUnion && ਵਸਤੂ_ਚੌੜਾਈ != ਚੌੜਾਈ && !ਵਿੱਚ_Template_Module) {
                        itemp->v3error("Hard packed union members must have equal size "
                                       "(IEEE 1800-2023 7.3.1)");
                    }
                    ਚੌੜਾਈ = std::max(ਚੌੜਾਈ, ਵਸਤੂ_ਚੌੜਾਈ);
                } ਨਹੀਂਤਾਂ {
                    lsb += itemp->ਚੌੜਾਈ();
                    ਚੌੜਾਈ += itemp->ਚੌੜਾਈ();
                }
                ਪਹਿਲਾ = ਝੂਠ;
            }
            nodep->ਚੌੜਾਈ_Force(ਚੌੜਾਈ, ਚੌੜਾਈ);  // Signing stays as-is, as parsed from declaration
        } ਨਹੀਂਤਾਂ {
            nodep->ਚੌੜਾਈ_Force(1, 1);
        }
        nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
        // UINFOTREE(9, nodep, "", "class-out");
    }
    ਖਾਲੀ ਦੇਖੋ(AstThisRef* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ਬੱਚਾ_DTypep()));
        ਜੇ (m_ftaskp && m_ftaskp->isStatic()) {
            nodep->v3error("Cannot use 'this' in a static method "
                           << m_ftaskp->prettyNameQ() << " (IEEE 1800-2023 8.10-8.11)");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        // TODO this maybe eventually required to properly resolve members,
        // though causes problems with t_class_forward.v, so for now avoided
        // userIterateChildren(nodep->classp(), nullptr);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstDot* nodep) override {
        // We can only reach this from constify called during V3Param (so before linkDotParam)
        // ... #(Cls#(...)::...) ...
        //                ^^~~~ this is our DOT
        nodep->v3warn(E_UNSUPPORTED, "dotted expressions in parameters\n"
                                         << nodep->warn_ਹੋਰ() << "... Suggest use a typedef");
        nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassExtends* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        ਜੇ (VN_IS(nodep->ਬੱਚਾ_DTypep(), ClassRefDType)) {
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->ਬੱਚਾ_DTypep()));
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberDType* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;  // This node is a dtype & not both PRELIMed+FINALed
        // Iterate into subDTypep() to resolve that type and update pointer.
        nodep->refDTypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        nodep->ਚੌੜਾਈ_From_Sub(nodep->subDTypep());
        nodep->dtypeFrom(nodep->subDTypep());
        ਜੇ (nodep->valuep()) {
            user_Iterate_And_ਅਗਲਾ(nodep->valuep(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
            iterate_Check_ਸੌਂਪੋ(nodep, "Initial value", nodep->valuep(), FINAL, nodep->dtypep());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override {
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        UINFO(5, "   MEMBERSEL " << nodep);
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        VL_RESTORER(m_underMemberSel);
        m_underMemberSel = ਸੱਚ;
        UINFOTREE(9, nodep, "", "mbs-in");
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        UINFOTREE(9, nodep, "", "mbs-ic");
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp()->dtypep(), nodep->fromp(), "Unlinked data type");
        Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        UINFO(9, "     from dt " << fromDtp);
        ਅਚਲ AstMemberSel* ਅਚਲ fromSel = VN_CAST(nodep->fromp(), MemberSel);
        ਜੇ (AstClocking* ਅਚਲ clockingp = fromSel && fromSel->varp()
                                               ? VN_CAST(fromSel->varp()->ਪਹਿਲਾ_Abovep(), Clocking)
                                               : ਖਾਲੀ_ਸੂਚਕ) {
            // In:  MEMBERSEL{MEMBERSEL{vifaceref, "cb", cb_event}, "clockvar", null}
            // Out: MEMBERSEL{vifaceref, "clockvar", clockvar}
            UINFO(9, "     from clocking " << clockingp);
            ਜੇ (AstVar* ਅਚਲ varp = memberSelClocking(nodep, clockingp)) {
                ਜੇ (!varp->did_ਚੌੜਾਈ()) userIterate(varp, ਖਾਲੀ_ਸੂਚਕ);
                AstMemberSel* fromp = VN_AS(nodep->fromp(), MemberSel);
                fromp->replaceWith(fromp->fromp()->unlink_Fr_ਪਿੱਠ());
                VL_DO_DANGLING(fromp->delete_ਰੁੱਖ(), fromp);
                nodep->dtypep(varp->dtypep());
                nodep->varp(varp);
                ਜੇ (nodep->access().isWriteOrRW()) {
                    V3LinkLValue::link_LValue_ਸੈੱਟ(nodep, nodep->access());
                }
                ਜੇ (AstIfaceRefDType* ਅਚਲ adtypep
                    = VN_CAST(nodep->fromp()->dtypep()->skipRefp(), IfaceRefDType)) {
                    nodep->varp()->sensIfacep(adtypep->ifacep());
                }
                UINFO(9, "     done clocking msel " << nodep);
                nodep->did_ਚੌੜਾਈ(ਸੱਚ);  // Must not visit again: will confuse scopes
                ਵਾਪਸ;
            }
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ adtypep = VN_CAST(fromDtp, ਗੰਢ_UOr_Struct_DType)) {
            ਜੇ (memberSelStruct(nodep, adtypep)) ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassRefDType* ਅਚਲ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ਜੇ (memberSelClass(nodep, adtypep)) ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (AstIfaceRefDType* ਅਚਲ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ਜੇ (Ast_ਗੰਢ* foundp = memberSelIface(nodep, adtypep)) {
                ਜੇ (AstClocking* ਅਚਲ clockingp = VN_CAST(foundp, Clocking))
                    foundp = clockingp->ensureEventp();
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(foundp, Var)) {
                    ਜੇ (!varp->did_ਚੌੜਾਈ()) userIterate(varp, ਖਾਲੀ_ਸੂਚਕ);
                    AstIface* ਅਚਲ ifacep = adtypep->iface_ਰਾਹੀਂ_Cellp();
                    varp->sensIfacep(ifacep);
                    // Member of a non-virtual interface reference, e.g. an element selected
                    // from a whole array of interfaces: reference it through the interface,
                    // as V3LinkDot does for 'iface.member'
                    ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep->fromp(), VarRef);
                    ਜੇ (refp && !adtypep->isVirtual() && refp->varp()->isIfaceRef()) {
                        AstVarXRef* ਅਚਲ newp = ਨਵਾਂ AstVarXRef{nodep->fileline(), varp,
                                                                refp->name(), nodep->access()};
                        newp->did_ਚੌੜਾਈ(ਸੱਚ);
                        nodep->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                        ਵਾਪਸ;
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_ਚੌੜਾਈ(ਸੱਚ);
                    ਵਾਪਸ;
                }
                ਜੇ (AstModport* ਅਚਲ modportp = VN_CAST(foundp, Modport)) {
                    // Modport selection (e.g. vif.passive_mp) is compile-time
                    // type narrowing: replace MemberSel with fromp re-typed.
                    AstIfaceRefDType* ਅਚਲ newDtypep = ਨਵਾਂ AstIfaceRefDType{
                        nodep->fileline(), nodep->fileline(), adtypep->cellName(),
                        adtypep->ifaceName(), modportp->name()};
                    newDtypep->ifacep(adtypep->ifacep());
                    newDtypep->cellp(adtypep->cellp());
                    newDtypep->modportp(modportp);
                    newDtypep->isVirtual(adtypep->isVirtual());
                    v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newDtypep);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
                    fromp->dtypep(newDtypep);
                    nodep->replaceWith(fromp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
                ਜੇ (AstCell* ਅਚਲ cellp = VN_CAST(foundp, Cell)) {
                    // Sub-interface cell selection (e.g. vif.tx): resolve to the
                    // companion __Viftop var created by V3LinkCells for its dtype.
                    ਜੇ (VN_IS(cellp->modp(), Iface)) {
                        ਅਚਲ string viftopName = cellp->name() + "__Viftop";
                        Ast_ਗੰਢ_Module* ਅਚਲ ਮਾਪਾ_Ifacep = adtypep->iface_ਰਾਹੀਂ_Cellp();
                        AstVar* viftopVarp = ਖਾਲੀ_ਸੂਚਕ;
                        ਲਈ (Ast_ਗੰਢ* itemp = ਮਾਪਾ_Ifacep->stmtsp(); itemp;
                             itemp = itemp->nextp()) {
                            ਜੇ (AstVar* ਅਚਲ vp = VN_CAST(itemp, Var)) {
                                ਜੇ (vp->name() == viftopName) {
                                    viftopVarp = vp;
                                    ਰੋਕੋ;
                                }
                            }
                        }
                        UASSERT_OBJ(viftopVarp, nodep,
                                    "No __Viftop variable for sub-interface cell");
                        ਜੇ (!viftopVarp->did_ਚੌੜਾਈ()) userIterate(viftopVarp, ਖਾਲੀ_ਸੂਚਕ);
                        Ast_ਗੰਢ_DType* subDtypep = viftopVarp->dtypep();
                        ਜੇ (adtypep->isVirtual()) {
                            // A sub-interface selected through a virtual interface
                            // handle is itself a virtual reference (a runtime
                            // instance pointer), not a static instance. Mark the
                            // dtype virtual so a method call on it dispatches per
                            // instance instead of inlining to one fixed scope.
                            ਜੇ (AstIfaceRefDType* ਅਚਲ subRefp
                                = VN_CAST(subDtypep->skipRefp(), IfaceRefDType)) {
                                AstIfaceRefDType* ਅਚਲ newDtypep = ਨਵਾਂ AstIfaceRefDType{
                                    nodep->fileline(), subRefp->cellName(), subRefp->ifaceName()};
                                newDtypep->ifacep(subRefp->ifacep());
                                newDtypep->cellp(subRefp->cellp());
                                newDtypep->isVirtual(ਸੱਚ);
                                v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newDtypep);
                                subDtypep = newDtypep;
                            }
                        }
                        nodep->dtypep(subDtypep);
                        nodep->varp(viftopVarp);
                        viftopVarp->sensIfacep(VN_AS(cellp->modp(), Iface));
                        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
                        ਵਾਪਸ;
                    }
                }
                ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp = VN_CAST(foundp, ਗੰਢ_FTask)) {
                    Ast_ਵਿਧੀ_Call* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    userIterate(newp, m_vup);
                    ਵਾਪਸ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(fromDtp, EnumDType)  //
                   || VN_IS(fromDtp, AssocArrayDType)  //
                   || VN_IS(fromDtp, WildcardArrayDType)  //
                   || VN_IS(fromDtp, UnpackArrayDType)  //
                   || VN_IS(fromDtp, DynArrayDType)  //
                   || VN_IS(fromDtp, ਕਤਾਰ_DType)  //
                   || VN_IS(fromDtp, ConstraintRefDType)  //
                   || VN_IS(fromDtp, BasicDType)) {
            // Method call on enum without following parenthesis, e.g. "ENUM.next"
            // Convert this into a method call, and let that visitor figure out what to do next
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(),
                                                    nodep->fromp()->unlink_Fr_ਪਿੱਠ(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            userIterate(newp, m_vup);
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Member selection of non-struct/union object '"
                           << nodep->fromp()->prettyTypeName() << "' which is a '"
                           << nodep->fromp()->dtypep()->prettyTypeName() << "'");
        }
        // Error handling
        nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਤਾਰਕਿਕ memberSelClass(AstMemberSel* nodep, AstClassRefDType* adtypep) {
        ਜੇ (nodep->name() == "rand_mode" || nodep->name() == "randomize") {
            Ast_ਵਿਧੀ_Call* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), nodep->name()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਦੇਖੋ(newp);
            ਵਾਪਸ ਸੱਚ;
        }
        // Returns true if ok
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ਅਚਲ ਪਹਿਲਾ_classp = adtypep->classp();
        UASSERT_OBJ(ਪਹਿਲਾ_classp, nodep, "Unlinked");
        ਲਈ (AstClass* classp = ਪਹਿਲਾ_classp; classp;) {
            ਜੇ (Ast_ਗੰਢ* ਅਚਲ foundp = m_memberMap.ਲੱਭੋ_Member(classp, nodep->name())) {
                ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(foundp, Var)) {
                    ਜੇ (!varp->did_ਚੌੜਾਈ()) userIterate(varp, ਖਾਲੀ_ਸੂਚਕ);
                    ਜੇ (varp->lifetime().isStatic() || varp->isParam()) {
                        // Static members are moved outside the class, so they shouldn't be
                        // accessed by member select on a class object
                        AstVarRef* ਅਚਲ varRefp
                            = ਨਵਾਂ AstVarRef{nodep->fileline(), varp, nodep->access()};
                        varRefp->classOrPackagep(classp);
                        nodep->replaceWith(varRefp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                        ਵਾਪਸ ਸੱਚ;
                    }
                    ਜੇ (nodep->access().isWriteOrRW() && varp->isConst()) {
                        nodep->v3warn(E_CONSTWRITTEN, "Writing to 'const' data-typed variable "
                                                          << nodep->prettyNameQ()
                                                          << " (IEEE 1800-2023 6.20.6)");
                    }
                    nodep->dtypep(foundp->dtypep());
                    nodep->varp(varp);
                    nodep->did_ਚੌੜਾਈ(ਸੱਚ);
                    ਜੇ (nodep->fromp()->same_ਰੁੱਖ(m_randomizeFromp) && varp->isRand())  // null-safe
                        V3LinkLValue::link_LValue_ਸੈੱਟ(nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
                ਜੇ (AstConstraint* constrp = VN_CAST(foundp, Constraint)) {
                    nodep->replaceWith(ਨਵਾਂ AstConstraintRef{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), constrp});
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
                ਜੇ (Ast_Enum_ਵਸਤੂ_Ref* ਅਚਲ adfoundp = VN_CAST(foundp, Enum_ਵਸਤੂ_Ref)) {
                    nodep->replaceWith(adfoundp->clone_ਰੁੱਖ(ਝੂਠ));
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
                ਜੇ (Ast_ਗੰਢ_FTask* ftaskp = VN_CAST(foundp, ਗੰਢ_FTask)) {
                    Ast_ਵਿਧੀ_Call* newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                        nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), nodep->name()};
                    newp->taskp(ftaskp);
                    newp->dtypep(ftaskp->dtypep());
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
                ਜੇ (VN_IS(foundp, Constraint)) {
                    // We don't support constraints yet, so just keep as unlinked for now
                    // Presumably we'll next see a constraint_mode AstMethodCall
                    nodep->dtypep(nodep->ਲੱਭੋ_Constraint_Ref_DType());
                    UINFO(9, "Unsupported constraint select " << nodep);
                    ਵਾਪਸ ਸੱਚ;
                }
                UINFO(1, "found object " << foundp);
                nodep->v3fatalSrc("MemberSel of non-variable\n"
                                  << nodep->warnContextPrimary() << '\n'
                                  << foundp->warnOther() << "... Location of found object\n"
                                  << foundp->warnContextSecondary());
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ;
        }

        VSpellCheck speller;
        ਲਈ (AstClass* classp = ਪਹਿਲਾ_classp; classp;) {
            ਲਈ (Ast_ਗੰਢ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (VN_IS(itemp, Constraint) || VN_IS(itemp, Enum_ਵਸਤੂ_Ref) || VN_IS(itemp, Var)) {
                    speller.ਧੱਕੋ_Candidate(itemp->prettyName());
                }
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ;
        }
        ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in " << ਪਹਿਲਾ_classp->verilogKwd()
                      << " " << ਪਹਿਲਾ_classp->prettyNameQ() << "\n"
                      << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->fileline()->warn_ਹੋਰ() + suggest));
        ਵਾਪਸ ਝੂਠ;  // Caller handles error
    }
    ਤਾਰਕਿਕ modport_Allows_ਵਸਤੂ(Ast_ਗੰਢ* nodep, ਅਚਲ AstIfaceRefDType* adtypep, ਅਚਲ Ast_ਗੰਢ* itemp) {
        // Variables, tasks/functions and clocking blocks of an interface are only visible
        // through a modport when the modport lists them
        ਅਚਲ AstModport* ਅਚਲ modportp = adtypep->modportp();
        ਜੇ (!modportp) ਵਾਪਸ ਸੱਚ;
        ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(itemp, Var)) {
            ਜੇ (varp->isParam()) ਵਾਪਸ ਸੱਚ;
        } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(itemp, ਗੰਢ_FTask) && !VN_IS(itemp, Clocking)) {
            ਵਾਪਸ ਸੱਚ;
        }
        ਸਵੈਚਾਲਿਤ pair = m_modport_ਨਾਂ.emplace(modportp, std::unordered_ਸੈੱਟ<string>{});
        ਜੇ (pair.ਦੂਜਾ) {
            ਲਈ (ਅਚਲ Ast_ਗੰਢ* mpItemp = modportp->varsp(); mpItemp; mpItemp = mpItemp->nextp()) {
                pair.ਪਹਿਲਾ->ਦੂਜਾ.ਪਾਓ(mpItemp->name());
            }
        }
        ਜੇ (pair.ਪਹਿਲਾ->ਦੂਜਾ.ਗਿਣਤੀ_2(itemp->name())) ਵਾਪਸ ਸੱਚ;
        nodep->v3error("Member " << itemp->prettyNameQ() << " found in interface "
                                 << adtypep->iface_ਰਾਹੀਂ_Cellp()->prettyNameQ()
                                 << " but not in modport " << modportp->prettyNameQ()
                                 << " (IEEE 1800-2023 25.5)");
        ਵਾਪਸ ਝੂਠ;
    }
    Ast_ਗੰਢ* memberSelIface(AstMemberSel* nodep, AstIfaceRefDType* adtypep) {
        // Returns node if ok
        // No need to width-resolve the interface, as it was done when we did the child
        // ifaceViaCellp() handles dtypes with cellp-only (no ifacep), as produced
        // by sub-interface selection, enabling chained access (e.g. vif.tx.Tx).
        Ast_ਗੰਢ_Module* ਅਚਲ ifacep = adtypep->iface_ਰਾਹੀਂ_Cellp();
        UASSERT_OBJ(ifacep, nodep, "Unlinked");
        VSpellCheck speller;
        ਲਈ (Ast_ਗੰਢ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (itemp->name() == nodep->name()) {
                ਜੇ (!modport_Allows_ਵਸਤੂ(nodep, adtypep, itemp)) {
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Caller handles error
                }
                ਵਾਪਸ itemp;
            }
            ਜੇ (VN_IS(itemp, Var) || VN_IS(itemp, Modport) || VN_IS(itemp, Cell)) {
                speller.ਧੱਕੋ_Candidate(itemp->prettyName());
            }
        }
        ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in interface "
                      << ifacep->prettyNameQ() << "\n"
                      << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->fileline()->warn_ਹੋਰ() + suggest));
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Caller handles error
    }
    AstVar* memberSelClocking(AstMemberSel* nodep, AstClocking* clockingp) {
        // Returns node if ok
        VSpellCheck speller;

        ਲਈ (Ast_ਗੰਢ* itemp = clockingp->itemsp(); itemp; itemp = itemp->nextp()) {
            ਜੇ (Ast_Clocking_ਵਸਤੂ* citemp = VN_CAST(itemp, Clocking_ਵਸਤੂ)) {
                ਜੇ (citemp->varp()->name() == nodep->name()) ਵਾਪਸ citemp->varp();
                speller.ਧੱਕੋ_Candidate(citemp->varp()->prettyName());
            }
        }
        ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(nodep->prettyName());
        nodep->v3error(
            "Member " << nodep->prettyNameQ() << " not found in clocking block "
                      << clockingp->prettyNameQ() << "\n"
                      << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->fileline()->warn_ਹੋਰ() + suggest));
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;  // Caller handles error
    }
    ਤਾਰਕਿਕ memberSelStruct(AstMemberSel* nodep, Ast_ਗੰਢ_UOr_Struct_DType* adtypep) {
        // Returns true if ok
        ਜੇ (AstMemberDType* ਅਚਲ memberp
            = VN_CAST(m_memberMap.ਲੱਭੋ_Member(adtypep, nodep->name()), MemberDType)) {
            ਜੇ (m_attrp) {  // Looking for the base of the attribute
                nodep->dtypep(memberp);
                UINFO(9, "   MEMBERSEL(attr) -> " << nodep);
                UINFO(9, "           dt-> " << nodep->dtypep());
            } ਨਹੀਂਤਾਂ ਜੇ (adtypep->packed()) {
                AstSel* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                                memberp->lsb(), memberp->ਚੌੜਾਈ()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
                newp->did_ਚੌੜਾਈ(ਸੱਚ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            } ਨਹੀਂਤਾਂ {
                AstStructSel* ਅਚਲ newp = ਨਵਾਂ AstStructSel{
                    nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), nodep->name()};
                // Must skip over the member to find the union; as the member may disappear later
                newp->dtypep(memberp->subDTypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump());
                newp->did_ਚੌੜਾਈ(ਸੱਚ);  // Don't replace dtype with basic type
                UINFO(9, "   MEMBERSEL -> " << newp);
                UINFO(9, "           dt-> " << newp->dtypep());
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                // Should be able to treat it as a normal-ish nodesel - maybe.
                // The lhsp() will be strange until this stage; create the number here?
            }
            ਵਾਪਸ ਸੱਚ;
        }
        nodep->v3error("Member " << nodep->prettyNameQ() << " not found in structure");
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        // Never created before V3Width, so no need to redo it
        UASSERT_OBJ(nodep->dtypep(), nodep, "CMETHODCALLs should have already been sized");
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਧੀ_Call* nodep) override {
        UINFO(5, "   METHODCALL " << nodep);
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        UINFOTREE(9, nodep, "", "mts-in");
        // Should check types the method requires, but at present we don't do much
        userIterate(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // Args are checked within each particular method's decode
        // Any AstWith is checked later when know types, in methodWithClause
        // Find the fromp dtype - should be a class
        UASSERT_OBJ(nodep->fromp() && nodep->fromp()->dtypep(), nodep, "Unsized expression");
        Ast_ਗੰਢ_DType* ਅਚਲ fromDtp = nodep->fromp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        AstBasicDType* ਅਚਲ basicp = fromDtp ? fromDtp->basicp() : ਖਾਲੀ_ਸੂਚਕ;
        UINFO(9, "     from dt " << fromDtp);
        userIterate(fromDtp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (nodep->name() == "rand_mode") {
            ਵਿਧੀ_Call_Rand_ਬਹੁਲਕ(nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "constraint_mode") {
            ਵਿਧੀ_Call_Constraint(nodep, ਖਾਲੀ_ਸੂਚਕ);
        } ਨਹੀਂਤਾਂ ਜੇ (AstEnumDType* ਅਚਲ adtypep = VN_CAST(fromDtp, EnumDType)) {
            ਵਿਧੀ_Call_Enum(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstAssocArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, AssocArrayDType)) {
            ਵਿਧੀ_Call_Assoc(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstWildcardArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, WildcardArrayDType)) {
            ਵਿਧੀ_Call_Wildcard(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstDynArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, DynArrayDType)) {
            ਵਿਧੀ_Call_Dyn(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਕਤਾਰ_DType* ਅਚਲ adtypep = VN_CAST(fromDtp, ਕਤਾਰ_DType)) {
            ਵਿਧੀ_Call_ਕਤਾਰ(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstClassRefDType* ਅਚਲ adtypep = VN_CAST(fromDtp, ClassRefDType)) {
            ਵਿਧੀ_Call_Class(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstIfaceRefDType* ਅਚਲ adtypep = VN_CAST(fromDtp, IfaceRefDType)) {
            ਵਿਧੀ_Call_Iface_Ref(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstUnpackArrayDType* ਅਚਲ adtypep = VN_CAST(fromDtp, UnpackArrayDType)) {
            ਵਿਧੀ_Call_Unpack(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (AstConstraintRefDType* ਅਚਲ adtypep = VN_CAST(fromDtp, ConstraintRefDType)) {
            ਵਿਧੀ_Call_Constraint(nodep, adtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isEvent()) {
            ਵਿਧੀ_Call_Event(nodep, basicp);
        } ਨਹੀਂਤਾਂ ਜੇ (basicp && basicp->isString()) {
            ਵਿਧੀ_Call_String(nodep, basicp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Member call on object '"
                                             << nodep->fromp()->prettyTypeName()
                                             << "' which is a '"
                                             << nodep->fromp()->dtypep()->prettyTypeName() << "'");
            nodep->dtype_ਸੈੱਟ_Void();
        }
    }
    AstWith* ਵਿਧੀ_With_Clause(Ast_ਗੰਢ_FTask_Ref* nodep, ਤਾਰਕਿਕ required, ਤਾਰਕਿਕ arbReturn,
                              Ast_ਗੰਢ_DType* returnDtp, Ast_ਗੰਢ_DType* ਸੂਚਕਾਂਕ_Dtp,
                              Ast_ਗੰਢ_DType* ਮੁੱਲ_Dtp) {
        UASSERT_OBJ(arbReturn || returnDtp, nodep, "Null return type");
        ਜੇ (AstWith* ਅਚਲ withp = nodep->withp()) {
            withp->ਸੂਚਕਾਂਕ_Arg_Refp()->dtypep(ਸੂਚਕਾਂਕ_Dtp);
            withp->ਮੁੱਲ_Arg_Refp()->dtypep(ਮੁੱਲ_Dtp);
            userIterate(withp, ਚੌੜਾਈ_VP{returnDtp, BOTH}.p());
            withp->unlink_Fr_ਪਿੱਠ();
            ਵਾਪਸ withp;
        }
        ਜੇ (required) {
            nodep->v3error("'with' statement is required for ." << nodep->prettyName()
                                                                << " method");
        }
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    ਖਾਲੀ ਵਿਧੀ_Ok_Arguments(Ast_ਗੰਢ_FTask_Ref* nodep, ਪੂਰਨ minArg, ਪੂਰਨ maxArg) {
        ਪੂਰਨ narg = 0;
        ਜੇ (AstWith* ਅਚਲ withp = nodep->withp()) {
            withp->v3error("'with' not legal on this method");
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(withp->unlink_Fr_ਪਿੱਠ()), withp);
        }
        ਲਈ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) ++narg;
        ਅਚਲ ਤਾਰਕਿਕ ok = (narg >= minArg) && (narg <= maxArg);
        ਜੇ (!ok) {
            nodep->v3error("The " << narg << " arguments passed to ." << nodep->prettyName()
                                  << " method does not match its requiring " << cvt_ਮੰਜ਼ਿਲ_Str(minArg)
                                  << (minArg == maxArg ? "" : " to " + cvt_ਮੰਜ਼ਿਲ_Str(maxArg))
                                  << " arguments");
            // Adjust to required argument counts, very bogus, but avoids core dump
            ਲਈ (; narg < minArg; ++narg) {
                nodep->ਜੋੜੋ_Argsp(
                    ਨਵਾਂ AstArg{nodep->fileline(), "",
                               ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}}});
            }
            ਲਈ (; narg > maxArg; --narg) {
                AstArg* argp = nodep->argsp();
                ਜਦੋਂ (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
                argp->unlink_Fr_ਪਿੱਠ();
                VL_DO_DANGLING(argp->delete_ਰੁੱਖ(), argp);
            }
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਧੀ_Arg(Ast_ਵਿਧੀ_Call* nodep, ਪੂਰਨ arg) {
        AstArg* argp = nodep->argsp();
        ਲਈ (ਪੂਰਨ narg = 0; narg < arg; ++narg) argp = VN_AS(argp->nextp(), Arg);
        UASSERT_OBJ(argp, nodep, "methodOkArguments() should have detected arg count error");
        ਵਾਪਸ argp->exprp();
    }

    ਖਾਲੀ ਵਿਧੀ_Call_Enum(Ast_ਵਿਧੀ_Call* nodep, AstEnumDType* adtypep) {
        // Method call on enum without following parenthesis, e.g. "ENUM.next"
        // Convert this into a method call, and let that visitor figure out what to do next
        ਜੇ (nodep->name() == "num"  //
            || nodep->name() == "first"  //
            || nodep->name() == "last") {
            // Constant value
            AstConst* newp = ਖਾਲੀ_ਸੂਚਕ;
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਜੇ (nodep->name() == "num") {
                ਪੂਰਨ ਚੀਜ਼ਾਂ = 0;
                ਲਈ (Ast_ਗੰਢ* itemp = adtypep->itemsp(); itemp; itemp = itemp->nextp()) ++ਚੀਜ਼ਾਂ;
                newp = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{}, ਚੀਜ਼ਾਂ);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "first") {
                ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = adtypep->itemsp();
                ਜੇ (!itemp) {
                    newp = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ਨਹੀਂਤਾਂ {
                    newp = VN_AS(itemp->valuep()->clone_ਰੁੱਖ(ਝੂਠ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "last") {
                ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = adtypep->itemsp();
                ਜਦੋਂ (itemp && itemp->nextp()) itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ);
                ਜੇ (!itemp) {
                    newp = ਨਵਾਂ AstConst(nodep->fileline(), AstConst::Signed32{},
                                        0);  // Spec doesn't say what to do
                } ਨਹੀਂਤਾਂ {
                    newp = VN_AS(itemp->valuep()->clone_ਰੁੱਖ(ਝੂਠ), Const);  // A const
                    newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
                }
            }
            UASSERT_OBJ(newp, nodep, "Enum method (perhaps enum item) not const");
            newp->fileline(nodep->fileline());  // Use method's filename/line number to be clearer;
                                                // may have warning disables
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "name" || nodep->name() == "next" || nodep->name() == "prev") {
            VAttrType attrType;
            ਜੇ (nodep->name() == "name") {
                attrType = VAttrType::ENUM_NAME;
                ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "next") {
                attrType = VAttrType::ENUM_ਅਗਲਾ;
                ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "prev") {
                attrType = VAttrType::ENUM_PREV;
                ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Bad case");
            }

            ਜੇ (nodep->name() != "name" && nodep->argsp()) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* stepp = ਵਿਧੀ_Arg(nodep, 0);
                VL_DO_DANGLING(V3Const::constifyParamsNoWarnEdit(stepp), stepp);
                stepp = ਵਿਧੀ_Arg(nodep, 0);
                nodep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::WIDTHEXPAND, ਸੱਚ);
                iterateCheckUInt32(nodep, "argument", stepp, BOTH);
                ਜੇ (!VN_IS(stepp, Const)) {
                    stepp->v3warn(E_UNSUPPORTED,
                                  "Unsupported: enum next/prev with non-constant argument");
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{stepp->fileline(), 1};
                    stepp->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(stepp), stepp);
                    stepp = newp;
                }
                ਅਚਲ uint32_t ਕਦਮ_ਚੌੜਾਈ = VN_AS(stepp, Const)->ਮੰਜ਼ਿਲ_UInt();
                ਜੇ (ਕਦਮ_ਚੌੜਾਈ == 0) {
                    // Step of 0 "legalizes" like $cast, use .next.prev
                    Ast_ਵਿਧੀ_Call* ਅਚਲ newp = ਨਵਾਂ Ast_ਵਿਧੀ_Call{
                        nodep->fileline(),
                        ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          "next"},
                        "prev"};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    ਵਾਪਸ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਕਦਮ_ਚੌੜਾਈ != 1) {
                    // Unroll of enumVar.next(k) to enumVar.next(1).next(k - 1)
                    ਧੱਕੋ_Deletep(nodep->argsp()->unlink_Fr_ਪਿੱਠ());
                    Ast_ਵਿਧੀ_Call* ਅਚਲ clonep = nodep->clone_ਰੁੱਖ(ਝੂਠ);
                    VN_AS(stepp, Const)->ਅੰਕ().ਸੈੱਟ_Long(1);
                    AstConst* ਅਚਲ constp = ਨਵਾਂ AstConst(nodep->fileline(), ਕਦਮ_ਚੌੜਾਈ - 1);
                    AstArg* ਅਚਲ argp = ਨਵਾਂ AstArg{nodep->fileline(), "", constp};
                    Ast_ਵਿਧੀ_Call* ਅਚਲ newp
                        = ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(), clonep, nodep->name(), argp};
                    // No dtype assigned, we will recurse the new method and replace
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                    ਵਾਪਸ;
                }
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                = enumSelect(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), adtypep, attrType);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown built-in enum method " << nodep->prettyNameQ());
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Wildcard(Ast_ਵਿਧੀ_Call* nodep, AstWildcardArrayDType* adtypep) {
        AstCMethodHard* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ASSOC_ਮਾਪ};  // So don't need num()
            newp->dtype_ਸੈੱਟ_Int();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev"  //
                   || nodep->name() == "unique_index"  //
                   || nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on wildcard associative arrays");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSelf(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), SELF, BOTH);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_Wildcard_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep, adtypep);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ASSOC_EXISTS, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
            newp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, 1, VSigning::SIGNED);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "delete") {  // function void delete([input integer index])
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ਜੇ (!nodep->argsp()) {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_ਸੈੱਟ_Void();
            } ਨਹੀਂਤਾਂ {
                iterateCheckSelf(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), SELF, BOTH);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_Wildcard_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep, adtypep);
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::ASSOC_ERASE, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
                newp->dtype_ਸੈੱਟ_Void();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->subDTypep(),
                                   adtypep->ਲੱਭੋ_String_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                   adtypep->ਲੱਭੋ_String_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "map") {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, adtypep->subDTypep(),
                                   adtypep->ਲੱਭੋ_String_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown wildcard associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ਜੇ (newp) {
            newp->protect(ਝੂਠ);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Assoc(Ast_ਵਿਧੀ_Call* nodep, AstAssocArrayDType* adtypep) {
        AstCMethodHard* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->withp() && adtypep->subDTypep()->isWide()) {
            nodep->v3warn(
                E_UNSUPPORTED,
                "Unsupported: `with` clause on assoc arrays with wide value types in method '"
                    << nodep->prettyName() << "'");
        }
        ਜੇ (nodep->name() == "num"  // function int num()
            || nodep->name() == "size") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ASSOC_ਮਾਪ};  // So don't need num()
            newp->dtype_ਸੈੱਟ_Int();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "first"  // function int first(ref index)
                   || nodep->name() == "last"  //
                   || nodep->name() == "next"  //
                   || nodep->name() == "prev") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), adtypep->ਕੁੰਜੀ_DTypep(), BOTH);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_Assoc_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep, adtypep);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, ਸੂਚਕਾਂਕ_exprp, VAccess::READWRITE);
            newp
                = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                     VCMethod::array_ਵਿਧੀ(nodep->name()),  // first/last/next/prev
                                     ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
            newp->dtype_ਸੈੱਟ_Int();
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "exists") {  // function int exists(input index)
            // IEEE really should have made this a "bit" return
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), adtypep->ਕੁੰਜੀ_DTypep(), BOTH);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_Assoc_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep, adtypep);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ASSOC_EXISTS, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
            newp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Unsized(32, 1, VSigning::SIGNED);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "delete") {  // function void delete([input integer index])
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ਜੇ (!nodep->argsp()) {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::ASSOC_CLEAR};
                newp->dtype_ਸੈੱਟ_Void();
            } ਨਹੀਂਤਾਂ {
                iterateCheckTyped(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), adtypep->ਕੁੰਜੀ_DTypep(),
                                  BOTH);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_Assoc_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep, adtypep);
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::ASSOC_ERASE, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
                newp->dtype_ਸੈੱਟ_Void();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "sort" || nodep->name() == "rsort"
                   || nodep->name() == "reverse" || nodep->name() == "shuffle") {
            nodep->v3error("Array method " << nodep->prettyNameQ()
                                           << " not legal on associative arrays");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->subDTypep(),
                                                    adtypep->ਕੁੰਜੀ_DTypep(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(
                nodep, ਝੂਠ, ਸੱਚ, ਖਾਲੀ_ਸੂਚਕ, nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            ਜੇ (nodep->name() == "unique_index") {
                newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->ਕੁੰਜੀ_DTypep()));
            } ਨਹੀਂਤਾਂ {
                newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            }
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last") {
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                                    adtypep->ਕੁੰਜੀ_DTypep(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                                    adtypep->ਕੁੰਜੀ_DTypep(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->ਕੁੰਜੀ_DTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "map") {
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, adtypep->subDTypep(),
                                                    adtypep->ਕੁੰਜੀ_DTypep(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown built-in associative array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ਜੇ (newp) {
            newp->protect(ਝੂਠ);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਧੀ_Call_Assoc_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(Ast_ਵਿਧੀ_Call* nodep, AstAssocArrayDType* adtypep) {
        Ast_ਗੰਢ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", ਸੂਚਕਾਂਕ_exprp, CONTEXT_DET, FINAL, adtypep->ਕੁੰਜੀ_DTypep(),
                     EXTEND_EXP);
        VL_DANGLING(ਸੂਚਕਾਂਕ_exprp);  // May have been edited
        ਵਾਪਸ nodep->argsp()->exprp();
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਧੀ_Call_Wildcard_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(Ast_ਵਿਧੀ_Call* nodep,
                                             AstWildcardArrayDType* adtypep) {
        Ast_ਗੰਢ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = nodep->argsp()->exprp();
        iterateCheck(nodep, "index", ਸੂਚਕਾਂਕ_exprp, CONTEXT_DET, FINAL, adtypep->ਲੱਭੋ_String_DType(),
                     EXTEND_EXP);
        VL_DANGLING(ਸੂਚਕਾਂਕ_exprp);  // May have been edited
        ਵਾਪਸ nodep->argsp()->exprp();
    }
    ਖਾਲੀ ਵਿਧੀ_Call_LValue_Recurse(Ast_ਵਿਧੀ_Call* nodep, Ast_ਗੰਢ* childp, ਅਚਲ VAccess& access) {
        ਜੇ (AstCMethodHard* ਅਚਲ ichildp = VN_CAST(childp, CMethodHard)) {
            ਤਾਰਕਿਕ methAt = ਝੂਠ;
            ਤਾਰਕਿਕ meth_At_ਪਿੱਠ = ਝੂਠ;
            ਤਾਰਕਿਕ methAtWrite = ਝੂਠ;
            ਚੋਣ (ichildp->ਵਿਧੀ()) {
            ਹਾਲਤ VCMethod::ARRAY_AT:  // FALLTHRU
                methAt = ਸੱਚ;
                ਰੋਕੋ;
            ਹਾਲਤ VCMethod::ARRAY_AT_ਪਿੱਠ:  // FALLTHRU
                meth_At_ਪਿੱਠ = ਸੱਚ;
                ਰੋਕੋ;
            ਹਾਲਤ VCMethod::ARRAY_AT_WRITE:  // FALLTHRU
            ਹਾਲਤ VCMethod::DYN_AT_WRITE_APPEND:  // FALLTHRU
            ਹਾਲਤ VCMethod::DYN_AT_WRITE_APPEND_ਪਿੱਠ:  //
                methAtWrite = ਸੱਚ;
                ਰੋਕੋ;
            ਮੂਲ: ਰੋਕੋ;
            }
            ਜੇ (methAt || meth_At_ਪਿੱਠ || methAtWrite) {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ fromDtypep = ichildp->fromp()->dtypep()->skipRefp();
                ਜੇ (VN_IS(fromDtypep, ਕਤਾਰ_DType) || VN_IS(fromDtypep, DynArrayDType)) {
                    // Change access methods to writable ones
                    ਜੇ (methAt) {
                        ichildp->ਵਿਧੀ(VCMethod::ARRAY_AT_WRITE);
                    } ਨਹੀਂਤਾਂ ਜੇ (meth_At_ਪਿੱਠ) {
                        ichildp->ਵਿਧੀ(VCMethod::DYN_AT_WRITE_APPEND_ਪਿੱਠ);
                    }
                }
                ਵਿਧੀ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
                ਵਾਪਸ;
            }
        }
        ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(childp, ਗੰਢ_Var_Ref)) {
            varrefp->access(access);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstMemberSel* ਅਚਲ ichildp = VN_CAST(childp, MemberSel)) {
            ਵਿਧੀ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ ichildp = VN_CAST(childp, StructSel)) {
            ਵਿਧੀ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_Sel* ਅਚਲ ichildp = VN_CAST(childp, ਗੰਢ_Sel)) {
            ਵਿਧੀ_Call_LValue_Recurse(nodep, ichildp->fromp(), access);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(childp, LambdaArgRef) || VN_IS(childp, FuncRef)) {
            // NOP
        } ਨਹੀਂਤਾਂ {
            UINFO(1, "    Related node: " << childp);
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: Non-variable on LHS of built-in method '"
                                             << nodep->prettyName() << "'");
        }
    }
    AstCMethodHard* ਵਿਧੀ_Call_Array(Ast_ਵਿਧੀ_Call* nodep, Ast_ਗੰਢ_DType* adtypep) {
        AstCMethodHard* newp = ਖਾਲੀ_ਸੂਚਕ;

        ਜੇ (nodep->name() == "reverse" || nodep->name() == "shuffle" || nodep->name() == "sort"
            || nodep->name() == "rsort") {
            AstWith* withp = ਖਾਲੀ_ਸੂਚਕ;
            ਜੇ (nodep->name() == "sort" || nodep->name() == "rsort") {
                withp = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਸੱਚ, ਖਾਲੀ_ਸੂਚਕ, nodep->ਲੱਭੋ_UInt32_DType(),
                                         adtypep->subDTypep());
            }
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "min" || nodep->name() == "max" || nodep->name() == "unique"
                   || nodep->name() == "unique_index") {
            AstWith* ਅਚਲ withp = ਵਿਧੀ_With_Clause(
                nodep, ਝੂਠ, ਸੱਚ, ਖਾਲੀ_ਸੂਚਕ, nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            ਜੇ (nodep->name() == "unique_index") {
                newp->dtypep(newp->ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType());
            } ਨਹੀਂਤਾਂ {
                newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            }
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "find" || nodep->name() == "find_first"
                   || nodep->name() == "find_last" || nodep->name() == "find_index") {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "find_index" || nodep->name() == "find_first_index"
                   || nodep->name() == "find_last_index") {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->withp(withp);
            newp->dtypep(newp->ਲੱਭੋ_ਕਤਾਰ_ਸੂਚਕਾਂਕ_DType());
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "map") {
            // map() - IEEE 1800-2023 7.12.5
            // Returns a queue with same element count, each element is the with expression result
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਸੱਚ, ਝੂਠ, adtypep->subDTypep(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ARRAY_MAP};
            newp->withp(withp);
            newp->dtypep(ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(withp ? withp->dtypep() : adtypep->subDTypep()));
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        }
        ਵਾਪਸ newp;
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Dyn(Ast_ਵਿਧੀ_Call* nodep, AstDynArrayDType* adtypep) {
        AstCMethodHard* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->name() == "at") {  // Created internally for []
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ARRAY_AT};
            newp->dtypeFrom(adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atWrite") {  // Created internally for []
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::ARRAY_AT_WRITE};
            newp->dtypeFrom(adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "size") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::DYN_ਮਾਪ};
            newp->dtype_ਸੈੱਟ_Int();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "delete") {  // function void delete()
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::DYN_CLEAR};
            newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            // All value return
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->subDTypep(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ ((newp = ਵਿਧੀ_Call_Array(nodep, adtypep))) {
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/unknown built-in dynamic array method "
                                             << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ਜੇ (newp) {
            newp->protect(ਝੂਠ);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_ਕਤਾਰ(Ast_ਵਿਧੀ_Call* nodep, Ast_ਕਤਾਰ_DType* adtypep) {
        AstCMethodHard* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->name() == "at" || nodep->name() == "atBack") {  // Created internally for []
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atWriteAppend"
                   || nodep->name() == "atWriteAppendBack") {  // Created internally for []
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "num"  // function int num()
                   || nodep->name() == "size") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::DYN_ਮਾਪ};
            newp->dtype_ਸੈੱਟ_Int();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "delete") {  // function void delete([input integer index])
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            ਜੇ (!nodep->argsp()) {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::DYN_CLEAR};
                newp->dtype_ਸੈੱਟ_Void();
            } ਨਹੀਂਤਾਂ {
                iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_ਕਤਾਰ_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep);
                ਜੇ (ਸੂਚਕਾਂਕ_exprp->isZero()) {  // delete(0) is a pop_front
                    newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                              VCMethod::DYN_ਕੱਢੋ_FRONT};
                    newp->dtypeFrom(adtypep->subDTypep());
                    newp->dtype_ਸੈੱਟ_Void();
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                              VCMethod::DYN_ERASE, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
                    newp->dtype_ਸੈੱਟ_Void();
                }
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "insert") {
            ਵਿਧੀ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "index", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = ਵਿਧੀ_Call_ਕਤਾਰ_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(nodep);
            AstArg* ਅਚਲ argp = VN_AS(nodep->argsp()->nextp(), Arg);
            iterateCheckTyped(nodep, "insert value", argp->exprp(), adtypep->subDTypep(), BOTH);
            ਜੇ (ਸੂਚਕਾਂਕ_exprp->isZero()) {  // insert(0, ...) is a push_front
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::DYN_ਧੱਕੋ_FRONT, argp->exprp()->unlink_Fr_ਪਿੱਠ()};
                newp->dtype_ਸੈੱਟ_Void();
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::DYN_ਪਾਓ, ਸੂਚਕਾਂਕ_exprp->unlink_Fr_ਪਿੱਠ()};
                newp->ਜੋੜੋ_Pinsp(argp->exprp()->unlink_Fr_ਪਿੱਠ());
                newp->dtype_ਸੈੱਟ_Void();
            }
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "pop_front" || nodep->name() == "pop_back") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            // Returns element, so method both consumes (reads) and modifies the queue
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READWRITE);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name())};
            newp->dtypeFrom(adtypep->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "push_back" || nodep->name() == "push_front") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckTyped(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), adtypep->subDTypep(), BOTH);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::WRITE);
            AstArg* ਅਚਲ argp = nodep->argsp();
            iterateCheckTyped(nodep, "push value", argp->exprp(), adtypep->subDTypep(), BOTH);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ(nodep->name()),
                                      argp->exprp()->unlink_Fr_ਪਿੱਠ()};
            newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "and" || nodep->name() == "or" || nodep->name() == "xor"
                   || nodep->name() == "sum" || nodep->name() == "product") {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->subDTypep(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                      VCMethod::array_ਵਿਧੀ("r_" + nodep->name())};
            newp->withp(withp);
            newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
            ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ ((newp = ਵਿਧੀ_Call_Array(nodep, adtypep))) {
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported/unknown built-in queue method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
        ਜੇ (newp) {
            newp->protect(ਝੂਠ);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਵਿਧੀ_Call_ਕਤਾਰ_ਸੂਚਕਾਂਕ_ਵਿਅੰਜਕ(Ast_ਵਿਧੀ_Call* nodep) {
        Ast_ਗੰਢ* ਅਚਲ ਸੂਚਕਾਂਕ_exprp = nodep->argsp()->exprp();
        iterateCheckSigned32(nodep, "index", ਸੂਚਕਾਂਕ_exprp, BOTH);
        VL_DANGLING(ਸੂਚਕਾਂਕ_exprp);  // May have been edited
        ਵਾਪਸ nodep->argsp()->exprp();
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Warn_Timing(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ nodep, ਅਚਲ std::string& className) {
        ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
            nodep->v3warn(E_NOTIMING,
                          className << "::" << nodep->prettyName() << "() requires --timing");
        } ਨਹੀਂਤਾਂ ਜੇ (!v3Global.opt.timing().is_ਸੈੱਟ_True()) {
            nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how "
                                               << className << "::" << nodep->name()
                                               << "() should be handled");
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Iface_Ref(Ast_ਵਿਧੀ_Call* nodep, AstIfaceRefDType* adtypep) {
        // ifaceViaCellp() resolves the interface via cellp when ifacep is null,
        // as for a sub-interface selected through a virtual interface handle.
        AstIface* ਅਚਲ ifacep = adtypep->iface_ਰਾਹੀਂ_Cellp();
        UINFO(5, __FUNCTION__ << ":" << nodep);
        ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp
            = VN_CAST(m_memberMap.ਲੱਭੋ_Member(ifacep, nodep->name()), ਗੰਢ_FTask)) {
            UINFO(5, __FUNCTION__ << "AstNodeFTask" << nodep);
            ਜੇ (!modport_Allows_ਵਸਤੂ(nodep, adtypep, ftaskp)) {
                nodep->dtype_ਸੈੱਟ_Void();
                ਵਾਪਸ;
            }
            ਜੇ (adtypep->isVirtual()) {
                ਲਈ (Ast_ਗੰਢ* itemp = ifacep->stmtsp(); itemp; itemp = itemp->nextp()) {
                    ਜੇ (AstVar* ਅਚਲ mvarp = VN_CAST(itemp, Var)) { mvarp->sensIfacep(ifacep); }
                }
            }
            userIterate(ftaskp, ਖਾਲੀ_ਸੂਚਕ);
            ਜੇ (ftaskp->isStatic()) {
                AstArg* ਅਚਲ argsp = nodep->argsp();
                ਜੇ (argsp) argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                Ast_ਗੰਢ_FTask_Ref* newp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (VN_IS(ftaskp, Task)) {
                    newp = ਨਵਾਂ AstTaskRef{nodep->fileline(), VN_AS(ftaskp, Task), argsp};
                } ਨਹੀਂਤਾਂ {
                    newp = ਨਵਾਂ AstFuncRef{nodep->fileline(), VN_AS(ftaskp, Func), argsp};
                }
                newp->classOrPackagep(ifacep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            } ਨਹੀਂਤਾਂ {
                nodep->taskp(ftaskp);
                nodep->dtypeFrom(ftaskp);
                nodep->classOrPackagep(ifacep);
                ਜੇ (VN_IS(ftaskp, Task)) nodep->dtype_ਸੈੱਟ_Void();
                process_FTask_Ref_ਦਲੀਲਾਂ(nodep);
            }
            ਵਾਪਸ;
        }
        nodep->v3error("Member reference from interface to "
                       << nodep->prettyNameQ() << " is not referencing a valid task or function ");
    }
    ਖਾਲੀ handle_Randomize_ਦਲੀਲਾਂ(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ nodep, AstClass* ਅਚਲ classp) {
        ਤਾਰਕਿਕ has_Non_Null_ਦਲੀਲਾਂ = ਝੂਠ;
        AstConst* nullp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            AstVar* randVarp = ਖਾਲੀ_ਸੂਚਕ;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp = argp->exprp();
            ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(exprp, Const)) {
                ਜੇ (constp->ਅੰਕ().isNull()) {
                    nullp = constp;
                    ਜਾਰੀ;
                }
            }
            has_Non_Null_ਦਲੀਲਾਂ = ਸੱਚ;
            AstVar* fromVarp = ਖਾਲੀ_ਸੂਚਕ;  // If it's a method call, the leftmost element
                                         // of the dot hierarchy
            ਜੇ (Ast_ਵਿਧੀ_Call* ਵਿਧੀ_Callp = VN_CAST(nodep, ਵਿਧੀ_Call)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp = ਵਿਧੀ_Callp->fromp();
                ਜਦੋਂ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(fromp, MemberSel)) {
                    fromp = memberSelp->fromp();
                }
                AstVarRef* ਅਚਲ varrefp = VN_AS(fromp, VarRef);
                fromVarp = varrefp->varp();
            }
            ਜੇ (!VN_IS(exprp, VarRef) && !VN_IS(exprp, MemberSel)) {
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
                ਜਾਰੀ;
            }
            ਜਦੋਂ (exprp) {
                ਜੇ (AstMemberSel* ਅਚਲ memberSelp = VN_CAST(exprp, MemberSel)) {
                    randVarp = memberSelp->varp();
                    exprp = memberSelp->fromp();
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (AstVarRef* ਅਚਲ varrefp = VN_CAST(exprp, VarRef)) {
                        randVarp = varrefp->varp();
                    } ਨਹੀਂਤਾਂ {
                        argp->v3warn(
                            E_UNSUPPORTED,
                            "Unsupported: Non-variable expression as 'randomize()' argument");
                        VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
                    }
                    exprp = ਖਾਲੀ_ਸੂਚਕ;
                }
                // All variables in the dot hierarchy must be randomizable
                ਜੇ (randVarp && !randVarp->isRand()) randVarp->rand(VRandAttr::RAND_INLINE);
            }
            // randVarp is now the leftmost element from the dot hierarchy in argp->exprp()
            ਜੇ (randVarp == fromVarp) {
                // The passed in variable is MemberSel'ected from the MethodCall target
            } ਨਹੀਂਤਾਂ ਜੇ (classp->existsMember([&](ਅਚਲ AstClass*, ਅਚਲ AstVar* memberVarp) {
                           ਵਾਪਸ memberVarp == randVarp;
                       })) {
                // The passed in variable is contained in the method call target
            } ਨਹੀਂਤਾਂ {
                // Passed in a constant or complex expression, or the above conditions are not
                // met
                argp->v3error("'randomize()' argument must be a variable contained in "
                              << (fromVarp ? fromVarp->prettyNameQ() : classp->prettyNameQ()));
                VL_DO_DANGLING(argp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), argp);
            }
        }
        ਜੇ (nullp && has_Non_Null_ਦਲੀਲਾਂ) {
            nullp->v3error("Cannot pass more arguments to 'randomize(null)'");
        }
        // A solo 'null' arg is left in place; V3Randomize lowers it into a
        // check-only solve so constraints are validated against the current
        // values of every rand member (IEEE 1800-2023 18.11).
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Class(Ast_ਵਿਧੀ_Call* nodep, AstClassRefDType* adtypep) {
        // No need to width-resolve the class, as it was done when we did the child
        AstClass* ਅਚਲ ਪਹਿਲਾ_classp = adtypep->classp();
        AstWith* withp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->name() == "randomize") {
            VL_RESTORER(m_randomizeFromp);
            m_randomizeFromp = nodep->fromp();
            withp = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->ਲੱਭੋ_Void_DType(),
                                     adtypep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(), adtypep);
            ਲਈ (AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                ਜੇ (argp->exprp()) userIterate(argp->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            }
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newRandomizeFunc(m_memberMap, ਪਹਿਲਾ_classp);
            handle_Randomize_ਦਲੀਲਾਂ(nodep, ਪਹਿਲਾ_classp);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "srandom") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
            V3Randomize::newSRandomFunc(m_memberMap, ਪਹਿਲਾ_classp);
        }
        UASSERT_OBJ(ਪਹਿਲਾ_classp, nodep, "Unlinked");
        ਲਈ (AstClass* classp = ਪਹਿਲਾ_classp; classp;) {
            ਜੇ (nodep->fileline()->timingOn()) {
                ਜੇ (classp->name() == "semaphore" || classp->name() == "process"
                    || VString::startsWith(classp->name(), "mailbox")) {
                    // Find the package the class is in
                    AstPackage* ਅਚਲ packagep = get_ਵਸਤੂ_Package(classp);
                    // Check if it's std
                    ਜੇ (packagep && packagep->name() == "std") {
                        ਜੇ (classp->name() == "process") {
                            ਵਿਧੀ_Call_Warn_Timing(nodep, "process");
                        } ਨਹੀਂਤਾਂ ਜੇ (classp->name() == "semaphore" && nodep->name() == "get") {
                            ਵਿਧੀ_Call_Warn_Timing(nodep, "semaphore");
                        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "put" || nodep->name() == "get"
                                   || nodep->name() == "peek") {
                            ਵਿਧੀ_Call_Warn_Timing(nodep, "mailbox");
                        }
                    }
                }
            }
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, nodep->name()), ਗੰਢ_FTask)) {
                userIterate(ftaskp, ਖਾਲੀ_ਸੂਚਕ);
                ਜੇ (ftaskp->isStatic()) {
                    AstArg* ਅਚਲ argsp = nodep->argsp();
                    ਜੇ (argsp) argsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
                    Ast_ਗੰਢ_FTask_Ref* newp = ਖਾਲੀ_ਸੂਚਕ;
                    // We use m_vup to determine task or function, so that later error checks
                    // for funcref->task and taskref->func will pick up properly
                    ਜੇ (!m_vup) {  // Called as task
                        newp = ਨਵਾਂ AstTaskRef{nodep->fileline(), nodep->name(), argsp};
                    } ਨਹੀਂਤਾਂ {
                        newp = ਨਵਾਂ AstFuncRef{nodep->fileline(), nodep->name(), argsp};
                    }
                    newp->taskp(ftaskp);  // Not passed above as might be func vs task mismatch
                    newp->classOrPackagep(classp);
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                } ਨਹੀਂਤਾਂ {
                    nodep->taskp(ftaskp);
                    nodep->dtypeFrom(ftaskp);
                    nodep->classOrPackagep(classp);
                    ਜੇ (VN_IS(ftaskp, Task)) {
                        ਜੇ (m_vup && m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ)) {
                            nodep->v3error(
                                "Cannot call a task/void-function as a member function: "
                                << nodep->prettyNameQ());
                        }
                        nodep->dtype_ਸੈੱਟ_Void();
                    }
                    ਜੇ (withp) nodep->withp(withp);
                    process_FTask_Ref_ਦਲੀਲਾਂ(nodep);
                }
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "get_randstate") {
                ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
                ਪਹਿਲਾ_classp->ਅਧਾਰ_Most_Classp()->needRNG(ਸੱਚ);
                v3Global.useRandomizeMethods(ਸੱਚ);
                AstCMethodHard* ਅਚਲ newp
                    = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                         VCMethod::RNG_GET_RANDSTATE, ਖਾਲੀ_ਸੂਚਕ};
                newp->usePtr(ਸੱਚ);
                newp->dtype_ਸੈੱਟ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "set_randstate") {
                ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->argsp()->exprp();
                ਪਹਿਲਾ_classp->ਅਧਾਰ_Most_Classp()->needRNG(ਸੱਚ);
                v3Global.useRandomizeMethods(ਸੱਚ);
                AstCMethodHard* ਅਚਲ newp
                    = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                         VCMethod::RNG_ਸੈੱਟ_RANDSTATE, exprp->unlink_Fr_ਪਿੱਠ()};
                newp->usePtr(ਸੱਚ);
                newp->dtype_ਸੈੱਟ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "constraint_mode") {
                v3Global.useRandomizeMethods(ਸੱਚ);
                nodep->dtypep(nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INT));
            }
            classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ;
        }
        {
            VSpellCheck speller;
            ਲਈ (AstClass* classp = ਪਹਿਲਾ_classp; classp;) {
                ਲਈ (Ast_ਗੰਢ* itemp = classp->membersp(); itemp; itemp = itemp->nextp()) {
                    ਜੇ (VN_IS(itemp, ਗੰਢ_FTask)) speller.ਧੱਕੋ_Candidate(itemp->prettyName());
                }
                classp = classp->extendsp() ? classp->extendsp()->classp() : ਖਾਲੀ_ਸੂਚਕ;
            }
            ਅਚਲ string suggest = speller.ਉੱਤਮ_Candidate_ਸੁਨੇਹਾ(nodep->prettyName());
            nodep->v3error("Class method "
                           << nodep->prettyNameQ() << " not found in "
                           << ਪਹਿਲਾ_classp->verilogKwd() << " " << ਪਹਿਲਾ_classp->prettyNameQ()
                           << "\n"
                           << (suggest.ਖਾਲੀ_ਥਾਵਾਂ() ? "" : nodep->fileline()->warn_ਹੋਰ() + suggest));
        }
        nodep->dtype_ਸੈੱਟ_Int();  // Guess on error
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Constraint(Ast_ਵਿਧੀ_Call* nodep, AstConstraintRefDType*) {
        ਜੇ (nodep->name() == "constraint_mode") {
            // IEEE 1800-2023 18.9
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
            ਜੇ (nodep->argsp()) {
                iterateCheckBool(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
                nodep->dtypep(nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INT));
            } ਨਹੀਂਤਾਂ {
                nodep->dtype_ਸੈੱਟ_Void();
            }
            v3Global.useRandomizeMethods(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("No such constraint method " << nodep->prettyNameQ());
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Rand_ਬਹੁਲਕ(Ast_ਵਿਧੀ_Call* nodep) {
        ਵਿਧੀ_Ok_Arguments(nodep, 0, 1);
        // IEEE 1800-2023 18.8
        ਜੇ (nodep->argsp()) {
            iterateCheckBool(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            nodep->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ {
            nodep->dtypep(nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INT));
        }
        v3Global.useRandomizeMethods(ਸੱਚ);
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Unpack(Ast_ਵਿਧੀ_Call* nodep, AstUnpackArrayDType* adtypep) {
        ਗਿਣਤੀ : uint8_t {
            UNKNOWN = 0,
            ARRAY_OR,
            ARRAY_AND,
            ARRAY_XOR,
            ARRAY_ਜੋੜ,
            ARRAY_ਗੁਣਨਫਲ
        } ਵਿਧੀ_Id;

        ਵਿਧੀ_Id = UNKNOWN;
        ਜੇ (nodep->name() == "or") {
            ਵਿਧੀ_Id = ARRAY_OR;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "and") {
            ਵਿਧੀ_Id = ARRAY_AND;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "xor") {
            ਵਿਧੀ_Id = ARRAY_XOR;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "sum") {
            ਵਿਧੀ_Id = ARRAY_ਜੋੜ;
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "product") {
            ਵਿਧੀ_Id = ARRAY_ਗੁਣਨਫਲ;
        }

        ਜੇ (ਵਿਧੀ_Id) {
            AstWith* ਅਚਲ withp
                = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->subDTypep(),
                                   nodep->ਲੱਭੋ_UInt32_DType(), adtypep->subDTypep());
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            ਜੇ (withp) {
                ਵਿਧੀ_Call_LValue_Recurse(nodep, nodep->fromp(), VAccess::READ);
                AstCMethodHard* ਅਚਲ newp
                    = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(),
                                         VCMethod::array_ਵਿਧੀ("r_" + nodep->name())};
                newp->withp(withp);
                newp->dtypeFrom(withp ? withp->dtypep() : adtypep->subDTypep());
                ਜੇ (!nodep->ਪਹਿਲਾ_Abovep()) newp->dtype_ਸੈੱਟ_Void();
                newp->protect(ਝੂਠ);
                newp->did_ਚੌੜਾਈ(ਸੱਚ);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                // TODO use this code for small vectors, otherwise use runtime reduction.
                // Historically we expand all vectors.
                File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
                ਲਈ (ਪੂਰਨ i = 0; i < adtypep->elementsConst(); ++i) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ arrayRef = nodep->fromp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ selector = ਨਵਾਂ AstArraySel{fl, arrayRef, i};
                    ਜੇ (!newp) {
                        newp = selector;
                    } ਨਹੀਂਤਾਂ {
                        ਚੋਣ (ਵਿਧੀ_Id) {
                        ਹਾਲਤ ARRAY_OR: newp = ਨਵਾਂ AstOr{fl, newp, selector}; ਰੋਕੋ;
                        ਹਾਲਤ ARRAY_AND: newp = ਨਵਾਂ AstAnd{fl, newp, selector}; ਰੋਕੋ;
                        ਹਾਲਤ ARRAY_XOR: newp = ਨਵਾਂ AstXor{fl, newp, selector}; ਰੋਕੋ;
                        ਹਾਲਤ ARRAY_ਜੋੜ: newp = ਨਵਾਂ Ast_ਜੋੜੋ{fl, newp, selector}; ਰੋਕੋ;
                        ਹਾਲਤ ARRAY_ਗੁਣਨਫਲ: newp = ਨਵਾਂ AstMul{fl, newp, selector}; ਰੋਕੋ;
                        ਮੂਲ: nodep->v3fatalSrc("bad case");
                        }
                    }
                }
                nodep->replaceWith(newp);
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            }
        } ਨਹੀਂਤਾਂ ਜੇ (AstCMethodHard* newp = ਵਿਧੀ_Call_Array(nodep, adtypep)) {
            newp->protect(ਝੂਠ);
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown built-in array method " << nodep->prettyNameQ());
            nodep->dtypeFrom(adtypep->subDTypep());  // Best guess
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_Event(Ast_ਵਿਧੀ_Call* nodep, AstBasicDType*) {
        // Method call on event
        ਜੇ (nodep->name() == "triggered") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            AstCMethodHard* ਅਚਲ callp = ਨਵਾਂ AstCMethodHard{
                nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), VCMethod::EVENT_IS_TRIGGERED};
            callp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
            nodep->replaceWith(callp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown built-in event method " << nodep->prettyNameQ());
        }
    }
    ਖਾਲੀ ਵਿਧੀ_Call_String(Ast_ਵਿਧੀ_Call* nodep, AstBasicDType*) {
        // Method call on string
        ਜੇ (nodep->name() == "len") {
            // Constant value
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstLenN{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "itoa") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0d"), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "hextoa") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0x"), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "octtoa") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0o"), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "bintoa") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%0b"), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "realtoa") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckReal(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            VL_DO_DANGLING(replaceWithSFormat(nodep, "%g"), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "tolower") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ Ast_ਮੰਜ਼ਿਲ_ਹੇਠਲਾ_N{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "toupper") {
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ Ast_ਮੰਜ਼ਿਲ_ਉੱਤਲਾ_N{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ()};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "compare" || nodep->name() == "icompare") {
            ਅਚਲ ਤਾਰਕਿਕ ignoreCase = nodep->name()[0] == 'i';
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckString(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            AstArg* ਅਚਲ argp = nodep->argsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhs = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhs = argp->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstCompareNN{nodep->fileline(), lhs, rhs, ignoreCase};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "putc") {
            ਵਿਧੀ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            iterateCheckSigned8(nodep, "argument 1", ਵਿਧੀ_Arg(nodep, 1), BOTH);
            AstArg* ਅਚਲ arg0p = nodep->argsp();
            AstArg* ਅਚਲ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ਗੰਢ_Var_Ref* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), VarRef);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = arg0p->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thsp = arg1p->exprp()->unlink_Fr_ਪਿੱਠ();
            AstVarRef* ਅਚਲ varrefp
                = ਨਵਾਂ AstVarRef{nodep->fileline(), fromp->varp(), VAccess::READ};
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
                nodep->fileline(), fromp, ਨਵਾਂ AstPutcN{nodep->fileline(), varrefp, rhsp, thsp}};
            fromp->access(VAccess::WRITE);
            ਧੱਕੋ_Deletep(nodep->backp());
            VL_DO_DANGLING(nodep->backp()->replaceWith(newp), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "getc") {
            ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
            iterateCheckSigned32(nodep, "argument", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            AstArg* ਅਚਲ arg0p = nodep->argsp();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = arg0p->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstGetcN{nodep->fileline(), lhsp, rhsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "substr") {
            ਵਿਧੀ_Ok_Arguments(nodep, 2, 2);
            iterateCheckSigned32(nodep, "argument 0", ਵਿਧੀ_Arg(nodep, 0), BOTH);
            iterateCheckSigned32(nodep, "argument 1", ਵਿਧੀ_Arg(nodep, 1), BOTH);
            AstArg* ਅਚਲ arg0p = nodep->argsp();
            AstArg* ਅਚਲ arg1p = VN_AS(arg0p->nextp(), Arg);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->fromp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = arg0p->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ thsp = arg1p->exprp()->unlink_Fr_ਪਿੱਠ();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstSubstrN{nodep->fileline(), lhsp, rhsp, thsp};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atobin" || nodep->name() == "atohex"
                   || nodep->name() == "atoi" || nodep->name() == "atooct"
                   || nodep->name() == "atoreal") {
            AstAtoN::FmtType fmt;
            ਜੇ (nodep->name() == "atobin") {
                fmt = AstAtoN::ATOBIN;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atohex") {
                fmt = AstAtoN::ATOHEX;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atoi") {
                fmt = AstAtoN::ATOI;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atooct") {
                fmt = AstAtoN::ATOOCT;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "atoreal") {
                fmt = AstAtoN::ATOREAL;
            } ਨਹੀਂਤਾਂ {
                nodep->v3fatalSrc("Bad case");
                fmt = AstAtoN::ATOI;
            }  // dummy assignment to suppress compiler warning
            ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ AstAtoN{nodep->fileline(), nodep->fromp()->unlink_Fr_ਪਿੱਠ(), fmt};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Unknown built-in string method " << nodep->prettyNameQ());
        }
    }
    Ast_ਕਤਾਰ_DType* ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ_By(Ast_ਗੰਢ_DType* ਸੂਚਕਾਂਕ_DTypep) {
        // Return a Queue data type with the specified index, remembering so can use again if
        // needed
        ਜੇ (Ast_ਕਤਾਰ_DType* ਅਚਲ queuep = m_ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ[ਸੂਚਕਾਂਕ_DTypep]) {
            ਵਾਪਸ queuep;
        } ਨਹੀਂਤਾਂ {
            Ast_ਕਤਾਰ_DType* ਅਚਲ newp
                = ਨਵਾਂ Ast_ਕਤਾਰ_DType{ਸੂਚਕਾਂਕ_DTypep->fileline(), ਸੂਚਕਾਂਕ_DTypep, ਖਾਲੀ_ਸੂਚਕ};
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newp);
            m_ਕਤਾਰ_DType_ਸੂਚਕਾਂਕਿਤ[ਸੂਚਕਾਂਕ_DTypep] = newp;
            ਵਾਪਸ newp;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstNew* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        AstClass* classp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ ਸੌਂਪੋ_2 = ਝੂਠ;
        ਜੇ (VN_IS(nodep->backp(), ਸੌਂਪੋ)) {  // assignment case
            ਸੌਂਪੋ_2 = ਸੱਚ;
            Ast_ਗੰਢ* warnp = ਖਾਲੀ_ਸੂਚਕ;
            AstClassRefDType* refp = ਖਾਲੀ_ਸੂਚਕ;

            ਜੇ (nodep->isScoped()) {  // = ClassOrPackage::new
                UASSERT_OBJ(nodep->classOrPackagep(), nodep, "Unlinked classOrPackage");
                warnp = nodep->classOrPackagep();
                ਜੇ (AstClass* ਅਚਲ clsp = VN_CAST(warnp, Class)) {
                    AstClassRefDType* ਅਚਲ adtypep
                        = ਨਵਾਂ AstClassRefDType{nodep->fileline(), clsp, ਖਾਲੀ_ਸੂਚਕ};
                    v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(adtypep);
                    refp = adtypep;
                }
            } ਨਹੀਂਤਾਂ {  // = new
                warnp = m_vup->dtypeNullp();
                refp = m_vup ? VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType) : ਖਾਲੀ_ਸੂਚਕ;
            }
            ਜੇ (!refp) {  // e.g. int a = new;
                nodep->v3error(
                    "new() assignment not legal to non-class "
                    + (VN_IS(warnp, ਗੰਢ_DType)
                           ? ("data type "s + VN_AS(warnp, ਗੰਢ_DType)->prettyDTypeNameQ())
                       : warnp ? warnp->prettyNameQ()
                               : ""));
                nodep->dtypep(m_vup->dtypep());
                ਵਾਪਸ;
            }
            nodep->dtypep(refp);

            classp = refp->classp();
            UASSERT_OBJ(classp, nodep, "Unlinked");
            ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp
                = VN_CAST(m_memberMap.ਲੱਭੋ_Member(classp, "new"), Func)) {
                nodep->taskp(ftaskp);
                nodep->classOrPackagep(classp);
            } ਨਹੀਂਤਾਂ {
                // Either made explicitly or V3LinkDot made implicitly
                classp->v3fatalSrc("Can't find class's new");
            }
        } ਨਹੀਂਤਾਂ {  // super.new case
            // in this case class and taskp() should be properly linked in V3LinkDot.cpp during
            // "super" reference resolution
            classp = VN_CAST(nodep->classOrPackagep(), Class);
            UASSERT_OBJ(classp, nodep, "Unlinked classOrPackagep()");
            UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked taskp()");
        }
        process_FTask_Ref_ਦਲੀਲਾਂ(nodep);
        ਜੇ (!ਸੌਂਪੋ_2) nodep->dtypeFrom(nodep->taskp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstNewCopy* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        AstClassRefDType* ਅਚਲ refp = VN_CAST(m_vup->dtypeNullSkipRefp(), ClassRefDType);
        ਜੇ (!refp) {  // e.g. int a = new;
            nodep->v3error("new() cannot copy from non-class data type "
                           + (m_vup->dtypeNullp() ? m_vup->dtypep()->prettyDTypeNameQ() : ""));
            nodep->dtypep(m_vup->dtypep());
            ਵਾਪਸ;
        }
        nodep->dtypep(refp);
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (!similarDTypeRecurse(nodep->dtypep(), nodep->rhsp()->dtypep())) {
            nodep->rhsp()->v3error("New-as-copier passed different data type '"
                                   << nodep->dtypep()->prettyTypeName() << "' than expected '"
                                   << nodep->rhsp()->dtypep()->prettyTypeName() << "'");
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstNewDynamic* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        AstDynArrayDType* ਅਚਲ adtypep = VN_CAST(m_vup->dtypeNullSkipRefp(), DynArrayDType);
        ਜੇ (!adtypep) {  // e.g. int a = new;
            nodep->v3error(
                "dynamic new() not expected in this context (data type must be dynamic array)");
            ਵਾਪਸ;
        }
        // The AstNodeAssign visitor will be soon be replacing this node, make sure it gets it
        ਜੇ (!VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)) {
            UINFO(1, "Got backp " << nodep->backp());
            nodep->v3error(
                "dynamic new() not expected in this context (expected under an assign)");
            ਵਾਪਸ;
        }
        nodep->dtypep(adtypep);
        ਜੇ (m_vup && m_vup->prelim()) {
            iterateCheckSigned32(nodep, "new() size", nodep->sizep(), BOTH);
        }
        ਜੇ (nodep->rhsp()) {
            iterateCheckTyped(nodep, "Dynamic array new RHS", nodep->rhsp(), adtypep, BOTH);
            // A keyed or defaulted initializer became a constructor, which takes the size
            AstConsDynArray* ਅਚਲ consp = VN_CAST(nodep->rhsp(), ConsDynArray);
            ਜੇ (consp && (consp->defaultp() || (consp->itemsp() && consp->itemsp()->keyp()))) {
                consp->sizep(nodep->sizep()->unlink_Fr_ਪਿੱਠ());
                nodep->replaceWith(consp->unlink_Fr_ਪਿੱਠ());
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_Tagged_ਵਿਅੰਜਕ* nodep) override {
        // Tagged union expressions are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged union");
        // Set a placeholder type to allow further processing
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        userIterateChildren(nodep, m_vup);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Tagged_ਨਮੂਨਾ* nodep) override {
        // Tagged patterns are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: tagged pattern");
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        userIterateChildren(nodep, m_vup);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਨਮੂਨਾ_Var* nodep) override {
        // Pattern variable bindings are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern variable");
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਨਮੂਨਾ_ਤਾਰਾ* nodep) override {
        // Pattern wildcards are currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: pattern wildcard");
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
    }
    ਖਾਲੀ ਦੇਖੋ(AstMatches* nodep) override {
        // Matches operator is currently unsupported
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: matches operator");
        // Matches returns a boolean
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        userIterateChildren(nodep, m_vup);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਨਮੂਨਾ* nodep) override {
        ਜੇ (m_ਵਿੱਚ_Param_Override && !nodep->ਬੱਚਾ_DTypep() && !m_vup->dtypeNullp()) {
            // The specialized module types an override pattern, so only type its members here
            VL_RESTORER(m_ਵਿੱਚ_Param_Override);
            ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                m_ਵਿੱਚ_Param_Override = ਝੂਠ;
                user_Iterate_And_ਅਗਲਾ(patp->repp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                // Struct member names and type keys need no typing
                ਜੇ (!VN_IS(patp->keyp(), ਪਾਠ_2) && !VN_IS(patp->keyp(), ਗੰਢ_DType)) {
                    user_Iterate_And_ਅਗਲਾ(patp->keyp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                }
                ਲਈ (Ast_ਗੰਢ *nextp, *valuep = patp->lhssp(); valuep; valuep = nextp) {
                    nextp = valuep->nextp();
                    // A nested pattern stays untyped, any other value is typed now
                    m_ਵਿੱਚ_Param_Override = VN_IS(valuep, ਨਮੂਨਾ);
                    userIterate(valuep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                }
            }
            ਵਾਪਸ;
        }
        // Any other pattern is typed here, so its members aren't override patterns
        VL_RESTORER(m_ਵਿੱਚ_Param_Override);
        m_ਵਿੱਚ_Param_Override = ਝੂਠ;
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        UINFO(9, "PATTERN " << nodep);
        ਜੇ (nodep->ਬੱਚਾ_DTypep()) {  // data_type '{ pattern }
            nodep->dtypep(iterateEditMoveDTypep(nodep, nodep->subDTypep()));
        }
        ਜੇ (!nodep->dtypep() && m_vup->dtypeNullp()) {  // Get it from parent assignment/pin/etc
            nodep->dtypep(m_vup->dtypep());
        }
        Ast_ਗੰਢ_DType* dtypep = nodep->dtypep();
        ਜੇ (!dtypep) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported/Illegal: Assignment pattern"
                                         " member not underneath a supported construct: "
                                             << nodep->backp()->prettyTypeName());

            ਜੇ (nodep->backp() && (VN_IS(nodep->backp(), Eq) || VN_IS(nodep->backp(), Neq)))
                ਵਾਪਸ;
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }
        {
            dtypep = dtypep->skipRefp();
            nodep->dtypep(dtypep);
            UINFO(9, "  dtypep " << dtypep);
            nodep->dtypep(dtypep);
            ਜੇ (VN_IS(dtypep, UnionDType)) {
                nodep->v3error("Assignment pattern cannot be used for union"
                               " (IEEE 1800-2023 10.9)");
                ਵਾਪਸ;
            }
            // Determine replication count, and replicate initial value as
            // widths need to be individually determined
            ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ਅਚਲ ਪੂਰਨ times = ਦੇਖੋ_Pat_Member_Rep(patp);
                ਲਈ (ਪੂਰਨ i = 1; i < times; i++) {
                    AstPatMember* ਅਚਲ newp = patp->clone_ਰੁੱਖ(ਝੂਠ);
                    patp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
                    // This loop will see the new elements as part of nextp()
                }
            }
            // Convert any PatMember with multiple items to multiple PatMembers
            ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
                 patp = VN_AS(patp->nextp(), PatMember)) {
                ਜੇ (patp->lhssp()->nextp()) {
                    // Can't just addNext, as would add to end of all members.
                    // So detach, add next and reattach
                    VNRelinker relinkHandle;
                    patp->unlink_Fr_ਪਿੱਠ(&relinkHandle);
                    ਜਦੋਂ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ movep = VN_AS(patp->lhssp()->nextp(), ਗੰਢ_ਵਿਅੰਜਕ)) {
                        movep->unlink_Fr_ਪਿੱਠ();  // Not unlinkFrBackWithNext, just one
                        Ast_ਗੰਢ* newkeyp = ਖਾਲੀ_ਸੂਚਕ;
                        ਜੇ (patp->keyp()) newkeyp = patp->keyp()->clone_ਰੁੱਖ(ਸੱਚ);
                        AstPatMember* ਅਚਲ newp
                            = ਨਵਾਂ AstPatMember{patp->fileline(), movep, newkeyp, ਖਾਲੀ_ਸੂਚਕ};
                        patp->ਜੋੜੋ_ਅਗਲਾ(newp);
                    }
                    relinkHandle.relink(patp);
                }
            }
            AstPatMember* defaultp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;) {
                AstPatMember* ਅਚਲ nextp = VN_AS(patp->nextp(), PatMember);
                ਜੇ (patp->isDefault()) {
                    ਜੇ (defaultp) {
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(defaultp), defaultp);
                        nodep->v3error("Multiple '{ default: } clauses");
                    }
                    defaultp = patp;
                    patp->unlink_Fr_ਪਿੱਠ();
                }
                patp = nextp;
            }
            ਜਦੋਂ (ਅਚਲ AstConstDType* ਅਚਲ vdtypep = VN_CAST(dtypep, ConstDType)) {
                dtypep = vdtypep->subDTypep()->skipRefp();
            }

            userIterate(dtypep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());

            ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ vdtypep = VN_CAST(dtypep, StructDType)) {
                ਨਮੂਨਾ_Struct(nodep, vdtypep, defaultp);
            } ਨਹੀਂਤਾਂ ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ vdtypep = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                ਨਮੂਨਾ_Array(nodep, vdtypep, defaultp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, AssocArrayDType)) {
                AstConsAssoc* ਅਚਲ consp = ਨਵਾਂ AstConsAssoc{nodep->fileline()};
                ਨਮੂਨਾ_Keyed<Ast_Cons_Assoc_ਵਸਤੂ>(nodep, consp, dtypep, defaultp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, WildcardArrayDType)) {
                AstConsWildcard* ਅਚਲ consp = ਨਵਾਂ AstConsWildcard{nodep->fileline()};
                ਨਮੂਨਾ_Keyed<Ast_Cons_Wildcard_ਵਸਤੂ>(nodep, consp, dtypep, defaultp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, DynArrayDType) || VN_IS(dtypep, ਕਤਾਰ_DType)) {
                ਨਮੂਨਾ_Dyn_Array_Or_ਕਤਾਰ(nodep, dtypep, defaultp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, BasicDType) && VN_AS(dtypep, BasicDType)->isRanged()) {
                ਨਮੂਨਾ_Basic(nodep, dtypep, defaultp);
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Assignment pattern applies against non-struct/union data type: "
                        << dtypep->prettyDTypeNameQ());
                nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
            }

            // Done with the Pattern
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਜੇ (defaultp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(defaultp), defaultp);
        }
    }
    ਖਾਲੀ ਨਮੂਨਾ_Struct(Ast_ਨਮੂਨਾ* nodep, AstStructDType* vdtypep, AstPatMember* defaultp) {
        // Due to "default" and tagged patterns, we need to determine
        // which member each AstPatMember corresponds to before we can
        // determine the dtypep for that PatMember's value, and then
        // width the initial value appropriately.
        ਵਰਤੋਂ PatMap = std::map<ਅਚਲ AstMemberDType*, AstPatMember*>;
        PatMap patmap;  // Store member: value
        DTypeMap dtypemap;  // Store data_type: default_value
        {
            ਅਚਲ AstMemberDType* memp = vdtypep->membersp();
            AstPatMember* patp = VN_CAST(nodep->itemsp(), PatMember);
            ਜਦੋਂ (patp) {
                ਕਰੋ {
                    ਜੇ (patp->keyp()) {
                        // '{member:value} or '{data_type: default_value}
                        ਜੇ (ਅਚਲ Ast_ਪਾਠ* textp = VN_CAST(patp->keyp(), ਪਾਠ_2)) {
                            // member: value
                            memp = VN_CAST(m_memberMap.ਲੱਭੋ_Member(vdtypep, textp->ਪਾਠ()),
                                           MemberDType);
                            ਜੇ (!memp) {
                                patp->keyp()->v3error("Assignment pattern key '"
                                                      << textp->ਪਾਠ() << "' not found as member");
                                ਰੋਕੋ;
                            } ਨਹੀਂਤਾਂ {
                                ਅਚਲ std::pair<PatMap::iterator, ਤਾਰਕਿਕ> ret
                                    = patmap.emplace(memp, patp);
                                ਜੇ (!ret.ਦੂਜਾ) {
                                    patp->v3error("Assignment pattern contains duplicate entry: "
                                                  << VN_AS(patp->keyp(), ਪਾਠ_2)->ਪਾਠ());
                                }
                                memp = VN_AS(memp->nextp(), MemberDType);
                            }
                        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_DType* nodedtypep = VN_CAST(patp->keyp(), ਗੰਢ_DType)) {
                            // data_type: default_value
                            userIterate(nodedtypep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                            ਅਚਲ string dtype = nodedtypep->dtypep()->prettyDTypeName(ਸੱਚ);
                            // Override stored default_value
                            dtypemap[dtype] = patp;
                        } ਨਹੀਂਤਾਂ {
                            // Undefined pattern
                            patp->keyp()->v3error(
                                "Assignment pattern key not supported/understood: "
                                << patp->keyp()->prettyTypeName());
                        }
                    } ਨਹੀਂਤਾਂ ਜੇ (memp) {
                        // constant expr
                        ਅਚਲ std::pair<PatMap::iterator, ਤਾਰਕਿਕ> ret = patmap.emplace(memp, patp);
                        ਜੇ (!ret.ਦੂਜਾ) {
                            patp->v3error("Assignment pattern contains duplicate entry: "
                                          << VN_AS(patp->keyp(), ਪਾਠ_2)->ਪਾਠ());
                        }
                        memp = VN_AS(memp->nextp(), MemberDType);
                    } ਨਹੀਂਤਾਂ {
                        patp->v3error(
                            "Assignment pattern contains more entries than structure members");
                    }
                } ਜਦੋਂ (ਝੂਠ);

                //ਅਗਲਾ
                ਜੇ (patp) patp = VN_AS(patp->nextp(), PatMember);
            }
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (vdtypep->packed()) {
            ਲਈ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = patmap.ਲੱਭੋ(memp);
                ਜੇ (it == patmap.ਸਿਰਾ()) {  // default or default_type assignment
                    ਜੇ (AstStructDType* ਅਚਲ memp_nested_vdtypep
                        = VN_CAST(memp->virtRefDTypep(), StructDType)) {
                        newp = nestedvalue_Concat_ਨਮੂਨਾ_Struct(memp_nested_vdtypep, defaultp, newp,
                                                               nodep, dtypemap);
                    } ਨਹੀਂਤਾਂ {
                        AstPatMember* ਅਚਲ patp
                            = default_Patp_ਨਮੂਨਾ_Struct(nodep, memp, defaultp, dtypemap);
                        newp = ਮੁੱਲ_Concat_ਨਮੂਨਾ_Struct(patp, newp, memp, nodep);
                        ਜੇ (patp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(patp), patp);
                    }
                } ਨਹੀਂਤਾਂ {  // member assignment
                    newp = ਮੁੱਲ_Concat_ਨਮੂਨਾ_Struct(it->ਦੂਜਾ, newp, memp, nodep);
                }
            }
        } ਨਹੀਂਤਾਂ {  // Unpacked
            AstConsStruct* ਅਚਲ consp = ਨਵਾਂ AstConsStruct{nodep->fileline(), vdtypep};
            ਲਈ (AstMemberDType* memp = vdtypep->membersp(); memp;
                 memp = VN_AS(memp->nextp(), MemberDType)) {
                ਅਚਲ ਸਵੈਚਾਲਿਤ it = patmap.ਲੱਭੋ(memp);
                AstPatMember* patp = ਖਾਲੀ_ਸੂਚਕ;
                ਜੇ (it == patmap.ਸਿਰਾ()) {  // Default or default_type assignment
                    patp = default_Patp_ਨਮੂਨਾ_Struct(nodep, memp, defaultp, dtypemap);
                    ਧੱਕੋ_Deletep(patp);  // patp used below
                    patp = defaultPatp_forDType(patp, memp->virtRefDTypep(), dtypemap);
                    ਧੱਕੋ_Deletep(patp);  // patp used below
                } ਨਹੀਂਤਾਂ {
                    patp = it->ਦੂਜਾ;  // Member assignment
                }
                patp->dtypep(memp);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
                consp->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Cons_Struct_ਵਸਤੂ{patp->fileline(), memp, valuep});
            }
            newp = consp;
        }
        ਜੇ (newp) {
            nodep->replaceWith(newp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Assignment pattern with no members");
        }
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* nestedvalue_Concat_ਨਮੂਨਾ_Struct(AstStructDType* memp_vdtypep,
                                                 AstPatMember* defaultp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp,
                                                 Ast_ਨਮੂਨਾ* nodep, ਅਚਲ DTypeMap& dtypemap) {
        ਲਈ (AstMemberDType* memp_nested = memp_vdtypep->membersp(); memp_nested;
             memp_nested = VN_AS(memp_nested->nextp(), MemberDType)) {
            ਜੇ (AstStructDType* ਅਚਲ memp_multinested_vdtypep
                = VN_CAST(memp_nested->virtRefDTypep(), StructDType)) {
                newp = nestedvalue_Concat_ਨਮੂਨਾ_Struct(memp_multinested_vdtypep, defaultp, newp,
                                                       nodep, dtypemap);
            } ਨਹੀਂਤਾਂ {
                AstPatMember* ਅਚਲ patp
                    = default_Patp_ਨਮੂਨਾ_Struct(nodep, memp_nested, defaultp, dtypemap);
                newp = ਮੁੱਲ_Concat_ਨਮੂਨਾ_Struct(patp, newp, memp_nested, nodep);
                ਜੇ (patp) VL_DO_DANGLING(ਧੱਕੋ_Deletep(patp), patp);
            }
        }
        ਵਾਪਸ newp;
    }

    AstPatMember* default_Patp_ਨਮੂਨਾ_Struct(Ast_ਨਮੂਨਾ* nodep, AstMemberDType* memp,
                                            AstPatMember* defaultp, ਅਚਲ DTypeMap& dtypemap) {
        ਅਚਲ string memp_DType = memp->virtRefDTypep()->prettyDTypeName(ਸੱਚ);
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = dtypemap.ਲੱਭੋ(memp_DType);
        ਜੇ (it != dtypemap.ਸਿਰਾ()) {
            // default_value for data_type
            ਵਾਪਸ it->ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ);
        }
        ਜੇ (defaultp) {
            // default_value for any unmatched member yet
            ਵਾਪਸ defaultp->clone_ਰੁੱਖ(ਝੂਠ);
        }
        ਜੇ (memp->valuep()) {
            ਵਾਪਸ ਨਵਾਂ AstPatMember{nodep->fileline(),
                                    VN_AS(memp->valuep()->clone_ਰੁੱਖ(ਝੂਠ), ਗੰਢ_ਵਿਅੰਜਕ),
                                    ਨਵਾਂ Ast_ਪਾਠ{nodep->fileline(), memp->name()}, ਖਾਲੀ_ਸੂਚਕ};
        }
        nodep->v3error("Assignment pattern missed initializing elements: "
                       << memp->virtRefDTypep()->prettyDTypeNameQ() << " " << memp->prettyNameQ());
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਮੁੱਲ_Concat_ਨਮੂਨਾ_Struct(AstPatMember* patp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp,
                                           AstMemberDType* memp, Ast_ਨਮੂਨਾ* nodep) {
        ਜੇ (patp) {
            patp->dtypep(memp);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
            ਜੇ (!newp) {
                newp = valuep;
            } ਨਹੀਂਤਾਂ {
                AstConcat* ਅਚਲ concatp = ਨਵਾਂ AstConcat{patp->fileline(), newp, valuep};
                // Have resolved widths, no reason to warn about widths on the concat itself
                newp->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::WIDTHCONCAT, ਸੱਚ);
                valuep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::WIDTHCONCAT, ਸੱਚ);
                newp = concatp;
                newp->dtypeChgSigned(nodep->dtypep()->numeric());
            }
        }
        ਵਾਪਸ newp;
    }

    AstPatMember* defaultPatp_forDType(AstPatMember* defaultp, Ast_ਗੰਢ_DType* elemDTypep,
                                       ਅਚਲ DTypeMap& dtypemap = DTypeMap{}) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = defaultp->lhssp()->clone_ਰੁੱਖ(ਝੂਠ);
        Ast_ਗੰਢ_DType* ਅਚਲ elemDTypeSkipRefp = elemDTypep->skipRefp();
        ਅਚਲ AstStructDType* ਅਚਲ structp = VN_CAST(elemDTypeSkipRefp, StructDType);
        ਅਚਲ ਤਾਰਕਿਕ unpackedStruct = structp && !structp->packed();

        ਜੇ (!VN_IS(elemDTypeSkipRefp, UnpackArrayDType) && !unpackedStruct) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            ਵਾਪਸ defaultp->clone_ਰੁੱਖ(ਝੂਠ);
        }
        ਜੇ (VN_IS(valuep, ਨਮੂਨਾ)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            ਵਾਪਸ defaultp->clone_ਰੁੱਖ(ਝੂਠ);
        }
        ਜੇ (!valuep->dtypep()) userIterate(valuep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਤਾਰਕਿਕ ਸਾਰਾ_Element = unpackedStruct;
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ ਮੁੱਲ_DTypep = valuep->dtypep()) {
            // An array value only if of the matching type, else it applies to its elements
            // (IEEE 1800-2023 10.9.1)
            ਸਾਰਾ_Element = unpackedStruct ? !ਮੁੱਲ_DTypep->skipRefp()->isIntegralOrPacked()
                                          : ਮੁੱਲ_DTypep->similarDType(elemDTypeSkipRefp);
        }
        ਜੇ (ਸਾਰਾ_Element) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            ਵਾਪਸ defaultp->clone_ਰੁੱਖ(ਝੂਠ);
        }

        AstPatMember* ਅਚਲ nestedDefaultp
            = ਨਵਾਂ AstPatMember{defaultp->fileline(), valuep, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
        nestedDefaultp->isDefault(ਸੱਚ);
        // Propagate the outer 'data_type: value' entries into the nested aggregate
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : dtypemap) nestedDefaultp->ਜੋੜੋ_ਅਗਲਾ(ਇੰਦਰਾਜ.ਦੂਜਾ->clone_ਰੁੱਖ(ਝੂਠ));
        Ast_ਨਮੂਨਾ* ਅਚਲ recursivePatternp = ਨਵਾਂ Ast_ਨਮੂਨਾ{defaultp->fileline(), nestedDefaultp};
        ਵਾਪਸ ਨਵਾਂ AstPatMember{defaultp->fileline(), recursivePatternp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
    }

    // Return the value of a pattern default for elements of the given type, or nullptr if no
    // default. For aggregate elements, it applies to their elements (IEEE 1800-2023 10.9.1).
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਨਮੂਨਾ_Default_Valuep(AstPatMember* defaultp, Ast_ਗੰਢ_DType* elemDTypep) {
        ਜੇ (!defaultp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        AstPatMember* ਅਚਲ elemDefaultp = defaultPatp_forDType(defaultp, elemDTypep);
        elemDefaultp->dtypep(elemDTypep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(elemDefaultp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(elemDefaultp), elemDefaultp);
        ਵਾਪਸ valuep;
    }
    // An unpacked array concatenation, with each item an element, or an array of the elements
    // spliced in (IEEE 1800-2023 10.10)
    ਖਾਲੀ ਨਮੂਨਾ_Concat_Array(Ast_ਨਮੂਨਾ* nodep, AstUnpackArrayDType* arrayDtp) {
        ਅਚਲ VNumRange range = arrayDtp->declRange();
        ਅਚਲ ਪੂਰਨ elements = range.elements();
        AstConsArray* ਅਚਲ newp = ਨਵਾਂ AstConsArray{nodep->fileline(), arrayDtp};
        ਪੂਰਨ entn = 0;  // Number of elements given so far, from the left
        ਤਾਰਕਿਕ error = ਝੂਠ;  // Reported an error, so the value is not complete
        ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
            ਅਚਲ uint64_t ਸੂਚਕਾਂਕ = range.ascending() ? entn : elements - 1 - entn;
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ spliceDtp
                = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
            ਜੇ (spliceDtp && spliceDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                entn += spliceDtp->elementsConst();
                newp->splice_ਮੁੱਲਾਂ(ਸੂਚਕਾਂਕ, valuep);
                ਜਾਰੀ;
            }
            ਅਚਲ VCastable castable = Ast_ਗੰਢ::computeCastable(
                arrayDtp->subDTypep()->skipRefp(), valuep->dtypep()->skipRefp(), valuep);
            ਜੇ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE) {
                valuep->v3error("Unpacked array concatenation item is neither an element nor"
                                " an array of elements of its target (IEEE 1800-2023 10.10)"
                                << warnDTypes(valuep, arrayDtp->subDTypep(), valuep->dtypep(),
                                              "Element", "Item"));
                error = ਸੱਚ;
            }
            newp->valuep(ਸੂਚਕਾਂਕ, valuep);
            ++entn;
        }
        ਜੇ (!error && entn != elements) {
            nodep->v3error("Unpacked array concatenation has "
                           << entn << " elements, but its target has " << elements
                           << " (IEEE 1800-2023 10.10)");
            error = ਸੱਚ;
        }
        ਜੇ (error) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(newp), newp);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
        } ਨਹੀਂਤਾਂ {
            nodep->replaceWith(newp);
        }
    }
    ਖਾਲੀ ਨਮੂਨਾ_Array(Ast_ਨਮੂਨਾ* nodep, Ast_ਗੰਢ_Array_DType* arrayDtp, AstPatMember* defaultp) {
        ਜੇ (nodep->itemsp() && VN_AS(nodep->itemsp(), PatMember)->isConcat()) {
            ਨਮੂਨਾ_Concat_Array(nodep, VN_AS(arrayDtp, UnpackArrayDType));
            ਵਾਪਸ;
        }
        ਅਚਲ VNumRange range = arrayDtp->declRange();
        PatVecMap patmap = pat_ਸਦਿਸ਼_Map(nodep, range);
        UINFO(9, "ent " << range.ਖੱਬੇ() << " to " << range.ਸੱਜੇ());
        Ast_ਗੰਢ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਤਾਰਕਿਕ missed = ਝੂਠ;  // Some element missed, so not a complete value
        ਲਈ (ਪੂਰਨ entn = 0, ent = range.ਖੱਬੇ(); entn < range.elements();
             ++entn, ent += range.ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = patmap.ਲੱਭੋ(ent);
            ਜੇ (it == patmap.ਸਿਰਾ()) {
                ਜੇ (defaultp) {
                    valuep = ਨਮੂਨਾ_Default_Valuep(defaultp, arrayDtp->subDTypep());
                } ਨਹੀਂਤਾਂ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                    missed = ਸੱਚ;
                }
            } ਨਹੀਂਤਾਂ {
                AstPatMember* ਅਚਲ patp = it->ਦੂਜਾ;
                patmap.erase(it);
                patp->dtypep(arrayDtp->subDTypep());
                valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
                // Only an unpacked array concatenation splices in arrays
                ਅਚਲ AstUnpackArrayDType* ਅਚਲ memberDtp
                    = VN_CAST(valuep->dtypep()->skipRefp(), UnpackArrayDType);
                ਜੇ (memberDtp && memberDtp->subDTypep()->similarDType(arrayDtp->subDTypep())) {
                    valuep->v3error("Assignment pattern member is an array of the elements, not"
                                    " an element (IEEE 1800-2023 10.9.1)"
                                    << warnDTypes(valuep, arrayDtp->subDTypep(), memberDtp,
                                                  "Element", "Pattern member")
                                    << "\n"
                                    << valuep->warn_ਹੋਰ()
                                    << "... Suggest use an unpacked array concatenation {...} to "
                                       "splice arrays");
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
                    valuep = ਨਵਾਂ AstConst{patp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
                }
            }

            ਜੇ (valuep) {
                ਜੇ (VN_IS(arrayDtp, UnpackArrayDType)) {
                    ਜੇ (!newp) newp = ਨਵਾਂ AstConsArray{nodep->fileline(), arrayDtp};
                    VN_AS(newp, ConsArray)->valuep(ent - range.lo(), valuep);
                } ਨਹੀਂਤਾਂ {  // Packed. Convert to concat for now.
                    ਜੇ (!newp) {
                        newp = valuep;
                    } ਨਹੀਂਤਾਂ {
                        AstConcat* ਅਚਲ concatp
                            = ਨਵਾਂ AstConcat{valuep->fileline(), VN_AS(newp, ਗੰਢ_ਵਿਅੰਜਕ), valuep};
                        newp = concatp;
                        newp->dtypeChgSigned(nodep->dtypep()->numeric());
                    }
                }
            }
        }
        ਜੇ (!patmap.ਖਾਲੀ_ਥਾਵਾਂ()) nodep->v3error("Assignment pattern with too many elements");
        ਜੇ (missed && VN_IS(newp, ConsArray)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(newp), newp);
            newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
        }
        ਜੇ (newp) {
            nodep->replaceWith(newp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    // Return the key of an array pattern member, which must be a constant expression,
    // or nullptr after reporting an error
    AstConst* ਨਮੂਨਾ_Const_Keyp(AstPatMember* patp) {
        ਜੇ (VN_IS(patp->keyp(), ਗੰਢ_DType)) {
            patp->keyp()->v3warn(E_UNSUPPORTED,
                                 "Unsupported: Data-type key in array assignment pattern");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (patp->ਕੁੰਜੀ_Exprp()) V3Const::constifyParamsNoWarnEdit(patp->ਕੁੰਜੀ_Exprp());  // May change
        AstConst* ਅਚਲ constp = VN_CAST(patp->ਕੁੰਜੀ_Exprp(), Const);
        ਜੇ (!constp) {
            patp->keyp()->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
        }
        ਵਾਪਸ constp;
    }
    // Return the key of a pattern member of a non-aggregate key type, converted to the key type,
    // unlinked, or nullptr after reporting an error
    AstConst* ਨਮੂਨਾ_Scalar_Keyp(AstPatMember* patp, Ast_ਗੰਢ_DType* ਕੁੰਜੀ_DTypep) {
        ਜੇ (!patp->keyp()) {
            patp->v3error("Missing pattern key (need an expression then a ':')");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // An integral key is evaluated in the context of the key type, as an index converted to
        // it (IEEE 1800-2023 7.8.4), but not truncated before checking it for X or Z
        ਜੇ (ਕੁੰਜੀ_DTypep->skipRefp()->isIntegralOrPacked() && patp->ਕੁੰਜੀ_Exprp()) {
            userIterate(patp->ਕੁੰਜੀ_Exprp(), ਚੌੜਾਈ_VP{ਕੁੰਜੀ_DTypep, PRELIM}.p());
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(patp->ਕੁੰਜੀ_Exprp()->ਚੌੜਾਈ(), ਕੁੰਜੀ_DTypep->ਚੌੜਾਈ());
            Ast_ਗੰਢ_DType* ਅਚਲ contextDTypep
                = ਕੁੰਜੀ_DTypep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, ਚੌੜਾਈ, ਕੁੰਜੀ_DTypep->numeric(), ਸੱਚ);
            iterateCheck(patp, "key", patp->ਕੁੰਜੀ_Exprp(), CONTEXT_DET, FINAL, contextDTypep,
                         EXTEND_EXP, ਝੂਠ);
        }
        AstConst* ਅਚਲ constp = ਨਮੂਨਾ_Const_Keyp(patp);
        ਜੇ (!constp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (constp->ਅੰਕ().isFourState()) {
            patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // Convert to the key type. The conversion is inserted above 'constp', so take the key
        // from the member again rather than from what iterateCheck returns.
        iterateCheck(patp, "key", constp, CONTEXT_DET, FINAL, ਕੁੰਜੀ_DTypep, EXTEND_EXP, ਝੂਠ);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ convp = patp->ਕੁੰਜੀ_Exprp();
        // Fold the conversion, so the key is a Const again
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ keyp = V3Const::constifyEdit(convp);
        UASSERT_OBJ(VN_IS(keyp, Const), patp, "Key not constant after conversion");
        ਵਾਪਸ VN_AS(keyp->unlink_Fr_ਪਿੱਠ(), Const);
    }
    // Return the key of a keyed pattern member, for the kind of constructor, unlinked, or nullptr
    // after reporting an error. Only associative arrays can have aggregate keys.
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਨਮੂਨਾ_Keyp(AstConsAssoc*, AstPatMember* patp, Ast_ਗੰਢ_DType* arrayDtp) {
        Ast_ਗੰਢ_DType* ਅਚਲ ਕੁੰਜੀ_DTypep = VN_AS(arrayDtp, AssocArrayDType)->ਕੁੰਜੀ_DTypep();
        ਜੇ (!ਕੁੰਜੀ_DTypep->skipRefp()->isAggregateType() || !patp->ਕੁੰਜੀ_Exprp()) {
            ਵਾਪਸ ਨਮੂਨਾ_Scalar_Keyp(patp, ਕੁੰਜੀ_DTypep);
        }
        // An aggregate key is folded, and constant if made of constant values only
        iterateCheckTyped(patp, "key", patp->ਕੁੰਜੀ_Exprp(), ਕੁੰਜੀ_DTypep, BOTH);
        V3Const::constifyParamsNoWarnEdit(patp->ਕੁੰਜੀ_Exprp());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ foldedp = patp->ਕੁੰਜੀ_Exprp();
        ਜੇ (!foldedp->is_Constant_ਰੁੱਖ()) {
            foldedp->v3error(
                "Assignment pattern key is not a constant expression (IEEE 1800-2023 10.9)");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (foldedp->exists([](ਅਚਲ AstConst* constp) { ਵਾਪਸ constp->ਅੰਕ().isFourState(); })) {
            foldedp->v3error("Assignment pattern key cannot contain X or Z");
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ foldedp->unlink_Fr_ਪਿੱਠ();
    }
    AstConst* ਨਮੂਨਾ_Keyp(AstConsWildcard*, AstPatMember* patp, Ast_ਗੰਢ_DType* arrayDtp) {
        ਵਾਪਸ ਨਮੂਨਾ_Scalar_Keyp(patp, arrayDtp->ਲੱਭੋ_String_DType());
    }
    AstConst* ਨਮੂਨਾ_Keyp(AstConsDynArray*, AstPatMember* patp, Ast_ਗੰਢ_DType* arrayDtp) {
        // Converted to longint, not int, so a key wider than 32 bits is out of bounds rather than
        // truncated to a valid index
        ਵਾਪਸ ਨਮੂਨਾ_Scalar_Keyp(patp, arrayDtp->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::LONGINT));
    }
    // Sort constructor items by key, so equal values have equal trees, and report duplicate
    // keys (IEEE 1800-2023 10.9.1). Keys are compared after conversion to the key type, e.g.
    // the wildcard keys "CCC" and 23'h434343 are equal.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਵਸਤੂ, ਕਿਸਮ_ਨਾਂ T_Cons>
    ਖਾਲੀ ਛਾਂਟੋ_Cons_ਚੀਜ਼ਾਂ(T_Cons* nodep) {
        std::ਸਦਿਸ਼<T_ਵਸਤੂ*> itemps;
        ਜਦੋਂ (T_ਵਸਤੂ* ਅਚਲ itemp = nodep->itemsp()) {
            itemp->unlink_Fr_ਪਿੱਠ();
            itemps.ਧੱਕੋ_ਪਿੱਠ(itemp);
        }
        ਅਚਲ ਸਵੈਚਾਲਿਤ ਕੁੰਜੀ_ਦਾ = [](ਅਚਲ T_ਵਸਤੂ* itemp) -> ਅਚਲ V3_ਸੰਖਿਆ& {
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ keyp = itemp->keyp();
            ਵਾਪਸ VN_AS(keyp, Const)->ਅੰਕ();
        };
        std::stable_ਛਾਂਟੋ(itemps.begin(), itemps.ਸਿਰਾ(), [&](ਅਚਲ T_ਵਸਤੂ* ap, ਅਚਲ T_ਵਸਤੂ* bp) {
            // Any total order will do, it need not match the runtime order of the keys
            ਵਾਪਸ ਕੁੰਜੀ_ਦਾ(ap).isLtXZ(ਕੁੰਜੀ_ਦਾ(bp));
        });
        ਅਚਲ T_ਵਸਤੂ* firstp = ਖਾਲੀ_ਸੂਚਕ;  // First item with the current key
        ਲਈ (T_ਵਸਤੂ* ਅਚਲ itemp : itemps) {
            ਜੇ (firstp && ਕੁੰਜੀ_ਦਾ(firstp).isCaseEq(ਕੁੰਜੀ_ਦਾ(itemp))) {
                // Sort is stable, so 'firstp' is the first one in the source
                itemp->keyp()->v3error("Assignment pattern key used multiple times"
                                       " (IEEE 1800-2023 10.9.1)\n"
                                       << itemp->keyp()->warnContextPrimary() << '\n'
                                       << firstp->keyp()->warnOther()
                                       << "... Location of first use\n"
                                       << firstp->keyp()->warnContextSecondary());
            } ਨਹੀਂਤਾਂ {
                firstp = itemp;
            }
            nodep->ਜੋੜੋ_Itemsp(itemp);
        }
    }
    // Convert a keyed pattern into the constructor 'consp' of an associative, wildcard or
    // dynamic array
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਵਸਤੂ, ਕਿਸਮ_ਨਾਂ T_Cons>
    ਖਾਲੀ ਨਮੂਨਾ_Keyed(Ast_ਨਮੂਨਾ* nodep, T_Cons* consp, Ast_ਗੰਢ_DType* arrayDtp,
                      AstPatMember* defaultp) {
        consp->dtypeFrom(arrayDtp);
        consp->defaultp(ਨਮੂਨਾ_Default_Valuep(defaultp, arrayDtp->subDTypep()));
        ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ keyp = ਨਮੂਨਾ_Keyp(consp, patp, arrayDtp);
            patp->dtypep(arrayDtp->subDTypep());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
            ਜੇ (keyp) {
                consp->ਜੋੜੋ_Itemsp(ਨਵਾਂ T_ਵਸਤੂ{patp->fileline(), keyp, valuep});
            } ਨਹੀਂਤਾਂ {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
            }
        }
        // Aggregate keys are not sorted or checked for duplicates
        ਅਚਲ AstAssocArrayDType* ਅਚਲ assocDtp = VN_CAST(arrayDtp, AssocArrayDType);
        ਜੇ (!assocDtp || !assocDtp->ਕੁੰਜੀ_DTypep()->skipRefp()->isAggregateType()) {
            ਛਾਂਟੋ_Cons_ਚੀਜ਼ਾਂ<T_ਵਸਤੂ>(consp);
        }
        nodep->replaceWith(consp);
        // UINFOTREE(9, consp, "", "apat-out");
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Cons_Dyn_Array_Or_ਕਤਾਰ(File_ਸਤਰ* fileline, ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਜੇ (VN_IS(dtypep, DynArrayDType)) ਵਾਪਸ ਨਵਾਂ AstConsDynArray{fileline};
        ਜੇ (VN_IS(dtypep, ਕਤਾਰ_DType)) ਵਾਪਸ ਨਵਾਂ Ast_Cons_ਕਤਾਰ{fileline};
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }
    // Add 'valuep' to a dynamic array or queue constructor: as one element, or as an array whose
    // elements are spliced in. A constructor being spliced (e.g. a concatenation's left operand)
    // has its items moved over, or added one by one to convert them if its kind or element type
    // differs.
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_ਵਸਤੂ, ਕਿਸਮ_ਨਾਂ T_Cons>
    ਖਾਲੀ ਜੋੜੋ_Cons_ਵਸਤੂ(T_Cons* consp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        Ast_ਗੰਢ_DType* ਅਚਲ arrayDtp = consp->dtypep()->skipRefp();
        // An element if assignable to the element type, else an array (IEEE 1800-2023 10.10)
        ਅਚਲ ਤਾਰਕਿਕ isElement
            = Ast_ਗੰਢ::computeCastable(arrayDtp->subDTypep(), valuep->dtypep(), ਖਾਲੀ_ਸੂਚਕ)
                  .isAssignable();
        // A bounded queue to splice is constructed first, to truncate it
        ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ sub_ਕਤਾਰ_Dtp = VN_CAST(valuep->dtypep()->skipRefp(), ਕਤਾਰ_DType);
        ਜੇ (!isElement && !(sub_ਕਤਾਰ_Dtp && sub_ਕਤਾਰ_Dtp->boundp())) {
            T_Cons* ਅਚਲ subp = Ast_ਗੰਢ::cast<T_Cons>(valuep);
            ਜੇ (subp && subp->dtypep()->skipRefp()->subDTypep() == arrayDtp->subDTypep()) {
                ਜੇ (subp->itemsp()) consp->ਜੋੜੋ_Itemsp(subp->itemsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(subp), subp);
                ਵਾਪਸ;
            }
            ਜੇ (Ast_Cons_ਕਤਾਰ* ਅਚਲ queuep = VN_CAST(valuep, Cons_ਕਤਾਰ)) {
                ਜੋੜੋ_Cons_ਚੀਜ਼ਾਂ(consp, queuep);
                ਵਾਪਸ;
            }
            ਜੇ (AstConsDynArray* ਅਚਲ dynp = VN_CAST(valuep, ConsDynArray)) {
                ਜੋੜੋ_Cons_ਚੀਜ਼ਾਂ(consp, dynp);
                ਵਾਪਸ;
            }
        }
        T_ਵਸਤੂ* ਅਚਲ itemp = ਨਵਾਂ T_ਵਸਤੂ{valuep->fileline(), valuep};
        consp->ਜੋੜੋ_Itemsp(itemp);
        // Width the value now, as constructors are widthed when created
        ਜੇ (isElement) {
            iterateCheckTyped(itemp, "value", valuep, arrayDtp->subDTypep(), FINAL);
        } ਨਹੀਂਤਾਂ {
            user_Iterate_And_ਅਗਲਾ(valuep, ਚੌੜਾਈ_VP{arrayDtp, FINAL}.p());
        }
    }
    ਖਾਲੀ ਜੋੜੋ_Cons_ਵਸਤੂ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* consp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep) {
        ਜੇ (AstConsDynArray* ਅਚਲ dynp = VN_CAST(consp, ConsDynArray)) {
            ਜੋੜੋ_Cons_ਵਸਤੂ<Ast_Cons_Dyn_Array_ਵਸਤੂ>(dynp, valuep);
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Cons_ਵਸਤੂ<Ast_Cons_ਕਤਾਰ_ਵਸਤੂ>(VN_AS(consp, Cons_ਕਤਾਰ), valuep);
        }
    }
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Sub>
    ਖਾਲੀ ਜੋੜੋ_Cons_ਚੀਜ਼ਾਂ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* consp, T_Sub* subp) {
        ਜਦੋਂ (ਸਵੈਚਾਲਿਤ* ਅਚਲ itemp = subp->itemsp()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = itemp->valuep()->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(itemp->unlink_Fr_ਪਿੱਠ()), itemp);
            ਜੋੜੋ_Cons_ਵਸਤੂ(consp, valuep);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(subp), subp);
    }
    ਖਾਲੀ ਨਮੂਨਾ_Dyn_Array_Or_ਕਤਾਰ(Ast_ਨਮੂਨਾ* nodep, Ast_ਗੰਢ_DType* arrayp, AstPatMember* defaultp) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = new_Cons_Dyn_Array_Or_ਕਤਾਰ(nodep->fileline(), arrayp);
        UASSERT_OBJ(newp, nodep, "Expected dynamic array or queue data type");
        newp->dtypeFrom(arrayp);
        // By the syntax, members are all keyed or all positional
        ਅਚਲ AstPatMember* ਅਚਲ firstp = VN_AS(nodep->itemsp(), PatMember);
        ਅਚਲ ਤਾਰਕਿਕ keyed = defaultp || (firstp && firstp->keyp());
        ਜੇ (keyed && VN_IS(arrayp, DynArrayDType) && VN_IS(nodep->backp(), NewDynamic)) {
            // The initializer of new[], visit(AstNewDynamic) then adds the size
            ਨਮੂਨਾ_Keyed<Ast_Cons_Dyn_Array_ਵਸਤੂ>(nodep, VN_AS(newp, ConsDynArray), arrayp, defaultp);
            ਵਾਪਸ;
        }
        ਅਚਲ ਅੱਖਰ* ਅਚਲ kindp = VN_IS(arrayp, DynArrayDType) ? "dynamic array" : "queue";
        ਜੇ (defaultp) defaultp->v3error("Assignment pattern default cannot be used for " << kindp);
        ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ਜੇ (patp->keyp()) {
                patp->keyp()->v3error("Assignment pattern key cannot be used for " << kindp);
            }
            patp->dtypep(arrayp->subDTypep());
            ਜੋੜੋ_Cons_ਵਸਤੂ(newp, ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp));
        }
        nodep->replaceWith(newp);
        // UINFOTREE(9, newp, "", "apat-out");
    }
    ਖਾਲੀ ਨਮੂਨਾ_Basic(Ast_ਨਮੂਨਾ* nodep, Ast_ਗੰਢ_DType* vdtypep, AstPatMember* defaultp) {
        ਅਚਲ AstBasicDType* bdtypep = VN_AS(vdtypep, BasicDType);
        ਅਚਲ VNumRange range = bdtypep->declRange();
        PatVecMap patmap = pat_ਸਦਿਸ਼_Map(nodep, range);
        UINFO(9, "ent " << range.hi() << " to " << range.lo());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ ਪੂਰਨ ent : range.seq_Hi_ਮੰਜ਼ਿਲ_Lo()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ ਸਵੈਚਾਲਿਤ it = patmap.ਲੱਭੋ(ent);
            ਜੇ (it == patmap.ਸਿਰਾ()) {
                ਜੇ (defaultp) {
                    valuep = ਨਮੂਨਾ_Default_Valuep(defaultp, nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
                } ਨਹੀਂਤਾਂ {
                    nodep->v3error("Assignment pattern missed initializing elements: " << ent);
                }
            } ਨਹੀਂਤਾਂ {
                AstPatMember* ਅਚਲ patp = it->ਦੂਜਾ;
                patmap.erase(it);
                patp->dtypep(nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
                valuep = ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(patp);
            }
            ਜੇ (valuep) {  // Packed. Convert to concat for now.
                ਜੇ (!newp) {
                    newp = valuep;
                } ਨਹੀਂਤਾਂ {
                    AstConcat* ਅਚਲ concatp = ਨਵਾਂ AstConcat{valuep->fileline(), newp, valuep};
                    newp = concatp;
                    newp->dtypeChgSigned(nodep->dtypep()->numeric());
                }
            }
        }
        ਜੇ (!patmap.ਖਾਲੀ_ਥਾਵਾਂ()) nodep->v3error("Assignment pattern with too many elements");
        ਜੇ (newp) {
            nodep->replaceWith(newp);
        } ਨਹੀਂਤਾਂ {
            nodep->v3error("Assignment pattern with no members");
        }
        // UINFOTREE(9, newp, "", "apat-out");
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਨਮੂਨਾ_Member_ਮੁੱਲ_Iterate(AstPatMember* patp) {
        // Determine values - might be another ConsArray
        userIterate(patp, ਚੌੜਾਈ_VP{patp->dtypep(), BOTH}.p());
        // Convert to ConsArray or constify immediately
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = patp->lhssp()->unlink_Fr_ਪਿੱਠ();
        ਜੇ (VN_IS(valuep, Const)) {
            // Forming a AstConcat will cause problems with
            // unsized (uncommitted sized) constants
            ਜੇ (AstConst* ਅਚਲ newp = V3_ਚੌੜਾਈ_Commit::new_If_Const_Commit_ਮਾਪ(VN_AS(valuep, Const))) {
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);
                valuep = newp;
            }
        }
        ਵਾਪਸ valuep;
    }

    ਸਥਿਰ ਖਾਲੀ checkEventAssignment(ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ asgnp) {
        string unsupEvtAsgn;
        ਜੇ (!ਵਰਤਦਾ_Dynamic_Scheduler(asgnp->lhsp())) unsupEvtAsgn = "to";
        ਜੇ (VN_IS(asgnp->rhsp(), CReset)) ਵਾਪਸ;
        ਜੇ (asgnp->rhsp()->dtypep()->isEvent() && !ਵਰਤਦਾ_Dynamic_Scheduler(asgnp->rhsp())) {
            unsupEvtAsgn += (unsupEvtAsgn.ਖਾਲੀ_ਥਾਵਾਂ() ? "from" : " and from");
        }
        ਜੇ (!unsupEvtAsgn.ਖਾਲੀ_ਥਾਵਾਂ()) {
            asgnp->v3warn(E_UNSUPPORTED, "Assignment "
                                             << unsupEvtAsgn
                                             << " event in statically scheduled context.\n"
                                             << asgnp->warn_ਹੋਰ()
                                             << "Static event "
                                                "scheduling won't be able to handle this.\n"
                                             << asgnp->warn_ਹੋਰ()
                                             << "... Suggest move the event into a "
                                                "completely dynamic context, eg. a class,  and "
                                                "reference it only from such context.");
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਵਰਤਦਾ_Dynamic_Scheduler(Ast_ਗੰਢ* nodep) {
        UASSERT_OBJ(nodep->dtypep()->isEvent(), nodep, "Node does not have an event dtype");
        ਜਦੋਂ (ਸੱਚ) {
            AstVarRef* ਅਚਲ vrefp = VN_CAST(nodep, VarRef);
            ਜੇ (vrefp) ਵਾਪਸ ਵਰਤਦਾ_Dynamic_Scheduler(vrefp);
            ਜੇ (VN_IS(nodep, MemberSel)) {
                ਵਾਪਸ ਸੱਚ;
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Sel* selp = VN_CAST(nodep, ਗੰਢ_Sel)) {
                nodep = selp->fromp();
            } ਨਹੀਂਤਾਂ {
                ਵਾਪਸ ਝੂਠ;
            }
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ ਵਰਤਦਾ_Dynamic_Scheduler(AstVarRef* vrefp) {
        ਵਾਪਸ VN_IS(vrefp->classOrPackagep(), Class) || vrefp->varp()->isFuncLocal();
    }

    ਖਾਲੀ ਦੇਖੋ(AstPatMember* nodep) override {
        Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypeNullp();
        UASSERT_OBJ(vdtypep, nodep, "Pattern member type not assigned by AstPattern visitor");
        nodep->dtypep(vdtypep);
        UINFO(9, "   PATMEMBER " << nodep);
        UASSERT_OBJ(!nodep->lhssp()->nextp(), nodep,
                    "PatMember value should be singular w/replicates removed");
        // Need to propagate assignment type downwards, even on prelim
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
        iterateCheck(nodep, "Pattern value", nodep->lhssp(), ਸੌਂਪੋ_3, FINAL, vdtypep, EXTEND_LHS);
    }
    ਪੂਰਨ ਦੇਖੋ_Pat_Member_Rep(AstPatMember* nodep) {
        uint32_t times = 1;
        ਜੇ (nodep->repp()) {  // else repp()==nullptr shorthand for rep count 1
            iterateCheckSizedSelf(nodep, "LHS", nodep->repp(), SELF, BOTH);
            V3Const::constifyParamsEdit(nodep->repp());  // repp may change
            ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep->repp(), Const);
            ਜੇ (!constp) {
                nodep->v3error("Replication value isn't a constant.");
                times = 0;
            } ਨਹੀਂਤਾਂ {
                times = constp->ਮੰਜ਼ਿਲ_UInt();
            }
            ਜੇ (times == 0) {
                nodep->v3error("Pattern replication value of 0 is not legal.");
                times = 1;
            }
            nodep->repp()
                ->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()
                ->delete_ਰੁੱਖ();  // Done with replicate before cloning
        }
        ਵਾਪਸ times;
    }

    ਖਾਲੀ ਦੇਖੋ(AstPropSpec* nodep) override {
        VL_RESTORER(m_seqUnsupp);
        VL_RESTORER(m_hasSExpr);
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // Only bare strong until uses V3AssertPre's timing-based lowering. Embedded forms
            // are lowered without timing by V3AssertNfa.
            Ast_ਗੰਢ* propp = nodep->propp();
            ਜਦੋਂ (AstLogNot* ਅਚਲ notp = VN_CAST(propp, LogNot)) propp = notp->lhsp();
            AstUntil* ਅਚਲ untilp = VN_CAST(propp, Until);
            ਜੇ (untilp && VN_IS(nodep->backp(), ਗੰਢ_Cover_Or_Assert) && untilp->isStrong()
                && (v3Global.opt.timing().is_ਸੈੱਟ_False() || !v3Global.opt.timing().is_ਸੈੱਟ_True())) {
                untilp->v3warn(E_NOTIMING, untilp->verilogKwd() << " requires --timing");
                untilp->replaceWith(ਨਵਾਂ AstConst{untilp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(untilp), untilp);
            }
            iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->sensesp(), ਖਾਲੀ_ਸੂਚਕ);
            ਜੇ (nodep->disablep()) {
                iterateCheckBool(nodep, "Disable", nodep->disablep(),
                                 BOTH);  // it's like an if() condition.
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }

    ਤਾਰਕਿਕ ਪਹਿਲਾ_New_Statement_Ok_Recurse(Ast_ਗੰਢ* nodep) {
        ਜੇ (AstVar* ਅਚਲ varp = VN_CAST(nodep, Var)) {
            ਜੇ (!varp->valuep() || VN_CAST(varp->valuep(), Const) || varp->isIO()) ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (Ast_ਸੌਂਪੋ* ਅਚਲ aitemp = VN_CAST(nodep, ਸੌਂਪੋ)) {
            ਜੇ (VN_IS(aitemp->rhsp(), Const) || VN_IS(aitemp->rhsp(), CReset)) ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (AstInitialStaticStmt* ਅਚਲ aitemp = VN_CAST(nodep, InitialStaticStmt)) {
            ਲਈ (Ast_ਗੰਢ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (!ਪਹਿਲਾ_New_Statement_Ok_Recurse(stmtp)) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        }
        ਜੇ (AstInitialAutomaticStmt* ਅਚਲ aitemp = VN_CAST(nodep, InitialAutomaticStmt)) {
            ਲਈ (Ast_ਗੰਢ* stmtp = aitemp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
                ਜੇ (!ਪਹਿਲਾ_New_Statement_Ok_Recurse(stmtp)) ਵਾਪਸ ਝੂਠ;
            }
            ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }

    //--------------------
    // Top levels

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Case_ਵਸਤੂ>
    ਖਾਲੀ handleCaseType(Ast_ਗੰਢ* casep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Case_ਵਸਤੂ* itemsp) {
        Ast_Attr_ਦਾ* ਅਚਲ exprap = VN_CAST(exprp, Attr_ਦਾ);
        ਜੇ (!exprap) ਵਾਪਸ;
        ਜੇ (exprap->attrType() != VAttrType::TYPEID) ਵਾਪਸ;

        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ ਵਿਅੰਜਕ_Dtp = exprap->dtypep();
        UINFO(9, "case type exprDtp " << ਵਿਅੰਜਕ_Dtp);
        // V3Param may have a pointer to this case statement, and we need
        // dotted references to remain properly named, so rather than
        // removing we convert it to a "normal" expression "case (1) ..."
        File_ਸਤਰ* ਅਚਲ newfl = casep->fileline();
        newfl->warnOff(V3_Error_ਕੋਡ::CASEINCOMPLETE, ਸੱਚ);  // Side effect of transform
        newfl->warnOff(V3_Error_ਕੋਡ::CASEOVERLAP, ਸੱਚ);  // Side effect of transform
        casep->fileline(newfl);
        ਲਈ (Case_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<Case_ਵਸਤੂ>(itemp->nextp())) {
            ਜੇ (itemp->isDefault()) ਜਾਰੀ;
            ਤਾਰਕਿਕ ਪ੍ਰਾਪਤੀ = ਝੂਠ;
            ਲਈ (Ast_ਗੰਢ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ਅਚਲ Ast_Attr_ਦਾ* ਅਚਲ condAttrp = VN_CAST(condp, Attr_ਦਾ);
                ਜੇ (!condAttrp) {
                    condp->v3error("Case(type) statement requires items that have type() items");
                } ਨਹੀਂਤਾਂ {
                    Ast_ਗੰਢ_DType* ਅਚਲ condDtp = condAttrp->dtypep();
                    ਜੇ (Ast_ਗੰਢ::computeCastable(ਵਿਅੰਜਕ_Dtp, condDtp, casep) == VCastable::SAMEISH) {
                        ਪ੍ਰਾਪਤੀ = ਸੱਚ;
                        ਰੋਕੋ;
                    }
                }
            }
            ਧੱਕੋ_Deletep(itemp->condsp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            // Item condition becomes constant 1 if hits else 0
            itemp->ਜੋੜੋ_Condsp(ਨਵਾਂ AstConst{newfl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, ਪ੍ਰਾਪਤੀ});
        }
        exprap->replaceWith(ਨਵਾਂ AstConst{newfl, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}});
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(exprap), exprap);
    }

    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ Case_ਵਸਤੂ>
    ਖਾਲੀ handleCase(Ast_ਗੰਢ* casep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Case_ਵਸਤੂ* itemsp) {
        // IEEE-2012 12.5:
        //    Width: MAX(expr, all items)
        //    Signed: Only if expr, and all items signed
        // Take width as maximum across all items, if any is real whole thing is real
        Ast_ਗੰਢ_DType* subDTypep = exprp->dtypep();
        ਲਈ (Case_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<Case_ਵਸਤੂ>(itemp->nextp())) {
            ਲਈ (Ast_ਗੰਢ* condp = itemp->condsp(); condp; condp = condp->nextp()) {
                ਜੇ (condp->dtypep() == subDTypep) ਜਾਰੀ;

                ਜੇ (condp->dtypep()->isDouble() || subDTypep->isDouble()) {
                    subDTypep = casep->ਲੱਭੋ_Double_DType();
                } ਨਹੀਂਤਾਂ ਜੇ (condp->dtypep()->isString() || subDTypep->isString()) {
                    subDTypep = casep->ਲੱਭੋ_String_DType();
                } ਨਹੀਂਤਾਂ {
                    ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(subDTypep->ਚੌੜਾਈ(), condp->ਚੌੜਾਈ());
                    ਅਚਲ ਪੂਰਨ mwidth = std::max(subDTypep->ਚੌੜਾਈ_Min(), condp->ਚੌੜਾਈ_Min());
                    ਅਚਲ ਤਾਰਕਿਕ issigned = subDTypep->isSigned() && condp->isSigned();
                    subDTypep = casep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                        ਚੌੜਾਈ, mwidth, VSigning::fromBool(issigned),
                        subDTypep->isFourstate() || condp->dtypep()->isFourstate());
                }
            }
        }

        // Apply width
        iterateCheck(casep, "Case expression", exprp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        ਲਈ (Case_ਵਸਤੂ* itemp = itemsp; itemp; itemp = Ast_ਗੰਢ::as<Case_ਵਸਤੂ>(itemp->nextp())) {
            ਲਈ (Ast_ਗੰਢ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Final may cause the node to get replaced
                iterateCheck(casep, "Case Item", condp, CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            }
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstGenCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression and case item conditions, but not bodies
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
        ਲਈ (Ast_Gen_Case_ਵਸਤੂ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            // Prelim may cause the node to get replaced, pick up next up front
            nextip = VN_AS(itemp->nextp(), Gen_Case_ਵਸਤੂ);
            ਲਈ (Ast_ਗੰਢ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                // Prelim may cause the node to get replaced, pick up next up front
                nextcp = condp->nextp();
                VL_DO_DANGLING(userIterate(condp, ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenFor* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ਅਗਲਾ(nodep->initsp(), ਖਾਲੀ_ਸੂਚਕ);
        iterateCheckBool(nodep, "For Test Condition", nodep->condp(), BOTH);
        user_Iterate_And_ਅਗਲਾ(nodep->incsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstGenIf* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCase* nodep) override {
        assertAtStatement(nodep);
        // Type check expression case item conditions and bodies
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
        ਲਈ (Ast_Case_ਵਸਤੂ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_ਵਸਤੂ);  // Prelim may cause the node to get replaced
            user_Iterate_And_ਅਗਲਾ(itemp->stmtsp(), ਖਾਲੀ_ਸੂਚਕ);
            ਲਈ (Ast_ਗੰਢ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                VL_DO_DANGLING(userIterate(condp, ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p()), condp);
            }
        }
        // Deal with case(type(data_type))
        handleCaseType(nodep, nodep->exprp(), nodep->itemsp());
        // Type check
        handleCase(nodep, nodep->exprp(), nodep->itemsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstRandCase* nodep) override {
        // IEEE says each item is a int (32-bits), and sizes are based on natural sizing,
        // but we'll sum to a 64-bit number then math is faster.
        assertAtStatement(nodep);
        Ast_ਗੰਢ_DType* ਅਚਲ ਵਸਤੂ_DTypep = nodep->ਲੱਭੋ_UInt32_DType();
        ਲਈ (Ast_Case_ਵਸਤੂ *nextip, *itemp = nodep->itemsp(); itemp; itemp = nextip) {
            nextip = VN_AS(itemp->nextp(), Case_ਵਸਤੂ);  // Prelim may cause the node to get replaced
            user_Iterate_And_ਅਗਲਾ(itemp->stmtsp(), ਖਾਲੀ_ਸੂਚਕ);
            ਲਈ (Ast_ਗੰਢ *nextcp, *condp = itemp->condsp(); condp; condp = nextcp) {
                nextcp = condp->nextp();  // Prelim may cause the node to get replaced
                iterateCheckTyped(itemp, "Randcase Item", condp, ਵਸਤੂ_DTypep, BOTH);
                VL_DANGLING(condp);  // Might have been replaced
            }
            VL_DANGLING(itemp);  // Might have been replaced
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstLoop* nodep) override {
        UASSERT_OBJ(!nodep->contsp(), nodep, "'contsp' only used before LinkJump");
        assertAtStatement(nodep);
        user_Iterate_And_ਅਗਲਾ(nodep->stmtsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstLoopTest* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Loop Condition", nodep->condp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRepeat* nodep) override { nodep->v3fatalSrc("'repeat' missed in LinkJump"); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        assertAtStatement(nodep);
        // UINFOTREE(1, nodep, "", "IfPre");
        user_Iterate_And_ਅਗਲਾ(nodep->thensp(), ਖਾਲੀ_ਸੂਚਕ);
        user_Iterate_And_ਅਗਲਾ(nodep->elsesp(), ਖਾਲੀ_ਸੂਚਕ);
        iterateCheckBool(nodep, "If", nodep->condp(), BOTH);  // it's like an if() condition.
        // UINFOTREE(1, nodep, "", "IfOut");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਵਿਅੰਜਕ_Stmt* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->stmtsp(), ਖਾਲੀ_ਸੂਚਕ);
        // expected result is same as parent's expected result
        user_Iterate_And_ਅਗਲਾ(nodep->resultp(), m_vup);
        nodep->dtypeFrom(nodep->resultp());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Foreach* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        ਅਚਲ Ast_Foreach_ਸਿਰਲੇਖ* ਅਚਲ headerp = nodep->headerp();
        // UINFOTREE(1, nodep, "", "foreach-old");
        user_Iterate_And_ਅਗਲਾ(headerp->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = headerp->fromp();
        UASSERT_OBJ(fromp->dtypep(), fromp, "Missing data type");
        Ast_ਗੰਢ_DType* fromDtp = fromp->dtypep()->skipRefp();
        // Major dimension first
        ਲਈ (Ast_ਗੰਢ *argsp = headerp->elementsp(), *ਅਗਲਾ_argsp; argsp; argsp = ਅਗਲਾ_argsp) {
            ਅਗਲਾ_argsp = argsp->nextp();
            ਅਚਲ ਤਾਰਕਿਕ ਖਾਲੀ_ਥਾਵਾਂ = VN_IS(argsp, ਖਾਲੀ_ਥਾਵਾਂ_2);
            AstVar* ਅਚਲ varp = VN_CAST(argsp, Var);
            UASSERT_OBJ(varp || ਖਾਲੀ_ਥਾਵਾਂ, argsp, "Missing foreach loop variable");
            ਜੇ (varp) varp->ਵਰਤਿਆ_Loop_Idx(ਸੱਚ);
            ਜੇ (!fromDtp) {
                argsp->v3error("foreach loop variables exceed number of indices of array");
                VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            }
            fromDtp = fromDtp->skipRefp();
            UINFO(9, "- foreachArg " << argsp);
            UINFO(9, "-   from on  " << fromp);
            UINFO(9, "-   from dtp " << fromDtp);

            ਜੇ (VN_IS(fromDtp, ਗੰਢ_Array_DType) || VN_IS(fromDtp, DynArrayDType)
                || VN_IS(fromDtp, ਕਤਾਰ_DType)) {
                // Nothing special here
            } ਨਹੀਂਤਾਂ ਜੇ (AstBasicDType* ਅਚਲ adtypep = VN_CAST(fromDtp, BasicDType)) {
                ਜੇ (!adtypep->isString() && !adtypep->isRanged()) {
                    argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                                << " data type");
                    VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                    ਵਾਪਸ;
                }
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstAssocArrayDType* ਅਚਲ adtypep
                       = VN_CAST(fromDtp, AssocArrayDType)) {
                varp->dtypeFrom(adtypep->ਕੁੰਜੀ_DTypep());
            } ਨਹੀਂਤਾਂ {
                argsp->v3error("Illegal 'foreach' loop on " << fromDtp->prettyTypeName()
                                                            << " data type");
                VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
                ਵਾਪਸ;
            }
            fromDtp = fromDtp->subDTypep();
        }
        // The parser validates we don't have "foreach (array[,,,])"
        Ast_ਗੰਢ* ਅਚਲ bodyp = nodep->bodyp();
        user_Iterate_And_ਅਗਲਾ(bodyp, ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (AstForeach* ਅਚਲ loopp = VN_CAST(nodep, Foreach)) {
            VL_DO_DANGLING2(V3Begin::convert_ਮੰਜ਼ਿਲ_While(loopp), loopp, nodep);
            ਵਾਪਸ;
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) override {
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
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LHS be untyped?");
            UASSERT_OBJ(nodep->lhsp()->dtypep()->ਚੌੜਾਈ_Sized(), nodep, "How can LHS be unsized?");
            // An interface instance, or an array or slice of them, is not a variable
            ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ irefp
                = VN_CAST(nodep->lhsp()->dtypep()->elemDTypep(), IfaceRefDType)) {
                ਜੇ (!irefp->isVirtual()) {
                    nodep->v3error(
                        "Illegal assignment to an interface instance (IEEE 1800-2023 10.4)");
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                    ਵਾਪਸ;
                }
            }
            nodep->dtypeFrom(nodep->lhsp());
            //
            // AstPattern needs to know the proposed data type of the lhs, so pass on the prelim
            // UINFOTREE(1, nodep, "", "assrhs");
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
            //
            // UINFOTREE(1, nodep, "", "assign");
            ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ(nodep);
            Ast_ਗੰਢ_DType* ਅਚਲ lhsDTypep = nodep->lhsp()->dtypep();

            // IEEE 1800-2023 7.6: For unpacked arrays to be assignment compatible,
            // the element types shall be equivalent (IEEE 1800-2023 6.22.2).
            // Note: Streaming operators and string literals have implicit conversion rules.
            ਜੇ (nodep->rhsp()->dtypep()) {  // May be null on earlier errors
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDtp = lhsDTypep->skipRefp();
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsDtp = nodep->rhsp()->dtypep()->skipRefp();
                // Only check if number of states match for unpacked array to unpacked array
                // assignments
                ਅਚਲ ਤਾਰਕਿਕ lhsIsUnpackArray = lhsDtp->isNonPackedArray();
                ਅਚਲ ਤਾਰਕਿਕ rhsIsUnpackArray = rhsDtp->isNonPackedArray();
                ਜੇ (lhsIsUnpackArray && rhsIsUnpackArray) {
                    ਜੇ (lhsDtp->isFourstate() != rhsDtp->isFourstate()) {
                        nodep->v3error("Assignment between 2-state and 4-state types requires "
                                       "equivalent element types (IEEE 1800-2023 6.22.2, 7.6)"
                                       << warnDTypes(nodep, lhsDtp, rhsDtp));
                    }
                }
                checkUnpackedArrayAssignmentCompatible<Ast_ਗੰਢ_Var_Ref, Ast_ਗੰਢ_Var_Ref>(
                    nodep, VN_CAST(nodep->lhsp(), ਗੰਢ_Var_Ref), VN_CAST(nodep->rhsp(), ਗੰਢ_Var_Ref));
                // Variable references were checked above, this catches e.g. method call results
                ਜੇ (lhsDtp->isIntegralOrPacked() && rhsDtp->isAggregateType()
                    && !VN_IS(nodep->rhsp(), ਗੰਢ_Var_Ref)) {
                    nodep->v3error("Illegal assignment betwen unpacked aggregate and "
                                   "integral type (IEEE 1800-2023 7.6)"
                                   << warnDTypes(nodep, lhsDtp, rhsDtp));
                }
            }

            iterate_Check_ਸੌਂਪੋ(nodep, "Assign RHS", nodep->rhsp(), FINAL, lhsDTypep);
            // UINFOTREE(1, nodep, "", "AssignOut");
        }
        ਜੇ (VN_IS(nodep, ਸੌਂਪੋ_Force)) checkForceReleaseLhs(nodep, nodep->lhsp());
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ controlp = nodep->timingControlp()) {
            ਜੇ (!m_underFork && VN_IS(m_ftaskp, Func)) {
                controlp->v3error("Timing controls are not legal in functions. Suggest use a task "
                                  "(IEEE 1800-2023 13.4.4)");
                VL_DO_DANGLING(controlp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ(), controlp);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->fileline()->timingOn() && v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                iterateNull(controlp);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (nodep->fileline()->timingOn()) {
                    ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                        controlp->v3warn(ASSIGNDLY, "Ignoring timing control on this "
                                                    "assignment/primitive due to --no-timing");
                    } ਨਹੀਂਤਾਂ {
                        controlp->v3warn(E_NEEDTIMINGOPT,
                                         "Use --timing or --no-timing to specify how "
                                         "timing controls should be handled");
                    }
                }
                VL_DO_DANGLING(controlp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ()->delete_ਰੁੱਖ(), controlp);
            }
        }
        ਜੇ (VN_IS(nodep->rhsp(), ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ)) {
            UINFO(9, "= {} -> .delete(): " << nodep);
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDtp = nodep->lhsp()->dtypep()->skipRefp();
            ਜੇ (!VN_IS(lhsDtp, ਕਤਾਰ_DType) && !VN_IS(lhsDtp, DynArrayDType)) {
                nodep->v3warn(E_UNSUPPORTED,
                              "Unsupported/Illegal: empty queue ('{}') in this assign context");
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
                ਵਾਪਸ;
            }
            Ast_ਵਿਧੀ_Call* ਅਚਲ newp
                = ਨਵਾਂ Ast_ਵਿਧੀ_Call{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(), "delete"};
            newp->dtype_ਸੈੱਟ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            // Need to now convert it
            ਦੇਖੋ(newp);
            ਵਾਪਸ;
        }
        ਜੇ (ਅਚਲ AstNewDynamic* ਅਚਲ dynp = VN_CAST(nodep->rhsp(), NewDynamic)) {
            UINFO(9, "= new[] -> .resize(): " << nodep);
            AstCMethodHard* newp;
            ਜੇ (!dynp->rhsp()) {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::DYN_RENEW, dynp->sizep()->unlink_Fr_ਪਿੱਠ()};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstCMethodHard{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),
                                          VCMethod::DYN_RENEW_COPY, dynp->sizep()->unlink_Fr_ਪਿੱਠ()};
                newp->ਜੋੜੋ_Pinsp(dynp->rhsp()->unlink_Fr_ਪਿੱਠ());
            }
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            newp->protect(ਝੂਠ);
            newp->dtype_ਸੈੱਟ_Void();
            nodep->replaceWith(newp->makeStmt());
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ;
        }

        // Queue slice on LHS: q[a:b] = rhs  ->  q.sliceAssign(a, b, rhs)
        // The LHS was lowered from AstSelExtract to CMethodHard(DYN_SLICE)
        // by V3WidthSel; that returns a temporary copy so the assignment is
        // silently discarded.  Transform into a mutating sliceAssign call.
        ਜੇ (AstCMethodHard* ਅਚਲ slicep = VN_CAST(nodep->lhsp(), CMethodHard)) {
            VCMethod ਸੌਂਪੋ_ਵਿਧੀ;
            ਜੇ (slicep->ਵਿਧੀ() == VCMethod::DYN_ਟੁਕੜਾ) {
                ਸੌਂਪੋ_ਵਿਧੀ = VCMethod::DYN_ਟੁਕੜਾ_ਸੌਂਪੋ;
            } ਨਹੀਂਤਾਂ ਜੇ (slicep->ਵਿਧੀ() == VCMethod::DYN_ਟੁਕੜਾ_ਪਿੱਠ_ਪਿੱਠ) {
                ਸੌਂਪੋ_ਵਿਧੀ = VCMethod::DYN_ਟੁਕੜਾ_ਸੌਂਪੋ_ਪਿੱਠ_ਪਿੱਠ;
            } ਨਹੀਂਤਾਂ ਜੇ (slicep->ਵਿਧੀ() == VCMethod::DYN_ਟੁਕੜਾ_FRONT_ਪਿੱਠ) {
                ਸੌਂਪੋ_ਵਿਧੀ = VCMethod::DYN_ਟੁਕੜਾ_ਸੌਂਪੋ_FRONT_ਪਿੱਠ;
            } ਨਹੀਂਤਾਂ {
                ਸੌਂਪੋ_ਵਿਧੀ = VCMethod::_ENUM_MAX;  // not a slice
            }
            ਜੇ (ਸੌਂਪੋ_ਵਿਧੀ != VCMethod::_ENUM_MAX) {
                UINFO(9, "LHS queue slice -> sliceAssign: " << nodep);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = slicep->fromp()->unlink_Fr_ਪਿੱਠ();
                // Collect existing slice index pins (lsb, msb)
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lsbp = slicep->pinsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ msbp = slicep->pinsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                AstCMethodHard* ਅਚਲ newp
                    = ਨਵਾਂ AstCMethodHard{nodep->fileline(), fromp, ਸੌਂਪੋ_ਵਿਧੀ};
                newp->ਜੋੜੋ_Pinsp(lsbp);
                newp->ਜੋੜੋ_Pinsp(msbp);
                newp->ਜੋੜੋ_Pinsp(rhsp);
                newp->did_ਚੌੜਾਈ(ਸੱਚ);
                newp->protect(ਝੂਠ);
                newp->dtype_ਸੈੱਟ_Void();
                nodep->replaceWith(newp->makeStmt());
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            }
        }

        ਜੇ (nodep->hasDType() && nodep->dtypep()->isEvent()) {
            checkEventAssignment(nodep);
            v3Global.ਸੈੱਟ_Assigns_Events();
        }
    }

    ਖਾਲੀ ਦੇਖੋ(AstRSRule* nodep) override {
        ਜੇ (nodep->weightp()) iterateCheckUInt32(nodep, "weight", nodep->weightp(), BOTH);
        user_Iterate_And_ਅਗਲਾ(nodep->prodlistsp(), ਖਾਲੀ_ਸੂਚਕ);
        user_Iterate_And_ਅਗਲਾ(nodep->ਭਾਰ_Stmtsp(), ਖਾਲੀ_ਸੂਚਕ);
    }

    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "How can LValue be untyped?");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->ਚੌੜਾਈ_Sized(), nodep, "How can LValue be unsized?");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }
    ਖਾਲੀ ਦੇਖੋ(AstDeassign* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(nodep->lhsp()->dtypep(), nodep, "L-value is untyped");
        UASSERT_OBJ(nodep->lhsp()->dtypep()->ਚੌੜਾਈ_Sized(), nodep, "L-value width is unsized");
        checkForceReleaseLhs(nodep, nodep->lhsp());
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isFormatNonNumericArg(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਵਾਪਸ dtypep->isString()  //
               || VN_IS(dtypep, AssocArrayDType)  //
               || VN_IS(dtypep, WildcardArrayDType)  //
               || VN_IS(dtypep, ClassRefDType)  //
               || VN_IS(dtypep, DynArrayDType)  //
               || VN_IS(dtypep, UnpackArrayDType)  //
               || VN_IS(dtypep, ਕਤਾਰ_DType)  //
               || (VN_IS(dtypep, ਗੰਢ_UOr_Struct_DType)
                   && !VN_AS(dtypep, ਗੰਢ_UOr_Struct_DType)->packed());
    }

    ਖਾਲੀ checkFormatNumericArg(Ast_ਗੰਢ* argp, ਅੱਖਰ ch) {
        // IEEE 1800-2023 21.2.1.1 suggests %d of a class is illegal.
        // Howver UVM tests require %0d print unique identifier of the class.
        ਜੇ (std::tolower(ch) == 'd') ਵਾਪਸ;
        // In contrast, %x of a class reference, causes errors on several simulators.
        ਜੇ (isFormatNonNumericArg(argp->dtypep()))
            argp->v3error(
                "$display-like format of '%"s + ch + "' illegal with non-numeric argument\n"
                << argp->warn_ਹੋਰ() << "... Suggest use '%s'");
    }

    ਖਾਲੀ ਦੇਖੋ(AstSFormatArg* nodep) override {}  // Done by AstSFormatF
    ਖਾਲੀ ਦੇਖੋ(AstSFormat* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ਅਗਲਾ(nodep->fmtp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੰਜ਼ਿਲ_String_N* nodep) override {
        // Inserted by V3Width only so we know has been resolved
        nodep->dtype_ਸੈੱਟ_String();
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{nodep->lhsp()->dtypep(), BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        // Excludes NodeDisplay, see below
        ਜੇ (m_vup && !m_vup->prelim()) ਵਾਪਸ;  // Can be called as statement or function
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;
        //
        UINFO(9, "  Display in " << nodep->ਪਾਠ());

        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (!nodep->ਵਿਅੰਜਕ_Format_2()) {  // Non-constant format, can't check format string types
            ਦੇਖੋ_sformatf_format(nodep);
        }

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* oldExprsp = nodep->exprsp();
        ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2() && oldExprsp) {
            VL_DO_DANGLING(iterateCheckString(nodep, "format", nodep->exprsp(), BOTH), oldExprsp);
            oldExprsp = VN_AS(nodep->exprsp()->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
        }
        ਜੇ (oldExprsp) oldExprsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
        ਜਦੋਂ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* argp = oldExprsp) {
            oldExprsp = VN_AS(oldExprsp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ);
            ਜੇ (oldExprsp) oldExprsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            ਜੇ (VN_IS(argp, SFormatArg)) {
                nodep->ਜੋੜੋ_Exprsp(argp);
                ਜਾਰੀ;
            }
            // Need to record formatAttr's at elaboration time, as later optimizations
            // may change an argument's data type. Plus need them for runtime formats
            VFormatAttr formatAttr = VFormatAttr::UNSIGNED;
            Ast_ਗੰਢ_DType* ਅਚਲ dtypep = argp->dtypep()->skipRefp();
            ਜੇ (dtypep->isDouble()) {
                formatAttr = VFormatAttr::DOUBLE;
            } ਨਹੀਂਤਾਂ ਜੇ (dtypep->isString()) {
                formatAttr = VFormatAttr::STRING;
            } ਨਹੀਂਤਾਂ ਜੇ (isFormatNonNumericArg(dtypep)) {
                ਜੇ (AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    ਜੇ (classRefp->classp()) {
                        classRefp->classp()->markPrintedFrom();
                        v3Global.hasPrintedObjects(ਸੱਚ);
                    }
                } ਨਹੀਂਤਾਂ {
                    // Class handles inside containers print type names, not their members.
                    Ast_ਗੰਢ_DType* ਗੰਢ_Dtypep = dtypep;
                    ਜਦੋਂ (ਗੰਢ_Dtypep) {
                        ਗੰਢ_Dtypep = ਗੰਢ_Dtypep->skipRefp();
                        ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ uOrStructDTypep
                            = VN_CAST(ਗੰਢ_Dtypep, ਗੰਢ_UOr_Struct_DType)) {
                            uOrStructDTypep->ਸੈੱਟ_Emit_ਮੰਜ਼ਿਲ_String();
                            v3Global.hasPrintedObjects(ਸੱਚ);
                        }
                        ਗੰਢ_Dtypep = ਗੰਢ_Dtypep->subDTypep();
                    }
                }
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ Ast_ਮੰਜ਼ਿਲ_String_N{argp->fileline(), argp};
                formatAttr = VFormatAttr::COMPLEX;
                argp = newp;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->ਵਿਅੰਜਕ_Format_2()) {
                ਜੇ (AstEnumDType* ਅਚਲ enumDtp = formatEnumDType(argp)) {
                    nodep->ਜੋੜੋ_Exprsp(newFormatEnumArg(argp, enumDtp));
                    ਜਾਰੀ;
                }
            }
            ਜੇ (formatAttr.isUnsigned() && dtypep->isSigned()) {
                formatAttr = VFormatAttr::SIGNED;
            }
            ਜੇ (formatAttr.isUnsigned()) {  // Save Ast space and imply the AstSFormatArg
                nodep->ਜੋੜੋ_Exprsp(argp);
            } ਨਹੀਂਤਾਂ {
                nodep->ਜੋੜੋ_Exprsp(ਨਵਾਂ AstSFormatArg{argp->fileline(), formatAttr, argp});
            }
        }

        UINFO(9, "  Display out " << nodep->ਪਾਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReset* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        nodep->dtypeFrom(m_vup->dtypep());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCReturn* nodep) override { nodep->v3fatalSrc("Should not exist yet"); }
    ਖਾਲੀ ਦੇਖੋ(AstConstraintRef* nodep) override { userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ); }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        assertAtStatement(nodep);
        ਜੇ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstElabDisplay* nodep) override {
        assertAtStatement(nodep);
        // Just let all arguments seek their natural sizes
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਜੇ (!m_paramsOnly) {
            nodep->foreach([ਇਹ](AstScopeName* nodep) {  //
                nodep->replaceWith(
                    ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, "<scope-unavailable>"});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            });
            V3Const::constifyParamsEdit(nodep->fmtp());  // fmtp may change
            string ਪਾਠ = VString::dequotePercent(nodep->fmtp()->ਪਾਠ());
            ਜੇ (ਪਾਠ.ਖਾਲੀ_ਥਾਵਾਂ()) ਪਾਠ = "Elaboration system task message (IEEE 1800-2023 20.11)";
            ਚੋਣ (nodep->displayType()) {
            ਹਾਲਤ VDisplayType::DT_INFO: nodep->v3warn(USERINFO, ਪਾਠ); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_ERROR: nodep->v3warn(USERERROR, ਪਾਠ); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_WARNING: nodep->v3warn(USERWARN, ਪਾਠ); ਰੋਕੋ;
            ਹਾਲਤ VDisplayType::DT_FATAL: nodep->v3warn(USERFATAL, ਪਾਠ); ਰੋਕੋ;
            ਮੂਲ: UASSERT_OBJ(ਝੂਠ, nodep, "Unexpected elaboration display type");
            }
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstDumpCtl* nodep) override {
        assertAtStatement(nodep);
        ਜੇ (nodep->exprp()) iterateCheckString(nodep, "LHS", nodep->exprp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpen* nodep) override {
        // Although a system function in IEEE, here a statement which sets the file pointer (MCD)
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        iterateCheckString(nodep, "mode", nodep->modep(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFOpenMcd* nodep) override {
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFClose* nodep) override {
        assertAtStatement(nodep);
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFError* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            // Could be string or packed array
            user_Iterate_And_ਅਗਲਾ(nodep->strp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFEof* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFFlush* nodep) override {
        assertAtStatement(nodep);
        ਜੇ (nodep->filep()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRewind* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ਖਾਲੀ ਦੇਖੋ(AstFTell* nodep) override { iterateCheckFileDesc(nodep, nodep->filep(), BOTH); }
    ਖਾਲੀ ਦੇਖੋ(AstFSeek* nodep) override {
        iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
        iterateCheckSigned32(nodep, "$fseek offset", nodep->ਵਿੱਥ(), BOTH);
        iterateCheckSigned32(nodep, "$fseek operation", nodep->operation(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFGetC* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFGetS* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->strgp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFUngetC* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            iterateCheckSigned32(nodep, "$fungetc character", nodep->charp(), BOTH);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFRead* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->memp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            ਜੇ (nodep->startp()) {
                iterateCheckSigned32(nodep, "$fread start", nodep->startp(), BOTH);
            }
            ਜੇ (nodep->countp()) {
                iterateCheckSigned32(nodep, "$fread count", nodep->countp(), BOTH);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckFileDesc(nodep, nodep->filep(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->exprsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->fromp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->exprsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਢੇਰ_Trace_F* nodep) override { nodep->dtype_ਸੈੱਟ_String(); }
    ਖਾਲੀ ਦੇਖੋ(AstSysIgnore* nodep) override {
        user_Iterate_And_ਅਗਲਾ(nodep->exprsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSystemF* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstSystemT* nodep) override {
        assertAtStatement(nodep);
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Read_Write_Mem* nodep) override {
        assertAtStatement(nodep);
        iterateCheckString(nodep, "filename", nodep->filenamep(), BOTH);
        user_Iterate_And_ਅਗਲਾ(nodep->memp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        ਅਚਲ Ast_ਗੰਢ_DType* subp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਅਚਲ AstAssocArrayDType* adtypep
            = VN_CAST(nodep->memp()->dtypep()->skipRefp(), AssocArrayDType)) {
            subp = adtypep->subDTypep();
            ਜੇ (!adtypep->ਕੁੰਜੀ_DTypep()->skipRefp()->basicp()
                || !adtypep->ਕੁੰਜੀ_DTypep()->skipRefp()->basicp()->keyword().isIntNumeric()) {
                nodep->memp()->v3error(nodep->verilogKwd()
                                       << " address/key must be integral (IEEE 1800-2023 21.4.1)");
            }
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), UnpackArrayDType)) {
            subp = adtypep->subDTypep();
        } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ adtypep
                   = VN_CAST(nodep->memp()->dtypep()->skipRefp(), DynArrayDType)) {
            subp = adtypep->subDTypep();
        } ਨਹੀਂਤਾਂ {
            nodep->memp()->v3warn(
                E_UNSUPPORTED,
                "Unsupported: " << nodep->verilogKwd()
                                << " into other than unpacked, dynamic, or associative array");
        }
        ਜੇ (subp
            && (!subp->skipRefp()->basicp()
                || !subp->skipRefp()->basicp()->keyword().isIntNumeric())) {
            nodep->memp()->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << nodep->verilogKwd()
                                                  << " array values must be integral");
        }
        user_Iterate_And_ਅਗਲਾ(nodep->lsbp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        user_Iterate_And_ਅਗਲਾ(nodep->msbp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Test_Plus_ਦਲੀਲਾਂ* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਮੁੱਲ_Plus_ਦਲੀਲਾਂ* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckString(nodep, "LHS", nodep->searchp(), BOTH);
            user_Iterate_And_ਅਗਲਾ(nodep->outp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਮਾਂ_Format* nodep) override {
        assertAtStatement(nodep);
        ਜੇ (nodep->unitsp()) iterateCheckSigned32(nodep, "units", nodep->unitsp(), BOTH);
        ਜੇ (nodep->precisionp())
            iterateCheckSigned32(nodep, "precision", nodep->precisionp(), BOTH);
        ਜੇ (nodep->suffixp()) iterateCheckString(nodep, "suffix", nodep->suffixp(), BOTH);
        ਜੇ (nodep->widthp()) iterateCheckSigned32(nodep, "width", nodep->widthp(), BOTH);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        // Just let all arguments seek their natural sizes
        assertAtStatement(nodep);
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssert* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ਅਗਲਾ(nodep->passsp(), ਖਾਲੀ_ਸੂਚਕ);
        user_Iterate_And_ਅਗਲਾ(nodep->failsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssertCtl* nodep) override {
        assertAtStatement(nodep);
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssertIntrinsic* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ਅਗਲਾ(nodep->passsp(), ਖਾਲੀ_ਸੂਚਕ);
        user_Iterate_And_ਅਗਲਾ(nodep->failsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCover* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
        user_Iterate_And_ਅਗਲਾ(nodep->passsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRestrict* nodep) override {
        assertAtStatement(nodep);
        iterateCheckBool(nodep, "Property", nodep->propp(), BOTH);  // it's like an if() condition.
    }

    // Select the part of a pin connection that belongs to an element of an instance array,
    // one dimension at a time, starting with 'rangep'. 'idx' is the row-major position of the
    // element in the dimensions from 'rangep' inwards, each counted from the left. The
    // connection has had its PRELIM visit. Returns true if the pin needs nothing else (the
    // selected connection matches the port, or there was an error), false if the pin still
    // needs the usual checks.
    ਤਾਰਕਿਕ pinInstArrayElement(AstPin* nodep, ਅਚਲ AstRange* rangep, ਪੂਰਨ idx) {
        Ast_ਗੰਢ_DType* ਅਚਲ modDTypep = nodep->modVarp()->dtypep()->skipRefp();
        Ast_ਗੰਢ_DType* ਅਚਲ conDTypep = nodep->exprp()->dtypep()->skipRefp();

        // If types match, then nothing else to do
        ਜੇ (conDTypep == modDTypep || similarDTypeRecurse(conDTypep, modDTypep)) ਵਾਪਸ ਸੱਚ;
        // Types don't match, but all dimensions done, still need the usual checks
        ਜੇ (!rangep) ਵਾਪਸ ਝੂਠ;

        ਅਚਲ AstRange* ਅਚਲ subRangep = VN_AS(rangep->nextp(), Range);

        // Number of dimensions from 'rangep' inwards, and of elements in those after 'rangep'
        uint32_t nDims = 1;
        ਪੂਰਨ subElems = 1;
        ਲਈ (ਅਚਲ AstRange* rp = subRangep; rp; rp = VN_AS(rp->nextp(), Range)) {
            ++nDims;
            subElems *= rp->elementsConst();
        }
        ਅਚਲ ਪੂਰਨ elements = rangep->elementsConst();
        ਅਚਲ ਪੂਰਨ from_ਖੱਬਾ = idx / subElems;

        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(conDTypep, UnpackArrayDType);
        ਅਚਲ uint32_t conDims = conDTypep->dimensions(ਝੂਠ).ਦੂਜਾ;
        ਅਚਲ uint32_t modDims = modDTypep->dimensions(ਝੂਠ).ਦੂਜਾ;

        // Unpacked array with other than the port's number of unpacked dimensions: its slowest
        // varying unpacked dimensions must match the instance array dimensions exactly in size,
        // and the rest be those of the port. Connect its element at the same position, counted
        // from the left. The inner dimensions then select from that element the same way.
        ਜੇ (arrp && conDims != modDims) {
            ਜੇ (conDims != nDims + modDims || arrp->elementsConst() != elements) {
                nodep->v3error(ucfirst(nodep->prettyOperatorName())
                               << " as part of a module instance array requires the connection's"
                                  " leading unpacked dimensions to match the instance array"
                                  " dimensions (IEEE 1800-2023 23.3.3.5)");
                user_Iterate_And_ਅਗਲਾ(nodep->exprp(),
                                   ਚੌੜਾਈ_VP{conDTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
                ਵਾਪਸ ਸੱਚ;
            }
            // ArraySel index is counted from lo
            ਅਚਲ ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2 = arrp->declRange().ascending() ? from_ਖੱਬਾ : elements - 1 - from_ਖੱਬਾ;
            // Connection is self-determined
            user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{conDTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_AS(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)->unlink_Fr_ਪਿੱਠ();
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elemp = newIfaceArrayElement(fromp, ਦੋਆਧਾਰੀ_ਅੰਕ_2)) {
                nodep->exprp(elemp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(fromp), fromp);
            } ਨਹੀਂਤਾਂ {
                AstArraySel* ਅਚਲ selp = ਨਵਾਂ AstArraySel{fromp->fileline(), fromp, ਦੋਆਧਾਰੀ_ਅੰਕ_2};
                selp->did_ਚੌੜਾਈ(ਸੱਚ);
                nodep->exprp(selp);
            }
            ਵਾਪਸ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise if not packed (including an unpacked array with the port's number of unpacked
        // dimensions), connect it to every element, if it is compatible with the port
        ਜੇ (!conDTypep->isIntegralOrPacked() || !modDTypep->isIntegralOrPacked()) ਵਾਪਸ ਝੂਠ;

        // Packed connection
        ਅਚਲ ਪੂਰਨ modwidth = modDTypep->ਚੌੜਾਈ();
        ਅਚਲ ਪੂਰਨ conwidth = conDTypep->ਚੌੜਾਈ();

        // Connection has the width of the port, connect it to every element
        ਜੇ (conwidth == modwidth) ਵਾਪਸ ਝੂਠ;

        // Connection has the width of all the elements, connect a slice to each element,
        // leftmost element to the leftmost (most significant) slice
        ਜੇ (conwidth == modwidth * elements * subElems) {
            ਜੇ (rangep->ascending()) {
                nodep->exprp()->v3warn(ASCRANGE, "Ascending instance range connecting to "
                                                 "vector: left < right of instance range: ["
                                                     << rangep->ਖੱਬਾ_Const() << ":"
                                                     << rangep->ਸੱਜਾ_Const() << "]");
            }
            ਅਚਲ ਪੂਰਨ selwidth = modwidth * subElems;
            // Connection is self-determined
            user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{conDTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ fromp = VN_AS(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ)->unlink_Fr_ਪਿੱਠ();
            AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{fromp->fileline(), fromp,
                                            selwidth * (elements - 1 - from_ਖੱਬਾ), selwidth};
            selp->did_ਚੌੜਾਈ(ਸੱਚ);
            nodep->exprp(selp);
            ਵਾਪਸ pinInstArrayElement(nodep, subRangep, idx % subElems);
        }

        // Otherwise it is an error, as it must connect to one element or to all of them
        nodep->v3error(ucfirst(nodep->prettyOperatorName())
                       << " as part of a module instance array" << " requires " << modwidth
                       << " or " << modwidth * elements * subElems << " bits, but connection's "
                       << nodep->exprp()->prettyTypeName() << " generates " << conwidth
                       << " bits. (IEEE 1800-2023 23.3.3)");
        user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{conDTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
        ਵਾਪਸ ਸੱਚ;
    }

    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        // UINFOTREE(1, nodep, "", "PinPre");
        // TOP LEVEL NODE
        ਜੇ (nodep->modVarp() && nodep->modVarp()->isGParam()) {
            // An unpacked array concatenation is a pattern of its items (IEEE 1800-2023 10.10)
            AstReplicate* ਅਚਲ concatp = VN_CAST(nodep->exprp(), Replicate);
            ਅਚਲ AstConst* ਅਚਲ countp = concatp ? VN_CAST(concatp->countp(), Const) : ਖਾਲੀ_ਸੂਚਕ;
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->modVarp()->subDTypep()->skipRefOrNullp();
            ਜੇ (countp && countp->ਮੰਜ਼ਿਲ_UInt() == 1
                && (VN_IS(dtypep, UnpackArrayDType) || VN_IS(dtypep, BracketArrayDType))) {
                Ast_ਨਮੂਨਾ* ਅਚਲ patternp = ਨਵਾਂ Ast_ਨਮੂਨਾ{concatp->fileline(), ਖਾਲੀ_ਸੂਚਕ};
                patConcatConvertRecurse(patternp, concatp->srcp());
                concatp->replaceWith(patternp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(concatp), concatp);
            }
            // An override pattern is left for the specialized module to type
            VL_RESTORER(m_ਵਿੱਚ_Param_Override);
            m_ਵਿੱਚ_Param_Override = is_Override_ਨਮੂਨਾ(nodep->exprp());
            userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        } ਨਹੀਂਤਾਂ ਜੇ (!m_paramsOnly) {
            ਜੇ (!nodep->modVarp()->did_ਚੌੜਾਈ()) {
                // Var hasn't been widthed, so make it so.
                userIterate(nodep->modVarp(), ਖਾਲੀ_ਸੂਚਕ);
            }
            ਜੇ (!nodep->exprp()) {  // No-connect
                ਜੇ (nodep->modVarp()->is_Iface_Array_ਵੰਡੋ()) expandIfaceArrayPin(nodep);
                ਵਾਪਸ;
            }
            // Very much like like an assignment, but which side is LH/RHS
            // depends on pin being a in/output/inout.
            ਅਚਲ VDirection pinDirection = nodep->modVarp()->direction();
            ਜੇ (VN_IS(nodep->exprp(), ਗੰਢ_ਧਾਰਾ) && pinDirection.isInoutOrRef()) {
                nodep->exprp()->v3error("Streaming concatenation cannot be connected to '"
                                        << pinDirection.prettyName()
                                        << "' port (IEEE 1800-2023 11.4.14)");
            }
            user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{nodep->modVarp()->dtypep(), PRELIM}.p());
            Ast_ਗੰਢ_DType* modDTypep = nodep->modVarp()->dtypep();
            Ast_ਗੰਢ_DType* conDTypep = nodep->exprp()->dtypep();
            UASSERT_OBJ(modDTypep, nodep, "Unlinked pin data type");
            UASSERT_OBJ(conDTypep, nodep, "Unlinked pin data type");
            modDTypep = modDTypep->skipRefp();
            conDTypep = conDTypep->skipRefp();
            // An interface port, or an array of them, needs interface instances, not virtual
            // interfaces
            ਜੇ (ਅਚਲ AstIfaceRefDType* ਅਚਲ modIrefp
                = VN_CAST(modDTypep->elemDTypep(), IfaceRefDType)) {
                ਅਚਲ AstIfaceRefDType* ਅਚਲ conIrefp
                    = VN_CAST(conDTypep->elemDTypep(), IfaceRefDType);
                ਜੇ (!modIrefp->isVirtual() && conIrefp && conIrefp->isVirtual()) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName()
                                              << ", interface port connected to a virtual"
                                                 " interface (IEEE 1800-2023 23.3.3.4)");
                }
            }
            Ast_ਗੰਢ_DType* subDTypep = modDTypep;
            ਅਚਲ ਪੂਰਨ modwidth = modDTypep->ਚੌੜਾਈ();
            ਪੂਰਨ conwidth = conDTypep->ਚੌੜਾਈ();
            ਜੇ (conDTypep == modDTypep  // If match, we're golden
                || similarDTypeRecurse(conDTypep, modDTypep)) {
                user_Iterate_And_ਅਗਲਾ(nodep->exprp(),
                                   ਚੌੜਾਈ_VP{subDTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
            } ਨਹੀਂਤਾਂ {
                ਜੇ (m_cellp->arrayIdx() >= 0) {
                    // Element of an instance array, select its part of the connection,
                    // then check it like any other pin, unless that is all done already
                    ਜੇ (pinInstArrayElement(nodep, m_cellp->rangep(), m_cellp->arrayIdx())) {
                        ਜੇ (nodep->modVarp()->is_Iface_Array_ਵੰਡੋ()) expandIfaceArrayPin(nodep);
                        ਵਾਪਸ;
                    }
                    conDTypep = nodep->exprp()->dtypep()->skipRefp();
                    conwidth = conDTypep->ਚੌੜਾਈ();
                }
                ਜੇ (nodep->modVarp()->direction() == VDirection::REF) {
                    nodep->v3error("Ref connection "
                                   << nodep->modVarp()->prettyNameQ()
                                   << " requires matching types;" << " ref requires "
                                   << modDTypep->prettyDTypeNameQ()
                                   << " data type but connection is "
                                   << conDTypep->prettyDTypeNameQ() << " data type.");
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->modVarp()->isTristate()) {
                    ਜੇ (modwidth != conwidth) {
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
                } ਨਹੀਂਤਾਂ ਜੇ (nodep->modVarp()->direction().isWritable()
                           && ((conDTypep->isDouble() && !modDTypep->isDouble())
                               || (!conDTypep->isDouble() && modDTypep->isDouble()))) {
                    nodep->v3warn(E_UNSUPPORTED,
                                  "Unsupported: " << ucfirst(nodep->prettyOperatorName())
                                                  << " connects real to non-real");
                }

                // Check if an interface is connected to a non-interface and vice versa
                ਜੇ ((VN_IS(modDTypep, IfaceRefDType) && !VN_IS(conDTypep, IfaceRefDType))
                    || (VN_IS(conDTypep, IfaceRefDType) && !VN_IS(modDTypep, IfaceRefDType))) {
                    nodep->v3error("Illegal " << nodep->prettyOperatorName() << ","
                                              << " mismatch between port which is"
                                              << (VN_CAST(modDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface," << " and expression which is"
                                              << (VN_CAST(conDTypep, IfaceRefDType) ? "" : " not")
                                              << " an interface.");
                }

                // TODO Simple dtype checking, should be a more general check
                ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ ਵਿਅੰਜਕ_Arrayp = VN_CAST(conDTypep, UnpackArrayDType);
                ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ modArrayp = VN_CAST(modDTypep, UnpackArrayDType);
                ਜੇ (ਵਿਅੰਜਕ_Arrayp && modArrayp && VN_IS(conDTypep->elemDTypep(), IfaceRefDType)
                    && unpacked_Array_ਮਾਪ(conDTypep) != unpacked_Array_ਮਾਪ(modDTypep)) {
                    nodep->v3error("Illegal "
                                   << nodep->prettyOperatorName() << ","
                                   << " mismatch between port which is an interface array of size "
                                   << unpacked_Array_ਮਾਪ(modDTypep) << ","
                                   << " and expression which is an interface array of size "
                                   << unpacked_Array_ਮਾਪ(conDTypep) << ".");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ਨਹੀਂਤਾਂ ਜੇ ((ਵਿਅੰਜਕ_Arrayp && !modArrayp) || (!ਵਿਅੰਜਕ_Arrayp && modArrayp)) {
                    nodep->v3error(
                        "Illegal "
                        << nodep->prettyOperatorName() << "," << " mismatch between port which is"
                        << (modArrayp ? "" : " not") << " an array," << " and expression which is"
                        << (ਵਿਅੰਜਕ_Arrayp ? "" : " not") << " an array. (IEEE 1800-2023 7.6)");
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                } ਨਹੀਂਤਾਂ {
                    checkUnpackedArrayAssignmentCompatible<AstVar, Ast_ਗੰਢ_Var_Ref>(
                        nodep, nodep->modVarp(), VN_CAST(nodep->exprp(), ਗੰਢ_Var_Ref));
                    UINFO(1, "    Related lo: " << modDTypep);
                    UINFO(1, "    Related hi: " << conDTypep);
                }
                iterate_Check_ਸੌਂਪੋ(nodep, "pin connection", nodep->exprp(), FINAL, subDTypep,
                                   nodep->modVarp()->direction().pinAccess());
            }
            // Interface array port: connect its elements
            ਜੇ (nodep->modVarp()->is_Iface_Array_ਵੰਡੋ()) expandIfaceArrayPin(nodep);
        }
        // UINFOTREE(1, nodep, "", "PinOut");
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        VL_RESTORER(m_cellp);
        m_cellp = nodep;
        ਜੇ (!m_paramsOnly) {
            ਜੇ (VN_IS(nodep->modp(), Not_ਮਿਲਿਆ_Module)) {
                // We've resolved parameters and hit a module that we couldn't resolve.  It's
                // finally time to report it.
                // Note only here in V3Width as this is first visitor after V3Dead.
                nodep->modNameFileline()->v3warn(
                    MODMISSING, "Cannot find file containing module: '"
                                    << nodep->modName() << "'\n"
                                    << nodep->modNameFileline()->warnContextPrimary()
                                    << V3Error::warnAdditionalInfo()
                                    << v3Global.opt.file_ਰਸਤਾ_Looked_ਸੁਨੇਹਾ(nodep->modNameFileline(),
                                                                      nodep->modName()));
            }
            ਜੇ (nodep->rangep()) user_Iterate_And_ਅਗਲਾ(nodep->rangep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->pinsp(), ਖਾਲੀ_ਸੂਚਕ);
            // If element of an instance array, pins are now connected, no longer need the ranges
            ਜੇ (nodep->arrayIdx() >= 0) {
                nodep->arrayIdx(-1);
                ਧੱਕੋ_Deletep(nodep->rangep()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
            }
        }
        user_Iterate_And_ਅਗਲਾ(nodep->paramsp(), ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstGatePin* nodep) override {
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->rangep(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            nodep->dtypeFrom(nodep->rangep());
            // Very much like like an pin
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ pinDTypep = nodep->exprp()->dtypep();
            ਅਚਲ ਪੂਰਨ ਅੰਕ_Insts = nodep->rangep()->elementsConst();
            ਅਚਲ ਪੂਰਨ modwidth = ਅੰਕ_Insts;
            ਅਚਲ ਪੂਰਨ pinwidth = pinDTypep->ਚੌੜਾਈ();
            ਜੇ (pinwidth == 1 && modwidth > 1) {  // Multiple connections
                Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = nodep->ਲੱਭੋ_Logic_DType(1, 1, pinDTypep->numeric());
                user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{subDTypep, FINAL}.p());
                Ast_ਗੰਢ* ਅਚਲ newp
                    = ਨਵਾਂ AstReplicate{nodep->fileline(), nodep->exprp()->unlink_Fr_ਪਿੱਠ(),
                                       static_cast<uint32_t>(ਅੰਕ_Insts)};
                nodep->replaceWith(newp);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (pinwidth != modwidth) {  // && is not generic interconnect (when supported)
                    nodep->exprp()->v3error("Gate primitive connection expects "
                                            << modwidth << " bits "
                                            << ((modwidth != 1) ? "or 1 bit "s : ""s)
                                            << "on the gate port, but the connection generates "
                                            << pinwidth << " bits (IEEE 1800-2023 28.3.6)");
                }
                // Eliminating so pass down all of vup
                user_Iterate_And_ਅਗਲਾ(nodep->exprp(), m_vup);
                nodep->replaceWith(nodep->exprp()->unlink_Fr_ਪਿੱਠ());
            }
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        // Grab width from the output variable (if it's a function)
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        ਜੇ (nodep->doing_ਚੌੜਾਈ()) {
            UINFO(5, "Recursive function or task call: " << nodep);
            nodep->recursive(ਸੱਚ);
            nodep->did_ਚੌੜਾਈ(ਸੱਚ);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->class_ਵਿਧੀ() && nodep->name() == "rand_mode") {
            nodep->v3error("The 'rand_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.8)");
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->class_ਵਿਧੀ() && nodep->name() == "constraint_mode") {
            nodep->v3error("The 'constraint_mode' method is built-in and cannot be overridden"
                           " (IEEE 1800-2023 18.9)");
        }
        ਜੇ (nodep->class_ਵਿਧੀ() && nodep->name() == "new") {
            ਜੇ (nodep->isVirtual())
                nodep->v3error("class 'new()' cannot be virual (IEEE 1800-2023 18.3)");
            ਜੇ (nodep->isStatic())
                nodep->v3error("class 'new()' cannot be static (IEEE 1800-2023 18.3)");
            Ast_ਗੰਢ* firstp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (Ast_ਗੰਢ* itemp = nodep->stmtsp(); itemp; itemp = itemp->nextp()) {
                ਜੇ (Ast_Stmt_ਵਿਅੰਜਕ* ਅਚਲ sep = VN_CAST(itemp, Stmt_ਵਿਅੰਜਕ)) {
                    ਜੇ (AstNew* ਅਚਲ newp = VN_CAST(sep->exprp(), New)) {
                        ਜੇ (firstp) {
                            UINFOTREE(1, firstp, "", "-earlier");
                            newp->v3warn(SUPERNFIRST,
                                         "'super.new' must be first statement in a 'function "
                                         "new' (IEEE 1800-2023 8.15)\n"
                                             << newp->warnContextPrimary() << '\n'
                                             << firstp->warnOther()
                                             << "... Location of earlier statement\n"
                                             << firstp->warnContextSecondary());
                            ਰੋਕੋ;
                        }
                    }
                    ਜਾਰੀ;
                }
                ਜੇ (ਪਹਿਲਾ_New_Statement_Ok_Recurse(itemp)) ਜਾਰੀ;
                firstp = itemp;
            }
        }
        // Function hasn't been widthed, so make it so.
        // Would use user1 etc, but V3Width called from too many places to spend a user
        nodep->doing_ਚੌੜਾਈ(ਸੱਚ);
        VL_RESTORER(m_funcp);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // First width the function variable, as if is a recursive function we need data type
        ਜੇ (nodep->fvarp()) userIterate(nodep->fvarp(), ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (nodep->isConstructor()) {
            // Pretend it's void so less special casing needed when look at dtypes
            nodep->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->fvarp()) {
            m_funcp = VN_AS(nodep, Func);
            UASSERT_OBJ(m_funcp, nodep, "FTask with function variable, but isn't a function");
            nodep->dtypeFrom(nodep->fvarp());  // Which will get it from fvarp()->dtypep()
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Property)) {
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
        ਚੌੜਾਈ_VP* vup = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (VN_IS(nodep, Property)) vup = ਚੌੜਾਈ_VP{SELF, BOTH}.p();
        userIterateChildren(nodep, vup);

        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
        ਜੇ (nodep->dpiImport() && !nodep->dpi_Open_ਮਾਪਾ() && markHasOpenArray(nodep)) {
            nodep->dpi_Open_ਮਾਪਾ_Inc();  // Mark so V3Task will wait for a child to build calling
                                        // func
        }
        ਜੇ (nodep->fvarp() && !nodep->dpiImport() && !nodep->ਸ਼ੁੱਧ_Virtual()
            && (!nodep->stmtsp() || !nodep->stmtsp()->exists_And_ਅਗਲਾ([&](ਅਚਲ AstVarRef* varrefp) {
                   ਵਾਪਸ varrefp->varp() == nodep->fvarp();
               }))) {
            nodep->v3warn(NORETURN,
                          "Non-void function " << nodep->prettyNameQ() << " has no return value");
        }

        std::ਸਦਿਸ਼<AstVar*> ports;
        std::ਸਦਿਸ਼<Ast_Attr_ਦਾ*> protos;
        Ast_Attr_ਦਾ* protop = ਖਾਲੀ_ਸੂਚਕ;  // Base prototype
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isIO() && !portp->isFuncReturn()) { ports.emplace_ਪਿੱਠ(portp); }
            } ਨਹੀਂਤਾਂ ਜੇ (Ast_Attr_ਦਾ* ਅਚਲ attrp = VN_CAST(stmtp, Attr_ਦਾ)) {
                ਜੇ (attrp->attrType() == VAttrType::FUNC_ARG_PROTO) { protos.emplace_ਪਿੱਠ(attrp); }
                ਜੇ (attrp->attrType() == VAttrType::FUNC_RETURN_PROTO) { protop = attrp; }
            }
        }
        ਜੇ (protop) {
            ਲਈ (ਮਾਪ_t i = 0; i < std::max(ports.ਮਾਪ(), protos.ਮਾਪ()); ++i) {
                ਜੇ (i >= ports.ਮਾਪ() || i >= protos.ਮਾਪ()) {
                    protop->v3warn(
                        PROTOTYPEMIS,
                        "In prototype for "
                            << nodep->prettyNameQ()
                            << ", the argument counts do not match the out-of-block declaration"
                            << " (IEEE 1800-2023 8.24)\n"
                            << protop->warnContextPrimary() << '\n'
                            << nodep->warnOther() << "... Location of out-of-block declaration\n"
                            << nodep->warnContextSecondary());
                    ਰੋਕੋ;
                } ਨਹੀਂਤਾਂ {
                    AstVar* ਅਚਲ portp = ports[i];
                    Ast_Attr_ਦਾ* ਅਚਲ rprotop = protos[i];
                    Ast_ਗੰਢ_DType* ਅਚਲ declDtp = portp->dtypep();
                    Ast_ਗੰਢ_DType* ਅਚਲ protoDtp = rprotop->fromp()->dtypep();
                    ਜੇ (portp->name() != rprotop->name()) {
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
                    } ਨਹੀਂਤਾਂ ਜੇ (!similarDTypeRecurse(protoDtp, declDtp)) {
                        protoDtp->v3warn(
                            PROTOTYPEMIS,
                            "In prototype for "
                                << nodep->prettyNameQ() << ", argument " << portp->prettyNameQ()
                                << " data-type does not match out-of-block"
                                   " declaration's data-type (IEEE 1800-2023 8.24)\n"
                                << protoDtp->warn_ਹੋਰ() << "... Prototype data type: "
                                << protoDtp->prettyDTypeNameQ() << '\n'
                                << protoDtp->warn_ਹੋਰ() << "... Declaration data type: "
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
    ਖਾਲੀ ਦੇਖੋ(AstConstraint* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        VL_RESTORER(m_constraintp);
        m_constraintp = nodep;
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstProperty* nodep) override {
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        ਜੇ (nodep->doing_ਚੌੜਾਈ()) {
            UINFO(5, "Recursive property call: " << nodep);
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Recursive property call: " << nodep->prettyNameQ());
            nodep->recursive(ਸੱਚ);
            nodep->did_ਚੌੜਾਈ(ਸੱਚ);
            ਵਾਪਸ;
        }
        nodep->doing_ਚੌੜਾਈ(ਸੱਚ);
        VL_RESTORER(m_ftaskp);
        m_ftaskp = nodep;
        // Property call will be replaced by property body in V3AssertPre. Property body has bit
        // dtype, so set it here too
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        ਲਈ (Ast_ਗੰਢ* propStmtp = nodep->stmtsp(); propStmtp; propStmtp = propStmtp->nextp()) {
            ਜੇ (VN_IS(propStmtp, Var)) {
                userIterate(propStmtp, ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(propStmtp, PropSpec)) {
                iterateCheckSelf(nodep, "PropSpec", propStmtp, SELF, BOTH);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(propStmtp, InitialStaticStmt)
                       || VN_IS(propStmtp, InitialAutomaticStmt)) {
                // Property-local variable initialization -- iterate this node only
                userIterate(propStmtp, ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                propStmtp->v3fatalSrc("Invalid statement under AstProperty");
            }
        }
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        nodep->doing_ਚੌੜਾਈ(ਝੂਠ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstReturn* nodep) override { nodep->v3fatalSrc("'return' missed in LinkJump"); }
    ਖਾਲੀ ਦੇਖੋ(AstSequence* nodep) override {
        // IEEE 1800-2023 16.7 (sequence declarations), 16.8 (sequence instances)
        // Width-check the sequence body so expressions are typed for later inlining.
        // Referenced sequences in assertion context will be inlined by V3AssertPre.
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (VN_IS(stmtp, Var)) {
                userIterate(stmtp, ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(stmtp, ਗੰਢ_ਵਿਅੰਜਕ)) {
                iterateCheckSelf(nodep, "SeqBody", stmtp, SELF, BOTH);
            } ਨਹੀਂਤਾਂ {
                stmtp->v3fatalSrc("Invalid statement under AstSequence");
            }
        }
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        // Keep all sequences for now; cleanup happens after V3AssertPre inlining.
    }

    AstPackage* get_ਵਸਤੂ_Package(Ast_ਗੰਢ* pkgItemp) {
        ਜਦੋਂ (pkgItemp->backp() && pkgItemp->backp()->nextp() == pkgItemp) {
            pkgItemp = pkgItemp->backp();
        }
        ਵਾਪਸ VN_CAST(pkgItemp->backp(), Package);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFuncRef* nodep) override {
        ਦੇਖੋ(static_cast<Ast_ਗੰਢ_FTask_Ref*>(nodep));
        ਜੇ (nodep->taskp() && VN_IS(nodep->taskp(), Task)) {
            UASSERT_OBJ(m_vup, nodep, "Function reference where widthed expression expectation");
            ਜੇ (m_vup->prelim() && !VN_IS(nodep->backp(), Stmt_ਵਿਅੰਜਕ))
                nodep->v3error(
                    "Cannot call a task/void-function as a function: " << nodep->prettyNameQ());
            nodep->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ {
            nodep->dtypeFrom(nodep->taskp());
        }
        ਜੇ (nodep->fileline()->timingOn()) {
            Ast_ਗੰਢ_Module* ਅਚਲ classp = nodep->classOrPackagep();
            ਜੇ (nodep->name() == "self" && classp->name() == "process") {
                // Find if package the class is in is std::
                AstPackage* ਅਚਲ packagep = get_ਵਸਤੂ_Package(classp);
                ਜੇ (packagep && packagep->name() == "std") {
                    ਵਿਧੀ_Call_Warn_Timing(nodep, "process");
                }
            }
        }
        // UINFOTREE(1, nodep, "", "FuncOut");
    }
    // Returns true if dtypep0 and dtypep1 have same dimensions
    ਸਥਿਰ ਤਾਰਕਿਕ are_Same_ਮਾਪ(AstUnpackArrayDType* dtypep0, AstUnpackArrayDType* dtypep1) {
        ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims0 = dtypep0->unpackDimensions();
        ਅਚਲ std::ਸਦਿਸ਼<AstUnpackArrayDType*> dims1 = dtypep1->unpackDimensions();
        ਜੇ (dims0.ਮਾਪ() != dims1.ਮਾਪ()) ਵਾਪਸ ਝੂਠ;
        ਲਈ (ਮਾਪ_t i = 0; i < dims0.ਮਾਪ(); ++i) {
            ਜੇ (dims0[i]->elementsConst() != dims1[i]->elementsConst()) ਵਾਪਸ ਝੂਠ;
        }
        ਵਾਪਸ ਸੱਚ;
    }
    // Makes sure that port and pin have same size and same datatype
    ਖਾਲੀ check_Unpacked_Array_ਦਲੀਲਾਂ(AstVar* portp, Ast_ਗੰਢ* pinp) {
        ਜੇ (AstUnpackArrayDType* ਅਚਲ ਪੋਰਟ_Dtypep
            = VN_CAST(portp->dtypep()->skipRefp(), UnpackArrayDType)) {
            ਜੇ (AstUnpackArrayDType* ਅਚਲ pinDtypep
                = VN_CAST(pinp->dtypep()->skipRefp(), UnpackArrayDType)) {
                ਜੇ (!are_Same_ਮਾਪ(ਪੋਰਟ_Dtypep, pinDtypep)) {
                    pinp->v3warn(E_UNSUPPORTED,
                                 "Shape of the argument does not match the shape of the parameter "
                                     << "(" << pinDtypep->prettyDTypeNameQ() << " v.s. "
                                     << ਪੋਰਟ_Dtypep->prettyDTypeNameQ() << ")");
                }
                ਅਚਲ AstBasicDType* ਅਚਲ ਪੋਰਟ_Basicp = ਪੋਰਟ_Dtypep->basicp();
                ਅਚਲ AstBasicDType* ਅਚਲ pinBasicp = pinDtypep->basicp();
                // Arrays of unpacked structures have no basic type
                ਅਚਲ ਤਾਰਕਿਕ sameElemDType
                    = ਪੋਰਟ_Basicp && pinBasicp
                          ? ਪੋਰਟ_Basicp->ਚੌੜਾਈ() == pinBasicp->ਚੌੜਾਈ()
                                && ਪੋਰਟ_Basicp->keyword().isSameish(pinBasicp->keyword())
                          : pinDtypep->elemDTypep()->similarDType(ਪੋਰਟ_Dtypep->elemDTypep());
                ਜੇ (!sameElemDType) {
                    pinp->v3warn(
                        E_UNSUPPORTED,
                        "Data type of the argument is incompatible with the parameter"
                            << warnDTypes(pinp, pinDtypep, ਪੋਰਟ_Dtypep, "Argument", "Parameter"));
                }
            } ਨਹੀਂਤਾਂ {
                pinp->v3warn(E_UNSUPPORTED, "Argument is not an unpacked array while parameter "
                                                << portp->prettyNameQ() << " is");
            }
        }
    }
    ਖਾਲੀ process_FTask_Ref_ਦਲੀਲਾਂ(Ast_ਗੰਢ_FTask_Ref* nodep) {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        UINFO(5, "  FTASKREF " << nodep);
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        userIterate(nodep->taskp(), ਖਾਲੀ_ਸੂਚਕ);
        //
        // And do the arguments to the task/function too
        ਕਰੋ {
        reloop:
            // taskConnects may create a new task, and change nodep->taskp()
            ਅਚਲ V3TaskConnects tconnects
                = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp(), &m_taskConnectState);
            ਜੇ (m_taskConnectState.didWrap()) m_memberMap.clear();  // As added a member
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
                ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
                AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinp = argp->exprp();
                ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
                // Prelim may cause the node to get replaced; we've lost our
                // pointer, so need to iterate separately later
                ਜੇ (portp->attrSFormat()
                    && (!VN_IS(pinp, SFormatF) || pinp->nextp())) {  // Not already done
                    UINFO(4, "   sformat via metacomment: " << nodep);
                    VNRelinker handle;
                    argp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ(&handle);  // Format + additional args, if any
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* argsp = ਖਾਲੀ_ਸੂਚਕ;
                    ਜਦੋਂ (AstArg* ਅਚਲ nextargp = VN_AS(argp->nextp(), Arg)) {
                        // Expression goes to SFormatF
                        argsp = argsp->ਜੋੜੋ_ਅਗਲਾ(nextargp->exprp()->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ());
                        nextargp->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ();  // Remove the call's Arg wrapper
                    }
                    string format;
                    ਜੇ (VN_IS(pinp, Const)) {
                        format = VN_AS(pinp, Const)->ਅੰਕ().ਮੰਜ਼ਿਲ_String();
                    } ਨਹੀਂਤਾਂ {
                        pinp->v3error(
                            "Format to $display-like function must have constant format string");
                    }
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(argp), argp);
                    AstSFormatF* ਅਚਲ newp
                        = ਨਵਾਂ AstSFormatF{nodep->fileline(), format, ਝੂਠ, argsp};
                    ਜੇ (!newp->scopeNamep() && newp->formatScopeTracking()) {
                        newp->scopeNamep(ਨਵਾਂ AstScopeName{newp->fileline(), ਸੱਚ});
                    }
                    handle.relink(ਨਵਾਂ AstArg{newp->fileline(), "", newp});
                    // Connection list is now incorrect (has extra args in it).
                    ਜਾਓ reloop;  // so exit early; next loop will correct it
                }  //
                ਨਹੀਂਤਾਂ ਜੇ (portp->basicp() && portp->basicp()->keyword() == VBasicDTypeKwd::STRING
                         && !VN_IS(pinp, CvtPackString)
                         && !VN_IS(pinp, SFormatF)  // Already generates a string
                         && !VN_IS(portp->dtypep(), UnpackArrayDType)  // Unpacked array must match
                         && !(VN_IS(pinp, VarRef) && VN_AS(pinp, VarRef)->varp()->basicp()
                              && VN_AS(pinp, VarRef)->varp()->basicp()->keyword()
                                     == VBasicDTypeKwd::STRING)) {
                    UINFO(4, "   Add CvtPackString: " << pinp);
                    VNRelinker handle;
                    pinp->unlink_Fr_ਪਿੱਠ(&handle);  // No next, that's the next pin
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstCvtPackString{pinp->fileline(), pinp};
                    handle.relink(newp);
                    pinp = newp;
                }
                ਜੇ (portp->isWritable()) {
                    V3LinkLValue::link_LValue_ਸੈੱਟ(pinp, portp->direction().pinAccess());
                }
                // AstPattern requires assignments to pass datatype on PRELIM
                VL_DO_DANGLING(userIterate(pinp, ਚੌੜਾਈ_VP{portp->dtypep(), PRELIM}.p()), pinp);
            }
        } ਜਦੋਂ (ਝੂਠ);
        // Stage 2
        {
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
                AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
                ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = argp->exprp();
                ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
                // Change data types based on above accept completion
                ਜੇ (nodep->taskp()->dpiImport()) check_Unpacked_Array_ਦਲੀਲਾਂ(portp, pinp);
                ਜੇ (portp->isDouble()) VL_DO_DANGLING(spliceCvtD(pinp), pinp);
            }
        }
        // Stage 3
        {
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
                ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
                ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ pinp = argp->exprp();
                ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
                // Do PRELIM again, because above accept may have exited early
                // due to node replacement
                userIterate(pinp, ਚੌੜਾਈ_VP{portp->dtypep(), PRELIM}.p());
            }
        }
        // Cleanup any open arrays
        ਜੇ (markHasOpenArray(nodep->taskp())) makeOpenArrayShell(nodep);
        // Stage 4
        {
            ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
                ਅਚਲ AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
                ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* pinp = argp->exprp();
                ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
                Ast_ਗੰਢ_DType* ਅਚਲ ਪੋਰਟ_DTypep = portp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                ਜੇ (VN_IS(pinp, ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ)) {
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = new_Cons_Dyn_Array_Or_ਕਤਾਰ(pinp->fileline(), ਪੋਰਟ_DTypep);
                    ਜੇ (!newp) {
                        pinp->v3warn(E_UNSUPPORTED,
                                     "Unsupported/Illegal: empty queue ('{}') in this context");
                        newp = ਨਵਾਂ AstConst{pinp->fileline(), AstConst::Unsized32{}, 0};
                    }
                    newp->dtypeFrom(ਪੋਰਟ_DTypep);
                    pinp->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(pinp), pinp);
                    pinp = newp;
                }
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ pinDTypep = pinp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
                ਅਚਲ AstIfaceRefDType* ਅਚਲ ਪੋਰਟ_Ifacep
                    = VN_CAST(ਪੋਰਟ_DTypep->elemDTypep(ਸੱਚ), IfaceRefDType);
                ਅਚਲ VCastable ifaceCastable
                    = ਪੋਰਟ_Ifacep ? Ast_ਗੰਢ::computeCastable(ਪੋਰਟ_DTypep, pinDTypep, pinp,
                                                            /* checkIfaceArgCompat */ ਸੱਚ)
                                 : VCastable{VCastable::UNSUPPORTED};
                ਅਚਲ ਤਾਰਕਿਕ matchingRefDTypes = ਪੋਰਟ_Ifacep
                                                   ? ifaceCastable == VCastable::SAMEISH
                                                   : similarDTypeRecurse(ਪੋਰਟ_DTypep, pinDTypep);
                ਅਚਲ ਤਾਰਕਿਕ pinIsNull = VN_IS(pinp, Const) && VN_AS(pinp, Const)->ਅੰਕ().isNull();
                ਜੇ ((portp->isRef() || portp->isConstRef()) && !matchingRefDTypes) {
                    ਜੇ (ਪੋਰਟ_Ifacep) {
                        pinp->v3error("Ref virtual interface argument "
                                      << portp->prettyNameQ()
                                      << " requires the same interface type, parameters, and "
                                         "modport.");
                    } ਨਹੀਂਤਾਂ {
                        pinp->v3error("Ref argument requires matching types;"
                                      << " port " << portp->prettyNameQ() << " requires "
                                      << ਪੋਰਟ_DTypep->prettyDTypeNameQ() << " but connection is "
                                      << pinDTypep->prettyDTypeNameQ() << ".");
                    }
                } ਨਹੀਂਤਾਂ ਜੇ (ਪੋਰਟ_Ifacep && ਪੋਰਟ_Ifacep->isVirtual() && portp->is_ਨਿਵੇਸ਼() && !pinIsNull
                           && !ifaceCastable.isAssignable()) {
                    pinp->v3error("Virtual interface argument "
                                  << portp->prettyNameQ()
                                  << " requires a compatible interface type, parameters, and "
                                     "modport.");
                } ਨਹੀਂਤਾਂ ਜੇ (portp->isWritable() && pinp->ਚੌੜਾਈ() != portp->ਚੌੜਾਈ()) {
                    pinp->v3widthWarn(portp->ਚੌੜਾਈ(), pinp->ਚੌੜਾਈ(),
                                      "Function output argument "
                                          << portp->prettyNameQ() << " requires " << portp->ਚੌੜਾਈ()
                                          << " bits, but connection's " << pinp->prettyTypeName()
                                          << " generates " << pinp->ਚੌੜਾਈ() << " bits.");
                    VNRelinker relinkHandle;
                    pinp->unlink_Fr_ਪਿੱਠ(&relinkHandle);
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstResizeLValue{pinp->fileline(), pinp};
                    relinkHandle.relink(newp);
                }
                ਜੇ (portp->direction() != VDirection::REF
                    && !(portp->basicp()
                         && portp->basicp()->untyped())  // for properties, handled in V3AssertPre
                    && ((VN_IS(ਪੋਰਟ_DTypep, BasicDType) && pinDTypep->isNonPackedArray())
                        || (VN_IS(pinDTypep, BasicDType) && ਪੋਰਟ_DTypep->isNonPackedArray()))) {
                    pinp->v3error("Function Argument expects " << ਪੋਰਟ_DTypep->prettyDTypeNameQ()
                                                               << ", got "
                                                               << pinDTypep->prettyDTypeNameQ());
                    ਜਾਰੀ;
                }
                // An untyped formal preserves the actual argument's self-determined type.
                ਜੇ (portp->basicp() && portp->basicp()->untyped()) {
                    iterateCheckSelf(nodep, "Function Argument", pinp, SELF, FINAL);
                } ਨਹੀਂਤਾਂ ਜੇ (!portp->basicp() || portp->basicp()->isOpaque()) {
                    // Output args: at return caller = callee, reverse direction.
                    check_Class_ਸੌਂਪੋ(nodep, "Function Argument", pinp, ਪੋਰਟ_DTypep,
                                     portp->direction() == VDirection::OUTPUT);
                    userIterate(pinp, ਚੌੜਾਈ_VP{ਪੋਰਟ_DTypep, FINAL, ਧਾਰਾ_USE_ਸੌਂਪੋ}.p());
                } ਨਹੀਂਤਾਂ {
                    iterate_Check_ਸੌਂਪੋ(nodep, "Function Argument", pinp, FINAL, ਪੋਰਟ_DTypep,
                                       portp->direction().pinAccess());
                }
            }
        }
    }

    ਖਾਲੀ handle_Std_Randomize_ਦਲੀਲਾਂ(Ast_ਗੰਢ_FTask_Ref* ਅਚਲ nodep) {
        AstConst* nullp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (AstArg *argp = nodep->argsp(), *nextp; argp; argp = nextp) {
            nextp = VN_AS(argp->nextp(), Arg);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = argp->exprp();
            ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(exprp, Const)) {
                ਜੇ (constp->ਅੰਕ().isNull()) {
                    nullp = constp;
                    ਜਾਰੀ;
                }
            }
            ਜੇ (VN_IS(exprp, MemberSel)) {
                // Non-standard usage: std::randomize() with class-scoped member
                // IEEE 1800-2023 (18.12) limits args to current scope variables.
                // Verilator accepts this for compatibility with other simulators.
                ਜਾਰੀ;
            }
            ਜੇ (VN_IS(exprp, VarRef) || VN_IS(exprp, ArraySel) || VN_IS(exprp, StructSel)) {
                // Valid usage
                ਜਾਰੀ;
            }
            ਜੇ (ਅਚਲ AstCMethodHard* ਅਚਲ methodp = VN_CAST(exprp, CMethodHard)) {
                ਜੇ (methodp->ਵਿਧੀ() == VCMethod::ARRAY_AT
                    || methodp->ਵਿਧੀ() == VCMethod::ARRAY_AT_WRITE) {
                    ਜਾਰੀ;
                }
            }
            argp->v3error("Non-variable arguments for 'std::randomize()'.");
        }
        ਜੇ (nullp) nullp->v3error("'std::randomize()' does not accept 'null' as arguments.");
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        // For arguments, is assignment-like context; see IEEE rules in AstNodeAssign
        // Function hasn't been widthed, so make it so.
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;
        UINFO(5, "  FTASKREF " << nodep);
        AstWith* withp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (nodep->name() == "rand_mode" || nodep->name() == "constraint_mode") {
            v3Global.useRandomizeMethods(ਸੱਚ);
            nodep->dtypep(nodep->ਲੱਭੋ_Basic_DType(VBasicDTypeKwd::INT));
            ਵਾਪਸ;  // Handled in V3Randomize
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "randomize" || nodep->name() == "srandom"
                   || (!nodep->taskp()
                       && (nodep->name() == "get_randstate"
                           || nodep->name() == "set_randstate"))) {
            // TODO perhaps this should move to V3LinkDot
            AstClass* ਅਚਲ classp = VN_CAST(nodep->classOrPackagep(), Class);
            ਜੇ (nodep->classOrPackagep() && nodep->classOrPackagep()->name() == "std") {
                v3Global.useRandomizeMethods(ਸੱਚ);
                Ast_ਗੰਢ_DType* ਅਚਲ adtypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType();
                withp = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->ਲੱਭੋ_Void_DType(),
                                         adtypep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(), adtypep);
                ਲਈ (ਅਚਲ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_ਅਗਲਾ(argp->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                }
                handle_Std_Randomize_ਦਲੀਲਾਂ(nodep);  // Provided args should be in current scope
                process_FTask_Ref_ਦਲੀਲਾਂ(nodep);
                ਜੇ (withp) nodep->withp(withp);
                nodep->did_ਚੌੜਾਈ(ਸੱਚ);
                ਵਾਪਸ;
            }
            UASSERT_OBJ(classp, nodep, "Classless rand-thing should have failed in V3LinkDot");
            ਜੇ (nodep->name() == "randomize") {
                AstClassRefDType* ਅਚਲ adtypep
                    = ਨਵਾਂ AstClassRefDType{nodep->fileline(), classp, ਖਾਲੀ_ਸੂਚਕ};
                v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(adtypep);
                withp = ਵਿਧੀ_With_Clause(nodep, ਝੂਠ, ਝੂਠ, adtypep->ਲੱਭੋ_Void_DType(),
                                         adtypep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(), adtypep);
                ਲਈ (ਅਚਲ AstArg* argp = nodep->argsp(); argp; argp = VN_AS(argp->nextp(), Arg)) {
                    user_Iterate_And_ਅਗਲਾ(argp->exprp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
                }
                handle_Randomize_ਦਲੀਲਾਂ(nodep, classp);
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "srandom") {
                nodep->taskp(V3Randomize::newSRandomFunc(m_memberMap, classp));
                m_memberMap.clear();
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "get_randstate") {
                ਵਿਧੀ_Ok_Arguments(nodep, 0, 0);
                classp->ਅਧਾਰ_Most_Classp()->needRNG(ਸੱਚ);
                v3Global.useRandomizeMethods(ਸੱਚ);
                AstCExpr* ਅਚਲ newp
                    = ਨਵਾਂ AstCExpr{nodep->fileline(), "__Vm_rng.get_randstate()", 1};
                newp->dtype_ਸੈੱਟ_String();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->name() == "set_randstate") {
                ਵਿਧੀ_Ok_Arguments(nodep, 1, 1);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ expr1p = nodep->argsp()->exprp();  // May edit
                iterateCheckString(nodep, "LHS", expr1p, BOTH);
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = nodep->argsp()->exprp();
                classp->ਅਧਾਰ_Most_Classp()->needRNG(ਸੱਚ);
                v3Global.useRandomizeMethods(ਸੱਚ);
                AstCExpr* ਅਚਲ newp
                    = ਨਵਾਂ AstCExpr{nodep->fileline(), "__Vm_rng.set_randstate(", 1};
                newp->ਜੋੜੋ(exprp->unlink_Fr_ਪਿੱਠ());
                newp->ਜੋੜੋ(")");
                newp->dtype_ਸੈੱਟ_Void();
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                UASSERT_OBJ(ਝੂਠ, nodep, "Bad case");
            }
        }
        UASSERT_OBJ(nodep->taskp(), nodep, "Unlinked");
        ਜੇ (nodep->did_ਚੌੜਾਈ()) {
            ਜੇ (withp) nodep->withp(withp);
            ਵਾਪਸ;
        }
        ਜੇ ((nodep->taskp()->class_ਵਿਧੀ() && !nodep->taskp()->isStatic())
            && !VN_IS(m_procedurep, InitialAutomatic) && !m_constraintp) {
            ਤਾਰਕਿਕ allow = ਝੂਠ;
            ਜੇ (m_ftaskp && m_ftaskp->class_ਵਿਧੀ() && !m_ftaskp->isStatic()) {
                ਜੇ (ਅਚਲ AstFuncRef* ਅਚਲ funcRefp = VN_CAST(nodep, FuncRef)) {
                    allow = funcRefp->superReference();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstTaskRef* ਅਚਲ taskRefp = VN_CAST(nodep, TaskRef)) {
                    allow = taskRefp->superReference();
                }
                ਜੇ (!allow) {
                    ਅਚਲ AstClass* callerClassp = m_containingClassFinder.ਲੱਭੋ(m_ftaskp);
                    ਜੇ (!callerClassp) {
                        callerClassp = m_containingClassFinder.ਲੱਭੋ(m_ftaskp->classOrPackagep());
                    }
                    ਅਚਲ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ਜੇ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ਲੱਭੋ(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(callerClassp, calleeClassp);
                }
            }
            ਜੇ (!allow) {
                // An embedded covergroup may call methods of its enclosing class
                // (IEEE 1800-2023 19.4); V3Covergroup routes the call through its handle.
                ਅਚਲ AstClass* ਅਚਲ cgClassp = m_containingClassFinder.ਲੱਭੋ(nodep);
                ਜੇ (cgClassp && cgClassp->covergroupEnclosingClassp()) {
                    ਅਚਲ AstClass* calleeClassp = VN_CAST(nodep->classOrPackagep(), Class);
                    ਜੇ (!calleeClassp) {
                        calleeClassp = m_containingClassFinder.ਲੱਭੋ(nodep->taskp());
                    }
                    allow = AstClass::isClassExtendedFrom(cgClassp->covergroupEnclosingClassp(),
                                                          calleeClassp);
                }
            }
            ਜੇ (!allow) {
                nodep->v3error("Cannot call non-static member function "
                               << nodep->prettyNameQ() << " without object (IEEE 1800-2023 8.10)");
            }
        }
        ਜੇ (nodep->taskp() && !nodep->scopeNamep()
            && (nodep->taskp()->dpiContext() || nodep->taskp()->dpiExport())) {
            nodep->scopeNamep(ਨਵਾਂ AstScopeName{nodep->fileline(), ਝੂਠ});
        }
        // And do the arguments to the task/function too
        process_FTask_Ref_ਦਲੀਲਾਂ(nodep);
        ਜੇ (withp) nodep->withp(withp);
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
        // See steps that follow in visit(AstFuncRef*)
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER(m_procedurep);
        m_procedurep = nodep;
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        ਜੇ (nodep->is_Combo_ਤਾਰਾ()) ਵਾਪਸ;
        UASSERT_OBJ(nodep->isClocked(), nodep, "Invalid edge");
        // Optimize concat/replicate senitems; this has to be done here at the latest, otherwise we
        // emit WIDTHCONCAT if there are unsized constants
        ਜੇ (VN_IS(nodep->sensp(), Concat) || VN_IS(nodep->sensp(), Replicate)) {
            ਸਵੈਚਾਲਿਤ* ਅਚਲ concatOrReplp = VN_CAST(nodep->sensp(), ਗੰਢ_Biop);
            ਸਵੈਚਾਲਿਤ* ਅਚਲ rhsp = concatOrReplp->rhsp()->unlink_Fr_ਪਿੱਠ();
            ਜੇ (nodep->ਕਿਨਾਰਾ_Type() == VEdgeType::ET_ਬਦਲਿਆ) {
                // If it's ET_CHANGED, split concatenations into multiple senitems
                ਸਵੈਚਾਲਿਤ* ਅਚਲ lhsp = concatOrReplp->lhsp()->unlink_Fr_ਪਿੱਠ();
                nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(ਨਵਾਂ Ast_Sen_ਵਸਤੂ{lhsp->fileline(), nodep->ਕਿਨਾਰਾ_Type(), lhsp});
            }  // Else only use the RHS
            nodep->replaceWith(ਨਵਾਂ Ast_Sen_ਵਸਤੂ{rhsp->fileline(), nodep->ਕਿਨਾਰਾ_Type(), rhsp});
            VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
        } ਨਹੀਂਤਾਂ {
            userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਜੇ (nodep->ਕਿਨਾਰਾ_Type().an_ਕਿਨਾਰਾ()) {
                Ast_ਗੰਢ_DType* ਅਚਲ sensDtp = nodep->sensp()->dtypep()->skipRefp();
                ਜੇ (sensDtp->isDouble()) {
                    nodep->sensp()->v3error(
                        "Edge event control not legal on real type (IEEE 1800-2023 6.12.1)");
                } ਨਹੀਂਤਾਂ ਜੇ (sensDtp->basicp() && !sensDtp->basicp()->keyword().isIntNumeric()) {
                    nodep->sensp()->v3error("Edge event control not legal on non-integral type "
                                            "(IEEE 1800-2023 9.4.2)");
                }
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Clocking_ਵਸਤੂ* nodep) override {
        nodep->exprp()->foreach(
            [nodep](AstVarRef* ਅਚਲ refp) { refp->access(nodep->direction().pinAccess()); });
        userIterateChildren(nodep, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
    }
    ਖਾਲੀ ਦੇਖੋ(AstWait* nodep) override {
        ਜੇ (!m_underFork && VN_IS(m_ftaskp, Func)) {
            nodep->v3error("Wait statements are not legal in functions. Suggest use a task "
                           "(IEEE 1800-2023 13.4.4)");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->fileline()->timingOn()) {
            ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
                iterateCheckBool(nodep, "Wait", nodep->condp(),
                                 BOTH);  // it's like an if() condition.
                // TODO check also inside complex event expressions
                ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ varrefp = VN_CAST(nodep->condp(), ਗੰਢ_Var_Ref)) {
                    ਜੇ (varrefp->isEvent()) {
                        varrefp->v3error("Wait statement conditions do not take raw events"
                                         " (IEEE 1800-2023 15.5.3)\n"
                                         << varrefp->warn_ਹੋਰ() << "... Suggest use '"
                                         << varrefp->prettyName() << ".triggered'");
                    }
                }
                iterateNull(nodep->stmtsp());
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_False()) {
                nodep->v3warn(E_NOTIMING, "Wait statements require --timing");
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(E_NEEDTIMINGOPT, "Use --timing or --no-timing to specify how wait "
                                               "statements should be handled");
            }
        }
        // If we ignore timing:
        // Statements we'll just execute immediately; equivalent to if they followed this
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ stmtsp = nodep->stmtsp()) {
            stmtsp->unlink_Fr_ਪਿੱਠ_With_ਅਗਲਾ();
            nodep->replaceWith(stmtsp);
        } ਨਹੀਂਤਾਂ {
            nodep->unlink_Fr_ਪਿੱਠ();
        }
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        // Should otherwise be underneath a method call
        Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypeNullSkipRefp();
        VL_RESTORER(m_withp);
        m_withp = nodep;
        userIterateChildren(nodep->ਸੂਚਕਾਂਕ_Arg_Refp(), ਖਾਲੀ_ਸੂਚਕ);
        userIterateChildren(nodep->ਮੁੱਲ_Arg_Refp(), ਖਾਲੀ_ਸੂਚਕ);
        ਜੇ (!nodep->exprp()) {
            nodep->dtype_ਸੈੱਟ_Void();
        } ਨਹੀਂਤਾਂ {
            ਜੇ (!nodep->exprp()->hasDType()) {
                user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਖਾਲੀ_ਸੂਚਕ);
            } ਨਹੀਂਤਾਂ {
                ਜੇ (vdtypep) {
                    user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{nodep->dtypep(), PRELIM}.p());
                } ਨਹੀਂਤਾਂ {  // 'sort with' allows arbitrary type
                    user_Iterate_And_ਅਗਲਾ(nodep->exprp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
                }
            }

            ਜੇ (!nodep->exprp()->hasDType()) {
                nodep->dtype_ਸੈੱਟ_Void();
            } ਨਹੀਂਤਾਂ {
                nodep->dtypeFrom(nodep->exprp());
                iterate_Check_ਸੌਂਪੋ(nodep, "'with' return value", nodep->exprp(), FINAL,
                                   nodep->dtypep());
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstLambdaArgRef* nodep) override {
        UASSERT_OBJ(m_withp, nodep, "LambdaArgRef not underneath 'with' lambda");
        ਜੇ (nodep->ਸੂਚਕਾਂਕ()) {
            nodep->dtypeFrom(m_withp->ਸੂਚਕਾਂਕ_Arg_Refp());
        } ਨਹੀਂਤਾਂ {
            nodep->dtypeFrom(m_withp->ਮੁੱਲ_Arg_Refp());
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        // Iterate modules backwards, in bottom-up order.  That's faster
        userIterateChildrenBackwardsConst(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }

    //--------------------
    // Default
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        ਜੇ (!nodep->did_ਚੌੜਾਈ()) {
            nodep->v3fatalSrc(
                "Visit function missing? Widthed function missing for math node: " << nodep);
        }
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }
    ਖਾਲੀ ਦੇਖੋ(AstResizeLValue* nodep) override {
        // RESIZELVALUE adjusts width of lvalues for assignments/function calls
        // The parent context determines the required width
        UINFO(9, "visit AstResizeLValue " << nodep << ਸਤਰ_ਅੰਤ);
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;

        UASSERT_OBJ(nodep->lhsp(), nodep, "ResizeLValue missing lhs expression");

        // First, process child to know its natural width
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
        }

        // Get the required width from parent context
        ਜੇ (Ast_ਗੰਢ_DType* ਅਚਲ vdtypep = m_vup->dtypeNullp()) {
            // Parent specified required width - use it
            nodep->dtypeFrom(vdtypep);
        } ਨਹੀਂਤਾਂ ਜੇ (!nodep->dtypep()) {
            // No parent context, use child's type
            nodep->dtypeFrom(nodep->lhsp());
        }

        // Verify we ended up with a valid datatype
        UASSERT_OBJ(nodep->dtypep(), nodep, "ResizeLValue still missing dtype after visiting");
        UASSERT_OBJ(nodep->ਚੌੜਾਈ() > 0, nodep, "ResizeLValue has invalid width");

        // Log the transformation for debugging
        UINFO(9, "  ResizeLValue: " << nodep->lhsp()->ਚੌੜਾਈ() << " bits -> " << nodep->ਚੌੜਾਈ()
                                    << " bits" << ਸਤਰ_ਅੰਤ);

        // Final processing
        user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, FINAL}.p());
        nodep->did_ਚੌੜਾਈ(ਸੱਚ);
    }
    ਖਾਲੀ ਦੇਖੋ_Class(AstClass* nodep) {
        ਜੇ (nodep->did_ਚੌੜਾਈ_And_ਸੈੱਟ()) ਵਾਪਸ;

        // If the class is std::process
        ਜੇ (nodep->name() == "process") {
            AstPackage* ਅਚਲ packagep = get_ਵਸਤੂ_Package(nodep);
            ਜੇ (packagep && packagep->name() == "std") {
                // Change type of m_process to VlProcessRef
                ਜੇ (AstVar* ਅਚਲ varp
                    = VN_CAST(m_memberMap.ਲੱਭੋ_Member(nodep, "m_process"), Var)) {
                    Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->get_ਬੱਚਾ_DTypep();
                    ਜੇ (!varp->dtypep()) {
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(dtypep->unlink_Fr_ਪਿੱਠ()), dtypep);
                    }
                    AstBasicDType* ਅਚਲ newdtypep = ਨਵਾਂ AstBasicDType{
                        nodep->fileline(), VBasicDTypeKwd::PROCESS_REFERENCE, VSigning::UNSIGNED};
                    v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(newdtypep);
                    varp->dtypep(newdtypep);
                }
                // Mark that self requires process instance
                ਜੇ (Ast_ਗੰਢ_FTask* ਅਚਲ ftaskp
                    = VN_CAST(m_memberMap.ਲੱਭੋ_Member(nodep, "self"), ਗੰਢ_FTask)) {
                    ftaskp->ਸੈੱਟ_Need_Process();
                    v3Global.ਸੈੱਟ_ਵਰਤਦਾ_Timing();
                }
            }
        }

        // Must do extends first, as we may in functions under this class
        // start following a tree of extends that takes us to other classes
        user_Iterate_And_ਅਗਲਾ(nodep->extendsp(), ਖਾਲੀ_ਸੂਚਕ);
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);  // First size all members
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        assertAtStatement(nodep);
        VL_RESTORER_COPY(m_enum_ਆਰਜ਼ੀ_ਨਾਂ);
        VL_RESTORER_COPY(m_inside_ਆਰਜ਼ੀ_ਨਾਂ);
        ਜੇ (AstClass* ਅਚਲ classp = VN_CAST(nodep, Class)) {
            ਦੇਖੋ_Class(classp);
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_modep);
            m_modep = nodep;
            // Collect local variables for ASSIGNIN hierarchical reference check
            m_curModVars.clear();
            nodep->foreach([ਇਹ](ਅਚਲ AstVar* varp) { m_curModVars.ਪਾਓ(varp); });
            userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        // Default: Just iterate
        UASSERT_OBJ(!m_vup, nodep,
                    "Visit function missing? Widthed expectation for this node: " << nodep);
        userIterateChildren(nodep, ਖਾਲੀ_ਸੂਚਕ);
    }

    //----------------------------------------------------------------------
    // WIDTH METHODs -- all iterate

    ਖਾਲੀ ਦੇਖੋ_Or_Lu64(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: AstBitsToRealD
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ਸੈੱਟ_Double();
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(64, 64, VSigning::UNSIGNED);
            // Self-determined operand
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstIToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ਸੈੱਟ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            ਜੇ (nodep->lhsp()->isSigned()) {
                nodep->replaceWith(
                    ਨਵਾਂ AstISToRD{nodep->fileline(), nodep->lhsp()->unlink_Fr_ਪਿੱਠ()});
                VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(AstISToRD* nodep) override {
        // Real: Output real
        // LHS presumed self-determined, then coerced to real
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            nodep->dtype_ਸੈੱਟ_Double();
            // Self-determined operand (TODO check if numeric type)
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Os32_Lr(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: RToI
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_Integer2_State();
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Ou64_Lr(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: RealToBits
        // Real: LHS real
        // LHS presumed self-determined, then coerced to real
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_UInt64();
        }
    }

    ਖਾਲੀ ਦੇਖੋ_log_not(AstLogNot* nodep) {
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
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->op1p(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
            // IEEE 1800-2023 16.12.3: property 'not' is not a sequence operator.
            // Boolean '!' is allowed in sequences (16.7 expression_or_dist).
            // The parser distinguishes the two via AstLogNot::fromProperty().
            ਜੇ (m_underSExpr && nodep->fromProperty()) {
                nodep->v3error("Unexpected 'not' in sequence expression context");
                AstConst* ਅਚਲ newp
                    = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
                newp->dtypeFrom(nodep);
                nodep->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ_log_and_or(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: LogAnd, LogOr, LogEq, LogIf
        // Widths: 1 bit out, lhs 1 bit, rhs 1 bit
        // IEEE-2012 Table 11-21:
        //   LHS is self-determined
        //   RHS is self-determined
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckBool(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(
                Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
    }
    ਖਾਲੀ ਦੇਖੋ_Abort_Prop(AstAbortOn* nodep) {
        // IEEE 1800-2023 16.12.14: abort condition is a 1-bit self-determined
        // Boolean; property subexpression carries its own type checking.
        // VAbortKind distinguishes accept/reject and sync/async, but the
        // type-check path is identical for all four.
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckBool(nodep, "cond", nodep->condp(), BOTH);
            iterateCheckBool(nodep, "prop", nodep->propp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ_red_and_or(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: RedAnd, RedOr, ...
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            iterateCheckSizedSelf(nodep, "LHS", nodep->lhsp(), SELF, BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(nodep->lhsp()->isFourstateOrNoDtype());
        }
    }
    ਖਾਲੀ ਦੇਖੋ_red_unknown(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: IsUnknown
        // Signed: Output unsigned, Lhs/Rhs/etc non-real (presumed, not in IEEE)
        // IEEE-2012: Table 11-21 and 11.8.1:
        //   LHS is self-determined
        //   Width: 1 bit out
        //   Sign: unsigned out (11.8.1)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }

    // IEEE 1800-2023 6.22.2 Equivalent types
    ਤਾਰਕਿਕ isEquivalentDType(ਅਚਲ Ast_ਗੰਢ_DType* lhs, ਅਚਲ Ast_ਗੰਢ_DType* rhs) {
        // a) If two types match, they are equivalent.
        ਜੇ (!lhs || !rhs) ਵਾਪਸ ਝੂਠ;
        lhs = lhs->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        rhs = rhs->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        ਜੇ (lhs == rhs) ਵਾਪਸ ਸੱਚ;
        // If both are basic types, check if they are the same type
        ਜੇ (VN_IS(lhs, BasicDType) && VN_IS(rhs, BasicDType)) {
            ਅਚਲ ਸਵੈਚਾਲਿਤ* lb = VN_CAST(lhs, BasicDType);
            ਅਚਲ ਸਵੈਚਾਲਿਤ* rb = VN_CAST(rhs, BasicDType);
            ਜੇ (lb->isString() != rb->isString()) ਵਾਪਸ ਝੂਠ;
        }

        // d) Unpacked fixed-size array types are equivalent if they have equivalent element types
        // and equal size; the actual range bounds may differ. Note that the element type of a
        // multidimensional array is itself an array type.
        ਅਚਲ ਤਾਰਕਿਕ lhsIsUnpackArray = VN_IS(lhs, UnpackArrayDType);
        ਅਚਲ ਤਾਰਕਿਕ rhsIsUnpackArray = VN_IS(rhs, UnpackArrayDType);
        ਜੇ (lhsIsUnpackArray || rhsIsUnpackArray) {
            ਜੇ (lhsIsUnpackArray && rhsIsUnpackArray) {
                ਅਚਲ AstUnpackArrayDType* ਅਚਲ lhsp = VN_CAST(lhs, UnpackArrayDType);
                ਅਚਲ AstUnpackArrayDType* ਅਚਲ rhsp = VN_CAST(rhs, UnpackArrayDType);
                ਅਚਲ ਪੂਰਨ lsz = lhsp->elementsConst();
                ਅਚਲ ਪੂਰਨ rsz = rhsp->elementsConst();
                ਜੇ (lsz >= 0 && rsz >= 0 && lsz != rsz) ਵਾਪਸ ਝੂਠ;
                ਵਾਪਸ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
            }
            ਵਾਪਸ ਝੂਠ;
        }

        // e) Dynamic array, associative array, and queue types are equivalent if they are the same
        // kind of array (dynamic, associative, or queue), have equivalent index types (for
        // associative arrays), and have equivalent element types.
        ਅਚਲ ਤਾਰਕਿਕ lhsIsDynArray = VN_IS(lhs, DynArrayDType);
        ਅਚਲ ਤਾਰਕਿਕ rhsIsDynArray = VN_IS(rhs, DynArrayDType);
        ਅਚਲ ਤਾਰਕਿਕ lhs_Is_ਕਤਾਰ = VN_IS(lhs, ਕਤਾਰ_DType);
        ਅਚਲ ਤਾਰਕਿਕ rhs_Is_ਕਤਾਰ = VN_IS(rhs, ਕਤਾਰ_DType);
        ਅਚਲ ਤਾਰਕਿਕ lhsIsAssocArray = VN_IS(lhs, AssocArrayDType);
        ਅਚਲ ਤਾਰਕਿਕ rhsIsAssocArray = VN_IS(rhs, AssocArrayDType);

        ਜੇ (lhsIsDynArray || rhsIsDynArray || lhs_Is_ਕਤਾਰ || rhs_Is_ਕਤਾਰ || lhsIsAssocArray
            || rhsIsAssocArray) {
            ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ lhsp = VN_CAST(lhs, DynArrayDType)) {
                ਜੇ (ਅਚਲ AstDynArrayDType* ਅਚਲ rhsp = VN_CAST(rhs, DynArrayDType)) {
                    ਵਾਪਸ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ lhsp = VN_CAST(lhs, ਕਤਾਰ_DType)) {
                ਜੇ (ਅਚਲ Ast_ਕਤਾਰ_DType* ਅਚਲ rhsp = VN_CAST(rhs, ਕਤਾਰ_DType)) {
                    ਵਾਪਸ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep());
                }
            }

            ਜੇ (ਅਚਲ AstAssocArrayDType* ਅਚਲ lhsp = VN_CAST(lhs, AssocArrayDType)) {
                ਜੇ (ਅਚਲ AstAssocArrayDType* ਅਚਲ rhsp = VN_CAST(rhs, AssocArrayDType)) {
                    ਵਾਪਸ isEquivalentDType(lhsp->subDTypep(), rhsp->subDTypep())
                           && isEquivalentDType(lhsp->ਕੁੰਜੀ_DTypep(), rhsp->ਕੁੰਜੀ_DTypep());
                }
            }

            ਵਾਪਸ ਝੂਠ;
        }

        // c) Packed arrays, packed structures, packed unions, and built-in integral
        // types are equivalent if they contain the same number of total bits, are either all
        // 2-state or all 4-state, and are either all signed or all unsigned.
        ਜੇ (lhs->isIntegralOrPacked() && rhs->isIntegralOrPacked()) {
            ਜੇ (lhs->ਚੌੜਾਈ() != rhs->ਚੌੜਾਈ()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (lhs->isFourstate() != rhs->isFourstate()) ਵਾਪਸ ਝੂਠ;
            ਜੇ (lhs->isSigned() != rhs->isSigned()) ਵਾਪਸ ਝੂਠ;
            ਵਾਪਸ ਸੱਚ;
        }

        // b) An anonymous enum, unpacked struct, or unpacked union type is equivalent to itself
        // among data objects declared within the same declaration statement and no other data
        // types.

        // DTypes compared above, if any remained then not equivalent.
        ਜੇ (VN_IS(lhs, EnumDType) || VN_IS(rhs, EnumDType)) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ !(VN_IS(lhs, ਗੰਢ_UOr_Struct_DType) || VN_IS(rhs, ਗੰਢ_UOr_Struct_DType));
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isAggregateType(ਅਚਲ Ast_ਗੰਢ* nodep) {
        ਜੇ (!nodep) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ Ast_ਗੰਢ_DType* dtypep = nodep->dtypep();
        ਜੇ (!dtypep) ਵਾਪਸ ਝੂਠ;
        dtypep = dtypep->skipRefp();
        ਜੇ (!dtypep) ਵਾਪਸ ਝੂਠ;
        ਵਾਪਸ dtypep->isAggregateType();
    }

    ਖਾਲੀ ਦੇਖੋ_cmp_eq_gt(Ast_ਗੰਢ_Biop* nodep, ਅਚਲ ComparisonType cmpType) {
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
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // Assignment patterns cannot self-determine their type.
            // Iterate the non-pattern operand first so its dtype is available,
            // then propagate that dtype to the pattern operand (analogous to
            // how assignments pass LHS dtype to RHS patterns).
            ਅਚਲ ਤਾਰਕਿਕ lhsIsPat = VN_IS(nodep->lhsp(), ਨਮੂਨਾ);
            ਅਚਲ ਤਾਰਕਿਕ rhsIsPat = VN_IS(nodep->rhsp(), ਨਮੂਨਾ);
            ਜੇ (lhsIsPat && !rhsIsPat) {
                user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{nodep->rhsp()->dtypep(), PRELIM}.p());
            } ਨਹੀਂਤਾਂ ਜੇ (rhsIsPat && !lhsIsPat) {
                user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{nodep->lhsp()->dtypep(), PRELIM}.p());
            } ਨਹੀਂਤਾਂ {
                user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
                user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            }

            ਅਚਲ ਤਾਰਕਿਕ isAggrLhs = isAggregateType(nodep->lhsp());
            ਅਚਲ ਤਾਰਕਿਕ isAggrRhs = isAggregateType(nodep->rhsp());

            ਜੇ ((isAggrLhs || isAggrRhs)
                && (cmpType == ComparisonType::ਸਾਧਾਰਨ_EQ || cmpType == ComparisonType::CASE)) {
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDType = nodep->lhsp()->dtypep();
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsDType = nodep->rhsp()->dtypep();

                ਜੇ (lhsDType && rhsDType && !isEquivalentDType(lhsDType, rhsDType)) {
                    nodep->v3error("Comparison requires matching data types"
                                   << warnDTypes(nodep, lhsDType, rhsDType));
                    Ast_ਗੰਢ* ਅਚਲ newp
                        = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
                // Elements are compared by their left-to-right order, as when assigned (7.4.6)
                convert_Unpacked_ਕ੍ਰਮ(nodep->rhsp(), nodep->lhsp()->dtypep());
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                ਜੇ (cmpType == ComparisonType::WILD) {
                    nodep->v3error("Real is illegal operand to ?== operator");
                    Ast_ਗੰਢ* ਅਚਲ newp
                        = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
                    nodep->replaceWith(newp);
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                    ਵਾਪਸ;
                }
                ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithNVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                    iterateCheckString(nodep, "LHS", nodep->lhsp(), FINAL);
                    iterateCheckString(nodep, "RHS", nodep->rhsp(), FINAL);
                }
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਤਾਰਕਿਕ signedFl = nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned();
                ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithUOrSVersion(nodep, signedFl)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(nodep->lhsp()->ਚੌੜਾਈ(), nodep->rhsp()->ਚੌੜਾਈ());
                ਅਚਲ ਪੂਰਨ ewidth = std::max(nodep->lhsp()->ਚੌੜਾਈ_Min(), nodep->rhsp()->ਚੌੜਾਈ_Min());
                ਤਾਰਕਿਕ warnOn = ਸੱਚ;
                ਜੇ (!signedFl && ਚੌੜਾਈ == 32) {
                    // Waive on unsigned < or <= if RHS is narrower, since can't give wrong answer
                    ਜੇ ((VN_IS(nodep, Lt) || VN_IS(nodep, Lte))
                        && (nodep->lhsp()->ਚੌੜਾਈ() >= nodep->rhsp()->ਚੌੜਾਈ_Min())) {
                        warnOn = ਝੂਠ;
                    }
                    // Waive on unsigned > or >= if RHS is wider, since can't give wrong answer
                    ਜੇ ((VN_IS(nodep, Gt) || VN_IS(nodep, Gte))
                        && (nodep->lhsp()->ਚੌੜਾਈ_Min() >= nodep->rhsp()->ਚੌੜਾਈ())) {
                        warnOn = ਝੂਠ;
                    }
                }
                iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL,
                             nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->lhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
                iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL,
                             nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(ਚੌੜਾਈ, ewidth,
                                                        VSigning::fromBool(signedFl),
                                                        nodep->rhsp()->isFourstateOrNoDtype()),
                             (signedFl ? EXTEND_LHS : EXTEND_ZERO), warnOn);
            }
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic(cmpType != ComparisonType::CASE
                                      && (nodep->lhsp()->isFourstateOrNoDtype()
                                          || ((cmpType == ComparisonType::ਸਾਧਾਰਨ_EQ
                                               || cmpType == ComparisonType::ਸਾਧਾਰਨ_GL)
                                              && nodep->rhsp()->isFourstateOrNoDtype())));
        }
    }
    ਖਾਲੀ ਦੇਖੋ_cmp_real(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: EqD, LtD
        // Widths: 1 bit out, lhs width == rhs width
        // Signed compare (not output) if both sides signed
        // Real if and only if real_allow set
        // IEEE, 11.4.4: relational compares (<,>,<=,>=,==,===,!=,!==) use
        // "zero padding" on unsigned
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ_cmp_string(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: EqN, LtN
        // Widths: 1 bit out, lhs width == rhs width
        // String compare (not output)
        // Real if and only if real_allow set
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            // See similar handling in visit_cmp_eq_gt where created
            iterateCheckString(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckString(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        }
    }
    ਖਾਲੀ ਦੇਖੋ_cmp_type(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: EqT, LtT
        // Widths: 1 bit out
        // Data type compare (not output)
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਅਚਲ Ast_Attr_ਦਾ* ਅਚਲ lhsap = VN_AS(nodep->lhsp(), Attr_ਦਾ);
            ਅਚਲ Ast_Attr_ਦਾ* ਅਚਲ rhsap = VN_AS(nodep->rhsp(), Attr_ਦਾ);
            UASSERT_OBJ(lhsap->attrType() == VAttrType::TYPEID, lhsap,
                        "Type compare expects type reference");
            UASSERT_OBJ(rhsap->attrType() == VAttrType::TYPEID, rhsap,
                        "Type compare expects type reference");
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDtp = lhsap->dtypep();
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsDtp = rhsap->dtypep();
            UINFO(9, "==type lhsDtp " << lhsDtp);
            UINFO(9, "==type rhsDtp " << lhsDtp);
            ਅਚਲ ਤਾਰਕਿਕ invert = VN_IS(nodep, NeqT);
            ਅਚਲ ਤਾਰਕਿਕ identical
                = Ast_ਗੰਢ::computeCastable(lhsDtp, rhsDtp, nodep) == VCastable::SAMEISH;
            UINFO(9, "== " << identical);
            ਅਚਲ ਤਾਰਕਿਕ eq = invert ^ identical;
            Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}, eq};
            nodep->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_negate_not(Ast_ਗੰਢ_Uniop* nodep, ਤਾਰਕਿਕ real_ok) {
        // CALLER: (real_ok=false) Not
        // CALLER: (real_ok=true) Negate - allow real numbers
        // Signed: From lhs
        // IEEE-2012 Table 11-21:
        //    Widths: out width = lhs width
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            ਜੇ (!real_ok) checkCvtUS(nodep->lhsp(), ਝੂਠ);
        }
        ਜੇ (real_ok && nodep->lhsp()->isDouble()) {
            spliceCvtD(nodep->lhsp());
            ਜੇ (Ast_ਗੰਢ_Uniop* ਅਚਲ newp = replaceWithDVersion(nodep)) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
                nodep->dtype_ਸੈੱਟ_Double();
                ਵਾਪਸ;
            }
        } ਨਹੀਂਤਾਂ {
            // Note there aren't yet uniops that need version changes
            // So no need to call replaceWithUOrSVersion(nodep, nodep->isSigned())
        }
        ਜੇ (m_vup->prelim()) nodep->dtypeFrom(nodep->lhsp());
        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            nodep->dtypep(expDTypep);  // Propagate expression type to negation
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            // Some warning suppressions
            ਤਾਰਕਿਕ lhsWarn = ਸੱਚ;
            ਜੇ (VN_IS(nodep, Negate)) {
                // Warn if user wants extra bit from carry
                ਜੇ (subDTypep->ਚੌੜਾਈ_Min() == (nodep->lhsp()->ਚੌੜਾਈ_Min() + 1)) lhsWarn = ਝੂਠ;
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         lhsWarn);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_signed_unsigned(Ast_ਗੰਢ_Uniop* nodep, VSigning rs_ਬਾਹਰਲਾ) {
        // CALLER: Signed, Unsigned
        // Width: lhs is self determined width
        // See IEEE-2012 6.24.1:
        //   Width: Returns packed array, of size $bits(expression).
        //   Sign: Output sign is as specified by operation
        //   TODO: Type: Two-state if input is two-state, else four-state
        UASSERT_OBJ(!nodep->op2p(), nodep, "For unary ops only!");
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ਸੱਚ);
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = nodep->lhsp()->ਚੌੜਾਈ();
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                ਚੌੜਾਈ, ਚੌੜਾਈ, rs_ਬਾਹਰਲਾ, nodep->lhsp()->isFourstateOrNoDtype());
            nodep->dtypep(expDTypep);
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            // The child's width is self determined
            iterateCheck(nodep, "LHS", nodep->lhsp(), SELF, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_ਖਿਸਕਾਓ_2(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: ShiftL, ShiftR, ShiftRS
        // Widths: Output width from lhs, rhs<33 bits
        // Signed: Output signed iff LHS signed; unary operator
        // See IEEE 2012 11.4.10:
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        iterate_ਖਿਸਕਾਓ_prelim(nodep);
        nodep->dtypeChgSigned(nodep->lhsp()->isSigned());
        ਅਚਲ Ast_ਗੰਢ_Biop* ਅਚਲ newp = iterate_ਖਿਸਕਾਓ_final(nodep);
        VL_DANGLING(nodep);
        (ਖਾਲੀ)newp;  // Ununused
    }
    ਖਾਲੀ iterate_ਖਿਸਕਾਓ_prelim(Ast_ਗੰਢ_Biop* nodep) {
        // Shifts
        // See IEEE-2012 11.4.10 and Table 11-21.
        //   RHS is self-determined. RHS is always treated as unsigned, has no effect on result.
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ਝੂਠ);
            iterateCheckSizedSelf(nodep, "RHS", nodep->rhsp(), SELF, BOTH);
            nodep->dtypeFrom(nodep->lhsp());
            ਜੇ (!nodep->isFourstateOrNoDtype() && nodep->rhsp()->isFourstateOrNoDtype()) {
                nodep->dtype_ਸੈੱਟ_Logic_Unsized(nodep->ਚੌੜਾਈ(), nodep->ਚੌੜਾਈ_Min(),
                                            nodep->dtypep()->numeric());
            }
        }
    }
    Ast_ਗੰਢ_Biop* iterate_ਖਿਸਕਾਓ_final(Ast_ਗੰਢ_Biop* nodep) {
        // Nodep maybe edited
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // ShiftRS converts to ShiftR, but not vice-versa
            ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_RS)) {
                ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithUOrSVersion(nodep, nodep->isSigned())) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
            }
            ਤਾਰਕਿਕ warnOn = ਸੱਚ;
            // No warning if "X = 1'b1<<N"; assume user is doing what they want
            ਜੇ (nodep->lhsp()->isOne() && VN_IS(nodep->backp(), ਗੰਢ_ਸੌਂਪੋ)) warnOn = ਝੂਠ;
            // We don't currently suppress these, as it's the upper operator (e.g. assign)
            // that reports the WIDTHEXPAND.
            AstConst* ਅਚਲ shiftp = VN_CAST(nodep->rhsp(), Const);
            ਜੇ (shiftp && !shiftp->ਅੰਕ().isFourState() && shiftp->ਚੌੜਾਈ() <= 32) {
                ਅਚਲ int64_t ਖਿਸਕਾਓ_Val = shiftp->ਅੰਕ().ਮੰਜ਼ਿਲ_SQuad();
                ਜੇ (VN_IS(nodep, ਖਿਸਕਾਓ_L)) {
                    ਜੇ (ਖਿਸਕਾਓ_Val > 0 && nodep->ਚੌੜਾਈ() == nodep->lhsp()->ਚੌੜਾਈ() + ਖਿਸਕਾਓ_Val)
                        warnOn = ਝੂਠ;
                }
            }
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP,
                         warnOn);
            ਜੇ (nodep->rhsp()->ਚੌੜਾਈ() > 32) {
                ਜੇ (shiftp && shiftp->ਅੰਕ().most_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_P1() <= 32) {
                    // If (number)<<96'h1, then make it into (number)<<32'h1
                    V3_ਸੰਖਿਆ ਅੰਕ{shiftp, 32, shiftp->ਅੰਕ()};
                    Ast_ਗੰਢ* ਅਚਲ shiftrhsp = nodep->rhsp();
                    nodep->rhsp()->replaceWith(ਨਵਾਂ AstConst{shiftrhsp->fileline(), ਅੰਕ});
                    VL_DO_DANGLING(shiftrhsp->delete_ਰੁੱਖ(), shiftrhsp);
                }
            }
        }
        ਵਾਪਸ nodep;  // May edit
    }

    ਖਾਲੀ ਦੇਖੋ_boolexpr_and_or(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: And, Or, Xor, ...
        // Lint widths: out width = lhs width = rhs width
        // Signed: if lhs & rhs signed
        // IEEE-2012 Table 11-21:
        //    Width: max(LHS, RHS)
        UASSERT_OBJ(nodep->rhsp(), nodep, "For binary ops only!");
        // If errors are off, we need to follow the spec; thus we really need to do the max()
        // because the rhs could be larger, and we need to have proper editing to get the widths
        // to be the same for our operations.
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            checkCvtUS(nodep->lhsp(), ਝੂਠ);
            checkCvtUS(nodep->rhsp(), ਝੂਠ);
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(nodep->lhsp()->ਚੌੜਾਈ(), nodep->rhsp()->ਚੌੜਾਈ());
            ਅਚਲ ਪੂਰਨ mwidth = std::max(nodep->lhsp()->ਚੌੜਾਈ_Min(), nodep->rhsp()->ਚੌੜਾਈ_Min());
            ਅਚਲ ਤਾਰਕਿਕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                ਚੌੜਾਈ, mwidth, VSigning::fromBool(expSigned),
                Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
        }
        ਜੇ (m_vup->final()) {
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // Error report and change sizes for suboperands of this node.
            iterateCheck(nodep, "LHS", nodep->lhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
            iterateCheck(nodep, "RHS", nodep->rhsp(), CONTEXT_DET, FINAL, subDTypep, EXTEND_EXP);
        }
    }

    ਖਾਲੀ ਦੇਖੋ_ਜੋੜੋ_sub_replace(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ real_ok, ਅਚਲ ਤਾਰਕਿਕ modOrDiv) {
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
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // Determine expression widths only relying on what's in the subops
            user_Iterate_And_ਅਗਲਾ(nodep->lhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            user_Iterate_And_ਅਗਲਾ(nodep->rhsp(), ਚੌੜਾਈ_VP{CONTEXT_DET, PRELIM}.p());
            ਜੇ (!real_ok) {
                checkCvtUS(nodep->lhsp(), ਝੂਠ);
                checkCvtUS(nodep->rhsp(), ਝੂਠ);
            }
            ਜੇ (nodep->lhsp()->isDouble() || nodep->rhsp()->isDouble()) {
                spliceCvtD(nodep->lhsp());
                spliceCvtD(nodep->rhsp());
                ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithDVersion(nodep)) {
                    VL_DANGLING(nodep);
                    nodep = newp;  // Process new node instead
                }
                nodep->dtype_ਸੈੱਟ_Double();
                iterateCheckReal(nodep, "LHS", nodep->lhsp(), FINAL);
                iterateCheckReal(nodep, "RHS", nodep->rhsp(), FINAL);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->lhsp()->isString() || nodep->rhsp()->isString()) {
                nodep->v3error(
                    "Operator "
                    << nodep->prettyTypeName()
                    << " is not legal on string data types (IEEE 1800-2023 6.16)\n"
                    << (VN_IS(nodep, ਜੋੜੋ_2)
                            ? (nodep->warn_ਹੋਰ()
                               + "... Suggest to concatenate strings use '{LHS, RHS, ...}'")
                            : ""));
                nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, ""});
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = std::max(nodep->lhsp()->ਚੌੜਾਈ(), nodep->rhsp()->ਚੌੜਾਈ());
                ਅਚਲ ਪੂਰਨ mwidth = std::max(nodep->lhsp()->ਚੌੜਾਈ_Min(), nodep->rhsp()->ਚੌੜਾਈ_Min());
                ਅਚਲ ਤਾਰਕਿਕ expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
                nodep->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                    ਚੌੜਾਈ, mwidth, VSigning::fromBool(expSigned),
                    (modOrDiv && !nodep->rhsp()->isNeqZero())
                        || Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(nodep->lhsp(), nodep->rhsp()));
            }
        }
        ਜੇ (m_vup->final()) {
            // Parent's data type was computed using the max(upper, nodep->dtype)
            Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = m_vup->dtypeOverridep(nodep->dtypep());
            Ast_ਗੰਢ_DType* ਅਚਲ subDTypep = expDTypep;
            nodep->dtypep(expDTypep);
            // We don't use LHS && RHS -- unspecified language corner, see t_math_signed5 test
            // bool expSigned = (nodep->lhsp()->isSigned() && nodep->rhsp()->isSigned());
            ਜੇ (Ast_ਗੰਢ_Biop* ਅਚਲ newp = replaceWithUOrSVersion(nodep, expDTypep->isSigned())) {
                VL_DANGLING(nodep);
                nodep = newp;  // Process new node instead
            }
            // Some warning suppressions
            ਤਾਰਕਿਕ lhsWarn = ਸੱਚ;
            ਤਾਰਕਿਕ rhsWarn = ਸੱਚ;
            ਜੇ (VN_IS(nodep, ਜੋੜੋ_2) || VN_IS(nodep, Sub)) {
                // Warn if user wants extra bit from carry
                ਜੇ (subDTypep->ਚੌੜਾਈ_Min() == (nodep->lhsp()->ਚੌੜਾਈ_Min() + 1)) lhsWarn = ਝੂਠ;
                ਜੇ (subDTypep->ਚੌੜਾਈ_Min() == (nodep->rhsp()->ਚੌੜਾਈ_Min() + 1)) rhsWarn = ਝੂਠ;
                ਜੇ (VN_IS(nodep, ਜੋੜੋ_2) && nodep->lhsp()->ਚੌੜਾਈ() == 1
                    && nodep->rhsp()->ਚੌੜਾਈ() != 1)
                    lhsWarn = ਝੂਠ;  // do_increment + ...
                ਜੇ (nodep->rhsp()->ਚੌੜਾਈ() == 1 && nodep->lhsp()->ਚੌੜਾਈ() != 1)
                    rhsWarn = ਝੂਠ;  // ... + do_increment
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(nodep, Mul) || VN_IS(nodep, MulS)) {
                ਜੇ (subDTypep->ਚੌੜਾਈ_Min() >= (nodep->lhsp()->ਚੌੜਾਈ_Min())) lhsWarn = ਝੂਠ;
                ਜੇ (subDTypep->ਚੌੜਾਈ_Min() >= (nodep->rhsp()->ਚੌੜਾਈ_Min())) rhsWarn = ਝੂਠ;
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
    ਖਾਲੀ ਦੇਖੋ_real_ਜੋੜੋ_sub(Ast_ਗੰਢ_Biop* nodep) {
        // CALLER: AddD, MulD, ...
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // Note similar steps in visit_add_sub_replace promotion to double
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            iterateCheckReal(nodep, "RHS", nodep->rhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_Double();
        }
    }
    ਖਾਲੀ ਦੇਖੋ_real_neg_ceil(Ast_ਗੰਢ_Uniop* nodep) {
        // CALLER: Negate, Ceil, Log, ...
        assert_At_ਵਿਅੰਜਕ(nodep);
        ਜੇ (m_vup->prelim()) {  // First stage evaluation
            // See alsl visit_negate_not conversion
            iterateCheckReal(nodep, "LHS", nodep->lhsp(), BOTH);
            nodep->dtype_ਸੈੱਟ_Double();
        }
    }

    ਖਾਲੀ ਦੇਖੋ_sformatf_format(AstSFormatF* nodep) {
        // For sformatf's with constant format, iterate/check arguments
        UASSERT_OBJ(!nodep->ਵਿਅੰਜਕ_Format_2(), nodep, "Assumes constant format");
        ਤਾਰਕਿਕ ਵਿੱਚ_Pct = ਝੂਠ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* argp = nodep->exprsp();
        string newFormat;
        ਲਈ (ਅੱਖਰ ch : nodep->ਪਾਠ()) {
            ਜੇ (!ਵਿੱਚ_Pct && ch == '%') {
                ਵਿੱਚ_Pct = ਸੱਚ;
                newFormat += ch;
            } ਨਹੀਂਤਾਂ ਜੇ (ਵਿੱਚ_Pct && (std::ਅੰਕ_ਹੈ(ch) || ch == '.' || ch == '-')) {
                newFormat += ch;
            } ਨਹੀਂਤਾਂ ਜੇ (!ਵਿੱਚ_Pct) {  // Normal text
                newFormat += ch;
            } ਨਹੀਂਤਾਂ {
                ਵਿੱਚ_Pct = ਝੂਠ;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ nextp = argp ? VN_AS(argp->nextp(), ਗੰਢ_ਵਿਅੰਜਕ) : ਖਾਲੀ_ਸੂਚਕ;
                AstSFormatArg* ਅਚਲ fargp = VN_CAST(argp, SFormatArg);  // May not exist yet
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* subargp = fargp ? fargp->exprp() : argp;
                ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep
                    = subargp ? subargp->dtypep()->skipRefp() : ਖਾਲੀ_ਸੂਚਕ;
                ch = std::tolower(ch);
                ਚੋਣ (ch) {
                ਹਾਲਤ '?':  // Unspecified by user, guess
                    ਜੇ (dtypep->isDouble()) {
                        ch = 'g';
                    } ਨਹੀਂਤਾਂ ਜੇ (dtypep->isString()) {
                        ch = 's';
                    } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(dtypep, BasicDType)) {
                        ch = nodep->missingArgChar();
                    } ਨਹੀਂਤਾਂ {
                        ch = 'p';
                    }
                    argp = nextp;
                    ਰੋਕੋ;
                ਹਾਲਤ '%': ਰੋਕੋ;  // %% - just output a %
                ਹਾਲਤ 'm': ਰੋਕੋ;  // %m - auto insert "name"
                ਹਾਲਤ 'l': ਰੋਕੋ;  // %m - auto insert "library"
                ਹਾਲਤ 'd':
                    ਜੇ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ਜੇ (subargp->isDouble()) spliceCvtS(subargp, ਸੱਚ, 64);
                        argp = nextp;
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 'b':  // FALLTHRU
                ਹਾਲਤ 'o':  // FALLTHRU
                ਹਾਲਤ 'x':
                    ਜੇ (subargp) {
                        // V3Randomize may make a %x on a string, or may
                        // have a runtime format which gets past this error
                        checkFormatNumericArg(subargp, ch);
                        argp = nextp;
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 't':  // Decimal/float time
                    ਜੇ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        UASSERT_OBJ(!nodep->timeunit().isNone(), nodep,
                                    "display %t has no time units");
                        argp = nextp;
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 'e':  // FALLTHRU
                ਹਾਲਤ 'f':  // FALLTHRU
                ਹਾਲਤ 'g':
                    ਜੇ (subargp) {
                        checkFormatNumericArg(subargp, ch);
                        ਜੇ (!subargp->isDouble()) {
                            iterateCheckReal(nodep, "Display argument", subargp, BOTH);
                        }
                        argp = nextp;
                    }
                    ਰੋਕੋ;
                ਹਾਲਤ 'p':  // FALLTHRU
                ਹਾਲਤ 's':
                    // As with enum.name(): valid values print the mnemonic, else numeric
                    ਜੇ (subargp && !fargp) {
                        ਜੇ (AstEnumDType* ਅਚਲ enumDtp = formatEnumDType(subargp)) {
                            VNRelinker relinker;
                            subargp->unlink_Fr_ਪਿੱਠ(&relinker);
                            relinker.relink(newFormatEnumArg(subargp, enumDtp));
                        }
                    }
                    argp = nextp;
                    ਰੋਕੋ;
                ਮੂਲ:  // Most operators, just move to next argument
                    argp = nextp;
                    ਰੋਕੋ;
                }  // switch
                newFormat += ch;
            }
        }
        nodep->ਪਾਠ(newFormat);
    }

    ਸਥਿਰ AstEnumDType* formatEnumDType(Ast_ਗੰਢ_ਵਿਅੰਜਕ* subargp) {
        AstEnumDType* enumDtp = VN_CAST(subargp->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
        ਜੇ (!enumDtp) {
            ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ varrefp = VN_CAST(subargp, VarRef)) {
                enumDtp = VN_CAST(varrefp->varp()->dtypep()->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType);
            }
        }
        ਵਾਪਸ enumDtp;
    }

    Ast_ਵਿਅੰਜਕ_Stmt* newEnumCapture(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, AstEnumDType* dtypep) {
        File_ਸਤਰ* ਅਚਲ flp = valuep->fileline();
        AstVar* ਅਚਲ varp = ਨਵਾਂ AstVar{flp, VVarType::XTEMP, m_enum_ਆਰਜ਼ੀ_ਨਾਂ.get(valuep), dtypep};
        varp->noSubst(ਸੱਚ);
        ਜੇ (m_ftaskp) {
            varp->funcLocal(ਸੱਚ);
            varp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
            // Declare before use, including in non-inlined class methods.
            m_ftaskp->prependStmtsp(varp);
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(m_modep, valuep, "Enum expression without a containing module");
            m_modep->ਜੋੜੋ_Stmtsp(varp);
        }
        iterate(varp);
        Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ exprp = ਨਵਾਂ Ast_ਵਿਅੰਜਕ_Stmt{
            flp, ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, ਨਵਾਂ AstVarRef{flp, varp, VAccess::WRITE}, valuep},
            ਨਵਾਂ AstVarRef{flp, varp, VAccess::READ}};
        iterate(exprp);
        ਵਾਪਸ exprp;
    }

    AstSFormatArg* newFormatEnumArg(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep, AstEnumDType* dtypep) {
        ਜੇ (valuep->is_ਸ਼ੁੱਧ()) valuep = V3Const::constifyEdit(valuep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* keyp;
        ਜੇ (VN_IS(valuep, Const) || VN_IS(valuep, VarRef) || (!m_modep && !m_ftaskp)) {
            // Standalone parameter expressions are checked for constness by V3Const.
            keyp = valuep->clone_ਰੁੱਖ(ਝੂਠ);
        } ਨਹੀਂਤਾਂ {
            Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ capturep = newEnumCapture(valuep, dtypep);
            valuep = capturep;
            keyp = capturep->resultp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        }
        ਅਚਲ VFormatAttr attr = dtypep->isSigned() ? VFormatAttr::ENUM_SIGNED : VFormatAttr::ENUM;
        AstSFormatArg* ਅਚਲ argp = ਨਵਾਂ AstSFormatArg{valuep->fileline(), attr, valuep};
        argp->dtypep(dtypep);
        argp->namep(enumSelect(keyp, dtypep, VAttrType::ENUM_NAME));
        ਵਾਪਸ argp;
    }

    //----------------------------------------------------------------------
    // LOWER LEVEL WIDTH METHODS  (none iterate)

    ਤਾਰਕਿਕ ਚੌੜਾਈ_Bad(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ_DType* expDTypep) {
        ਅਚਲ ਪੂਰਨ exp_ਚੌੜਾਈ = expDTypep->ਚੌੜਾਈ();
        ਪੂਰਨ exp_ਚੌੜਾਈ_Min = expDTypep->ਚੌੜਾਈ_Min();
        UASSERT_OBJ(nodep->dtypep(), nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no dtype?? Missing Visitor func?");
        ਜੇ (expDTypep->basicp()->untyped() || nodep->dtypep()->basicp()->untyped()) ਵਾਪਸ ਝੂਠ;
        // During DepGraph execution, expected width may be 0 if the type hasn't been
        UASSERT_OBJ(nodep->ਚੌੜਾਈ() != 0, nodep,
                    "Under node " << nodep->prettyTypeName()
                                  << " has no expected width?? Missing Visitor func?");
        UASSERT_OBJ(exp_ਚੌੜਾਈ != 0, nodep,
                    "Node " << nodep->prettyTypeName()
                            << " has no expected width?? Missing Visitor func?");
        ਜੇ (exp_ਚੌੜਾਈ_Min == 0) exp_ਚੌੜਾਈ_Min = exp_ਚੌੜਾਈ;
        ਜੇ (nodep->dtypep()->ਚੌੜਾਈ() == exp_ਚੌੜਾਈ) ਵਾਪਸ ਝੂਠ;
        ਜੇ (nodep->dtypep()->ਚੌੜਾਈ_Sized() && nodep->ਚੌੜਾਈ() != exp_ਚੌੜਾਈ_Min) ਵਾਪਸ ਸੱਚ;
        ਜੇ (!nodep->dtypep()->ਚੌੜਾਈ_Sized() && nodep->ਚੌੜਾਈ_Min() > exp_ਚੌੜਾਈ_Min) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ ਝੂਠ;
    }

    ਖਾਲੀ fix_ਚੌੜਾਈ_Extend(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Ast_ਗੰਢ_DType* expDTypep, Extend_ਨਿਯਮ extend_ਨਿਯਮ) {
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
        UINFO(4, "  widthExtend_(r=" << static_cast<ਪੂਰਨ>(extend_ਨਿਯਮ) << ") old: " << nodep);
        ਜੇ (extend_ਨਿਯਮ == EXTEND_OFF) ਵਾਪਸ;
        AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਅਚਲ ਪੂਰਨ exp_ਚੌੜਾਈ = expDTypep->ਚੌੜਾਈ();
        ਜੇ (constp && !constp->ਅੰਕ().isNegative()) {
            // Save later constant propagation work, just right-size it.
            V3_ਸੰਖਿਆ ਅੰਕ{nodep, exp_ਚੌੜਾਈ, constp->ਅੰਕ()};
            ਅੰਕ.isSigned(ਝੂਠ);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(constp), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ਨਹੀਂਤਾਂ ਜੇ (exp_ਚੌੜਾਈ < nodep->ਚੌੜਾਈ()) {
            // Trunc - Extract
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstSel{nodep->fileline(), nodep, 0, exp_ਚੌੜਾਈ};
            newp->did_ਚੌੜਾਈ(ਸੱਚ);  // Don't replace dtype with unsigned
            linker.relink(newp);
            nodep = newp;
        } ਨਹੀਂਤਾਂ {
            // Extend
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            ਤਾਰਕਿਕ doSigned = ਝੂਠ;
            ਚੋਣ (extend_ਨਿਯਮ) {
            ਹਾਲਤ EXTEND_ZERO: doSigned = ਝੂਠ; ਰੋਕੋ;
            ਹਾਲਤ EXTEND_EXP: doSigned = nodep->isSigned() && expDTypep->isSigned(); ਰੋਕੋ;
            ਹਾਲਤ EXTEND_LHS: doSigned = nodep->isSigned(); ਰੋਕੋ;
            ਮੂਲ: nodep->v3fatalSrc("bad case");
            }
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp
                = (doSigned ? static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtendS{nodep->fileline(), nodep})
                            : static_cast<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>(ਨਵਾਂ AstExtend{nodep->fileline(), nodep}));
            newp->did_ਚੌੜਾਈ(ਸੱਚ);
            linker.relink(newp);
            nodep = newp;
        }
        ਜੇ (expDTypep->isDouble() && !nodep->isDouble()) {
            // For AstVar init() among others
            // TODO do all to-real and to-integer conversions in this function
            // rather than in callers
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = spliceCvtD(nodep);
            nodep = newp;
        }
        ਜੇ (expDTypep->skipRefp()->isIntegralOrPacked()) {
            nodep->dtype_Chg_ਚੌੜਾਈ_Signed(exp_ਚੌੜਾਈ, expDTypep->ਚੌੜਾਈ_Min(), expDTypep->numeric());
        } ਨਹੀਂਤਾਂ {
            nodep->dtypeFrom(expDTypep);
        }
        UINFO(4, "             _new: " << nodep);
    }

    ਖਾਲੀ fix_ਚੌੜਾਈ_Reduce(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Fix the width mismatch by adding a reduction OR operator
        // IF (A(CONSTwide)) becomes IF (A(CONSTreduced))
        // IF (A(somewide)) on pointer becomes on pointer IF (A(NEQ(somewide, Null)))
        // IF (A(somewide)) elsewise becomes IF (A(REDOR(somewide, 0)))
        // Attempt to fix it quietly
        ਅਚਲ ਪੂਰਨ exp_ਚੌੜਾਈ = 1;
        ਅਚਲ ਪੂਰਨ expSigned = ਝੂਠ;
        ਜੇ (nodep->ਚੌੜਾਈ() == exp_ਚੌੜਾਈ) ਵਾਪਸ;
        UINFO(4, "  widthReduce_old: " << nodep);
        AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਜੇ (constp) {
            V3_ਸੰਖਿਆ ਅੰਕ(nodep, exp_ਚੌੜਾਈ);
            ਅੰਕ.opRedOr(constp->ਅੰਕ());
            ਅੰਕ.isSigned(expSigned);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), ਅੰਕ};
            constp->replaceWith(newp);
            VL_DO_DANGLING(constp->delete_ਰੁੱਖ(), constp);
            VL_DANGLING(nodep);
            nodep = newp;
        } ਨਹੀਂਤਾਂ {
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            Ast_ਗੰਢ_DType* ਅਚਲ dtypeSkipp = nodep->dtypep()->skipRefp();
            ਜੇ (VN_IS(dtypeSkipp, ClassRefDType) || VN_IS(dtypeSkipp, IfaceRefDType)) {
                // Cannot use AstRedOr, as we may be redurcing a class handle
                newp = ਨਵਾਂ AstNeq{nodep->fileline(),
                                  ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Null{}},

                                  nodep};
            } ਨਹੀਂਤਾਂ {
                // But, AstRedOr can be faster on wide
                newp = ਨਵਾਂ AstRedOr{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            nodep = newp;
        }
        nodep->dtype_Chg_ਚੌੜਾਈ_Signed(exp_ਚੌੜਾਈ, exp_ਚੌੜਾਈ, VSigning::fromBool(expSigned));
        UINFO(4, "             _new: " << nodep);
    }

    ਤਾਰਕਿਕ fixAutoExtend(Ast_ਗੰਢ_ਵਿਅੰਜਕ*& nodepr, ਪੂਰਨ exp_ਚੌੜਾਈ) {
        // For SystemVerilog '0,'1,'x,'z, autoextend and don't warn
        ਜੇ (AstConst* ਅਚਲ constp = VN_CAST(nodepr, Const)) {
            ਜੇ (constp->ਅੰਕ().autoExtend() && !constp->ਅੰਕ().sized() && constp->ਚੌੜਾਈ() == 1) {
                // Make it the proper size.  Careful of proper extension of 0's/1's
                V3_ਸੰਖਿਆ ਅੰਕ(constp, exp_ਚੌੜਾਈ);
                ਅੰਕ.opRepl(constp->ਅੰਕ(), exp_ਚੌੜਾਈ);  // {width{'1}}
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{constp->fileline(), ਅੰਕ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixAutoExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ਰੁੱਖ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ਵਾਪਸ ਸੱਚ;
            }
            // X/Z also upper bit extend.  In pre-SV only to 32-bits, SV forever.
            ਨਹੀਂਤਾਂ ਜੇ (!constp->ਅੰਕ().sized()
                     // Make it the proper size.  Careful of proper extension of 0's/1's
                     && exp_ਚੌੜਾਈ > 32 && constp->ਅੰਕ().isMsbXZ()) {
                constp->v3warn(WIDTHXZEXPAND, "Unsized constant being X/Z extended to "
                                                  << exp_ਚੌੜਾਈ
                                                  << " bits: " << constp->prettyName());
                V3_ਸੰਖਿਆ ਅੰਕ(constp, exp_ਚੌੜਾਈ);
                ਅੰਕ.opExtendXZ(constp->ਅੰਕ(), constp->ਚੌੜਾਈ());
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{constp->fileline(), ਅੰਕ};
                // Spec says always unsigned with proper width
                UINFOTREE(5, constp, "", "fixUnszExtend_old");
                UINFOTREE(5, newp, "", "_new");
                constp->replaceWith(newp);
                VL_DO_DANGLING(constp->delete_ਰੁੱਖ(), constp);
                // Tell caller the new constp, and that we changed it.
                nodepr = newp;
                ਵਾਪਸ ਸੱਚ;
            }
        }
        ਵਾਪਸ ਝੂਠ;  // No change
    }
    ਤਾਰਕਿਕ is_ਅਧਾਰ_Class_Recurse(ਅਚਲ AstClass* ਅਚਲ cls1p, ਅਚਲ AstClass* ਅਚਲ cls2p) {
        // Returns true if cls1p and cls2p are equal or if cls1p is a base class of cls2p
        ਜੇ (cls1p == cls2p) ਵਾਪਸ ਸੱਚ;
        ਲਈ (ਅਚਲ AstClassExtends* cextp = cls2p->extendsp(); cextp;
             cextp = VN_CAST(cextp->nextp(), ClassExtends)) {
            ਜੇ (is_ਅਧਾਰ_Class_Recurse(cls1p, cextp->classp())) ਵਾਪਸ ਸੱਚ;
        }
        ਵਾਪਸ ਝੂਠ;
    }
    // Lines to end a message on nodep with the data types of its operands
    ਸਥਿਰ string warnDTypes(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ Ast_ਗੰਢ_DType* lhsDtp,
                             ਅਚਲ Ast_ਗੰਢ_DType* rhsDtp, ਅਚਲ string& lhsName = "Left-hand",
                             ਅਚਲ string& rhsName = "Right-hand")
        VL_REQUIRES(V3Error::s().m_mutex) {
        string lhsState = " " + lhsDtp->stateDTypeName();
        string rhsState = " " + rhsDtp->stateDTypeName();
        ਜੇ (lhsState == rhsState) lhsState = rhsState = "";
        ਵਾਪਸ "\n" + nodep->warn_ਹੋਰ() + "... " + lhsName
               + " data type: " + lhsDtp->prettyDTypeNameQ() + lhsState + "\n" + nodep->warn_ਹੋਰ()
               + "... " + rhsName + " data type: " + rhsDtp->prettyDTypeNameQ() + rhsState;
    }
    // Fixed-size unpacked arrays are assignment compatible only with the same number of
    // elements in each dimension (IEEE 1800-2023 7.6); dynamically sized ones always are
    ਸਥਿਰ ਤਾਰਕਿਕ unpacked_Array_Sizes_ਮੇਲ(ਅਚਲ Ast_ਗੰਢ_DType* lhsDtp, ਅਚਲ Ast_ਗੰਢ_DType* rhsDtp) {
        ਜਦੋਂ (lhsDtp->isNonPackedArray() && rhsDtp->isNonPackedArray()) {
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ lhsArrayp = VN_CAST(lhsDtp, UnpackArrayDType);
            ਅਚਲ AstUnpackArrayDType* ਅਚਲ rhsArrayp = VN_CAST(rhsDtp, UnpackArrayDType);
            ਜੇ (lhsArrayp && rhsArrayp
                && lhsArrayp->elementsConst() != rhsArrayp->elementsConst()) {
                ਵਾਪਸ ਝੂਠ;
            }
            lhsDtp = lhsDtp->subDTypep()->skipRefp();
            rhsDtp = rhsDtp->subDTypep()->skipRefp();
        }
        ਵਾਪਸ ਸੱਚ;
    }
    // Checks whether two types are assignment-compatible according to IEEE 1800-2023 7.6
    // Currently, this function only supports variables, which are of following types:
    // - Fixed-size unpacked array
    // - Dynamic unpacked array
    // - Associative array
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T, ਕਿਸਮ_ਨਾਂ N>
    ਖਾਲੀ checkUnpackedArrayAssignmentCompatible(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ T* ਅਚਲ lhsRefp,
                                                ਅਚਲ N* ਅਚਲ rhsRefp) {
        static_assert(
            (std::is_same<T, AstVar>::ਮੁੱਲ || std::is_same<T, Ast_ਗੰਢ_Var_Ref>::ਮੁੱਲ)
                && (std::is_same<N, AstVar>::ਮੁੱਲ || std::is_same<N, Ast_ਗੰਢ_Var_Ref>::ਮੁੱਲ),
            "Unsupported types provided.");
        ਜੇ (!lhsRefp || !rhsRefp) ਵਾਪਸ;
        std::string lhsName;
        std::string rhsName;
        ਜੇ (VN_IS(nodep, Pin)) {
            lhsName = std::string{"Pin"};
            rhsName = std::string{"Expression"};
        } ਨਹੀਂਤਾਂ {
            lhsName = std::string{"Left-hand"};
            rhsName = std::string{"Right-hand"};
        }

        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsDtp = lhsRefp->dtypep()->skipRefp();
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsDtp = rhsRefp->dtypep()->skipRefp();
        ਅਚਲ ਤਾਰਕਿਕ isLhsAggregate = lhsDtp->isAggregateType();
        ਅਚਲ ਤਾਰਕਿਕ isRhsAggregate = rhsDtp->isAggregateType();
        ਜੇ (!isLhsAggregate && !isRhsAggregate) ਵਾਪਸ;
        ਜੇ (isLhsAggregate ^ isRhsAggregate) {
            nodep->v3error(
                "Illegal assignment: types are not assignment compatible (IEEE 1800-2023 7.6)"
                << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(lhsDtp, ਕਤਾਰ_DType) && VN_IS(rhsDtp, ਖਾਲੀ_ਥਾਵਾਂ_ਕਤਾਰ_DType)) {
            ਵਾਪਸ;
        }
        std::pair<uint32_t, uint32_t> lhsDim = lhsDtp->dimensions(ਝੂਠ),
                                      rhsDim = rhsDtp->dimensions(ਝੂਠ);
        // Check if unpacked array dimensions are matching
        ਜੇ (lhsDim.ਦੂਜਾ != rhsDim.ਦੂਜਾ) {
            nodep->v3error("Illegal assignment: Unmatched number of unpacked dimensions (IEEE "
                           "1800-2023 7.6)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
            ਵਾਪਸ;
        }

        ਅਚਲ Ast_ਗੰਢ_DType* lhsDtpIterp = lhsDtp;
        ਅਚਲ Ast_ਗੰਢ_DType* rhsDtpIterp = rhsDtp;
        ਲਈ (uint32_t dim = 0; dim < rhsDim.ਦੂਜਾ; dim++) {
            // Associative arrays are compatible only with each other
            ਜੇ (VN_IS(lhsDtpIterp, AssocArrayDType) ^ VN_IS(rhsDtpIterp, AssocArrayDType)) {
                nodep->v3error("Illegal assignment: Associative arrays are assignment compatible "
                               "only with associative arrays (IEEE 1800-2023 7.6)"
                               << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
                ਵਾਪਸ;
            }
            lhsDtpIterp = lhsDtpIterp->subDTypep()->skipRefp();
            rhsDtpIterp = rhsDtpIterp->subDTypep()->skipRefp();
        }
        // Element types of source and target shall be equivalent
        ਜੇ (!isEquivalentDType(lhsDtpIterp, rhsDtpIterp)) {
            nodep->v3error("Illegal assignment: Array element types are not equivalent (IEEE "
                           "1800-2023 6.22.2)"
                           << warnDTypes(nodep, lhsDtp, rhsDtp, lhsName, rhsName));
        }
    }
    ਖਾਲੀ check_Class_ਸੌਂਪੋ(ਅਚਲ Ast_ਗੰਢ* nodep, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* rhsp,
                          Ast_ਗੰਢ_DType* ਅਚਲ lhsDTypep, ਤਾਰਕਿਕ isOutputArg = ਝੂਠ) {
        UASSERT_OBJ(rhsp->dtypep(), rhsp, "Node has no type");
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ lhsRawDTypep = lhsDTypep->skipRefp();
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsRawDTypep = rhsp->dtypep()->skipRefp();
        ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ lhsClassRefp = VN_CAST(lhsRawDTypep, ClassRefDType)) {
            ਜੇ (ਅਚਲ AstClassRefDType* ਅਚਲ rhsClassRefp
                = VN_CAST(rhsRawDTypep, ClassRefDType)) {
                // Output arg swaps sides: caller actual = callee formal at return.
                ਅਚਲ AstClass* ਅਚਲ dstp
                    = isOutputArg ? rhsClassRefp->classp() : lhsClassRefp->classp();
                ਅਚਲ AstClass* ਅਚਲ srcp
                    = isOutputArg ? lhsClassRefp->classp() : rhsClassRefp->classp();
                ਜੇ (is_ਅਧਾਰ_Class_Recurse(dstp, srcp)) ਵਾਪਸ;
            } ਨਹੀਂਤਾਂ ਜੇ (rhsp->isNull()) {
                ਵਾਪਸ;
            }
            nodep->v3error(side << " expects a " << lhsClassRefp->prettyTypeName() << ", got "
                                << rhsRawDTypep->prettyTypeName());
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(rhsRawDTypep, ClassRefDType)) {
            nodep->v3error(side << " " << rhsRawDTypep->prettyDTypeNameQ()
                                << " cannot be assigned to non-class "
                                << lhsDTypep->prettyDTypeNameQ());
        }
        ਜੇ (VN_IS(lhsRawDTypep, DynArrayDType) && VN_IS(rhsRawDTypep, UnpackArrayDType)) {
            VNRelinker relinker;
            rhsp->unlink_Fr_ਪਿੱਠ(&relinker);
            relinker.relink(
                ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_ਕਤਾਰ{rhsp->fileline(), VN_AS(rhsp, ਗੰਢ_ਵਿਅੰਜਕ), lhsDTypep});
        }
    }
    ਸਥਿਰ ਤਾਰਕਿਕ similarDTypeRecurse(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ node1p,
                                    ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ node2p) {
        ਵਾਪਸ node1p->skipRefp()->similarDType(node2p->skipRefp());
    }
    ਖਾਲੀ iterateCheckFileDesc(Ast_ਗੰਢ* parentp, Ast_ਗੰਢ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = underp->ਲੱਭੋ_UInt32_DType();
        underp
            = iterateCheck(parentp, "file_descriptor", underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterateCheckReal(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        // Coerce child to real if not already. Child is self-determined
        // e.g. parentp=ADDD, underp=ADD in ADDD(ADD(a,b), real-CONST)
        // Don't need separate PRELIM and FINAL(double) calls;
        // as if resolves to double, the BOTH correctly resolved double,
        // otherwise self-determined was correct
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ਲੱਭੋ_Double_DType(), stage);
    }
    ਖਾਲੀ iterateCheckSigned8(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        // Coerce child to signed8 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ਲੱਭੋ_Signed8_DType(), stage);
    }
    ਖਾਲੀ iterateCheckSigned32(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        // Coerce child to signed32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ਲੱਭੋ_Int_DType(), stage);
    }
    ਖਾਲੀ iterateCheckUInt32(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        // Coerce child to unsigned32 if not already. Child is self-determined
        iterateCheckTypedSelfPrelim(parentp, side, underp, parentp->ਲੱਭੋ_UInt32_DType(), stage);
    }
    ਖਾਲੀ iterateCheckDelay(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        // Coerce child to 64-bit delay if not already. Child is self-determined
        // underp may change as a result of replacement
        ਜੇ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        }
        ਜੇ (stage & FINAL) {
            Ast_ਗੰਢ_DType* expDTypep;
            ਜੇ (underp->dtypep()->skipRefp()->isDouble()) {  // V3Timing will later convert double
                expDTypep = parentp->ਲੱਭੋ_Double_DType();
            } ਨਹੀਂਤਾਂ {
                File_ਸਤਰ* ਅਚਲ newFl = ਨਵਾਂ File_ਸਤਰ{underp->fileline()};
                newFl->warnOff(V3_Error_ਕੋਡ::WIDTHEXPAND, ਸੱਚ);
                underp->fileline(newFl);
                expDTypep = parentp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType(64, 64, VSigning::UNSIGNED);
            }
            underp
                = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP, ਝੂਠ);
        }
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterateCheckString(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        iterateCheckTyped(parentp, side, underp, parentp->ਲੱਭੋ_String_DType(), stage);
    }
    ਖਾਲੀ iterateCheckTyped(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp,
                           Ast_ਗੰਢ_DType* expDTypep, Stage stage) {
        ਜੇ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{expDTypep, PRELIM}.p());
        }
        ਜੇ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterateCheckTypedSelfPrelim(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp,
                                     Ast_ਗੰਢ_DType* expDTypep, Stage stage) {
        // underp may change as a result of replacement
        ਜੇ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        }
        ਜੇ (stage & FINAL) {
            underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        }
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterateCheckIntegralSelf(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp,
                                  Determ determ, Stage stage) {
        iterateCheckSelf(parentp, side, underp, determ, stage, ਸੱਚ);
    }
    ਖਾਲੀ iterateCheckSelf(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Determ determ,
                          Stage stage, ਤਾਰਕਿਕ integralOnly = ਝੂਠ) {
        // Coerce child to any data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ਜੇ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        }
        underp
            = VN_IS(underp, ਗੰਢ_ਵਿਅੰਜਕ) ? checkCvtUS(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ), integralOnly) : underp;
        Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterateCheckSizedSelf(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Determ determ,
                               Stage stage, ਤਾਰਕਿਕ deferUntyped = ਝੂਠ) {
        // Coerce child to any sized-number data type; child is self-determined
        // i.e. isolated from expected type.
        // e.g. parentp=CONCAT, underp=lhs in CONCAT(lhs,rhs)
        UASSERT_OBJ(determ == SELF, parentp, "Bad call");
        UASSERT_OBJ(stage == FINAL || stage == BOTH, parentp, "Bad call");
        // underp may change as a result of replacement
        ਜੇ (stage & PRELIM) {
            underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, PRELIM}.p());
        }
        underp = VN_IS(underp, ਗੰਢ_ਵਿਅੰਜਕ) ? checkCvtUS(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ), ਝੂਠ) : underp;
        Ast_ਗੰਢ_DType* ਅਚਲ expDTypep = underp->dtypep();
        underp = iterateCheck(parentp, side, underp, SELF, FINAL, expDTypep, EXTEND_EXP);
        Ast_ਗੰਢ_DType* ਅਚਲ checkDtp = expDTypep->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        // Sampling functions may still reference an untyped property formal here.
        // V3AssertPre checks the actual type after property argument substitution.
        ਜੇ (!checkDtp->isIntegralOrPacked()
            && !(deferUntyped && checkDtp->basicp() && checkDtp->basicp()->untyped())) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << checkDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
        }
        (ਖਾਲੀ)underp;  // cppcheck
    }
    ਖਾਲੀ iterate_Check_ਸੌਂਪੋ(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* rhsp, Stage stage,
                            Ast_ਗੰਢ_DType* lhsDTypep, VAccess rhsAccess = VAccess::READ) {
        // Check using assignment-like context rules
        // UINFOTREE(1, parentp, "", "checkass");
        UASSERT_OBJ(stage == FINAL, parentp, "Bad width call");
        // Create unpacked byte from string see IEEE 1800-2023 5.9
        ਜੇ (AstConst* constp = VN_CAST(rhsp, Const)) {
            ਜੇ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp
                = VN_CAST(lhsDTypep->skipRefp(), UnpackArrayDType)) {
                ਜੇ (AstBasicDType* basicp = VN_CAST(arrayp->subDTypep()->skipRefp(), BasicDType)) {
                    ਜੇ (basicp->ਚੌੜਾਈ() == 8 && constp->ਅੰਕ().isFromString()) {
                        AstConsArray* newp = ਨਵਾਂ AstConsArray{
                            constp->fileline(), lhsDTypep,
                            ਨਵਾਂ AstConst{constp->fileline(), AstConst::Widthed_ਮੁੱਲ{}, 8, 0}};
                        ਲਈ (ਅਚਲ ਪੂਰਨ aindex : arrayp->declRange().seq_Lo_ਮੰਜ਼ਿਲ_Hi()) {
                            ਪੂਰਨ cindex = arrayp->declRange().ascending() ? (arrayp->hi() - aindex)
                                                                         : (aindex - arrayp->lo());
                            V3_ਸੰਖਿਆ selected{constp, 8};
                            selected.opSel(constp->ਅੰਕ(), cindex * 8 + 7, cindex * 8);
                            UINFO(0, "   aindex=" << aindex << "  cindex=" << cindex
                                                  << "  c=" << selected);
                            ਜੇ (!selected.isFourState()) {
                                ਜੇ (ਅਚਲ uint32_t c = selected.ਮੰਜ਼ਿਲ_UInt()) {
                                    newp->valuep(aindex,
                                                 ਨਵਾਂ AstConst{constp->fileline(),
                                                              AstConst::Widthed_ਮੁੱਲ{}, 8, c});
                                }
                            }
                        }
                        UINFO(6, "   unpackFromString: " << parentp);
                        rhsp->replaceWith(newp);
                        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
                        rhsp = newp;
                    }
                }
            }
        }
        // We iterate and size the RHS based on the result of RHS evaluation
        check_Class_ਸੌਂਪੋ(parentp, side, rhsp, lhsDTypep);
        ਅਚਲ ਤਾਰਕਿਕ lhs_ਧਾਰਾ = (VN_IS(parentp, ਗੰਢ_ਸੌਂਪੋ)
                                && VN_IS(VN_AS(parentp, ਗੰਢ_ਸੌਂਪੋ)->lhsp(), ਗੰਢ_ਧਾਰਾ));
        rhsp = iterateCheck(parentp, side, rhsp, ਸੌਂਪੋ_3, FINAL, lhsDTypep,
                            lhs_ਧਾਰਾ ? EXTEND_OFF : EXTEND_LHS, ਸੱਚ, ਧਾਰਾ_USE_NONE, rhsAccess);
        // UINFOTREE(1, parentp, "", "checkout");
        (ਖਾਲੀ)rhsp;  // cppcheck
    }
    // Convert an unpacked array value to an equivalent type with some dimension reversed,
    // keeping elements in left-to-right order (IEEE 1800-2023 7.6)
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* convert_Unpacked_ਕ੍ਰਮ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, Ast_ਗੰਢ_DType* dtypep) {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        ਜੇ (!arrayp || !arrayp->is_ਉਲਟ_ਦਾ(exprp->dtypep())) ਵਾਪਸ exprp;
        VNRelinker relinker;
        exprp->unlink_Fr_ਪਿੱਠ(&relinker);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ Ast_Cvt_Unpack_ਮੰਜ਼ਿਲ_Unpack{exprp->fileline(), exprp, dtypep};
        relinker.relink(newp);
        ਵਾਪਸ newp;
    }

    ਸਥਿਰ ਤਾਰਕਿਕ isLogicalOperator(ਅਚਲ Ast_ਗੰਢ* nodep) {
        // IEEE 1800-2023 Table 11-1 logical operators, which take only integral or real operands
        ਵਾਪਸ VN_IS(nodep, LogAnd) || VN_IS(nodep, LogEq) || VN_IS(nodep, LogIf)
               || VN_IS(nodep, LogNot) || VN_IS(nodep, LogOr);
    }
    ਖਾਲੀ iterateCheckBool(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Stage stage) {
        UASSERT_OBJ(stage == BOTH, parentp,
                    "Bad call");  // Booleans always self-determined so do BOTH at once
        // Underp is used in a self-determined but boolean context, reduce a
        // multibit number to one bit
        // stage is always BOTH so not passed as argument
        // underp may change as a result of replacement
        UASSERT_OBJ(underp, parentp, "Node has no child");
        underp = userIterateSubtreeReturnEdits(underp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        //
        // For DOUBLE under a logical op, add implied test against zero, never a warning
        Ast_ਗੰਢ_DType* ਅਚਲ underVDTypep = underp ? underp->dtypep()->skipRefp() : ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (underp && underVDTypep->isDouble()) {
            UINFO(6, "   spliceCvtCmpD0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ AstNeqD{parentp->fileline(), VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ),
                              ਨਵਾਂ AstConst{parentp->fileline(), AstConst::RealDouble{}, 0.0}};
            linker.relink(newp);
        } ਨਹੀਂਤਾਂ ਜੇ (underVDTypep->isString() && !isLogicalOperator(parentp)) {
            // For STRING as a condition, likewise test against the empty string, never a
            // warning; a string holds no NUL, so nonzero means non-empty (IEEE 1800-2023 6.16)
            UINFO(6, "   spliceCvtCmpN0: " << underp);
            VNRelinker linker;
            underp->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ* ਅਚਲ newp
                = ਨਵਾਂ AstNeqN{parentp->fileline(), VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ),
                              ਨਵਾਂ AstConst{parentp->fileline(), AstConst::String{}, ""}};
            linker.relink(newp);
        } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(underVDTypep, ClassRefDType) || VN_IS(underVDTypep, IfaceRefDType)
                   || (VN_IS(underVDTypep, BasicDType)
                       && VN_AS(underVDTypep, BasicDType)->isCHandle())) {
            // Allow warning-free "if (handle)"
            VL_DO_DANGLING(fix_ਚੌੜਾਈ_Reduce(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ)), underp);  //ਬਦਲਿਆ
        } ਨਹੀਂਤਾਂ ਜੇ (!underVDTypep->basicp() || underVDTypep->isAggregateType()
                   || underVDTypep->isString()) {
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expects a numeric " << side << " operand, but it has data type "
                             << underVDTypep->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
            underp->replaceWith(ਨਵਾਂ AstConst{parentp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}});
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(underp), underp);
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਤਾਰਕਿਕ bad = ਚੌੜਾਈ_Bad(underp, parentp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType());
            ਜੇ (bad) {
                {  // if (warnOn), but not needed here
                    UINFOTREE(5, parentp->backp(), "", "back");
                    parentp->v3widthWarn(1, underp->ਚੌੜਾਈ(),
                                         "Logical operator "
                                             << parentp->prettyTypeName()
                                             << " expects 1 bit on the " << side << ", but "
                                             << side << "'s " << underp->prettyTypeName()
                                             << " generates " << underp->ਚੌੜਾਈ()
                                             << (underp->ਚੌੜਾਈ() != underp->ਚੌੜਾਈ_Min()
                                                     ? " or " + cvt_ਮੰਜ਼ਿਲ_Str(underp->ਚੌੜਾਈ_Min())
                                                     : "")
                                             << " bits.");
                }
            }
            // Reduce to one bit even when the width is not reported, such as an unsized
            // (b & 1), so later passes see a 1-bit condition
            ਜੇ (Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ ਵਿਅੰਜਕ_Stmtp = VN_CAST(underp, ਵਿਅੰਜਕ_Stmt)) {
                // Reduce only the result, leaving the statements in place
                fix_ਚੌੜਾਈ_Reduce(ਵਿਅੰਜਕ_Stmtp->resultp());
                ਵਿਅੰਜਕ_Stmtp->dtypeFrom(ਵਿਅੰਜਕ_Stmtp->resultp());
            } ਨਹੀਂਤਾਂ ਜੇ (underp->ਚੌੜਾਈ() != 1) {
                VL_DO_DANGLING(fix_ਚੌੜਾਈ_Reduce(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ)), underp);  //ਬਦਲਿਆ
            }
        }
    }

    ਸਥਿਰ ਖਾਲੀ checkEnum(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* parentp, ਅਚਲ Ast_ਗੰਢ_DType* ਨਿਸ਼ਾਨਾ_DTypep,
                          ਅਚਲ Ast_ਗੰਢ_DType* ਸਰੋਤ_DTypep, ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਸਰੋਤ_Exprp) {
        ਜੇ (ਅਚਲ AstEnumDType* ਅਚਲ ਨਿਸ਼ਾਨਾ_Enump
            = VN_CAST(ਨਿਸ਼ਾਨਾ_DTypep->skip_Ref_ਮੰਜ਼ਿਲ_Enump(), EnumDType)) {
            ਅਚਲ VCastable castable
                = Ast_ਗੰਢ::computeCastable(ਨਿਸ਼ਾਨਾ_Enump, ਸਰੋਤ_DTypep, ਸਰੋਤ_Exprp);
            ਜੇ (castable != VCastable::SAMEISH && castable != VCastable::COMPATIBLE
                && castable != VCastable::ENUM_IMPLICIT) {
                nodep->v3warn(ENUMVALUE, "Implicit conversion to enum "
                                             << ਨਿਸ਼ਾਨਾ_DTypep->prettyDTypeNameQ() << " from "
                                             << ਸਰੋਤ_DTypep->prettyDTypeNameQ()
                                             << " (IEEE 1800-2023 6.19.3)\n"
                                             << parentp->warn_ਹੋਰ()
                                             << "... Suggest use enum's mnemonic, or static cast");
            }
        }
    }

    Ast_ਗੰਢ* iterateCheck(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side, Ast_ਗੰਢ* underp, Determ determ,
                          Stage stage, Ast_ਗੰਢ_DType* expDTypep, Extend_ਨਿਯਮ extend_ਨਿਯਮ,
                          ਤਾਰਕਿਕ warnOn = ਸੱਚ, ਧਾਰਾ_Use ਧਾਰਾ_Use_2 = ਧਾਰਾ_USE_NONE,
                          VAccess rhsAccess = VAccess::READ) {
        // Perform data type check on underp, which is underneath parentp used for error reporting
        // Returns the new underp
        // Conversion to/from doubles and integers are before iterating.
        UASSERT_OBJ(stage == FINAL, parentp, "Bad state to iterateCheck");
        UASSERT_OBJ(underp, parentp, "Node has no child");
        UASSERT_OBJ(underp->dtypep(), underp,
                    "Node has no type");  // Perhaps forgot to do a prelim visit on it?
        ਅਚਲ ਧਾਰਾ_Use ਬੱਚਾ_ਧਾਰਾ_Use
            = ਧਾਰਾ_Use_2 != ਧਾਰਾ_USE_NONE ? ਧਾਰਾ_Use_2 : ਧਾਰਾ_Use_For_Determ(determ);
        ਜੇ (rhsAccess.isReadOnly()) {
            ਜੇ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_CAST(underp, ਗੰਢ_ਵਿਅੰਜਕ)) {
                // A value only read is converted to a reversed expected type.
                underp = convert_Unpacked_ਕ੍ਰਮ(exprp, expDTypep);
            }
        }
        ਜੇ (VN_IS(underp, ਗੰਢ_DType)) {  // Note the node itself, not node's data type
            // Must be near top of these checks as underp->dtypep() will look normal
            underp->v3error(ucfirst(parentp->prettyOperatorName())
                            << " expected non-datatype " << side << " but "
                            << underp->prettyNameQ() << " is a datatype.");
        } ਨਹੀਂਤਾਂ ਜੇ (expDTypep == underp->dtypep()) {  // Perfect
            underp = userIterateSubtreeReturnEdits(underp,
                                                   ਚੌੜਾਈ_VP{expDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
        } ਨਹੀਂਤਾਂ ਜੇ (expDTypep->isDouble() && underp->isDouble()) {  // Also good
            underp = userIterateSubtreeReturnEdits(underp,
                                                   ਚੌੜਾਈ_VP{expDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
        } ਨਹੀਂਤਾਂ ਜੇ (expDTypep->isDouble() && !underp->isDouble()) {
            Ast_ਗੰਢ* ਅਚਲ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtD(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ));
            underp = userIterateSubtreeReturnEdits(oldp, ਚੌੜਾਈ_VP{SELF, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
        } ਨਹੀਂਤਾਂ ਜੇ (!expDTypep->isDouble() && underp->isDouble()) {
            Ast_ਗੰਢ* ਅਚਲ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtS(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ), ਸੱਚ, expDTypep->ਚੌੜਾਈ());  // Round RHS
            underp = userIterateSubtreeReturnEdits(oldp, ਚੌੜਾਈ_VP{SELF, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
        } ਨਹੀਂਤਾਂ ਜੇ (expDTypep->isString() && !underp->dtypep()->isString()) {
            Ast_ਗੰਢ* ਅਚਲ oldp
                = underp;  // Need FINAL on children; otherwise splice would block it
            spliceCvtString(VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ));
            underp = userIterateSubtreeReturnEdits(oldp, ਚੌੜਾਈ_VP{SELF, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ underDtp = underp->dtypep()->skipRefp();
            ਜੇ (determ != ਸੌਂਪੋ_3 && expDTypep->skipRefp()->isIntegralOrPacked()
                && underDtp->isAggregateType()) {
                parentp->v3error(ucfirst(parentp->prettyOperatorName())
                                 << " expects a numeric " << side
                                 << " operand, but it has data type "
                                 << underDtp->prettyDTypeNameQ() << " (IEEE 1800-2023 11.3)");
                Ast_ਗੰਢ* ਅਚਲ newp
                    = ਨਵਾਂ AstConst{underp->fileline(), AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False_Erroring{}};
                underp->replaceWith(newp);
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(underp), underp);
                underp = newp;
            }
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ expDtp = expDTypep->skipRefp();
            ਜੇ (determ == ਸੌਂਪੋ_3 && VN_IS(expDtp, ਗੰਢ_UOr_Struct_DType) && expDtp->isAggregateType()
                && !underDtp->isAggregateType()) {
                parentp->v3error("Illegal assignment: types are not assignment compatible (IEEE "
                                 "1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            // Connections of interface arrays have their own check, with a better message
            ਜੇ (determ == ਸੌਂਪੋ_3 && !isIfaceArrayDType(expDtp)
                && !unpacked_Array_Sizes_ਮੇਲ(expDtp, underDtp)) {
                parentp->v3error("Illegal assignment: Unmatched array sizes (IEEE 1800-2023 7.6)"
                                 << warnDTypes(parentp, expDtp, underDtp));
            }
            ਅਚਲ AstBasicDType* ਅਚਲ expBasicp = expDTypep->basicp();
            ਅਚਲ AstBasicDType* ਅਚਲ underBasicp = underp->dtypep()->basicp();
            ਜੇ (expBasicp && underBasicp) {
                ਜੇ (!m_enumItemp && !parentp->fileline()->warnIsOff(V3_Error_ਕੋਡ::ENUMVALUE)
                    && warnOn) {
                    ਜੇ (rhsAccess.isReadOrRW()) {
                        checkEnum(underp, parentp, expDTypep, underp->dtypep(),
                                  VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ));
                    }
                    ਜੇ (rhsAccess.isWriteOrRW()) {
                        checkEnum(underp, parentp, underp->dtypep(), expDTypep, ਖਾਲੀ_ਸੂਚਕ);
                    }
                }
                Ast_ਗੰਢ_DType* subDTypep = expDTypep;
                // We then iterate FINAL before width fixes, as if the under-operation
                // is e.g. an ADD, the ADD will auto-adjust to the proper data type
                // or if another operation e.g. ATOI will not.
                ਜੇ (determ == SELF) {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ਚੌੜਾਈ_VP{SELF, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
                } ਨਹੀਂਤਾਂ ਜੇ (determ == ਸੌਂਪੋ_3) {
                    // IEEE: Signedness is solely determined by the RHS
                    // (underp), not by the LHS (expDTypep)
                    ਜੇ (underp->isSigned() != subDTypep->isSigned()
                        || underp->ਚੌੜਾਈ() != subDTypep->ਚੌੜਾਈ()) {
                        subDTypep = parentp->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_DType(
                            std::max(subDTypep->ਚੌੜਾਈ(), underp->ਚੌੜਾਈ()),
                            std::max(subDTypep->ਚੌੜਾਈ_Min(), underp->ਚੌੜਾਈ_Min()),
                            VSigning::fromBool(underp->isSigned()),
                            subDTypep->skipRefp()->isFourstate()
                                || underp->dtypep()->skipRefp()->isFourstate());
                        UINFO(9, "Assignment of opposite-signed RHS to LHS: " << parentp);
                    }
                    underp = userIterateSubtreeReturnEdits(
                        underp, ਚੌੜਾਈ_VP{subDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
                } ਨਹੀਂਤਾਂ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ਚੌੜਾਈ_VP{subDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
                }
                // Note the check uses the expected size, not the child's subDTypep as we want the
                // child node's width to end up correct for the assignment (etc)
                ਚੌੜਾਈ_Check_Sized(parentp, side, VN_AS(underp, ਗੰਢ_ਵਿਅੰਜਕ), expDTypep, extend_ਨਿਯਮ,
                                warnOn);
            } ਨਹੀਂਤਾਂ ਜੇ (!VN_IS(parentp, Eq) && !VN_IS(parentp, Neq)
                       && !VN_IS(expDTypep->skipRefp(), IfaceRefDType)
                       && VN_IS(underp->dtypep()->skipRefp(), IfaceRefDType)) {
                underp->v3error(ucfirst(parentp->prettyOperatorName())
                                << " expected non-interface on " << side << " but "
                                << underp->prettyNameQ() << " is an interface.");
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstIfaceRefDType* expIfaceRefp
                       = VN_CAST(expDTypep->skipRefp(), IfaceRefDType)) {
                ਅਚਲ AstIfaceRefDType* underIfaceRefp
                    = VN_CAST(underp->dtypep()->skipRefp(), IfaceRefDType);
                ਜੇ (VN_IS(underp, Const) && VN_AS(underp, Const)->ਅੰਕ().isNull()) {
                    // '= null' is ok
                } ਨਹੀਂਤਾਂ ਜੇ (!underIfaceRefp) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ਰਾਹੀਂ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is not an interface.");
                } ਨਹੀਂਤਾਂ ਜੇ (expIfaceRefp->iface_ਰਾਹੀਂ_Cellp() != underIfaceRefp->iface_ਰਾਹੀਂ_Cellp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected " << expIfaceRefp->iface_ਰਾਹੀਂ_Cellp()->prettyNameQ()
                                    << " interface on " << side << " but " << underp->prettyNameQ()
                                    << " is a different interface ("
                                    << underIfaceRefp->iface_ਰਾਹੀਂ_Cellp()->prettyNameQ() << ").");
                } ਨਹੀਂਤਾਂ ਜੇ (underIfaceRefp->modportp()
                           && expIfaceRefp->modportp() != underIfaceRefp->modportp()) {
                    underp->v3error(ucfirst(parentp->prettyOperatorName())
                                    << " expected "
                                    << (expIfaceRefp->modportp()
                                            ? expIfaceRefp->modportp()->prettyNameQ()
                                            : "no")
                                    << " interface modport on " << side << " but got "
                                    << underIfaceRefp->modportp()->prettyNameQ() << " modport.");
                } ਨਹੀਂਤਾਂ {
                    underp = userIterateSubtreeReturnEdits(
                        underp, ਚੌੜਾਈ_VP{expDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
                }
            } ਨਹੀਂਤਾਂ {
                // Hope it just works out (perhaps a cast will deal with it)
                underp = userIterateSubtreeReturnEdits(
                    underp, ਚੌੜਾਈ_VP{expDTypep, FINAL, ਬੱਚਾ_ਧਾਰਾ_Use}.p());
            }
        }
        ਵਾਪਸ underp;
    }

    ਖਾਲੀ
    ਚੌੜਾਈ_Check_Sized(Ast_ਗੰਢ* parentp, ਅਚਲ ਅੱਖਰ* side,
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* underp,  // Node to be checked or have typecast added in front of
                    Ast_ਗੰਢ_DType* expDTypep, Extend_ਨਿਯਮ extend_ਨਿਯਮ, ਤਾਰਕਿਕ warnOn = ਸੱਚ) {
        // Issue warnings on sized number width mismatches, then do appropriate size extension
        // Generally iterateCheck is what is wanted instead of this
        // UINFO(9,"wchk "<<side<<endl<<"  "<<parentp<<endl<<"  "<<underp<<endl<<"
        // e="<<expDTypep<<" i"<<warnOn<<endl);
        ਅਚਲ AstBasicDType* ਅਚਲ expBasicp = expDTypep->basicp();
        ਅਚਲ AstBasicDType* ਅਚਲ underBasicp = underp->dtypep()->basicp();
        ਜੇ (expDTypep == underp->dtypep()) {
            ਵਾਪਸ;  // Same type must match
        } ਨਹੀਂਤਾਂ ਜੇ (!expBasicp || expBasicp->isDouble() || !underBasicp
                   || underBasicp->isDouble()) {
            // This is perhaps a v3fatalSrc as we should have checked the types
            // before calling widthCheck, but we may have missed a non-sized
            // check in earlier code, so might as well assume it is the users'
            // fault.
            parentp->v3error(ucfirst(parentp->prettyOperatorName())
                             << " expected non-complex non-double " << side << " in width check");
#ਜੇ VL_DEBUG
            parentp->v3fatalSrc("widthCheckSized should not be called on doubles/complex types");
#ਜੇ_ਅੰਤ
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਪੂਰਨ exp_ਚੌੜਾਈ = expDTypep->ਚੌੜਾਈ();
            ਪੂਰਨ exp_ਚੌੜਾਈ_Min = expDTypep->ਚੌੜਾਈ_Min();
            ਜੇ (exp_ਚੌੜਾਈ_Min == 0) exp_ਚੌੜਾਈ_Min = exp_ਚੌੜਾਈ;
            ਅਚਲ ਤਾਰਕਿਕ bad = ਚੌੜਾਈ_Bad(underp, expDTypep);
            ਜੇ ((bad || underp->ਚੌੜਾਈ() != exp_ਚੌੜਾਈ) && fixAutoExtend(underp /*ref*/, exp_ਚੌੜਾਈ)) {
                underp = ਖਾਲੀ_ਸੂਚਕ;  // Changes underp
                ਵਾਪਸ;
            }
            // If user has a sizing cast, assume they know what they are doing
            // (for better or worse)
            ਜੇ (VN_IS(parentp->backp(), Cast_ਮਾਪ)) warnOn = ਝੂਠ;
            ਜੇ (parentp->fileline()->erroringOn() || underp->fileline()->erroringOn())
                warnOn = ਝੂਠ;
            ਜੇ (VN_IS(underp, Const) && VN_AS(underp, Const)->ਅੰਕ().isFromString()
                && exp_ਚੌੜਾਈ > underp->ਚੌੜਾਈ()
                && (((exp_ਚੌੜਾਈ - underp->ਚੌੜਾਈ()) % 8) == 0)) {  // At least it's character sized
                // reg [31:0] == "foo" we'll consider probably fine.
                // Maybe this should be a special warning?  Not for now.
                warnOn = ਝੂਠ;
            }
            ਜੇ ((VN_IS(parentp, ਜੋੜੋ_2) && underp->ਚੌੜਾਈ() == 1 && underp->isOne())
                || (VN_IS(parentp, Sub) && underp->ਚੌੜਾਈ() == 1 && underp->isOne()
                    && 0 == std::strcmp(side, "RHS"))) {
                // "foo + 1'b1", or "foo - 1'b1" are very common, people assume
                // they extend correctly
                warnOn = ਝੂਠ;
            }
            ਜੇ (bad && warnOn) {
                UINFOTREE(5, parentp->backp(), "", "back");

                parentp->v3widthWarn(
                    exp_ਚੌੜਾਈ, underp->ਚੌੜਾਈ(),
                    ucfirst(parentp->prettyOperatorName())
                        << " expects " << exp_ਚੌੜਾਈ
                        << (exp_ਚੌੜਾਈ != exp_ਚੌੜਾਈ_Min ? " or " + cvt_ਮੰਜ਼ਿਲ_Str(exp_ਚੌੜਾਈ_Min) : "")
                        << " bits on the " << side << ", but " << side << "'s "
                        << underp->prettyTypeName() << " generates " << underp->ਚੌੜਾਈ()
                        << (underp->ਚੌੜਾਈ() != underp->ਚੌੜਾਈ_Min()
                                ? " or " + cvt_ਮੰਜ਼ਿਲ_Str(underp->ਚੌੜਾਈ_Min())
                                : "")
                        << " bits.");
            }
            ਜੇ (bad || underp->ਚੌੜਾਈ() != exp_ਚੌੜਾਈ) {
                // If we're in an NodeAssign, don't truncate the RHS if the LHS is
                // a NodeStream. The streaming operator changes the rules regarding
                // which bits to truncate.
                ਅਚਲ Ast_ਗੰਢ_ਸੌਂਪੋ* assignp = VN_CAST(parentp, ਗੰਢ_ਸੌਂਪੋ);
                ਅਚਲ AstPin* pinp = VN_CAST(parentp, Pin);
                ਜੇ (assignp && VN_IS(assignp->lhsp(), ਗੰਢ_ਧਾਰਾ)) {
                } ਨਹੀਂਤਾਂ ਜੇ (pinp && pinp->modVarp()->direction() != VDirection::ਨਿਵੇਸ਼) {
                    // V3Inst::pinReconnectSimple must deal
                    UINFO(5, "pinInSizeMismatch: " << pinp);
                } ਨਹੀਂਤਾਂ ਜੇ (assignp && VN_IS(underp, ਗੰਢ_ਧਾਰਾ) && exp_ਚੌੜਾਈ > underp->ਚੌੜਾਈ()) {
                    // IEEE 1800-2023 11.4.14: When assigning a stream to a wider fixed-size
                    // target, widen by filling zero bits on the right. That is, left-justify
                    // the stream bits within the target width.
                    // Build: ShiftL(Extend(stream, expWidth), expWidth - streamWidth)
                    UINFO(5, "Widen NodeStream RHS with left-justify per 11.4.14");
                    VNRelinker linker;
                    ਅਚਲ ਪੂਰਨ ਖਿਸਕਾਓ_2 = exp_ਚੌੜਾਈ - underp->ਚੌੜਾਈ();
                    underp->unlink_Fr_ਪਿੱਠ(&linker);
                    AstExtend* ਅਚਲ widenedp = ਨਵਾਂ AstExtend{underp->fileline(), underp};
                    widenedp->did_ਚੌੜਾਈ(ਸੱਚ);
                    // Shift left so zeros fill on the right
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ shiftedp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_L{
                        underp->fileline(), widenedp,
                        ਨਵਾਂ AstConst{underp->fileline(), static_cast<uint32_t>(ਖਿਸਕਾਓ_2)}, exp_ਚੌੜਾਈ};
                    // Final dtype should match expected
                    shiftedp->dtypep(expDTypep);
                    linker.relink(shiftedp);
                } ਨਹੀਂਤਾਂ {
                    VL_DO_DANGLING(fix_ਚੌੜਾਈ_Extend(underp, expDTypep, extend_ਨਿਯਮ), underp);
                }
            }
        }
    }

    //----------------------------------------------------------------------
    // SIGNED/DOUBLE METHODS

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* checkCvtUS(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਤਾਰਕਿਕ fatal) {
        ਜੇ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            ਜੇ (fatal) {
                nodep->v3error("Expected integral input to " << nodep->backp()->prettyTypeName());
            } ਨਹੀਂਤਾਂ {
                nodep->v3warn(REALCVT,
                              "Implicit conversion of real to integer; expected integral input to "
                                  << nodep->backp()->prettyTypeName());
            }
            nodep = spliceCvtS(nodep, ਝੂਠ, 32);
        }
        ਵਾਪਸ nodep;
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* spliceCvtD(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // For integer used in REAL context, convert to real
        // We don't warn here, "2.0 * 2" is common and reasonable
        ਜੇ (nodep && !nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtD: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
            ਜੇ (nodep->dtypep()->skipRefp()->isSigned()) {
                newp = ਨਵਾਂ AstISToRD{nodep->fileline(), nodep};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstIToRD{nodep->fileline(), nodep};
            }
            linker.relink(newp);
            ਵਾਪਸ newp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ nodep;
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* spliceCvtS(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਤਾਰਕਿਕ warnOn, ਪੂਰਨ ਚੌੜਾਈ) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ਜੇ (nodep && nodep->dtypep()->skipRefp()->isDouble()) {
            UINFO(6, "   spliceCvtS: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
                // We convert to/from int32_t rather than use floor() as want to make sure is
                // representable in integer's number of bits
                ਜੇ (constp->isDouble() && V3_ਸੰਖਿਆ::epsilonIntegral(constp->ਅੰਕ().ਮੰਜ਼ਿਲ_Double())) {
                    warnOn = ਝੂਠ;
                }
            }
            ਜੇ (warnOn) nodep->v3warn(REALCVT, "Implicit conversion of real to integer");
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstRToIRoundS{nodep->fileline(), nodep};
            linker.relink(newp);
            newp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Sized(ਚੌੜਾਈ, VSigning::SIGNED);
            ਵਾਪਸ newp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ nodep;
        }
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* spliceCvtString(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // IEEE-2012 11.8.1: Signed: Type coercion creates signed
        // 11.8.2: Argument to convert is self-determined
        ਜੇ (nodep && !(nodep->dtypep()->basicp() && nodep->dtypep()->basicp()->isString())) {
            UINFO(6, "   spliceCvtString: " << nodep);
            VNRelinker linker;
            nodep->unlink_Fr_ਪਿੱਠ(&linker);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstCvtPackString{nodep->fileline(), nodep};
            linker.relink(newp);
            ਵਾਪਸ newp;
        } ਨਹੀਂਤਾਂ {
            ਵਾਪਸ nodep;
        }
    }
    Ast_ਗੰਢ_Biop* replaceWithUOrSVersion(Ast_ਗੰਢ_Biop* nodep, ਤਾਰਕਿਕ signedFlavorNeeded) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ਜੇ (signedFlavorNeeded == nodep->signedFlavor()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (!nodep->dtypep()) nodep->dtypeFrom(nodep->lhsp());
        // To simplify callers, some node types don't need to change
        ਚੋਣ (nodep->type()) {
        ਹਾਲਤ VNType::Eq: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::Neq: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::EqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::NeqCase: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::EqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::NeqWild: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::ਜੋੜੋ_2: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::Sub: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਹਾਲਤ VNType::ਖਿਸਕਾਓ_L: nodep->dtypeChgSigned(signedFlavorNeeded); ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਮੂਲ: ਰੋਕੋ;
        }
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਚੋਣ (nodep->type()) {
        ਹਾਲਤ VNType::Gt: newp = ਨਵਾਂ AstGtS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::GtS: newp = ਨਵਾਂ AstGt{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Gte: newp = ਨਵਾਂ AstGteS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::GteS: newp = ਨਵਾਂ AstGte{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lt: newp = ਨਵਾਂ AstLtS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::LtS: newp = ਨਵਾਂ AstLt{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lte: newp = ਨਵਾਂ AstLteS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::LteS: newp = ਨਵਾਂ AstLte{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Div: newp = ਨਵਾਂ AstDivS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::DivS: newp = ਨਵਾਂ AstDiv{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::ModDiv: newp = ਨਵਾਂ AstModDivS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::ModDivS: newp = ਨਵਾਂ AstModDiv{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Mul: newp = ਨਵਾਂ AstMulS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::MulS: newp = ਨਵਾਂ AstMul{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::ਖਿਸਕਾਓ_R: newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_RS{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::ਖਿਸਕਾਓ_RS: newp = ਨਵਾਂ Ast_ਖਿਸਕਾਓ_R{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਮੂਲ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs sign change, but bad case: " << nodep);
            ਰੋਕੋ;
        }
        UINFO(6, "   ReplaceWithUOrSVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_Biop* replaceWithDVersion(Ast_ਗੰਢ_Biop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ਜੇ (nodep->doubleFlavor()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* newp = ਖਾਲੀ_ਸੂਚਕ;
        // No width change on output;...                // All below have bool or double outputs
        ਚੋਣ (nodep->type()) {
        ਹਾਲਤ VNType::ਜੋੜੋ_2: newp = ਨਵਾਂ Ast_ਜੋੜੋ_D{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Sub: newp = ਨਵਾਂ AstSubD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Pow: newp = ਨਵਾਂ AstPowD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Eq:
        ਹਾਲਤ VNType::EqCase: newp = ਨਵਾਂ AstEqD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Neq:
        ਹਾਲਤ VNType::NeqCase: newp = ਨਵਾਂ AstNeqD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Gt:
        ਹਾਲਤ VNType::GtS: newp = ਨਵਾਂ AstGtD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Gte:
        ਹਾਲਤ VNType::GteS: newp = ਨਵਾਂ AstGteD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lt:
        ਹਾਲਤ VNType::LtS: newp = ਨਵਾਂ AstLtD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lte:
        ਹਾਲਤ VNType::LteS: newp = ਨਵਾਂ AstLteD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Div:
        ਹਾਲਤ VNType::DivS: newp = ਨਵਾਂ AstDivD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Mul:
        ਹਾਲਤ VNType::MulS: newp = ਨਵਾਂ AstMulD{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਮੂਲ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ਰੋਕੋ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or double) is correct
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_Biop* replaceWithNVersion(Ast_ਗੰਢ_Biop* nodep) {
        // Given a signed/unsigned node type, replace with string version
        // Return new node or nullptr if nothing
        ਜੇ (nodep->stringFlavor()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Biop* newp = ਖਾਲੀ_ਸੂਚਕ;
        // No width change on output;...                // All below have bool or double outputs
        ਚੋਣ (nodep->type()) {
        ਹਾਲਤ VNType::Eq:
        ਹਾਲਤ VNType::EqCase:
        ਹਾਲਤ VNType::EqWild: newp = ਨਵਾਂ AstEqN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Neq:
        ਹਾਲਤ VNType::NeqCase:
        ਹਾਲਤ VNType::NeqWild: newp = ਨਵਾਂ AstNeqN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Gt:
        ਹਾਲਤ VNType::GtS: newp = ਨਵਾਂ AstGtN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Gte:
        ਹਾਲਤ VNType::GteS: newp = ਨਵਾਂ AstGteN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lt:
        ਹਾਲਤ VNType::LtS: newp = ਨਵਾਂ AstLtN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਹਾਲਤ VNType::Lte:
        ਹਾਲਤ VNType::LteS: newp = ਨਵਾਂ AstLteN{fl, lhsp, rhsp}; ਰੋਕੋ;
        ਮੂਲ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to string, but bad case: " << nodep);
            ਰੋਕੋ;
        }
        UINFO(6, "   ReplaceWithNVersion: " << nodep << " w/ " << newp);
        nodep->replaceWith(newp);
        // No width change; the default created type (bool or string) is correct
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_Uniop* replaceWithDVersion(Ast_ਗੰਢ_Uniop* nodep) {
        // Given a signed/unsigned node type, create the opposite type
        // Return new node or nullptr if nothing
        ਜੇ (nodep->doubleFlavor()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_Uniop* newp = ਖਾਲੀ_ਸੂਚਕ;
        ਚੋਣ (nodep->type()) {
        ਹਾਲਤ VNType::Negate: newp = ਨਵਾਂ AstNegateD{fl, lhsp}; ਰੋਕੋ;
        ਮੂਲ:  // LCOV_EXCL_LINE
            nodep->v3fatalSrc("Node needs conversion to double, but bad case: " << nodep);
            ਰੋਕੋ;
        }
        UINFO(6, "   ReplaceWithDVersion: " << nodep << " w/ " << newp);
        nodep->replaceWithKeepDType(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
        ਵਾਪਸ newp;
    }

    //----------------------------------------------------------------------
    // METHODS - strings

    ਖਾਲੀ replaceWithSFormat(Ast_ਵਿਧੀ_Call* nodep, ਅਚਲ string& format) {
        // For string.itoa and similar, replace with SFormatF
        AstArg* ਅਚਲ argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep,
                    "Argument needed for string method, call methodOkArguments before here");
        Ast_ਗੰਢ_Var_Ref* ਅਚਲ fromp = VN_AS(nodep->fromp()->unlink_Fr_ਪਿੱਠ(), VarRef);
        Ast_ਗੰਢ* ਅਚਲ newp = ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(), fromp,
            ਨਵਾਂ AstSFormatF{nodep->fileline(), format, ਝੂਠ, argp->exprp()->unlink_Fr_ਪਿੱਠ()}};
        fromp->access(VAccess::WRITE);
        ਧੱਕੋ_Deletep(nodep->backp());
        VL_DO_DANGLING(nodep->backp()->replaceWith(newp), newp);
    }

    //----------------------------------------------------------------------
    // METHODS - data types

    Ast_ਗੰਢ_DType* iterateEditMoveDTypep(Ast_ਗੰਢ* parentp, Ast_ਗੰਢ_DType* dtnodep) {
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
        ਅਚਲ ਤਾਰਕਿਕ child1 = (parentp->get_ਬੱਚਾ_DTypep() == dtnodep);
        ਅਚਲ ਤਾਰਕਿਕ child2 = (parentp->getChild2DTypep() == dtnodep);
        ਜੇ (child1 || child2) {
            UINFO(9, "iterateEditMoveDTypep child iterating " << dtnodep);
            // Iterate, this might edit the dtypes which means dtnodep now lost
            VL_DO_DANGLING(userIterate(dtnodep, ਖਾਲੀ_ਸੂਚਕ), dtnodep);
            // Figure out the new dtnodep, remained a child of parent so find it there
            dtnodep = child1 ? parentp->get_ਬੱਚਾ_DTypep() : parentp->getChild2DTypep();
            UASSERT_OBJ(dtnodep, parentp, "iterateEditMoveDTypep lost pointer to child");
            UASSERT_OBJ(dtnodep->did_ਚੌੜਾਈ(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution of "
                            << dtnodep->prettyTypeName());

            // Move to under netlist
            UINFO(9, "iterateEditMoveDTypep child moving " << dtnodep);
            dtnodep->unlink_Fr_ਪਿੱਠ();
            v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(dtnodep);
        }
        ਜੇ (!dtnodep->did_ਚੌੜਾਈ()) {
            UINFO(9, "iterateEditMoveDTypep pointer iterating " << dtnodep);
            // See notes in visit(AstBracketArrayDType*)
            UASSERT_OBJ(!VN_IS(dtnodep, BracketArrayDType), parentp,
                        "Brackets should have been iterated as children");
            userIterate(dtnodep, ਖਾਲੀ_ਸੂਚਕ);
            UASSERT_OBJ(dtnodep->did_ਚੌੜਾਈ(), parentp,
                        "iterateEditMoveDTypep didn't get width resolution");
        }
        ਵਾਪਸ dtnodep;
    }

    AstConst* dimension_ਮੁੱਲ(File_ਸਤਰ* fileline, Ast_ਗੰਢ_DType* nodep, VAttrType attrType,
                             ਪੂਰਨ dim) {
        // Return the dimension value for the specified attribute and constant dimension
        Ast_ਗੰਢ_DType* dtypep = nodep->skipRefp();
        VNumRange declRange;  // ranged() set false
        ਲਈ (ਪੂਰਨ i = 1; i <= dim; ++i) {
            // UINFO(9, "   dim at "<<dim<<"  "<<dtypep);
            declRange = VNumRange{};  // ranged() set false
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                declRange = adtypep->declRange();
                ਜੇ (i < dim) dtypep = adtypep->subDTypep()->skipRefp();
                ਜਾਰੀ;
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ adtypep
                       = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
                declRange = adtypep->declRange();
                ਰੋਕੋ;  // Sub elements don't look like arrays and can't iterate into
            } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ adtypep = VN_CAST(dtypep, BasicDType)) {
                ਜੇ (adtypep->isRanged()) declRange = adtypep->declRange();
                ਰੋਕੋ;
            }
            ਰੋਕੋ;  // LCOV_EXCL_LINE
        }
        AstConst* valp = ਖਾਲੀ_ਸੂਚਕ;  // If nullptr, construct from val
        ਪੂਰਨ val = 0;
        ਚੋਣ (attrType) {
        ਹਾਲਤ VAttrType::DIM_ਬਿੱਟ: {
            ਪੂਰਨ ਬਿੱਟ = 1;
            ਜਦੋਂ (dtypep) {
                // UINFO(9, "   bits at " << bits << "  " << dtypep);
                ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ adtypep = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                    ਬਿੱਟ *= adtypep->declRange().elements();
                    dtypep = adtypep->subDTypep()->skipRefp();
                    ਜਾਰੀ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ adtypep
                           = VN_CAST(dtypep, ਗੰਢ_UOr_Struct_DType)) {
                    ਬਿੱਟ *= adtypep->isStreamableFixedAggregate() ? adtypep->ਚੌੜਾਈ_ਧਾਰਾ()
                                                                  : adtypep->ਚੌੜਾਈ();
                    ਰੋਕੋ;
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ adtypep = VN_CAST(dtypep, BasicDType)) {
                    ਬਿੱਟ *= adtypep->ਚੌੜਾਈ();
                    ਰੋਕੋ;
                }
                ਰੋਕੋ;
            }
            ਜੇ (dim == 0) {
                val = 0;
            } ਨਹੀਂਤਾਂ ਜੇ (dim == 1 && !declRange.ranged()
                       && ਬਿੱਟ == 1) {  // $bits should be sane for non-arrays
                val = nodep->ਚੌੜਾਈ();
            } ਨਹੀਂਤਾਂ {
                val = ਬਿੱਟ;
            }
            ਰੋਕੋ;  // LCOV_EXCL_LINE
        }
        ਹਾਲਤ VAttrType::DIM_ਉੱਚਾ: val = !declRange.ranged() ? 0 : declRange.hi(); ਰੋਕੋ;
        ਹਾਲਤ VAttrType::DIM_ਖੱਬਾ: val = !declRange.ranged() ? 0 : declRange.ਖੱਬੇ(); ਰੋਕੋ;
        ਹਾਲਤ VAttrType::DIM_ਨੀਵਾਂ: val = !declRange.ranged() ? 0 : declRange.lo(); ਰੋਕੋ;
        ਹਾਲਤ VAttrType::DIM_ਸੱਜਾ: val = !declRange.ranged() ? 0 : declRange.ਸੱਜੇ(); ਰੋਕੋ;
        ਹਾਲਤ VAttrType::DIM_INCREMENT:
            val = (declRange.ranged() && declRange.ascending()) ? -1 : 1;
            ਰੋਕੋ;
        ਹਾਲਤ VAttrType::DIM_ਮਾਪ: val = !declRange.ranged() ? 0 : declRange.elements(); ਰੋਕੋ;
        ਮੂਲ: nodep->v3fatalSrc("Missing DIM ATTR type case"); ਰੋਕੋ;
        }
        ਜੇ (!valp) valp = ਨਵਾਂ AstConst{fileline, AstConst::Signed32{}, val};
        UINFO(9, " $dimension " << attrType.ascii() << "(" << cvt_ਮੰਜ਼ਿਲ_Hex(dtypep) << "," << dim
                                << ")=" << valp);
        ਵਾਪਸ valp;
    }
    AstVarRef* dimensionVarRefp(Ast_ਗੰਢ_DType* nodep, VAttrType attrType, uint32_t msbdim) {
        // Return a reference to a constant table which has the specified dimension properties
        // for this data type. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_ਸਾਰਣੀ_Map[std::make_pair(nodep, attrType)];
        ਜੇ (cachedpr) ਵਾਪਸ cachedpr->clone_ਰੁੱਖ(ਝੂਠ);

        Ast_ਗੰਢ_Array_DType* ਅਚਲ vardtypep = ਨਵਾਂ AstUnpackArrayDType{
            nodep->fileline(), nodep->ਲੱਭੋ_Int_DType(), ਨਵਾਂ AstRange(nodep->fileline(), msbdim, 0)};
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(vardtypep);
        userIterate(vardtypep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{nodep->fileline(), vardtypep};
        ਲਈ (ਬਿਨਾਨਿਸ਼ਾਨੀ i = 0; i < msbdim + 1; ++i) {
            consp->valuep(i, dimension_ਮੁੱਲ(nodep->fileline(), nodep, attrType, i));
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ਲੱਭੋ(consp);
        VL_DO_DANGLING(consp->delete_ਰੁੱਖ(), consp);  // V3ConstPool::find clones it
        cachedpr->varp()->did_ਚੌੜਾਈ(ਸੱਚ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_ਚੌੜਾਈ(ਸੱਚ);
        ਧੱਕੋ_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ਵਾਪਸ cachedpr->clone_ਰੁੱਖ(ਝੂਠ);
    }
    ਸਥਿਰ uint64_t enum_Max_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ* errNodep, ਅਚਲ AstEnumDType* adtypep) {
        // Most enums unless overridden are 32 bits, so we size array
        // based on max enum value used.
        // Ideally we would have a fast algorithm when a number is
        // of small width and complete and so can use an array, and
        // a map for when the value is many bits and sparse.
        uint64_t maxval = 0;
        ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = adtypep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
            ਅਚਲ AstConst* ਅਚਲ vconstp = VN_CAST(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, errNodep, "Enum item without constified value");
            ਜੇ (!vconstp->ਅੰਕ().isAnyXZ() && vconstp->ਮੰਜ਼ਿਲ_UQuad() >= maxval)
                maxval = vconstp->ਮੰਜ਼ਿਲ_UQuad();
        }
        ਜੇ (adtypep->itemsp()->ਚੌੜਾਈ() > 64) {
            errNodep->v3warn(E_UNSUPPORTED,
                             "Unsupported: enum next/prev method on enum with > 64 bits");
            ਵਾਪਸ 64;
        }
        ਵਾਪਸ maxval;
    }
    AstVarRef* enumVarRefp(AstEnumDType* ਅਚਲ nodep, VAttrType attrType, ਤਾਰਕਿਕ assoc,
                           uint32_t msbdim) {
        // Return a reference to a constant table which has the specified attrType information
        // for this enum. The reference is cached, so the table is built only once.
        AstVarRef*& cachedpr = m_ਸਾਰਣੀ_Map[std::make_pair(nodep, attrType)];
        ਜੇ (cachedpr) ਵਾਪਸ cachedpr->clone_ਰੁੱਖ(ਝੂਠ);

        UINFO(9, "Construct Venumtab attr=" << attrType.ascii() << " assoc=" << assoc
                                            << " max=" << msbdim << " for " << nodep);
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        Ast_ਗੰਢ_DType* basep;
        ਜੇ (attrType == VAttrType::ENUM_NAME) {
            basep = nodep->ਲੱਭੋ_String_DType();
        } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_VALID) {
            // TODO in theory we could bit-pack the bits in the table, but
            // would require additional operations to extract, so only
            // would be worth it for larger tables which perhaps could be
            // better handled with equation generation?
            basep = nodep->ਲੱਭੋ_ਦੋਆਧਾਰੀ_ਅੰਕ_DType();
        } ਨਹੀਂਤਾਂ {
            basep = nodep->dtypep();
        }
        Ast_ਗੰਢ_DType* vardtypep;
        ਜੇ (assoc) {
            // Key by the base type of the enum, not the enum itself, so identical maps of
            // different enum types can be shared in the constant pool
            vardtypep = ਨਵਾਂ AstAssocArrayDType{flp, basep, nodep->subDTypep()};
        } ਨਹੀਂਤਾਂ {
            vardtypep = ਨਵਾਂ AstUnpackArrayDType{flp, basep, ਨਵਾਂ AstRange(flp, msbdim, 0)};
        }
        v3Global.rootp()->typeTablep()->ਜੋੜੋ_Typesp(vardtypep);

        // Default for all unspecified values
        AstConst* defaultp = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (attrType == VAttrType::ENUM_NAME) {
            defaultp = ਨਵਾਂ AstConst{flp, AstConst::String{}, ""};
        } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_ਅਗਲਾ || attrType == VAttrType::ENUM_PREV) {
            defaultp = ਨਵਾਂ AstConst{flp, V3_ਸੰਖਿਆ{nodep, nodep->ਚੌੜਾਈ(), 0}};
        } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_VALID) {
            defaultp = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}};
        } ਨਹੀਂਤਾਂ {
            nodep->v3fatalSrc("Bad case");
        }

        // Find valid values and populate
        UASSERT_OBJ(nodep->itemsp(), nodep, "enum without items");
        std::map<uint64_t, Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਮੁੱਲਾਂ;
        {
            uint64_t ordinal = 0;
            Ast_Enum_ਵਸਤੂ* ਅਚਲ firstp = nodep->itemsp();
            ਅਚਲ Ast_Enum_ਵਸਤੂ* prevp = firstp;  // Prev must start with last item
            ਜਦੋਂ (prevp->nextp()) prevp = VN_AS(prevp->nextp(), Enum_ਵਸਤੂ);
            ਲਈ (Ast_Enum_ਵਸਤੂ* itemp = firstp; itemp;) {
                Ast_Enum_ਵਸਤੂ* ਅਚਲ nextp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ);
                ਅਚਲ AstConst* ਅਚਲ vconstp = VN_AS(itemp->valuep(), Const);
                UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
                ਜੇ (!vconstp->ਅੰਕ().isAnyXZ()) {  // Can 2-state runtime decode
                    ਅਚਲ uint64_t i = nodep->isWide() ? ++ordinal : vconstp->ਮੰਜ਼ਿਲ_UQuad();
                    ਜੇ (attrType == VAttrType::ENUM_NAME) {
                        ਮੁੱਲਾਂ[i] = ਨਵਾਂ AstConst{flp, AstConst::String{},
                                                 V3_ਸੰਖਿਆ::displayedEnumName(itemp)};
                    } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_ਅਗਲਾ) {
                        ਮੁੱਲਾਂ[i]
                            = (nextp ? nextp : firstp)->valuep()->clone_ਰੁੱਖ(ਝੂਠ);  // A const
                    } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_PREV) {
                        ਮੁੱਲਾਂ[i] = prevp->valuep()->clone_ਰੁੱਖ(ਝੂਠ);  // A const
                    } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_VALID) {
                        ਮੁੱਲਾਂ[i] = ਨਵਾਂ AstConst{flp, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_True{}};
                    } ਨਹੀਂਤਾਂ {
                        nodep->v3fatalSrc("Bad case");
                    }
                }
                prevp = itemp;
                itemp = nextp;
            }
        }
        // Add all specified values to table
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* valuep = ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (assoc) {
            // Keyed by the enum's base type
            AstConsAssoc* ਅਚਲ consp = ਨਵਾਂ AstConsAssoc{flp, defaultp};
            consp->dtypep(vardtypep);
            ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& itr : ਮੁੱਲਾਂ) {
                V3_ਸੰਖਿਆ ਕੁੰਜੀ{nodep, nodep->ਚੌੜਾਈ()};
                ਕੁੰਜੀ.ਸੈੱਟ_Quad(itr.ਪਹਿਲਾ);
                consp->ਜੋੜੋ_Itemsp(ਨਵਾਂ Ast_Cons_Assoc_ਵਸਤੂ{flp, ਨਵਾਂ AstConst{flp, ਕੁੰਜੀ}, itr.ਦੂਜਾ});
            }
            valuep = consp;
        } ਨਹੀਂਤਾਂ {
            AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{flp, vardtypep, defaultp};
            ਲਈ (uint64_t i = 0; i < (msbdim + 1); ++i) {
                ਜੇ (ਮੁੱਲਾਂ[i]) consp->valuep(i, ਮੁੱਲਾਂ[i]);
            }
            valuep = consp;
        }

        // Share identical tables via the constant pool
        cachedpr = V3ConstPool::ਲੱਭੋ(valuep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(valuep), valuep);  // V3ConstPool::find clones it
        cachedpr->varp()->did_ਚੌੜਾਈ(ਸੱਚ);
        // Keep the dtype of the value, as the entry might have a different, compatible one
        cachedpr->did_ਚੌੜਾਈ(ਸੱਚ);
        ਧੱਕੋ_Deletep(cachedpr);  // Deleted with the visitor - always cloned
        ਵਾਪਸ cachedpr->clone_ਰੁੱਖ(ਝੂਠ);
    }

    Ast_ਗੰਢ_ਵਿਅੰਜਕ* enumSelect(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, AstEnumDType* adtypep, VAttrType attrType) {
        // Return expression to get given attrType information from a enum's value (nodep)
        ਜੇ (attrType == VAttrType::ENUM_NAME) {
            ਜੇ (nodep->is_ਸ਼ੁੱਧ()) nodep = V3Const::constifyEdit(nodep);
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{},
                                                       constp->ਅੰਕ().displayedEnumName(adtypep)};
                VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
                ਵਾਪਸ newp;
            }
        }
        ਜੇ (attrType == VAttrType::ENUM_NAME && !nodep->is_ਸ਼ੁੱਧ() && (m_modep || m_ftaskp)) {
            Ast_ਵਿਅੰਜਕ_Stmt* ਅਚਲ capturep = newEnumCapture(nodep, adtypep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ valuep = capturep->resultp()->unlink_Fr_ਪਿੱਠ();
            capturep->resultp(enumSelect(valuep, adtypep, attrType));
            capturep->dtype_ਸੈੱਟ_String();
            ਵਾਪਸ capturep;
        }
        ਜੇ (attrType == VAttrType::ENUM_NAME && adtypep->isWide()) {
            // Wide values select an ordinal in the shared name table.
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* indexp = ਨਵਾਂ AstConst{nodep->fileline(), 0};
            uint32_t ordinal = 0;
            ਲਈ (ਅਚਲ Ast_Enum_ਵਸਤੂ* itemp = adtypep->itemsp(); itemp;
                 itemp = VN_AS(itemp->nextp(), Enum_ਵਸਤੂ)) {
                ਅਚਲ AstConst* ਅਚਲ constp = VN_AS(itemp->valuep(), Const);
                ਜੇ (constp->ਅੰਕ().isAnyXZ()) ਜਾਰੀ;
                indexp = ਨਵਾਂ AstCond{nodep->fileline(),
                                     ਨਵਾਂ AstEq{nodep->fileline(), nodep->clone_ਰੁੱਖ(ਝੂਠ),
                                               itemp->valuep()->clone_ਰੁੱਖ(ਝੂਠ)},
                                     ਨਵਾਂ AstConst{nodep->fileline(), ++ordinal}, indexp};
            }
            AstVarRef* ਅਚਲ tabRefp = enumVarRefp(adtypep, attrType, ਝੂਠ, ordinal);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newp = ਨਵਾਂ AstArraySel{nodep->fileline(), tabRefp, indexp};
            newp->dtype_ਸੈੱਟ_String();
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
            ਵਾਪਸ newp;
        }
        ਅਚਲ uint64_t msbdim = enum_Max_ਮੁੱਲ(nodep, adtypep);
        ਅਚਲ ਤਾਰਕਿਕ assoc = msbdim > ENUM_LOOKUP_ਬਿੱਟ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp;
        ਜੇ (assoc) {
            AstVarRef* ਅਚਲ tabRefp = enumVarRefp(adtypep, attrType, ਸੱਚ, 0);
            newp = ਨਵਾਂ AstAssocSel{nodep->fileline(), tabRefp, nodep};
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਪੂਰਨ selwidth = V3_ਸੰਖਿਆ::log2b(msbdim) + 1;  // Width to address a bit
            AstVarRef* ਅਚਲ tabRefp
                = enumVarRefp(adtypep, attrType, ਝੂਠ, (1ULL << selwidth) - 1);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ boundp
                = attrType == VAttrType::ENUM_NAME
                          && (adtypep->ਚੌੜਾਈ() > selwidth || msbdim != (1ULL << selwidth) - 1)
                      ? ਨਵਾਂ AstGt{nodep->fileline(), nodep->clone_ਰੁੱਖ(ਝੂਠ),
                                  ਨਵਾਂ AstConst{nodep->fileline(),
                                               V3_ਸੰਖਿਆ{nodep, adtypep->ਚੌੜਾਈ(),
                                                        static_cast<uint32_t>(msbdim)}}}
                      : ਖਾਲੀ_ਸੂਚਕ;
            newp = ਨਵਾਂ AstArraySel{
                nodep->fileline(), tabRefp,
                // Select in case widths are off due to msblen!=width
                // We return "random" values if outside the range, which is fine
                // as next/previous on illegal values just need something good out
                ਨਵਾਂ AstSel{nodep->fileline(), nodep, 0, selwidth}};
            ਜੇ (boundp) {
                newp = ਨਵਾਂ AstCond{nodep->fileline(), boundp,
                                   ਨਵਾਂ AstConst{nodep->fileline(), AstConst::String{}, ""}, newp};
            }
        }
        ਜੇ (attrType == VAttrType::ENUM_NAME) {
            newp->dtype_ਸੈੱਟ_String();
        } ਨਹੀਂਤਾਂ ਜੇ (attrType == VAttrType::ENUM_VALID) {
            newp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ();
        } ਨਹੀਂਤਾਂ {
            newp->dtypeFrom(adtypep);  // To prevent a later ENUMVALUE
        }
        ਵਾਪਸ newp;
    }
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* enumTestValid(Ast_ਗੰਢ_ਵਿਅੰਜਕ* valp, AstEnumDType* enumDtp) {
        ਅਚਲ uint64_t maxval = enum_Max_ਮੁੱਲ(valp, enumDtp);
        ਅਚਲ ਤਾਰਕਿਕ assoc = maxval > ENUM_LOOKUP_ਬਿੱਟ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* testp = ਖਾਲੀ_ਸੂਚਕ;
        File_ਸਤਰ* ਅਚਲ fl_novalue = ਨਵਾਂ File_ਸਤਰ{valp->fileline()};
        fl_novalue->warnOff(V3_Error_ਕੋਡ::ENUMVALUE, ਸੱਚ);
        fl_novalue->warnOff(V3_Error_ਕੋਡ::CMPCONST, ਸੱਚ);
        ਜੇ (assoc) {
            AstVarRef* ਅਚਲ tabRefp = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ਸੱਚ, 0);
            testp = ਨਵਾਂ AstAssocSel{fl_novalue, tabRefp, valp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ)};
        } ਨਹੀਂਤਾਂ {
            ਅਚਲ ਪੂਰਨ selwidth = V3_ਸੰਖਿਆ::log2b(maxval) + 1;  // Width to address a bit
            AstVarRef* ਅਚਲ tabRefp
                = enumVarRefp(enumDtp, VAttrType::ENUM_VALID, ਝੂਠ, (1ULL << selwidth) - 1);
            File_ਸਤਰ* ਅਚਲ fl_nowidth = ਨਵਾਂ File_ਸਤਰ{fl_novalue};
            fl_nowidth->warnOff(V3_Error_ਕੋਡ::ਚੌੜਾਈ_2, ਸੱਚ);
            testp = ਨਵਾਂ AstCond{
                fl_novalue,
                ਨਵਾਂ AstGt{fl_nowidth, valp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                          ਨਵਾਂ AstConst{fl_nowidth, AstConst::Unsized64{}, maxval}},
                ਨਵਾਂ AstConst{fl_novalue, AstConst::ਦੋਆਧਾਰੀ_ਅੰਕ_False{}},
                ਨਵਾਂ AstArraySel{fl_novalue, tabRefp,
                                ਨਵਾਂ AstSel{fl_novalue, valp->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ), 0, selwidth}}};
        }
        ਵਾਪਸ testp;
    }

    PatVecMap pat_ਸਦਿਸ਼_Map(Ast_ਨਮੂਨਾ* nodep, ਅਚਲ VNumRange& range) {
        PatVecMap patmap;
        ਪੂਰਨ element = range.ਖੱਬੇ();
        ਲਈ (AstPatMember* patp = VN_AS(nodep->itemsp(), PatMember); patp;
             patp = VN_AS(patp->nextp(), PatMember)) {
            ਜੇ (patp->keyp()) {
                ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = ਨਮੂਨਾ_Const_Keyp(patp)) {
                    ਜੇ (constp->ਅੰਕ().isFourState()) {
                        patp->keyp()->v3error("Assignment pattern key cannot contain X or Z");
                    } ਨਹੀਂਤਾਂ {
                        element = constp->ਮੰਜ਼ਿਲ_SInt();
                    }
                }
            }
            ਅਚਲ ਤਾਰਕਿਕ new_ਇੰਦਰਾਜ = patmap.emplace(element, patp).ਦੂਜਾ;
            ਜੇ (!new_ਇੰਦਰਾਜ) {
                patp->v3error("Assignment pattern key used multiple times: " << element);
            }
            element += range.ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc();
        }
        ਵਾਪਸ patmap;
    }

    // Add to patternp the items of nodep, a concatenation or a single item
    ਖਾਲੀ patConcatConvertRecurse(Ast_ਨਮੂਨਾ* patternp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (AstConcat* ਅਚਲ concatp = VN_CAST(nodep, Concat)) {
            patConcatConvertRecurse(patternp, concatp->lhsp());
            patConcatConvertRecurse(patternp, concatp->rhsp());
        } ਨਹੀਂਤਾਂ {
            AstPatMember* ਅਚਲ newp
                = ਨਵਾਂ AstPatMember{nodep->fileline(), nodep->unlink_Fr_ਪਿੱਠ(), ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ};
            newp->isConcat(ਸੱਚ);
            patternp->ਜੋੜੋ_Itemsp(newp);
        }
    }

    ਖਾਲੀ makeOpenArrayShell(Ast_ਗੰਢ_FTask_Ref* nodep) {
        UINFO(4, "Replicate openarray function " << nodep->taskp());
        Ast_ਗੰਢ_FTask* ਅਚਲ oldTaskp = nodep->taskp();
        oldTaskp->dpi_Open_ਮਾਪਾ_Inc();
        UASSERT_OBJ(!oldTaskp->dpi_Open_ਬੱਚਾ(), oldTaskp,
                    "DPI task should be parent or child, not both");
        Ast_ਗੰਢ_FTask* ਅਚਲ newTaskp = oldTaskp->clone_ਰੁੱਖ(ਝੂਠ);
        newTaskp->dpi_Open_ਬੱਚਾ(ਸੱਚ);
        newTaskp->dpi_Open_ਮਾਪਾ_Clear();
        newTaskp->name(newTaskp->name() + "__Vdpioc" + cvt_ਮੰਜ਼ਿਲ_Str(oldTaskp->dpi_Open_ਮਾਪਾ()));
        oldTaskp->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newTaskp);
        // Relink reference to new function
        nodep->taskp(newTaskp);
        nodep->name(nodep->taskp()->name());
        // Replace open array arguments with the callee's task
        ਅਚਲ V3TaskConnects tconnects = V3Task::taskConnects(nodep, nodep->taskp()->stmtsp());
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& tconnect : tconnects) {
            AstVar* ਅਚਲ portp = tconnect.ਪਹਿਲਾ;
            ਅਚਲ AstArg* ਅਚਲ argp = tconnect.ਦੂਜਾ;
            ਅਚਲ Ast_ਗੰਢ* ਅਚਲ pinp = argp->exprp();
            ਜੇ (!pinp) ਜਾਰੀ;  // Argument error we'll find later
            ਜੇ (hasOpenArrayDTypeRecurse(portp->dtypep())) {
                Ast_ਗੰਢ_DType* ਅਚਲ oldDTypep = portp->dtypep();
                portp->dtypep(pinp->dtypep());
                newTaskp->foreach([oldDTypep, portp](Ast_ਗੰਢ* nodep) {
                    ਜੇ (nodep->dtypep() == oldDTypep) nodep->dtypep(portp->dtypep());
                });
            }
        }
    }

    ਤਾਰਕਿਕ markHasOpenArray(Ast_ਗੰਢ_FTask* nodep) {
        ਤਾਰਕਿਕ hasOpen = ਝੂਠ;
        ਲਈ (Ast_ਗੰਢ* stmtp = nodep->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (AstVar* ਅਚਲ portp = VN_CAST(stmtp, Var)) {
                ਜੇ (portp->isDpiOpenArray() || hasOpenArrayDTypeRecurse(portp->dtypep())) {
                    portp->isDpiOpenArray(ਸੱਚ);
                    hasOpen = ਸੱਚ;
                }
            }
        }
        ਵਾਪਸ hasOpen;
    }
    ਤਾਰਕਿਕ hasOpenArrayDTypeRecurse(Ast_ਗੰਢ_DType* nodep) {
        // Return true iff this datatype or child has an openarray
        ਜੇ (VN_IS(nodep, UnsizedArrayDType)) ਵਾਪਸ ਸੱਚ;
        ਜੇ (nodep->subDTypep()) ਵਾਪਸ hasOpenArrayDTypeRecurse(nodep->subDTypep()->skipRefp());
        ਵਾਪਸ ਝੂਠ;
    }
    Ast_ਗੰਢ_DType* dtypeNotIntAtomOrVecRecurse(Ast_ਗੰਢ_DType* nodep, ਤਾਰਕਿਕ ranged = ਝੂਠ) {
        // If node is _not_ integer or atomic, return node that makes it fail
        nodep = nodep->skip_Ref_ਮੰਜ਼ਿਲ_Enump();
        ਜੇ (AstBasicDType* ਅਚਲ dtp = VN_CAST(nodep, BasicDType)) {
            ਜੇ (ranged && (!dtp->is_ਦੋਆਧਾਰੀ_ਅੰਕ_Logic() || dtp->isRanged()))
                ਵਾਪਸ dtp;  // Packed when already packed
            ਜੇ (dtp->keyword().isIntNumeric()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            ਵਾਪਸ dtp;
        } ਨਹੀਂਤਾਂ ਜੇ (AstPackArrayDType* ਅਚਲ dtp = VN_CAST(nodep, PackArrayDType)) {
            ਜੇ (ranged) ਵਾਪਸ dtp;  // Packed when already packed
            ਵਾਪਸ dtypeNotIntAtomOrVecRecurse(nodep->subDTypep(), ਸੱਚ);
        }
        ਵਾਪਸ nodep;
    }
    Ast_ਗੰਢ_DType* dtypeNot4StateIntegralRecurse(Ast_ਗੰਢ_DType* nodep) {
        // If node is _not_ inet valid data type, 4-state integral packed or union, return node
        // that makes it fail
        nodep = nodep->skipRefp();
        ਜੇ (AstBasicDType* ਅਚਲ dtp = VN_CAST(nodep, BasicDType)) {
            ਜੇ (!dtp->keyword().isFourstate()) ਵਾਪਸ dtp;
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Array_DType* ਅਚਲ dtp = VN_CAST(nodep, ਗੰਢ_Array_DType)) {
            ਵਾਪਸ dtypeNot4StateIntegralRecurse(dtp->subDTypep());
        } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_UOr_Struct_DType* ਅਚਲ dtp = VN_CAST(nodep, ਗੰਢ_UOr_Struct_DType)) {
            ਲਈ (AstMemberDType* itemp = dtp->membersp(); itemp;
                 itemp = VN_AS(itemp->nextp(), MemberDType)) {
                Ast_ਗੰਢ_DType* ਅਚਲ badDtp = dtypeNot4StateIntegralRecurse(itemp->dtypep());
                ਜੇ (badDtp) ਵਾਪਸ badDtp;
            }
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        ਵਾਪਸ nodep;
    }
    ਖਾਲੀ checkForceReleaseLhs(Ast_ਗੰਢ* nodep, Ast_ਗੰਢ* lhsp) {
        // V3Force can't check as vector may have expanded, or propagated constant into index
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ selNodep = V3_ਚੌੜਾਈ::selectNonConstantRecurse(lhsp))
            nodep->v3error((VN_IS(nodep, Release) ? "Release"s : "Force"s)
                               + " left-hand-side must not have variable bit/part select "
                                 "(IEEE 1800-2023 10.6.2)\n"
                           << nodep->warnContextPrimary() << '\n'
                           << selNodep->warnOther() << "... Location of non-constant index\n"
                           << selNodep->warnContextSecondary());
    }

    //----------------------------------------------------------------------
    // METHODS - special type detection

    ਖਾਲੀ assert_At_ਵਿਅੰਜਕ(Ast_ਗੰਢ* nodep) {
        ਜੇ (VL_UNCOVERABLE(!m_vup)) {
            nodep->v3fatalSrc("Unexpected '" << nodep->prettyTypeName() << "' expression under '"
                                             << nodep->backp()->prettyTypeName() << "'");
        }
    }
    ਖਾਲੀ assertAtStatement(Ast_ਗੰਢ* nodep) {
        ਜੇ (VL_UNCOVERABLE(m_vup && !m_vup->selfDtm())) {
            UINFO(1, "-: " << m_vup);
            nodep->v3fatalSrc("No dtype expected at statement " << nodep->prettyTypeName());
        }
    }
    ਖਾਲੀ checkConstantOrReplace(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ noFourState, ਅਚਲ string& ਸੁਨੇਹਾ) {
        // See also V3WidthSel::checkConstantOrReplace
        // Note can't call V3Const::constifyParam(nodep) here, as constify may change nodep on us!
        AstConst* ਅਚਲ constp = VN_CAST(nodep, Const);
        ਜੇ (!constp || (noFourState && constp->ਅੰਕ().isFourState())) {
            nodep->v3error(ਸੁਨੇਹਾ);
            nodep->replaceWith(ਨਵਾਂ AstConst{nodep->fileline(), AstConst::Unsized32{}, 1});
            VL_DO_DANGLING2(ਧੱਕੋ_Deletep(nodep), nodep, constp);
            ਵਾਪਸ;
        }
    }
    Ast_ਗੰਢ* ਗੰਢ_For_Unsized_Warning(Ast_ਗੰਢ* nodep) {
        // Return a nodep to use for unsized warnings, reporting on child if can
        ਜੇ (nodep->op1p() && nodep->op1p()->dtypep() && !nodep->op1p()->dtypep()->ਚੌੜਾਈ_Sized()) {
            ਵਾਪਸ nodep->op1p();
        } ਨਹੀਂਤਾਂ ਜੇ (nodep->op2p() && nodep->op2p()->dtypep()
                   && !nodep->op2p()->dtypep()->ਚੌੜਾਈ_Sized()) {
            ਵਾਪਸ nodep->op2p();
        }
        ਵਾਪਸ nodep;  // By default return this
    }
    AstRefDType* check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(Ast_ਗੰਢ* nodep, AstTypedef* typedefp) {
        // Recurse all children looking for self reference
        // This avoids iterateEditMoveDTypep going into a hard to resolve loop
        // Only call once for any given typedef, or will become O(n^2)
        ਜੇ (VL_LIKELY(!nodep)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = VN_CAST(nodep, RefDType)) {
            ਜੇ (refp->typedefp() == typedefp) ਵਾਪਸ refp;
        }
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(nodep->op1p(), typedefp)) ਵਾਪਸ refp;
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(nodep->op2p(), typedefp)) ਵਾਪਸ refp;
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(nodep->op3p(), typedefp)) ਵਾਪਸ refp;
        ਜੇ (ਸਵੈਚਾਲਿਤ* ਅਚਲ refp = check_Ref_ਮੰਜ਼ਿਲ_Typedef_Recurse(nodep->op4p(), typedefp)) ਵਾਪਸ refp;
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    //----------------------------------------------------------------------
    // METHODS - interface arrays

    // True if an unpacked array of non-virtual interfaces
    ਸਥਿਰ ਤਾਰਕਿਕ isIfaceArrayDType(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਜੇ (!VN_IS(dtypep->skipRefp(), UnpackArrayDType)) ਵਾਪਸ ਝੂਠ;
        ਅਚਲ AstIfaceRefDType* ਅਚਲ irefp = VN_CAST(dtypep->elemDTypep(), IfaceRefDType);
        ਵਾਪਸ irefp && !irefp->isVirtual();
    }

    // Number of elements of each unpacked dimension of 'dtypep', from the left, e.g. "2x3"
    ਸਥਿਰ std::string unpacked_Array_ਮਾਪ(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        std::string ਮਾਪ;
        dtypep = dtypep->skipRefp();
        ਜਦੋਂ (ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(dtypep, UnpackArrayDType)) {
            ਜੇ (!ਮਾਪ.ਖਾਲੀ_ਥਾਵਾਂ()) ਮਾਪ += "x";
            ਮਾਪ += std::ਮੰਜ਼ਿਲ_string(arrp->elementsConst());
            dtypep = arrp->subDTypep()->skipRefp();
        }
        ਵਾਪਸ ਮਾਪ;
    }

    // A whole array of interfaces is an AstConsArray of references to its elements, so select
    // its element 'bit' (counted from lo) directly. Returns nullptr if not such an array, or
    // 'bit' is out of range.
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* newIfaceArrayElement(Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        AstConsArray* ਅਚਲ consp = VN_CAST(fromp, ConsArray);
        ਜੇ (!consp || !isIfaceArrayDType(consp->dtypep())) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp
            = VN_AS(consp->dtypep()->skipRefp(), UnpackArrayDType);
        ਜੇ (ਦੋਆਧਾਰੀ_ਅੰਕ_2 < 0 || ਦੋਆਧਾਰੀ_ਅੰਕ_2 >= arrp->elementsConst()) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਵਾਪਸ consp->valuep(ਦੋਆਧਾਰੀ_ਅੰਕ_2)->clone_ਰੁੱਖ(ਝੂਠ);
    }

    // Widthed references to the element variables of the whole interface array referenced by
    // 'refp', in the dimensions of 'dtypep' inwards. 'name' is the element name so far, the
    // element variables being named 'name' + 'suffix'. 'elemVarpr' is the element variable of
    // the leftmost element in the dimensions of 'dtypep', and is moved to the right past them.
    // E.g. 'ifc' of dimensions [0:1][3:2] -> '{'{ifc[0][3], ifc[0][2]}, '{ifc[1][3], ifc[1][2]}}
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Iface_Array_ਮੁੱਲ(ਅਚਲ Ast_ਗੰਢ_Var_Ref* refp, Ast_ਗੰਢ_DType* dtypep,
                                    ਅਚਲ std::string& name, ਅਚਲ std::string& suffix,
                                    AstVar*& elemVarpr) {
        File_ਸਤਰ* ਅਚਲ flp = refp->fileline();
        AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: reference the element variable when no dimensions left
        ਜੇ (!arrp) {
            AstVar* ਅਚਲ varp = elemVarpr;
            UASSERT_OBJ(varp && varp->name() == name + suffix, refp,
                        "Missing element variable of interface array " << name);
            elemVarpr = VN_CAST(varp->nextp(), Var);
            Ast_ਗੰਢ_Var_Ref* newp;
            ਜੇ (ਅਚਲ AstVarXRef* ਅਚਲ xrefp = VN_CAST(refp, VarXRef)) {
                newp = ਨਵਾਂ AstVarXRef{flp, varp, xrefp->dotted(), refp->access()};
            } ਨਹੀਂਤਾਂ {
                newp = ਨਵਾਂ AstVarRef{flp, varp, refp->access()};
            }
            userIterate(newp, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
            ਵਾਪਸ newp;
        }

        // Enumerate the current dimension given by 'arrp', from the left index to the right,
        // as the element variables are in that order, indexed from lo
        AstConsArray* ਅਚਲ consp = ਨਵਾਂ AstConsArray{flp, arrp};
        ਅਚਲ VNumRange range = arrp->declRange();
        ਲਈ (ਅਚਲ ਪੂਰਨ i : range.seq_ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ()) {
            ਅਚਲ std::string s = name + "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(i) + "__KET__";
            consp->valuep(i - range.lo(),
                          new_Iface_Array_ਮੁੱਲ(refp, arrp->subDTypep(), s, suffix, elemVarpr));
        }
        ਵਾਪਸ consp;
    }

    // Replace a reference to a whole interface array variable (a __Viftop companion, or an
    // interface array port) split by V3Param, with the AstConsArray of references to its
    // elements, which is how whole arrays of interfaces are handled from here on
    ਖਾਲੀ replace_ਵੰਡੋ_Iface_Array_Ref(Ast_ਗੰਢ_Var_Ref* nodep) {
        AstVar* ਅਚਲ varp = nodep->varp();
        UASSERT_OBJ(varp->is_Iface_Array_ਵੰਡੋ(), nodep, "Not a split interface array");
        std::string name = varp->name();
        std::string suffix;
        ਜੇ (varp->is_Iface_ਮਾਪਾ()) {
            name = name.substr(0, name.rfind("__Viftop"));
            suffix = "__Viftop";
        }
        // The element variables follow the whole array variable in order (see V3Param)
        AstVar* elemVarp = VN_CAST(varp->nextp(), Var);
        nodep->replaceWith(new_Iface_Array_ਮੁੱਲ(nodep, varp->dtypep(), name, suffix, elemVarp));
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    // Add a pin for each element port of interface array port of pin 'nodep', split by V3Param,
    // in the dimensions of 'dtypep' inwards, connected to the element at the same position,
    // counted from the left, of 'conp', which is the corresponding (sub)array of the pin's
    // connection, if any. 'suffix' is the name suffix of the dimensions outside 'dtypep'.
    // 'elemVarpr' is the element port variable of the rightmost element in the dimensions of
    // 'dtypep', and is moved to the left past them.
    ਖਾਲੀ expandIfaceArrayPinDimensions(AstPin* nodep, ਅਚਲ Ast_ਗੰਢ_DType* dtypep,
                                       Ast_ਗੰਢ_ਵਿਅੰਜਕ* conp, ਅਚਲ std::string& suffix,
                                       AstVar*& elemVarpr) {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ arrp = VN_CAST(dtypep->skipRefp(), UnpackArrayDType);
        // Base case: add the pin of the element port after 'nodep' when no dimensions left
        ਜੇ (!arrp) {
            UASSERT_OBJ(elemVarpr && elemVarpr->name() == nodep->modVarp()->name() + suffix, nodep,
                        "Missing element variable of interface array port " << suffix);
            AstPin* ਅਚਲ newp = nodep->clone_ਰੁੱਖ(ਝੂਠ);
            newp->name(nodep->name() + suffix);
            newp->modVarp(elemVarpr);
            elemVarpr = VN_CAST(elemVarpr->backp(), Var);
            ਜੇ (conp) newp->exprp(conp->clone_ਰੁੱਖ(ਝੂਠ));
            nodep->ਜੋੜੋ_ਅਗਲਾ_ਇੱਥੇ(newp);
            ਵਾਪਸ;
        }

        // Enumerate the current dimension given by 'arrp'
        // Each element is added right after 'nodep', so go from right to left,
        // to end with an enumeration from the left index to the right index.
        ਅਚਲ VNumRange range = arrp->declRange();
        ਲਈ (ਪੂਰਨ n = range.elements() - 1; n >= 0; --n) {
            ਅਚਲ ਪੂਰਨ i = range.ਖੱਬੇ() + n * range.ਖੱਬਾ_ਮੰਜ਼ਿਲ_ਸੱਜਾ_Inc();
            ਅਚਲ std::string s = suffix + "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(i) + "__KET__";
            // Element of the connection, if any, which is only cloned at the leaves
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ elemConnp = [&]() -> Ast_ਗੰਢ_ਵਿਅੰਜਕ* {
                ਜੇ (!conp) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                ਅਚਲ AstUnpackArrayDType* ਅਚਲ conArrp
                    = VN_AS(conp->dtypep()->skipRefp(), UnpackArrayDType);
                ਅਚਲ ਪੂਰਨ elements = conArrp->elementsConst();
                // Counted from lo
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ resultp
                    = VN_AS(conp, ConsArray)
                          ->valuep(conArrp->declRange().ascending() ? n : elements - 1 - n);
                UASSERT_OBJ(resultp, conp, "Interface array connection of mismatched shape");
                ਵਾਪਸ resultp;
            }();
            expandIfaceArrayPinDimensions(nodep, arrp->subDTypep(), elemConnp, s, elemVarpr);
        }
    }

    // Replace the given interface array port pin with a pin for each element port split by V3Param
    ਖਾਲੀ expandIfaceArrayPin(AstPin* nodep) {
        AstVar* ਅਚਲ portp = nodep->modVarp();
        UASSERT_OBJ(portp->is_Iface_Array_ਵੰਡੋ(), nodep, "Not a split interface array port");
        // The element variables follow the port in order (see V3Param). Pins are added from the
        // right, so start at the last.
        AstVar* elemVarp = portp;
        ਲਈ (uint32_t n = portp->dtypep()->arrayUnpackedElements(); n; --n) {
            elemVarp = VN_CAST(elemVarp->nextp(), Var);
            UASSERT_OBJ(elemVarp, portp, "Missing element variables of interface array port");
        }
        // Remove the connection, so the element pins do not clone it all. Deletion is deferred,
        // so its elements can still be cloned from.
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = VN_AS(nodep->exprp(), ਗੰਢ_ਵਿਅੰਜਕ);
        ਜੇ (exprp) ਧੱਕੋ_Deletep(exprp->unlink_Fr_ਪਿੱਠ());
        // Connect nothing if not a whole array of interfaces of the shape of the port, which is
        // reported elsewhere, so the element pins do not report it again
        ਅਚਲ ਤਾਰਕਿਕ connect
            = exprp && isIfaceArrayDType(exprp->dtypep())
              && unpacked_Array_ਮਾਪ(exprp->dtypep()) == unpacked_Array_ਮਾਪ(portp->dtypep());
        expandIfaceArrayPinDimensions(nodep, portp->dtypep(), connect ? exprp : ਖਾਲੀ_ਸੂਚਕ, "",
                                      elemVarp);
        UASSERT_OBJ(elemVarp == portp, portp, "Mismatched element port variable");
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep->unlink_Fr_ਪਿੱਠ()), nodep);
    }

    //----------------------------------------------------------------------
    // METHODS - special iterators
    // These functions save/restore the AstNUser information so it can pass to child nodes.

    Ast_ਗੰਢ* userIterateSubtreeReturnEdits(Ast_ਗੰਢ* nodep, ਚੌੜਾਈ_VP* vup) {
        ਜੇ (!nodep) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        Ast_ਗੰਢ* ret;
        {
            VL_RESTORER(m_vup);
            m_vup = vup;
            ret = iterateSubtreeReturnEdits(nodep);
        }
        ਵਾਪਸ ret;
    }
    ਖਾਲੀ userIterate(Ast_ਗੰਢ* nodep, ਚੌੜਾਈ_VP* vup) {
        ਜੇ (!nodep) ਵਾਪਸ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate(nodep);
    }
    ਖਾਲੀ user_Iterate_And_ਅਗਲਾ(Ast_ਗੰਢ* nodep, ਚੌੜਾਈ_VP* vup) {
        ਜੇ (!nodep) ਵਾਪਸ;
        ਜੇ (nodep->did_ਚੌੜਾਈ()) ਵਾਪਸ;  // Avoid iterating list we have already iterated
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterate_And_ਅਗਲਾ_Null(nodep);
    }
    ਖਾਲੀ userIterateChildren(Ast_ਗੰਢ* nodep, ਚੌੜਾਈ_VP* vup) {
        ਜੇ (!nodep) ਵਾਪਸ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildren(nodep);
    }
    ਖਾਲੀ userIterateChildrenBackwardsConst(Ast_ਗੰਢ* nodep, ਚੌੜਾਈ_VP* vup) {
        ਜੇ (!nodep) ਵਾਪਸ;
        VL_RESTORER(m_vup);
        m_vup = vup;
        iterateChildrenBackwardsConst(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਚੌੜਾਈ_Visitor(ਤਾਰਕਿਕ paramsOnly,  // [in] TRUE if we are considering parameters only.
                 ਤਾਰਕਿਕ doGenerate)  // [in] TRUE if we are inside a generate statement and
        //                           // don't wish to trigger errors
        : m_enum_ਆਰਜ਼ੀ_ਨਾਂ{"__Venum"}
        , m_inside_ਆਰਜ਼ੀ_ਨਾਂ{"__VInside"}
        , m_paramsOnly{paramsOnly}
        , m_doGenerate{doGenerate} {}
    Ast_ਗੰਢ* mainAcceptEdit(Ast_ਗੰਢ* nodep) {
        nodep = userIterateSubtreeReturnEdits(nodep, ਚੌੜਾਈ_VP{SELF, BOTH}.p());
        // All references to the whole interface array variables split by V3Param are now replaced
        ਲਈ (AstVar* ਅਚਲ varp : m_ਵੰਡੋ_Iface_Array_Varps) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(varp->unlink_Fr_ਪਿੱਠ()), varp);
        }
        m_ਵੰਡੋ_Iface_Array_Varps.clear();
        ਵਾਪਸ nodep;
    }
    ਸਥਿਰ ਤਾਰਕਿਕ ਹੇਠਲਾ_As_Fixed_Aggregate(ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep) {
        ਵਾਪਸ dtypep->isStreamableFixedAggregate() && dtypep->ਸ਼ਾਮਲ_ਹੈ_Unpacked_Struct();
    }
    ਖਾਲੀ ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        Ast_ਗੰਢ_DType* lhsDTypep = nodep->lhsp()->dtypep();
        // Check width of stream and wrap if needed
        ਜੇ (Ast_ਗੰਢ_ਧਾਰਾ* ਅਚਲ streamp = VN_CAST(nodep->rhsp(), ਗੰਢ_ਧਾਰਾ)) {
            Ast_ਗੰਢ_DType* ਅਚਲ lhsDTypeSkippedRefp = lhsDTypep->skipRefp();
            ਅਚਲ ਪੂਰਨ lwidth = lhsDTypeSkippedRefp->ਚੌੜਾਈ_ਧਾਰਾ();
            ਅਚਲ ਪੂਰਨ rwidth = streamp->lhsp()->dtypep()->skipRefp()->ਚੌੜਾਈ_ਧਾਰਾ();
            ਜੇ (lwidth != 0 && lwidth < rwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Target fixed size variable ("
                                       << lwidth << " bits) is narrower than the stream ("
                                       << rwidth << " bits) (IEEE 1800-2023 11.4.14)");
            }
            ਜੇ (VN_IS(streamp->lhsp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)
                && !VN_IS(nodep->lhsp()->dtypep()->skipRefp(), ਕਤਾਰ_DType)) {
                ਅਚਲ ਪੂਰਨ ਕਤਾਰ_Element_ਮਾਪ = streamp->lhsp()->dtypep()->subDTypep()->ਚੌੜਾਈ();
                UASSERT_OBJ(ਕਤਾਰ_Element_ਮਾਪ <= lwidth, nodep, "LHS < RHS");
            }
            ਜੇ (VN_IS(lhsDTypeSkippedRefp, UnpackArrayDType)
                || ਹੇਠਲਾ_As_Fixed_Aggregate(lhsDTypeSkippedRefp)) {
                streamp->unlink_Fr_ਪਿੱਠ();
                nodep->rhsp(
                    ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{streamp->fileline(), streamp, lhsDTypeSkippedRefp});
            }
        }
        ਜੇ (Ast_ਗੰਢ_ਧਾਰਾ* ਅਚਲ streamp = VN_CAST(nodep->lhsp(), ਗੰਢ_ਧਾਰਾ)) {
            ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ rhsDTypep = nodep->rhsp()->dtypep()->skipRefp();
            Ast_ਗੰਢ_DType* ਅਚਲ lhs_ਧਾਰਾ_DTypep = streamp->lhsp()->dtypep()->skipRefp();
            ਅਚਲ ਪੂਰਨ lwidth = lhs_ਧਾਰਾ_DTypep->ਚੌੜਾਈ_ਧਾਰਾ();
            ਅਚਲ ਪੂਰਨ rwidth = rhsDTypep->ਚੌੜਾਈ_ਧਾਰਾ();
            ਜੇ (rwidth != 0 && rwidth < lwidth) {
                nodep->v3widthWarn(lwidth, rwidth,
                                   "Stream target requires "
                                       << lwidth << " bits, but source expression only provides "
                                       << rwidth << " bits (IEEE 1800-2023 11.4.14.3)");
            }
            ਜੇ (ਹੇਠਲਾ_As_Fixed_Aggregate(lhs_ਧਾਰਾ_DTypep)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ ਧਾਰਾ_Exprp = nodep->lhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ dstp = streamp->lhsp()->unlink_Fr_ਪਿੱਠ();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* srcp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                ਜੇ (VN_IS(streamp, ਧਾਰਾ_L)) {
                    streamp->lhsp(srcp);
                    streamp->dtype_ਸੈੱਟ_ਦੋਆਧਾਰੀ_ਅੰਕ_Or_Logic_Unsized(
                        srcp->ਚੌੜਾਈ(), srcp->ਚੌੜਾਈ_Min(), VSigning::UNSIGNED,
                        Ast_ਗੰਢ_ਵਿਅੰਜਕ::isFourstateOrNoDtype(ਧਾਰਾ_Exprp, srcp));
                    srcp = ਧਾਰਾ_Exprp;
                } ਨਹੀਂਤਾਂ {
                    ਜੇ (srcp->ਚੌੜਾਈ() > lwidth) {
                        srcp = ਨਵਾਂ AstSel{streamp->fileline(), srcp, srcp->ਚੌੜਾਈ() - lwidth,
                                          lwidth};
                    }
                    VL_DO_DANGLING(ਧੱਕੋ_Deletep(ਧਾਰਾ_Exprp), ਧਾਰਾ_Exprp);
                }
                nodep->lhsp(dstp);
                nodep->rhsp(ਨਵਾਂ Ast_Cvt_Packed_ਮੰਜ਼ਿਲ_Array{srcp->fileline(), srcp, lhs_ਧਾਰਾ_DTypep});
                nodep->dtypeFrom(dstp);
            } ਨਹੀਂਤਾਂ ਜੇ (VN_IS(rhsDTypep, UnpackArrayDType)) {
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp()->unlink_Fr_ਪਿੱਠ();
                nodep->rhsp(ਨਵਾਂ Ast_Cvt_Array_ਮੰਜ਼ਿਲ_Packed{rhsp->fileline(), rhsp, streamp->dtypep()});
            }
        }
    }
    ~ਚੌੜਾਈ_Visitor() override = ਮੂਲ;
};

//######################################################################
// Width class functions

ਖਾਲੀ V3_ਚੌੜਾਈ::ਚੌੜਾਈ(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        // We should do it in bottom-up module order, but it works in any order.
        ਅਚਲ ਚੌੜਾਈ_Clear_Visitor cvisitor{nodep};
        // Typedefs and classes are named by the scopes declaring them, so index those
        ਅਚਲ VDTypeNameScopes scopes{nodep};
        ਚੌੜਾਈ_Visitor visitor{ਝੂਠ, ਝੂਠ};
        (ਖਾਲੀ)visitor.mainAcceptEdit(nodep);
        ਚੌੜਾਈ_Remove_Visitor rvisitor;
        (ਖਾਲੀ)rvisitor.mainAcceptEdit(nodep);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("width", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
//! Single node parameter propagation
//! Smaller step... Only do a single node for parameter propagation
Ast_ਗੰਢ* V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Params_Edit(Ast_ਗੰਢ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    ਚੌੜਾਈ_Visitor visitor{ਸੱਚ, ਝੂਠ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ਵਾਪਸ nodep;
}

ਖਾਲੀ V3_ਚੌੜਾਈ::ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ_Edit(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
    UINFO(4, __FUNCTION__ << ": " << nodep);
    ਚੌੜਾਈ_Visitor visitor{ਝੂਠ, ਝੂਠ};
    visitor.ਧਾਰਾ_ਸੌਂਪੋ_ਹੇਠਲਾ(nodep);
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
Ast_ਗੰਢ* V3_ਚੌੜਾਈ::ਚੌੜਾਈ_Generate_Params_Edit(
    Ast_ਗੰਢ* nodep) {  //!< [in] AST whose parameters widths are to be analyzed.
    UINFO(4, __FUNCTION__ << ": " << nodep);
    // We should do it in bottom-up module order, but it works in any order.
    ਚੌੜਾਈ_Visitor visitor{ਸੱਚ, ਸੱਚ};
    nodep = visitor.mainAcceptEdit(nodep);
    // No WidthRemoveVisitor, as don't want to drop $signed etc inside gen blocks
    ਵਾਪਸ nodep;
}

Ast_ਗੰਢ* V3_ਚੌੜਾਈ::selectNonConstantRecurse(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ ਵਿੱਚ_Sel) {
    // If node has a non-constant select, return that select
    Ast_ਗੰਢ* resultp = ਖਾਲੀ_ਸੂਚਕ;
    ਜੇ (Ast_ਗੰਢ_Sel* ਅਚਲ anodep = VN_CAST(nodep, ਗੰਢ_Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), ਵਿੱਚ_Sel);
        ਜੇ (resultp) ਵਾਪਸ resultp;
        resultp = selectNonConstantRecurse(anodep->bitp(), ਸੱਚ);
    } ਨਹੀਂਤਾਂ ਜੇ (AstSel* ਅਚਲ anodep = VN_CAST(nodep, Sel)) {
        resultp = selectNonConstantRecurse(anodep->fromp(), ਵਿੱਚ_Sel);
        ਜੇ (resultp) ਵਾਪਸ resultp;
        resultp = selectNonConstantRecurse(anodep->lsbp(), ਸੱਚ);
    } ਨਹੀਂਤਾਂ ਜੇ (Ast_ਗੰਢ_Var_Ref* ਅਚਲ anodep = VN_CAST(nodep, ਗੰਢ_Var_Ref)) {
        ਜੇ (ਵਿੱਚ_Sel && !anodep->varp()->isParam() && !anodep->varp()->isGenVar()) ਵਾਪਸ anodep;
    } ਨਹੀਂਤਾਂ {
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op1p()) resultp = selectNonConstantRecurse(refp, ਵਿੱਚ_Sel);
        ਜੇ (resultp) ਵਾਪਸ resultp;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op2p()) resultp = selectNonConstantRecurse(refp, ਵਿੱਚ_Sel);
        ਜੇ (resultp) ਵਾਪਸ resultp;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op3p()) resultp = selectNonConstantRecurse(refp, ਵਿੱਚ_Sel);
        ਜੇ (resultp) ਵਾਪਸ resultp;
        ਜੇ (Ast_ਗੰਢ* ਅਚਲ refp = nodep->op4p()) resultp = selectNonConstantRecurse(refp, ਵਿੱਚ_Sel);
    }
    ਵਾਪਸ resultp;
}
