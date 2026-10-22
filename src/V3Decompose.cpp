// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Split arrays and structs into separate variables
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
// V3Decompose replaces variables of aggregate type (unpacked array, unpacked
// struct, multidimensional packed array, or packed struct) with one variable
// per element or member, called a component. This avoids false combinational
// loops (UNOPTFLAT) through the parts of a variable, and lets downstream passes
// optimize each component separately. Note specifically that one dimensional
// packed arrays (vectors, e.g. logic [31:0]) are not split into individual
// bits by this pass.
//
// Components that are themselves aggregates can be split further. The pass
// decides for the variable, and for each of its components at any depth,
// whether to split it.
//
// The pass proceeds in three steps:
//   - DecomposeRecord walks the netlist and records necessary information about
//     accesses to aggregate variables, without changing the tree.
//   - DecomposeDecision decides which variables and components to split, in one
//     iterative analysis of the data structures recorded by DecomposeRecord.
//   - DecomposeRewrite replaces references to, and expands assignments between
//     split variables and components, in a single rewrite across all constructs
//     involved.
// This structure ensures there is only a single traversal of the netlist, the
// complete splitting decision is made globally, independent of iteration order,
// and the rewriting is done after all decisions are made, so there is no need
// to re-analyze, or keep track of stale references anywhere.
//
// The core concept and data structure that enables the decomposition of the
// three steps is the 'place'. A 'place' represents a possible storage location:
// - either an original variable,
// - or one of its components, recursively.
// 'places' form a tree, with the original variable as the root. For example,
// the tree of 'places' involved for the aggregate variable 's' are:
//
//   struct packed {
//     logic [1:0][3:0] m;
//     logic [2:0] f;
//   } s [2];
//
//   s                      'place' for the unpacked array of 2 packed structs
//   +-- s[0]               'place' for element 0 of 's', a packed struct
//   |   +-- s[0].m         'place' for member 'm' of 's[0]', a packed array of 2 elements
//   |   |   +-- s[0].m[0]  'place' logic [3:0], a non splittable leaf
//   |   |   +-- s[0].m[1]  'place' logic [3:0], a non splittable leaf
//   |   +-- s[0].f         'place' logic [2:0], a non splittable leaf
//   +-- s[1]               Same subtree structure as 's[0]'
//       +-- s[1].m
//       |   +-- s[1].m[0]
//       |   +-- s[1].m[1]
//       +-- s[1].f
//
// A place is automatically split if it can be, and it wants to be.
// It can be split if all of the following are true:
//   - it is an aggregate variable
//   - the original variable is eligible for splitting (not public, etc.)
//   - its parent is split (if it is a component)
//   - it is not blocked, by being referenced other than in splittable constructs
// It wants to be split if any of the following are true:
//   - one of its components is addressed by a constant select
//   - the original variable is marked with split_var
//   - a copy assignment between it and another split place exists
// Note specifically that a reference outside a constant indexed select, or
// a copy assignment will block splitting, so effectively the variable is split
// mostly if it is only accessed via constant indexed selects, and it's only
// assigned whole (recursively for its components).
//
// E.g. this will split 'arr' into 'arr[0]' and 'arr[1]':
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[1];  // All indices are constant, so 'arr' is split
//
// This will not split 'arr', as it is indexed with a non-constant index:
//
//   logic [6:0] arr [2];
//   assign arr[0] = in;
//   assign arr[1] = arr[0] + 7'd1;
//   assign out = arr[i];  // 'i' is not a constant, so 'arr' is not split
//
// Copy assignments propagate splitting. An assignment between two places is
// recorded as a copy. When a place is split, its copies are split at the
// boundaries of its components. For example:
//
//   typedef struct packed {
//     logic [3:0] hi;
//     logic [3:0] lo;
//   } pair_t;
//   pair_t a, b;
//   assign b = {a.lo + 4'd1, in};
//   assign a = b;
//   assign out = a.hi;
//
// 'a' is split, as its components are selected. This splits the 'a = b' copy
// into copies between 'a.hi' and 'b[7:4]', and 'a.lo' and 'b[3:0]', each covering
// only one component of 'b', so 'b' is split too. The result is a list of
// assignments between the components of 'a' and 'b':
//
//   assign b__DOT__hi = a__DOT__lo + 4'd1;
//   assign b__DOT__lo = in;
//   assign a__DOT__hi = b__DOT__hi;
//   assign a__DOT__lo = b__DOT__lo;
//   assign out = a__DOT__hi;
//
// Which later passes can simplify into 'out = in + 4'd1', with intermediate
// variables discarded when possible.
//
//*************************************************************************

#شامل "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#شامل "V3Decompose.h"

#شامل "V3AstUserAllocator.h"
#شامل "V3MemberMap.h"
#شامل "V3SharedTmps.h"
#شامل "V3Stats.h"

#شامل <algorithm>
#شامل <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

جماعت Decompose_بنیاد VL_NOT_FINAL {
محفوظ:
    // TYPES

    // A component (element or member) of an aggregate type
    ساخت Component final {
        Ast_گرہ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        صحیح declIdx;  // The declared index of the element, if an array
        صحیح lsb;  // LSB in the whole value, if packed
        صحیح msb;  // MSB in the whole value, if packed
    };

    ساخت رکھو;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ساخت Copy final {
        رکھو* otherp;  // The Place at the other end
        // Following only for packed places
        صحیح lsb;  // The first bit covered of this end
        صحیح otherLsb;  // The first bit covered of the other end
        صحیح چوڑائی;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ساخت رکھو final {
        رکھو* parentp = خالی_اشاریہ;  // The Place this is a component of, nullptr if original
        رکھو* rootp = خالی_اشاریہ;  // The original variable (root of the tree), root points to itself
        Ast_گرہ_DType* dtypep = خالی_اشاریہ;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = خالی_اشاریہ;
        // The component Places created so far, by index, sized on the first one created
        std::سمتیہ<std::unique_ptr<رکھو>> childrenp;
        std::سمتیہ<Copy> copies;  // The copies covering it, until split
        منطقی splittable = سچ;  // Can be split (there is no reason not to)
        منطقی چاہتا_بانٹو = جھوٹ;  // Should be split (if possible, do split)
        منطقی بانٹو = جھوٹ;  // Place was split
        منطقی queued = جھوٹ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ساخت Select final {
        Ast_گرہ_اظہاریہ* exprp;  // The select
        رکھو* placep;  // The component
        صحیح lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ساخت Assignment final {
        Ast_گرہ_تفویض* assp;  // The assignment
        رکھو* lPlacep;  // The Place the LHS addresses exactly, if any
        رکھو* rPlacep;  // The Place the RHS addresses exactly, if any
    };

عوامی:
    // The state of the pass, shared by the steps
    جماعت State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ثابت VNUser4_اندر_Use m_user4_اندر_Use;

    عوامی:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_گرہ_DType, std::سمتیہ<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, رکھو> places;
        std::سمتیہ<رکھو*> rootps;  // The original Places, in creation order
        std::سمتیہ<std::سمتیہ<Select>> chains;  // Select chains addressing components
        std::سمتیہ<Assignment> assignments;  // Assignments to expand if a side is split
    };

محفوظ:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    صریح Decompose_بنیاد(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ساکن منطقی isUnpacked(ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (VN_IS(dtypep, UnpackArrayDType)) واپس سچ;
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        واپس structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ساکن منطقی isPacked(ثابت Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            واپس arrayp->subDTypep()->چوڑائی() > 1;
        }
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        واپس structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ساکن منطقی isSplittableType(ثابت Ast_گرہ_DType* dtypep) {
        اگر (isPacked(dtypep)) واپس v3Global.opt.fDecomposePacked();
        اگر (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            اگر (ثابت AstStructDType* ثابت structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                برائے (ثابت AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    اگر (mp->valuep()) واپس جھوٹ;
                }
            }
            واپس v3Global.opt.fDecomposeUnpacked();
        }
        واپس جھوٹ;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ساکن Ast_گرہ_DType* dtype_کا(ثابت Ast_گرہ_اظہاریہ* nodep) {
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) واپس refp->varp()->dtypep();
        واپس nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ثابت std::سمتیہ<Component>& dtypeComponents(Ast_گرہ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::سمتیہ<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        اگر (compsr.خالی_خانے()) {
            اگر (ثابت Ast_گرہ_Array_DType* ثابت arrayp = VN_CAST(dtypep, گرہ_Array_DType)) {
                ثابت منطقی packed = isPacked(dtypep);
                ثابت VNumRange range = arrayp->declRange();
                ثابت منطقی rev = packed && range.ascending();  // Only for naming components
                Ast_گرہ_DType* ثابت subp = arrayp->subDTypep();
                برائے (صحیح i = 0; i < range.elements(); ++i) {
                    ثابت صحیح lsb = packed ? i * subp->چوڑائی() : 0;
                    ثابت صحیح msb = packed ? lsb + subp->چوڑائی() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ثابت صحیح declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ڈالو_عقب({subp, خالی_اشاریہ, declIdx, lsb, msb});
                }
            } ورنہ {
                ثابت AstStructDType* ثابت structp = VN_AS(dtypep, StructDType);
                ثابت منطقی packed = isPacked(dtypep);
                برائے (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ثابت صحیح lsb = packed ? mp->lsb() : 0;
                    ثابت صحیح msb = packed ? lsb + mp->چوڑائی() - 1 : 0;
                    compsr.ڈالو_عقب({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::الٹ(compsr.begin(), compsr.انتہا());
            }
        }

        // The components of this data type
        واپس compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    سائز_t component_اشاریہ(Ast_گرہ_DType* dtypep, صحیح ثنائی_ہندسہ_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ثابت std::سمتیہ<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ثابت خودکار it = std::زیریں_bound(compsr.begin(), compsr.انتہا(), ثنائی_ہندسہ_2,
                                         [](ثابت Component& comp, صحیح b) {  //
                                             واپس b > comp.msb;
                                         });
        واپس it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    رکھو* component_کا(رکھو* placep, سائز_t idx) {
        ثابت std::سمتیہ<Component>& comps = dtypeComponents(placep->dtypep);
        اگر (placep->childrenp.خالی_خانے()) placep->childrenp.resize(comps.سائز());
        std::unique_ptr<رکھو>& childpr = placep->childrenp.at(idx);
        اگر (!childpr) {
            childpr = std::make_unique<رکھو>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->چاہتا_بانٹو = placep->rootp->vscp->varp()->attr_بانٹو_Var();
        }
        واپس childpr.get();
    }

    // Is 'nodep' cheap to clone
    ساکن منطقی isCheap(ثابت Ast_گرہ_اظہاریہ* nodep) {
        // Constants are cheap
        اگر (VN_IS(nodep, Const)) واپس سچ;
        // So are resets
        اگر (VN_IS(nodep, CReset)) واپس سچ;
        // Variable references are cheap for the most part
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            اگر (refp->varp()->isForced()) واپس جھوٹ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            اگر (refp->varp()->isSc()) واپس جھوٹ;
            // Otherwise the path rooted here is cheap
            واپس سچ;
        }
        // Bit selects only with a constant LSB
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            واپس VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        اگر (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
            واپس isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {  //
            واپس isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        اگر (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {  //
            واپس isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        واپس جھوٹ;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

جماعت Decompose_ریکارڈ final : عوامی VNVisitorConst, عوامی Decompose_بنیاد {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_گرہ_تفویض* m_assignp = خالی_اشاریہ;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساکن خالی warn_No_بانٹو(AstVar* varp, ثابت Ast_گرہ* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        اگر (!varp->attr_بانٹو_Var()) واپس;
        // A packed variable might still be split by V3BitBlast, which warns if not
        اگر (isPacked(varp->dtypep())) واپس;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_رمز::SPLITVAR, سچ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ساکن منطقی isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        اگر (!varp->user1()) {
            ثابت منطقی eligible = [&]() {
                // Check data type
                اگر (!isSplittableType(varp->dtypep())) واپس جھوٹ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_بانٹو_قسم_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                اگر (reasonp) warn_No_بانٹو(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                واپس !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        واپس varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    رکھو* رکھو_کا(ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        اگر (!isEligible(vscp->varp())) واپس خالی_اشاریہ;
        رکھو& رکھو_2 = m_state.places(vscp);
        اگر (!رکھو_2.rootp) {
            رکھو_2.rootp = &رکھو_2;
            رکھو_2.dtypep = vscp->varp()->dtypep();
            رکھو_2.vscp = vscp;
            رکھو_2.چاہتا_بانٹو = vscp->varp()->attr_بانٹو_Var();
            m_state.rootps.ڈالو_عقب(&رکھو_2);
        }
        واپس &رکھو_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ساکن خالی بلاک(رکھو* placep, ثابت Ast_گرہ* wherep) {
        placep->splittable = جھوٹ;
        // Warn on the variable only, a component is split as far as possible, as requested
        اگر (!placep->parentp) {
            warn_No_بانٹو(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    رکھو* resolveChain(Ast_گرہ_اظہاریہ* exprp, std::سمتیہ<Select>& chain) {
        // Array selects correspond one to one with a component select
        اگر (AstArraySel* ثابت selp = VN_CAST(exprp, ArraySel)) {
            اگر (رکھو* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ثابت AstConst* ثابت bitp = VN_CAST(selp->bitp(), Const);
                اگر (bitp && bitp->مقصد_UQuad() < dtypeComponents(fromp->dtypep).سائز()) {
                    رکھو* ثابت childp = component_کا(fromp, bitp->مقصد_UQuad());
                    fromp->چاہتا_بانٹو = سچ;
                    chain.ڈالو_عقب({selp, childp, 0});
                    واپس childp;
                }
                // Otherwise block splitting of the Place
                بلاک(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            واپس خالی_اشاریہ;
        }
        // Struct selects correspond one to one with a component select
        اگر (AstStructSel* ثابت selp = VN_CAST(exprp, StructSel)) {
            اگر (رکھو* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                اگر (!isUnpacked(fromp->dtypep)) واپس خالی_اشاریہ;
                AstStructDType* ثابت structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                اگر (!structp->user1_سیٹ_Once()) {
                    ثابت std::سمتیہ<Component>& compsr = dtypeComponents(structp);
                    برائے (سائز_t i = 0; i < compsr.سائز(); ++i) compsr[i].memberp->user1(i);
                }
                ثابت Ast_گرہ* ثابت memberp = m_memberMap.ڈھونڈو_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                رکھو* ثابت childp = component_کا(fromp, memberp->user1());
                fromp->چاہتا_بانٹو = سچ;
                chain.ڈالو_عقب({selp, childp, 0});
                واپس childp;
            }
            واپس خالی_اشاریہ;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        اگر (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            رکھو* placep = resolveChain(selp->fromp(), chain);
            اگر (!placep) {
                iterateConst(selp->lsbp());
                واپس خالی_اشاریہ;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            اگر (!lsbp || lsbp->مقصد_SInt() + selp->چوڑائی_Const() > placep->dtypep->چوڑائی()) {
                بلاک(placep, selp->fromp());
                iterateConst(selp->lsbp());
                واپس خالی_اشاریہ;
            }
            // Descend into the Place containing the bits, until exact
            صحیح lsb = lsbp->مقصد_SInt();
            صحیح msb = lsb + selp->چوڑائی_Const() - 1;
            جب (lsb != 0 || msb != placep->dtypep->چوڑائی() - 1) {
                // Within a component of a type not split
                اگر (!isPacked(placep->dtypep)) واپس خالی_اشاریہ;
                ثابت سائز_t idx = component_اشاریہ(placep->dtypep, lsb);
                // Crossing components prevents splitting
                اگر (idx != component_اشاریہ(placep->dtypep, msb)) {
                    بلاک(placep, selp->fromp());
                    واپس خالی_اشاریہ;
                }
                placep->چاہتا_بانٹو = سچ;
                ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_کا(placep, idx);
                chain.ڈالو_عقب({selp, placep, lsb});
            }
            واپس placep;
        }
        // Base case
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) واپس رکھو_کا(refp);
        // Not a select chain
        iterateConst(exprp);
        واپس خالی_اشاریہ;
    }

    // 'nodep' references the Place 'placep' whole
    خالی referenced_پورا(Ast_گرہ_اظہاریہ* nodep, رکھو* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        اگر (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            واپس;
        }

        // Otherwise must block splitting of this Place
        بلاک(placep, nodep);
    }

    // Visit a select expression
    خالی دورہ_Select(Ast_گرہ_اظہاریہ* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::سمتیہ<Select> chain;
        رکھو* ثابت placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        اگر (placep) referenced_پورا(nodep, placep);
        // Record the chain for the rewriting phase
        اگر (!chain.خالی_خانے()) m_state.chains.ڈالو_عقب(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ساکن منطقی hasReversedRange(ثابت Ast_گرہ_DType* aDTypep, ثابت Ast_گرہ_DType* bDTypep) {
        ثابت AstUnpackArrayDType* ثابت aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        اگر (!aArrayp || !bArrayp) واپس جھوٹ;
        اگر (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) واپس سچ;
        واپس hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    خالی دورہ_Assignment(Ast_گرہ_تفویض* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ثابت منطقی reversed = hasReversedRange(dtype_کا(nodep->lhsp()), dtype_کا(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? خالی_اشاریہ : nodep;
        iterateChildrenConst(nodep);
        اگر (!m_assignp) واپس;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_گرہ_اظہاریہ* ثابت lhsp = nodep->lhsp();
        Ast_گرہ_اظہاریہ* ثابت rhsp = nodep->rhsp();
        رکھو* ثابت lPlacep = lhsp->user1u().مقصد<رکھو*>();
        رکھو* ثابت rPlacep = rhsp->user1u().مقصد<رکھو*>();

        // Is it a splittable assignment
        ثابت منطقی splittable = [&]() {
            // Assignment between two Places is splittable
            اگر (lPlacep && rPlacep) واپس سچ;
            // If the LHS is a Place, the RHS must be a splittable expression
            اگر (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                اگر (isPacked(lPlacep->dtypep)) واپس سچ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                واپس isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            اگر (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                اگر (isPacked(rPlacep->dtypep)) واپس سچ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                واپس isCheap(lhsp);
            }
            // Otherwise not splittable
            واپس جھوٹ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        اگر (!splittable) {
            اگر (rPlacep) بلاک(rPlacep, rhsp);
            اگر (lPlacep) بلاک(lPlacep, lhsp);
            واپس;
        }

        // If both sides are places, record the whole copies for the Decision phase
        اگر (lPlacep && rPlacep) {
            ثابت صحیح چوڑائی = lPlacep->dtypep->چوڑائی();  // Unused when unpacked
            lPlacep->copies.ڈالو_عقب({rPlacep, 0, 0, چوڑائی});
            rPlacep->copies.ڈالو_عقب({lPlacep, 0, 0, چوڑائی});
        }
        // Record for the rewriting phase
        m_state.assignments.ڈالو_عقب({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    خالی دورہ(Ast_گرہ_DType*) override {}  // No references in data types
    خالی دورہ(Ast_گرہ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    خالی دورہ(AstArraySel* nodep) override { دورہ_Select(nodep); }
    خالی دورہ(AstStructSel* nodep) override { دورہ_Select(nodep); }
    خالی دورہ(AstSel* nodep) override { دورہ_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    خالی دورہ(Ast_تفویض* nodep) override { دورہ_Assignment(nodep); }
    خالی دورہ(Ast_تفویض_W* nodep) override { دورہ_Assignment(nodep); }
    خالی دورہ(Ast_تفویض_Dly* nodep) override { دورہ_Assignment(nodep); }
    // VISITORS - References to Places
    خالی دورہ(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        اگر (رکھو* ثابت placep = رکھو_کا(nodep)) referenced_پورا(nodep, placep);
    }
    خالی دورہ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ثابت varp = nodep->varp();
        اگر (!isEligible(varp)) واپس;
        varp->user1(2);  // Mark not eligible
        warn_No_بانٹو(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_ریکارڈ(State& state, AstNetlist* netlistp)
        : Decompose_بنیاد{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        برائے (رکھو* ثابت placep : m_state.rootps) {
            اگر (!isEligible(placep->vscp->varp())) placep->splittable = جھوٹ;
        }
    }

عوامی:
    ساکن خالی apply(State& state, AstNetlist* netlistp) { Decompose_ریکارڈ{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

جماعت DecomposeDecision final : عوامی Decompose_بنیاد {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    std::سمتیہ<رکھو*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::سمتیہ<AstVar*>> m_بانٹو_Varps;
    VDouble0 m_stat_بانٹو_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_بانٹو_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_بانٹو_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_بانٹو_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ساکن منطقی can_بانٹو(ثابت رکھو* placep) {
        // Not if not splittable
        اگر (!placep->splittable) واپس جھوٹ;
        // Not if already split
        اگر (placep->بانٹو) واپس جھوٹ;
        // Original variable or its parent was already split
        واپس !placep->parentp || placep->parentp->بانٹو;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    خالی قطار_میں_ڈالو(رکھو* placep) {
        اگر (placep->queued || !placep->چاہتا_بانٹو || !can_بانٹو(placep)) واپس;
        placep->queued = سچ;
        m_worklist.ڈالو_عقب(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    خالی createComponents(رکھو* placep) {
        AstVarScope* ثابت vscp = placep->vscp;
        AstVar* ثابت varp = vscp->varp();
        std::سمتیہ<AstVar*>& varps = m_بانٹو_Varps(varp);
        // Create the component AstVars on first encounter
        اگر (varps.خالی_خانے()) {
            ثابت منطقی packed = isPacked(varp->dtypep());
            اگر (placep->parentp) {
                ++(packed ? m_stat_بانٹو_Packed_Comp : m_stat_بانٹو_Unpacked_Comp);
            } ورنہ {
                ++(packed ? m_stat_بانٹو_Packed_Orig : m_stat_بانٹو_Unpacked_Orig);
            }
            File_سطر* ثابت flp = varp->fileline();
            ثابت VVarType varType = varp->varType();
            ثابت std::string name = varp->name();
            برائے (ثابت Component& comp : dtypeComponents(varp->dtypep())) {
                ثابت std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_گرہ::encode_نمبر(comp.declIdx) + "__KET__";
                AstVar* ثابت newp = نیا AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_بانٹو_Attr_From(varp);
                varps.ڈالو_عقب(newp);
                varp->جوڑو_یہاں_This_As_اگلا(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ثابت scopep = vscp->scopep();
        File_سطر* ثابت flp = vscp->fileline();
        برائے (سائز_t i = 0; i < varps.سائز(); ++i) {
            AstVarScope* ثابت newp = نیا AstVarScope{flp, scopep, varps[i]};
            component_کا(placep, i)->vscp = newp;
            vscp->جوڑو_یہاں_This_As_اگلا(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    خالی جوڑو_Copy(رکھو* placep, ثابت Copy& copy) {
        UASSERT(!placep->بانٹو, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        اگر (!placep->splittable) واپس;
        // Add the copy to the Place
        placep->copies.ڈالو_عقب(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        اگر (!isPacked(placep->dtypep)) واپس;
        اگر (copy.lsb != 0 || copy.چوڑائی != placep->dtypep->چوڑائی()) {
            placep->چاہتا_بانٹو = سچ;
            قطار_میں_ڈالو(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    خالی newCopy(رکھو* ap, صحیح aLsb, رکھو* bp, صحیح bLsb, صحیح چوڑائی) {
        UASSERT(ap != bp, "Copy within a Place");
        جوڑو_Copy(ap, {bp, aLsb, bLsb, چوڑائی});
        جوڑو_Copy(bp, {ap, bLsb, aLsb, چوڑائی});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    خالی بانٹو_Copy(رکھو* placep, ثابت Copy& copy) {
        رکھو* ثابت otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        اگر (isPacked(placep->dtypep)) {
            ثابت std::سمتیہ<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ثابت صحیح lsb = copy.lsb;
            ثابت صحیح msb = copy.lsb + copy.چوڑائی - 1;
            ثابت سائز_t پہلا = component_اشاریہ(placep->dtypep, lsb);
            برائے (سائز_t i = پہلا; i < comps.سائز() && comps[i].lsb <= msb; ++i) {
                ثابت Component& comp = comps[i];
                ثابت صحیح partLsb = std::max(lsb, comp.lsb);
                ثابت صحیح partMsb = std::min(msb, comp.msb);
                newCopy(component_کا(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            واپس;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->چاہتا_بانٹو = سچ;
        قطار_میں_ڈالو(otherp);
        // The sides are the same shape, so pairwise whole copies
        ثابت سائز_t سائز = dtypeComponents(placep->dtypep).سائز();
        برائے (سائز_t i = 0; i < سائز; ++i) {
            رکھو* ثابت ap = component_کا(placep, i);
            اگر (!ap->splittable) جاری;
            رکھو* ثابت bp = component_کا(otherp, i);
            اگر (!bp->splittable) جاری;
            newCopy(ap, 0, bp, 0, ap->dtypep->چوڑائی());
        }
    }

    // Split the Place
    خالی بانٹو_رکھو(رکھو* placep) {
        UASSERT(!placep->بانٹو, "Place split twice");
        // It is now split
        placep->بانٹو = سچ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ثابت سائز_t سائز = dtypeComponents(placep->dtypep).سائز();
        برائے (سائز_t i = 0; i < سائز; ++i) قطار_میں_ڈالو(component_کا(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        برائے (ثابت Copy& copy : placep->copies) {
            اگر (!copy.otherp->بانٹو) بانٹو_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    صریح DecomposeDecision(State& state)
        : Decompose_بنیاد{state} {
        // Enqueue the original variables
        برائے (رکھو* ثابت placep : m_state.rootps) قطار_میں_ڈالو(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        جب (!m_worklist.خالی_خانے()) {
            رکھو* ثابت placep = m_worklist.عقب();
            m_worklist.نکالو_عقب();
            placep->queued = جھوٹ;
            بانٹو_رکھو(placep);
        }

        V3Stats::جوڑو_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_بانٹو_Unpacked_Orig);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_بانٹو_Packed_Orig);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_بانٹو_Unpacked_Comp);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_بانٹو_Packed_Comp);
    }

عوامی:
    ساکن خالی apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

جماعت DecomposeRewrite final : عوامی VNDeleter, عوامی Decompose_بنیاد {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ثابت VNUser1_اندر_Use m_user1_اندر_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ساکن AstVarRef* newRef(File_سطر* flp, ثابت رکھو* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        واپس نیا AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    خالی rewriteChain(ثابت std::سمتیہ<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ثابت Select* selectp = خالی_اشاریہ;
        برائے (ثابت Select& select : chain) {
            اگر (select.placep->parentp->بانٹو) selectp = &select;
        }
        اگر (!selectp) واپس;
        Ast_گرہ_اظہاریہ* ثابت exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ثابت VAccess access = [&]() {
            ثابت Ast_گرہ_اظہاریہ* rootp = chain.front().exprp;
            جب (سچ) {
                اگر (ثابت AstArraySel* ثابت selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ورنہ اگر (ثابت AstStructSel* ثابت selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ورنہ اگر (ثابت AstSel* ثابت selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ورنہ {
                    واپس VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_سطر* ثابت flp = exprp->fileline();
        Ast_گرہ_اظہاریہ* newp = newRef(flp, selectp->placep, access);
        اگر (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ثابت صحیح چوڑائی = selp->چوڑائی_Const();
            اگر (چوڑائی != newp->چوڑائی()) newp = نیا AstSel{flp, newp, selectp->lsb, چوڑائی};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ڈالو_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_گرہ_اظہاریہ* newSelect(Ast_گرہ_اظہاریہ* fromp, Ast_گرہ_DType* dtypep, سائز_t idx) {
        File_سطر* ثابت flp = fromp->fileline();
        ثابت Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        اگر (isPacked(dtypep)) {
            AstSel* ثابت selp = نیا AstSel{flp, fromp, comp.lsb, comp.dtypep->چوڑائی()};
            selp->dtypep(comp.dtypep);
            واپس selp;
        }
        // If unpacked array, it's an ArraySel
        اگر (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ثابت صحیح i = static_cast<صحیح>(idx);
            UASSERT_OBJ(static_cast<سائز_t>(i) == idx, fromp, "Index overflow");
            واپس نیا AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ثابت selp = نیا AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        واپس selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ساکن std::سمتیہ<Ast_گرہ_اظہاریہ*> concatTerms(Ast_گرہ_اظہاریہ* nodep) {
        std::سمتیہ<Ast_گرہ_اظہاریہ*> termps;
        std::سمتیہ<Ast_گرہ_اظہاریہ*> ڈھیر{nodep};
        جب (!ڈھیر.خالی_خانے()) {
            Ast_گرہ_اظہاریہ* ثابت exprp = ڈھیر.عقب();
            ڈھیر.نکالو_عقب();
            اگر (AstConcat* ثابت concatp = VN_CAST(exprp, Concat)) {
                ڈھیر.ڈالو_عقب(concatp->lhsp());
                ڈھیر.ڈالو_عقب(concatp->rhsp());
                جاری;
            }
            اگر (AstReplicate* ثابت repp = VN_CAST(exprp, Replicate)) {
                ثابت AstConst* ثابت countp = VN_AS(repp->countp(), Const);
                برائے (uint32_t i = 0; i < countp->مقصد_UInt(); ++i) ڈھیر.ڈالو_عقب(repp->srcp());
                جاری;
            }
            termps.ڈالو_عقب(exprp);
        }
        واپس termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ساکن منطقی isSliceable(ثابت Ast_گرہ_اظہاریہ* nodep) {
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            واپس VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        اگر (ثابت AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            واپس isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        اگر (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            واپس isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        اگر (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            واپس isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        اگر (ثابت AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            واپس isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        اگر (ثابت AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            واپس isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        اگر (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            واپس isSliceable(extp->lhsp());
        }
        اگر (ثابت AstNot* ثابت notp = VN_CAST(nodep, Not)) {  //
            واپس isSliceable(notp->lhsp());
        }
        اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            واپس isSliceable(repp->srcp());
        }
        واپس isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ساکن Ast_گرہ_اظہاریہ* new_قاش(Ast_گرہ_اظہاریہ* nodep, صحیح lsb, صحیح چوڑائی) {
        File_سطر* ثابت flp = nodep->fileline();
        // A constant, as a new constant of the bits
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            V3_نمبر عدد{nodep, چوڑائی};
            عدد.opSel(constp->عدد(), lsb + چوڑائی - 1, lsb);
            واپس نیا AstConst{flp, عدد};
        }
        // A constant bit select, from the bits of what it selects from
        اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            اگر (ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                واپس new_قاش(selp->fromp(), lsbp->مقصد_SInt() + lsb, چوڑائی);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        اگر (AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            ثابت صحیح r_چوڑائی = catp->rhsp()->چوڑائی();
            ثابت صحیح msb = lsb + چوڑائی - 1;
            اگر (msb < r_چوڑائی) واپس new_قاش(catp->rhsp(), lsb, چوڑائی);
            اگر (lsb >= r_چوڑائی) واپس new_قاش(catp->lhsp(), lsb - r_چوڑائی, چوڑائی);
            واپس نیا AstConcat{flp, new_قاش(catp->lhsp(), 0, msb - r_چوڑائی + 1),
                                 new_قاش(catp->rhsp(), lsb, r_چوڑائی - lsb)};
        }
        اگر (AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            واپس نیا AstCond{flp, condp->condp()->clone_درخت_خالص(جھوٹ),
                               new_قاش(condp->thenp(), lsb, چوڑائی),
                               new_قاش(condp->elsep(), lsb, چوڑائی)};
        }
        اگر (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            واپس نیا AstAnd{flp, new_قاش(andp->lhsp(), lsb, چوڑائی),
                              new_قاش(andp->rhsp(), lsb, چوڑائی)};
        }
        اگر (AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            واپس نیا AstOr{flp, new_قاش(orp->lhsp(), lsb, چوڑائی),
                             new_قاش(orp->rhsp(), lsb, چوڑائی)};
        }
        اگر (AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            واپس نیا AstXor{flp, new_قاش(xorp->lhsp(), lsb, چوڑائی),
                              new_قاش(xorp->rhsp(), lsb, چوڑائی)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        اگر (AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            ثابت صحیح src_چوڑائی = extp->lhsp()->چوڑائی();
            ثابت صحیح msb = lsb + چوڑائی - 1;
            اگر (msb < src_چوڑائی) واپس new_قاش(extp->lhsp(), lsb, چوڑائی);
            اگر (lsb >= src_چوڑائی) واپس نیا AstConst{flp, AstConst::Widthed_قدر{}, چوڑائی, 0};
            واپس نیا AstConcat{
                flp, نیا AstConst{flp, AstConst::Widthed_قدر{}, msb - src_چوڑائی + 1, 0},
                new_قاش(extp->lhsp(), lsb, src_چوڑائی - lsb)};
        }
        اگر (AstNot* ثابت notp = VN_CAST(nodep, Not)) {
            واپس نیا AstNot{flp, new_قاش(notp->lhsp(), lsb, چوڑائی)};
        }
        // A replication, from the portions of the copies overlapping the bits
        اگر (AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            Ast_گرہ_اظہاریہ* ثابت srcp = repp->srcp();
            ثابت صحیح src_چوڑائی = srcp->چوڑائی();
            ثابت صحیح msb = lsb + چوڑائی - 1;
            Ast_گرہ_اظہاریہ* resultp = خالی_اشاریہ;
            برائے (صحیح copyLsb = lsb / src_چوڑائی * src_چوڑائی; copyLsb <= msb; copyLsb += src_چوڑائی) {
                ثابت صحیح partLsb = std::max(lsb, copyLsb) - copyLsb;
                ثابت صحیح partMsb = std::min(msb, copyLsb + src_چوڑائی - 1) - copyLsb;
                Ast_گرہ_اظہاریہ* ثابت bitsp = new_قاش(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? نیا AstConcat{flp, bitsp, resultp} : bitsp;
            }
            واپس resultp;
        }
        Ast_گرہ_اظہاریہ* ثابت clonep = nodep->clone_درخت_خالص(جھوٹ);
        اگر (lsb == 0 && چوڑائی == nodep->چوڑائی()) واپس clonep;
        واپس نیا AstSel{flp, clonep, lsb, چوڑائی};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::سمتیہ<Ast_گرہ_اظہاریہ*> new_تفویض_Rhsps(Ast_گرہ_اظہاریہ* nodep, Ast_گرہ_DType* dtypep) {
        ثابت std::سمتیہ<Component>& compsr = dtypeComponents(dtypep);
        std::سمتیہ<Ast_گرہ_اظہاریہ*> valueps;
        valueps.reserve(compsr.سائز());

        // If CReset, it is duplicated for each component
        اگر (AstCReset* ثابت cresetp = VN_CAST(nodep, CReset)) {
            برائے (ثابت Component& comp : compsr) {
                AstCReset* ثابت newp = cresetp->clone_درخت(جھوٹ);
                newp->dtypep(comp.dtypep);
                valueps.ڈالو_عقب(newp);
            }
            واپس valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        اگر (isPacked(dtypep)) {
            valueps.resize(compsr.سائز(), خالی_اشاریہ);
            صحیح lsb = 0;  // LSB of the current term
            برائے (Ast_گرہ_اظہاریہ* ثابت termp : concatTerms(nodep)) {
                ثابت صحیح msb = lsb + termp->چوڑائی() - 1;
                ثابت سائز_t پہلا = component_اشاریہ(dtypep, lsb);
                برائے (سائز_t i = پہلا; i < compsr.سائز() && compsr[i].lsb <= msb; ++i) {
                    ثابت Component& comp = compsr[i];
                    ثابت صحیح partLsb = std::max(lsb, comp.lsb);
                    ثابت صحیح partMsb = std::min(msb, comp.msb);
                    File_سطر* ثابت flp = termp->fileline();
                    Ast_گرہ_اظہاریہ* ثابت sp = new_قاش(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? نیا AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            واپس valueps;
        }

        // Otherwise unpacked, so select each component
        برائے (سائز_t i = 0; i < compsr.سائز(); ++i) {
            valueps.ڈالو_عقب(newSelect(nodep->clone_درخت(جھوٹ), dtypep, i));
        }
        واپس valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_گرہ_اظہاریہ* new_بٹ(File_سطر* flp, ثابت رکھو* placep, صحیح lsb, صحیح msb) {
        Ast_گرہ_DType* ثابت dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->بانٹو, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        اگر (!placep->بانٹو) {
            AstVarRef* ثابت refp = newRef(flp, placep, VAccess::READ);
            اگر (lsb == 0 && msb == dtypep->چوڑائی() - 1) واپس refp;
            واپس نیا AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ثابت std::سمتیہ<Component>& comps = dtypeComponents(dtypep);
        Ast_گرہ_اظہاریہ* resultp = خالی_اشاریہ;
        ثابت سائز_t پہلا = component_اشاریہ(dtypep, lsb);
        برائے (سائز_t i = پہلا; i < comps.سائز() && comps[i].lsb <= msb; ++i) {
            ثابت Component& comp = comps[i];
            ثابت صحیح partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ثابت صحیح partMsb = std::min(msb, comp.msb) - comp.lsb;
            رکھو* ثابت childp = placep->childrenp.at(i).get();
            Ast_گرہ_اظہاریہ* ثابت bitsp = new_بٹ(flp, childp, partLsb, partMsb);
            resultp = resultp ? نیا AstConcat{flp, bitsp, resultp} : bitsp;
        }
        واپس resultp;
    }

    ساکن خالی جوڑو_New_تفویض(Ast_گرہ_تفویض* origp, Ast_گرہ_اظہاریہ* lhsp, Ast_گرہ_اظہاریہ* rhsp) {
        origp->جوڑو_یہاں_This_As_اگلا(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    خالی expandSS(Ast_گرہ_تفویض* origp, رکھو* lPlacep, رکھو* rPlacep) {
        File_سطر* ثابت flp = origp->fileline();
        ثابت std::سمتیہ<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        اگر (isPacked(rPlacep->dtypep)) {
            برائے (سائز_t i = 0; i < compsr.سائز(); ++i) {
                رکھو* ثابت lChildp = lPlacep->childrenp.at(i).get();
                Ast_گرہ_اظہاریہ* ثابت rp = new_بٹ(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, خالی_اشاریہ, خالی_اشاریہ, rp);
            }
        } ورنہ {
            برائے (سائز_t i = 0; i < compsr.سائز(); ++i) {
                رکھو* ثابت lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, خالی_اشاریہ, rPlacep->childrenp.at(i).get(), خالی_اشاریہ);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    خالی expandSN(Ast_گرہ_تفویض* origp, رکھو* lPlacep, Ast_گرہ_اظہاریہ* rhsp) {
        ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*> valueps = new_تفویض_Rhsps(rhsp, lPlacep->dtypep);
        برائے (سائز_t i = 0; i < valueps.سائز(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), خالی_اشاریہ, خالی_اشاریہ, valueps[i]);
        }
        VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    خالی expandNS(Ast_گرہ_تفویض* origp, Ast_گرہ_اظہاریہ* lhsp, رکھو* rPlacep) {
        File_سطر* ثابت flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        اگر (isPacked(rPlacep->dtypep)) {
            جوڑو_New_تفویض(origp, lhsp, new_بٹ(flp, rPlacep, 0, rPlacep->dtypep->چوڑائی() - 1));
            واپس;
        }
        // Unpacked of the same shape, component-wise
        Ast_گرہ_DType* ثابت dtypep = dtype_کا(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ثابت منطقی readsWrittenVar = !VN_IS(origp, تفویض_Dly) && [&]() {
            ثابت VNUser2_اندر_Use user2_اندر_Use;
            واپس lhsp->exists([](ثابت AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                اگر (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                اگر (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                واپس vscp->user2() == 3;
            });
        }();
        اگر (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ثابت scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, خالی_اشاریہ, نیا AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, خالی_اشاریہ);
            جوڑو_New_تفویض(origp, lhsp, نیا AstVarRef{flp, vscp, VAccess::READ});
            واپس;
        }
        ثابت سائز_t سائز = dtypeComponents(dtypep).سائز();
        برائے (سائز_t i = 0; i < سائز; ++i) {
            Ast_گرہ_اظہاریہ* ثابت newLhsp = newSelect(lhsp->clone_درخت(جھوٹ), dtypep, i);
            expand(origp, خالی_اشاریہ, newLhsp, rPlacep->childrenp.at(i).get(), خالی_اشاریہ);
        }
        VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    خالی expand(Ast_گرہ_تفویض* origp,  //
                رکھو* lPlacep, Ast_گرہ_اظہاریہ* lhsp,  //
                رکھو* rPlacep, Ast_گرہ_اظہاریہ* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_سطر* ثابت flp = origp->fileline();
        // A Place not split is referenced
        اگر (lPlacep && !lPlacep->بانٹو) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = خالی_اشاریہ;
        }
        اگر (rPlacep && !rPlacep->بانٹو) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = خالی_اشاریہ;
        }
        اگر (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ورنہ اگر (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ورنہ اگر (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ورنہ {
            جوڑو_New_تفویض(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    منطقی is_بانٹو_Between(ثابت رکھو* placep, صحیح lsb, صحیح msb) {
        جب (placep->بانٹو) {
            ثابت سائز_t idx = component_اشاریہ(placep->dtypep, lsb);
            اگر (idx != component_اشاریہ(placep->dtypep, msb)) واپس سچ;
            ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        واپس جھوٹ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    خالی hoistTerms(Ast_گرہ_تفویض* origp, ثابت رکھو* lPlacep) {
        Ast_گرہ_اظہاریہ* ثابت rhsp = origp->rhsp();
        ثابت std::سمتیہ<Ast_گرہ_اظہاریہ*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ثابت VNUser2_اندر_Use user2_اندر_Use;
        برائے (Ast_گرہ_اظہاریہ* ثابت termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
        صحیح lsb = 0;  // LSB of the current term
        برائے (Ast_گرہ_اظہاریہ* ثابت termp : termps) {
            ثابت صحیح tLsb = lsb;
            ثابت صحیح tMsb = lsb + termp->چوڑائی() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            اگر (!termp->user2()) جاری;
            // Cheap to select from
            اگر (isCheap(termp)) {
                termp->user2(0);
                جاری;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            اگر (termp->is_خالص() && termp->user2() == 1 && !is_بانٹو_Between(lPlacep, tLsb, tMsb)) {
                جاری;
            }
            // Bits can be selected without evaluating it more than once
            اگر (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                جاری;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_سطر* ثابت flp = termp->fileline();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(نیا AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ثابت tmpRefp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
            origp->جوڑو_یہاں_This_As_اگلا(نیا Ast_تفویض{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ساکن خالی markComponents(ثابت رکھو* placep) {
        برائے (ثابت std::unique_ptr<رکھو>& childp : placep->childrenp) {
            childp->vscp->user2(سچ);
            اگر (childp->بانٹو) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    خالی rewriteAssignment(ثابت Assignment& assignment) {
        رکھو* ثابت lPlacep = assignment.lPlacep;
        رکھو* ثابت rPlacep = assignment.rPlacep;
        ثابت منطقی l_بانٹو = lPlacep && lPlacep->بانٹو;
        ثابت منطقی r_بانٹو = rPlacep && rPlacep->بانٹو;
        // Nothing to do if neither side is split
        اگر (!l_بانٹو && !r_بانٹو) واپس;

        Ast_گرہ_تفویض* ثابت origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        اگر (l_بانٹو && !r_بانٹو && !VN_IS(origp, تفویض_Dly)) {
            ثابت VNUser2_اندر_Use user2_اندر_Use;
            markComponents(lPlacep);
            Ast_گرہ_اظہاریہ* ثابت rhsp = origp->rhsp();
            اگر (rhsp->exists([](ثابت AstVarRef* refp) { واپس refp->varScopep()->user2(); })) {
                File_سطر* ثابت flp = rhsp->fileline();
                AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtype_کا(rhsp));
                rhsp->replaceWith(نیا AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ثابت tmpRefp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
                origp->جوڑو_یہاں_This_As_اگلا(نیا Ast_تفویض{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        اگر (l_بانٹو && !r_بانٹو && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_گرہ_اظہاریہ* ثابت lhsp = origp->lhsp()->unlink_Fr_عقب();
        Ast_گرہ_اظہاریہ* ثابت rhsp = origp->rhsp()->unlink_Fr_عقب();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_بانٹو ? lPlacep : خالی_اشاریہ, l_بانٹو ? خالی_اشاریہ : lhsp,  //
               r_بانٹو ? rPlacep : خالی_اشاریہ, r_بانٹو ? خالی_اشاریہ : rhsp);
        اگر (l_بانٹو) VL_DO_DANGLING(ڈالو_Deletep(lhsp), lhsp);
        اگر (r_بانٹو) VL_DO_DANGLING(ڈالو_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ڈالو_Deletep(origp->unlink_Fr_عقب()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساکن Ast_فعال* combo_فعال(AstScope* scopep) {
        اگر (!scopep->user1p()) scopep->user1p(scopep->comboActivep(سچ));
        واپس VN_AS(scopep->user1p(), فعال);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    خالی driveFromComponents(رکھو* placep, Ast_فعال* activep) {
        اگر (!placep->بانٹو) واپس;
        AstVarScope* ثابت vscp = placep->vscp;
        File_سطر* ثابت flp = vscp->fileline();
        برائے (سائز_t i = 0; i < placep->childrenp.سائز(); ++i) {
            رکھو* ثابت childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_گرہ_اظہاریہ* ثابت rhsp = نیا AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_گرہ_اظہاریہ* ثابت lhsRefp = نیا AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_گرہ_اظہاریہ* ثابت lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->جوڑو_Stmtsp(نیا AstAlways{نیا Ast_تفویض_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    صریح DecomposeRewrite(State& state)
        : Decompose_بنیاد{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        برائے (ثابت std::سمتیہ<Select>& chain : m_state.chains) rewriteChain(chain);
        برائے (ثابت Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        برائے (رکھو* ثابت placep : m_state.rootps) {
            اگر (!placep->بانٹو) جاری;
            ثابت AstVarScope* ثابت vscp = placep->vscp;
            // Only traced ones at this point
            اگر (!(vscp->varp()->isTrace() && vscp->isTrace())) جاری;
            Ast_فعال* ثابت activep = combo_فعال(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::جوڑو_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::جوڑو_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

عوامی:
    ساکن خالی apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

خالی V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_بنیاد::State state;
        Decompose_ریکارڈ::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("decompose", 0, dump_درخت_Either_Level() >= 3);
}
