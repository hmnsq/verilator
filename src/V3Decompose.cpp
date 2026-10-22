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

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3Decompose.h"

#ضمن "V3AstUserAllocator.h"
#ضمن "V3MemberMap.h"
#ضمن "V3SharedTmps.h"
#ضمن "V3Stats.h"

#ضمن <algorithm>
#ضمن <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

صنف Decompose_بنياد VL_NOT_FINAL {
محمي:
    // TYPES

    // A component (element or member) of an aggregate type
    بنية Component final {
        Ast_ڳنڍ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        صحيح declIdx;  // The declared index of the element, if an array
        صحيح lsb;  // LSB in the whole value, if packed
        صحيح msb;  // MSB in the whole value, if packed
    };

    بنية رکو;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    بنية Copy final {
        رکو* otherp;  // The Place at the other end
        // Following only for packed places
        صحيح lsb;  // The first bit covered of this end
        صحيح otherLsb;  // The first bit covered of the other end
        صحيح ويڪر;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    بنية رکو final {
        رکو* parentp = خالي_اشارو;  // The Place this is a component of, nullptr if original
        رکو* rootp = خالي_اشارو;  // The original variable (root of the tree), root points to itself
        Ast_ڳنڍ_DType* dtypep = خالي_اشارو;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = خالي_اشارو;
        // The component Places created so far, by index, sized on the first one created
        std::ويڪٽر<std::unique_ptr<رکو>> childrenp;
        std::ويڪٽر<Copy> copies;  // The copies covering it, until split
        منطقي splittable = سچ;  // Can be split (there is no reason not to)
        منطقي چاهي_ورهايو = خطأ;  // Should be split (if possible, do split)
        منطقي ورهايو = خطأ;  // Place was split
        منطقي queued = خطأ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    بنية Select final {
        Ast_ڳنڍ_اظهار* exprp;  // The select
        رکو* placep;  // The component
        صحيح lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    بنية Assignment final {
        Ast_ڳنڍ_سونپيو* assp;  // The assignment
        رکو* lPlacep;  // The Place the LHS addresses exactly, if any
        رکو* rPlacep;  // The Place the RHS addresses exactly, if any
    };

عام:
    // The state of the pass, shared by the steps
    صنف State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ثابت VNUser4_اندريون_Use m_user4_اندريون_Use;

    عام:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ڳنڍ_DType, std::ويڪٽر<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, رکو> places;
        std::ويڪٽر<رکو*> rootps;  // The original Places, in creation order
        std::ويڪٽر<std::ويڪٽر<Select>> chains;  // Select chains addressing components
        std::ويڪٽر<Assignment> assignments;  // Assignments to expand if a side is split
    };

محمي:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    واضح Decompose_بنياد(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ساكن منطقي isUnpacked(ثابت Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع سچ;
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ساكن منطقي isPacked(ثابت Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            أرجع arrayp->subDTypep()->ويڪر() > 1;
        }
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ساكن منطقي isSplittableType(ثابت Ast_ڳنڍ_DType* dtypep) {
        إذا (isPacked(dtypep)) أرجع v3Global.opt.fDecomposePacked();
        إذا (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            إذا (ثابت AstStructDType* ثابت structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                لكل (ثابت AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    إذا (mp->valuep()) أرجع خطأ;
                }
            }
            أرجع v3Global.opt.fDecomposeUnpacked();
        }
        أرجع خطأ;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ساكن Ast_ڳنڍ_DType* dtype_جو(ثابت Ast_ڳنڍ_اظهار* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp->varp()->dtypep();
        أرجع nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ثابت std::ويڪٽر<Component>& dtypeComponents(Ast_ڳنڍ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::ويڪٽر<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        إذا (compsr.خالي_جايون()) {
            إذا (ثابت Ast_ڳنڍ_Array_DType* ثابت arrayp = VN_CAST(dtypep, ڳنڍ_Array_DType)) {
                ثابت منطقي packed = isPacked(dtypep);
                ثابت VNumRange range = arrayp->declRange();
                ثابت منطقي rev = packed && range.ascending();  // Only for naming components
                Ast_ڳنڍ_DType* ثابت subp = arrayp->subDTypep();
                لكل (صحيح i = 0; i < range.elements(); ++i) {
                    ثابت صحيح lsb = packed ? i * subp->ويڪر() : 0;
                    ثابت صحيح msb = packed ? lsb + subp->ويڪر() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ثابت صحيح declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ڌڪو_پٺ({subp, خالي_اشارو, declIdx, lsb, msb});
                }
            } وإلا {
                ثابت AstStructDType* ثابت structp = VN_AS(dtypep, StructDType);
                ثابت منطقي packed = isPacked(dtypep);
                لكل (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ثابت صحيح lsb = packed ? mp->lsb() : 0;
                    ثابت صحيح msb = packed ? lsb + mp->ويڪر() - 1 : 0;
                    compsr.ڌڪو_پٺ({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::ابتو(compsr.begin(), compsr.پڇاڙي());
            }
        }

        // The components of this data type
        أرجع compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    سائيز_t component_اشاريو(Ast_ڳنڍ_DType* dtypep, صحيح ٻٽو_انگ_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ثابت std::ويڪٽر<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ثابت خودڪار it = std::هيٺاهون_bound(compsr.begin(), compsr.پڇاڙي(), ٻٽو_انگ_2,
                                         [](ثابت Component& comp, صحيح b) {  //
                                             أرجع b > comp.msb;
                                         });
        أرجع it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    رکو* component_جو(رکو* placep, سائيز_t idx) {
        ثابت std::ويڪٽر<Component>& comps = dtypeComponents(placep->dtypep);
        إذا (placep->childrenp.خالي_جايون()) placep->childrenp.resize(comps.سائيز());
        std::unique_ptr<رکو>& childpr = placep->childrenp.at(idx);
        إذا (!childpr) {
            childpr = std::make_unique<رکو>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->چاهي_ورهايو = placep->rootp->vscp->varp()->attr_ورهايو_Var();
        }
        أرجع childpr.get();
    }

    // Is 'nodep' cheap to clone
    ساكن منطقي isCheap(ثابت Ast_ڳنڍ_اظهار* nodep) {
        // Constants are cheap
        إذا (VN_IS(nodep, Const)) أرجع سچ;
        // So are resets
        إذا (VN_IS(nodep, CReset)) أرجع سچ;
        // Variable references are cheap for the most part
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            إذا (refp->varp()->isForced()) أرجع خطأ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            إذا (refp->varp()->isSc()) أرجع خطأ;
            // Otherwise the path rooted here is cheap
            أرجع سچ;
        }
        // Bit selects only with a constant LSB
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        إذا (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
            أرجع isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        إذا (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {  //
            أرجع isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        إذا (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {  //
            أرجع isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        أرجع خطأ;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

صنف Decompose_رڪارڊ final : عام VNVisitorConst, عام Decompose_بنياد {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ڳنڍ_سونپيو* m_assignp = خالي_اشارو;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساكن فراغ warn_No_ورهايو(AstVar* varp, ثابت Ast_ڳنڍ* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        إذا (!varp->attr_ورهايو_Var()) أرجع;
        // A packed variable might still be split by V3BitBlast, which warns if not
        إذا (isPacked(varp->dtypep())) أرجع;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_ڪوڊ::SPLITVAR, سچ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ساكن منطقي isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        إذا (!varp->user1()) {
            ثابت منطقي eligible = [&]() {
                // Check data type
                إذا (!isSplittableType(varp->dtypep())) أرجع خطأ;
                // Check properties
                ثابت حرف* ثابت reasonp = varp->cannot_ورهايو_قسم_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                إذا (reasonp) warn_No_ورهايو(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                أرجع !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        أرجع varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    رکو* رکو_جو(ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (!isEligible(vscp->varp())) أرجع خالي_اشارو;
        رکو& رکو_2 = m_state.places(vscp);
        إذا (!رکو_2.rootp) {
            رکو_2.rootp = &رکو_2;
            رکو_2.dtypep = vscp->varp()->dtypep();
            رکو_2.vscp = vscp;
            رکو_2.چاهي_ورهايو = vscp->varp()->attr_ورهايو_Var();
            m_state.rootps.ڌڪو_پٺ(&رکو_2);
        }
        أرجع &رکو_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ساكن فراغ بلاڪ(رکو* placep, ثابت Ast_ڳنڍ* wherep) {
        placep->splittable = خطأ;
        // Warn on the variable only, a component is split as far as possible, as requested
        إذا (!placep->parentp) {
            warn_No_ورهايو(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    رکو* resolveChain(Ast_ڳنڍ_اظهار* exprp, std::ويڪٽر<Select>& chain) {
        // Array selects correspond one to one with a component select
        إذا (AstArraySel* ثابت selp = VN_CAST(exprp, ArraySel)) {
            إذا (رکو* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ثابت AstConst* ثابت bitp = VN_CAST(selp->bitp(), Const);
                إذا (bitp && bitp->منزل_UQuad() < dtypeComponents(fromp->dtypep).سائيز()) {
                    رکو* ثابت childp = component_جو(fromp, bitp->منزل_UQuad());
                    fromp->چاهي_ورهايو = سچ;
                    chain.ڌڪو_پٺ({selp, childp, 0});
                    أرجع childp;
                }
                // Otherwise block splitting of the Place
                بلاڪ(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            أرجع خالي_اشارو;
        }
        // Struct selects correspond one to one with a component select
        إذا (AstStructSel* ثابت selp = VN_CAST(exprp, StructSel)) {
            إذا (رکو* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                إذا (!isUnpacked(fromp->dtypep)) أرجع خالي_اشارو;
                AstStructDType* ثابت structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                إذا (!structp->user1_سيٽ_Once()) {
                    ثابت std::ويڪٽر<Component>& compsr = dtypeComponents(structp);
                    لكل (سائيز_t i = 0; i < compsr.سائيز(); ++i) compsr[i].memberp->user1(i);
                }
                ثابت Ast_ڳنڍ* ثابت memberp = m_memberMap.ڳوليو_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                رکو* ثابت childp = component_جو(fromp, memberp->user1());
                fromp->چاهي_ورهايو = سچ;
                chain.ڌڪو_پٺ({selp, childp, 0});
                أرجع childp;
            }
            أرجع خالي_اشارو;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            رکو* placep = resolveChain(selp->fromp(), chain);
            إذا (!placep) {
                iterateConst(selp->lsbp());
                أرجع خالي_اشارو;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            إذا (!lsbp || lsbp->منزل_SInt() + selp->ويڪر_Const() > placep->dtypep->ويڪر()) {
                بلاڪ(placep, selp->fromp());
                iterateConst(selp->lsbp());
                أرجع خالي_اشارو;
            }
            // Descend into the Place containing the bits, until exact
            صحيح lsb = lsbp->منزل_SInt();
            صحيح msb = lsb + selp->ويڪر_Const() - 1;
            طالما (lsb != 0 || msb != placep->dtypep->ويڪر() - 1) {
                // Within a component of a type not split
                إذا (!isPacked(placep->dtypep)) أرجع خالي_اشارو;
                ثابت سائيز_t idx = component_اشاريو(placep->dtypep, lsb);
                // Crossing components prevents splitting
                إذا (idx != component_اشاريو(placep->dtypep, msb)) {
                    بلاڪ(placep, selp->fromp());
                    أرجع خالي_اشارو;
                }
                placep->چاهي_ورهايو = سچ;
                ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_جو(placep, idx);
                chain.ڌڪو_پٺ({selp, placep, lsb});
            }
            أرجع placep;
        }
        // Base case
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) أرجع رکو_جو(refp);
        // Not a select chain
        iterateConst(exprp);
        أرجع خالي_اشارو;
    }

    // 'nodep' references the Place 'placep' whole
    فراغ referenced_سڄو(Ast_ڳنڍ_اظهار* nodep, رکو* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        إذا (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            أرجع;
        }

        // Otherwise must block splitting of this Place
        بلاڪ(placep, nodep);
    }

    // Visit a select expression
    فراغ گهمو_Select(Ast_ڳنڍ_اظهار* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::ويڪٽر<Select> chain;
        رکو* ثابت placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        إذا (placep) referenced_سڄو(nodep, placep);
        // Record the chain for the rewriting phase
        إذا (!chain.خالي_جايون()) m_state.chains.ڌڪو_پٺ(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ساكن منطقي hasReversedRange(ثابت Ast_ڳنڍ_DType* aDTypep, ثابت Ast_ڳنڍ_DType* bDTypep) {
        ثابت AstUnpackArrayDType* ثابت aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        إذا (!aArrayp || !bArrayp) أرجع خطأ;
        إذا (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) أرجع سچ;
        أرجع hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    فراغ گهمو_Assignment(Ast_ڳنڍ_سونپيو* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ثابت منطقي reversed = hasReversedRange(dtype_جو(nodep->lhsp()), dtype_جو(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? خالي_اشارو : nodep;
        iterateChildrenConst(nodep);
        إذا (!m_assignp) أرجع;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ڳنڍ_اظهار* ثابت lhsp = nodep->lhsp();
        Ast_ڳنڍ_اظهار* ثابت rhsp = nodep->rhsp();
        رکو* ثابت lPlacep = lhsp->user1u().منزل<رکو*>();
        رکو* ثابت rPlacep = rhsp->user1u().منزل<رکو*>();

        // Is it a splittable assignment
        ثابت منطقي splittable = [&]() {
            // Assignment between two Places is splittable
            إذا (lPlacep && rPlacep) أرجع سچ;
            // If the LHS is a Place, the RHS must be a splittable expression
            إذا (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                إذا (isPacked(lPlacep->dtypep)) أرجع سچ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                أرجع isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            إذا (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                إذا (isPacked(rPlacep->dtypep)) أرجع سچ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                أرجع isCheap(lhsp);
            }
            // Otherwise not splittable
            أرجع خطأ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        إذا (!splittable) {
            إذا (rPlacep) بلاڪ(rPlacep, rhsp);
            إذا (lPlacep) بلاڪ(lPlacep, lhsp);
            أرجع;
        }

        // If both sides are places, record the whole copies for the Decision phase
        إذا (lPlacep && rPlacep) {
            ثابت صحيح ويڪر = lPlacep->dtypep->ويڪر();  // Unused when unpacked
            lPlacep->copies.ڌڪو_پٺ({rPlacep, 0, 0, ويڪر});
            rPlacep->copies.ڌڪو_پٺ({lPlacep, 0, 0, ويڪر});
        }
        // Record for the rewriting phase
        m_state.assignments.ڌڪو_پٺ({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    فراغ گهمو(Ast_ڳنڍ_DType*) override {}  // No references in data types
    فراغ گهمو(Ast_ڳنڍ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    فراغ گهمو(AstArraySel* nodep) override { گهمو_Select(nodep); }
    فراغ گهمو(AstStructSel* nodep) override { گهمو_Select(nodep); }
    فراغ گهمو(AstSel* nodep) override { گهمو_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    فراغ گهمو(Ast_سونپيو* nodep) override { گهمو_Assignment(nodep); }
    فراغ گهمو(Ast_سونپيو_W* nodep) override { گهمو_Assignment(nodep); }
    فراغ گهمو(Ast_سونپيو_Dly* nodep) override { گهمو_Assignment(nodep); }
    // VISITORS - References to Places
    فراغ گهمو(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        إذا (رکو* ثابت placep = رکو_جو(nodep)) referenced_سڄو(nodep, placep);
    }
    فراغ گهمو(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ثابت varp = nodep->varp();
        إذا (!isEligible(varp)) أرجع;
        varp->user1(2);  // Mark not eligible
        warn_No_ورهايو(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_رڪارڊ(State& state, AstNetlist* netlistp)
        : Decompose_بنياد{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        لكل (رکو* ثابت placep : m_state.rootps) {
            إذا (!isEligible(placep->vscp->varp())) placep->splittable = خطأ;
        }
    }

عام:
    ساكن فراغ apply(State& state, AstNetlist* netlistp) { Decompose_رڪارڊ{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

صنف DecomposeDecision final : عام Decompose_بنياد {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // STATE
    std::ويڪٽر<رکو*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::ويڪٽر<AstVar*>> m_ورهايو_Varps;
    VDouble0 m_stat_ورهايو_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_ورهايو_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_ورهايو_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_ورهايو_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ساكن منطقي can_ورهايو(ثابت رکو* placep) {
        // Not if not splittable
        إذا (!placep->splittable) أرجع خطأ;
        // Not if already split
        إذا (placep->ورهايو) أرجع خطأ;
        // Original variable or its parent was already split
        أرجع !placep->parentp || placep->parentp->ورهايو;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    فراغ قطار_اندر_وجهو(رکو* placep) {
        إذا (placep->queued || !placep->چاهي_ورهايو || !can_ورهايو(placep)) أرجع;
        placep->queued = سچ;
        m_worklist.ڌڪو_پٺ(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    فراغ createComponents(رکو* placep) {
        AstVarScope* ثابت vscp = placep->vscp;
        AstVar* ثابت varp = vscp->varp();
        std::ويڪٽر<AstVar*>& varps = m_ورهايو_Varps(varp);
        // Create the component AstVars on first encounter
        إذا (varps.خالي_جايون()) {
            ثابت منطقي packed = isPacked(varp->dtypep());
            إذا (placep->parentp) {
                ++(packed ? m_stat_ورهايو_Packed_Comp : m_stat_ورهايو_Unpacked_Comp);
            } وإلا {
                ++(packed ? m_stat_ورهايو_Packed_Orig : m_stat_ورهايو_Unpacked_Orig);
            }
            File_سٽ* ثابت flp = varp->fileline();
            ثابت VVarType varType = varp->varType();
            ثابت std::string name = varp->name();
            لكل (ثابت Component& comp : dtypeComponents(varp->dtypep())) {
                ثابت std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ڳنڍ::encode_عدد(comp.declIdx) + "__KET__";
                AstVar* ثابت newp = جديد AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_ورهايو_Attr_From(varp);
                varps.ڌڪو_پٺ(newp);
                varp->جوڙيو_هتي_This_As_ايندڙ(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ثابت scopep = vscp->scopep();
        File_سٽ* ثابت flp = vscp->fileline();
        لكل (سائيز_t i = 0; i < varps.سائيز(); ++i) {
            AstVarScope* ثابت newp = جديد AstVarScope{flp, scopep, varps[i]};
            component_جو(placep, i)->vscp = newp;
            vscp->جوڙيو_هتي_This_As_ايندڙ(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    فراغ جوڙيو_Copy(رکو* placep, ثابت Copy& copy) {
        UASSERT(!placep->ورهايو, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        إذا (!placep->splittable) أرجع;
        // Add the copy to the Place
        placep->copies.ڌڪو_پٺ(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        إذا (!isPacked(placep->dtypep)) أرجع;
        إذا (copy.lsb != 0 || copy.ويڪر != placep->dtypep->ويڪر()) {
            placep->چاهي_ورهايو = سچ;
            قطار_اندر_وجهو(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    فراغ newCopy(رکو* ap, صحيح aLsb, رکو* bp, صحيح bLsb, صحيح ويڪر) {
        UASSERT(ap != bp, "Copy within a Place");
        جوڙيو_Copy(ap, {bp, aLsb, bLsb, ويڪر});
        جوڙيو_Copy(bp, {ap, bLsb, aLsb, ويڪر});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    فراغ ورهايو_Copy(رکو* placep, ثابت Copy& copy) {
        رکو* ثابت otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        إذا (isPacked(placep->dtypep)) {
            ثابت std::ويڪٽر<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ثابت صحيح lsb = copy.lsb;
            ثابت صحيح msb = copy.lsb + copy.ويڪر - 1;
            ثابت سائيز_t پهريون = component_اشاريو(placep->dtypep, lsb);
            لكل (سائيز_t i = پهريون; i < comps.سائيز() && comps[i].lsb <= msb; ++i) {
                ثابت Component& comp = comps[i];
                ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                ثابت صحيح partMsb = std::min(msb, comp.msb);
                newCopy(component_جو(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            أرجع;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->چاهي_ورهايو = سچ;
        قطار_اندر_وجهو(otherp);
        // The sides are the same shape, so pairwise whole copies
        ثابت سائيز_t سائيز = dtypeComponents(placep->dtypep).سائيز();
        لكل (سائيز_t i = 0; i < سائيز; ++i) {
            رکو* ثابت ap = component_جو(placep, i);
            إذا (!ap->splittable) استمر;
            رکو* ثابت bp = component_جو(otherp, i);
            إذا (!bp->splittable) استمر;
            newCopy(ap, 0, bp, 0, ap->dtypep->ويڪر());
        }
    }

    // Split the Place
    فراغ ورهايو_رکو(رکو* placep) {
        UASSERT(!placep->ورهايو, "Place split twice");
        // It is now split
        placep->ورهايو = سچ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ثابت سائيز_t سائيز = dtypeComponents(placep->dtypep).سائيز();
        لكل (سائيز_t i = 0; i < سائيز; ++i) قطار_اندر_وجهو(component_جو(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        لكل (ثابت Copy& copy : placep->copies) {
            إذا (!copy.otherp->ورهايو) ورهايو_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    واضح DecomposeDecision(State& state)
        : Decompose_بنياد{state} {
        // Enqueue the original variables
        لكل (رکو* ثابت placep : m_state.rootps) قطار_اندر_وجهو(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        طالما (!m_worklist.خالي_جايون()) {
            رکو* ثابت placep = m_worklist.پٺ();
            m_worklist.ڪڍو_پٺ();
            placep->queued = خطأ;
            ورهايو_رکو(placep);
        }

        V3Stats::جوڙيو_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_ورهايو_Unpacked_Orig);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_ورهايو_Packed_Orig);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_ورهايو_Unpacked_Comp);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_ورهايو_Packed_Comp);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

صنف DecomposeRewrite final : عام VNDeleter, عام Decompose_بنياد {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ثابت VNUser1_اندريون_Use m_user1_اندريون_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ساكن AstVarRef* newRef(File_سٽ* flp, ثابت رکو* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        أرجع جديد AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    فراغ rewriteChain(ثابت std::ويڪٽر<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ثابت Select* selectp = خالي_اشارو;
        لكل (ثابت Select& select : chain) {
            إذا (select.placep->parentp->ورهايو) selectp = &select;
        }
        إذا (!selectp) أرجع;
        Ast_ڳنڍ_اظهار* ثابت exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ثابت VAccess access = [&]() {
            ثابت Ast_ڳنڍ_اظهار* rootp = chain.front().exprp;
            طالما (سچ) {
                إذا (ثابت AstArraySel* ثابت selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } وإلا إذا (ثابت AstStructSel* ثابت selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } وإلا إذا (ثابت AstSel* ثابت selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } وإلا {
                    أرجع VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_سٽ* ثابت flp = exprp->fileline();
        Ast_ڳنڍ_اظهار* newp = newRef(flp, selectp->placep, access);
        إذا (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ثابت صحيح ويڪر = selp->ويڪر_Const();
            إذا (ويڪر != newp->ويڪر()) newp = جديد AstSel{flp, newp, selectp->lsb, ويڪر};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ڌڪو_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ڳنڍ_اظهار* newSelect(Ast_ڳنڍ_اظهار* fromp, Ast_ڳنڍ_DType* dtypep, سائيز_t idx) {
        File_سٽ* ثابت flp = fromp->fileline();
        ثابت Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        إذا (isPacked(dtypep)) {
            AstSel* ثابت selp = جديد AstSel{flp, fromp, comp.lsb, comp.dtypep->ويڪر()};
            selp->dtypep(comp.dtypep);
            أرجع selp;
        }
        // If unpacked array, it's an ArraySel
        إذا (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ثابت صحيح i = static_cast<صحيح>(idx);
            UASSERT_OBJ(static_cast<سائيز_t>(i) == idx, fromp, "Index overflow");
            أرجع جديد AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ثابت selp = جديد AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        أرجع selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ساكن std::ويڪٽر<Ast_ڳنڍ_اظهار*> concatTerms(Ast_ڳنڍ_اظهار* nodep) {
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> termps;
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> ڍير{nodep};
        طالما (!ڍير.خالي_جايون()) {
            Ast_ڳنڍ_اظهار* ثابت exprp = ڍير.پٺ();
            ڍير.ڪڍو_پٺ();
            إذا (AstConcat* ثابت concatp = VN_CAST(exprp, Concat)) {
                ڍير.ڌڪو_پٺ(concatp->lhsp());
                ڍير.ڌڪو_پٺ(concatp->rhsp());
                استمر;
            }
            إذا (AstReplicate* ثابت repp = VN_CAST(exprp, Replicate)) {
                ثابت AstConst* ثابت countp = VN_AS(repp->countp(), Const);
                لكل (uint32_t i = 0; i < countp->منزل_UInt(); ++i) ڍير.ڌڪو_پٺ(repp->srcp());
                استمر;
            }
            termps.ڌڪو_پٺ(exprp);
        }
        أرجع termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ساكن منطقي isSliceable(ثابت Ast_ڳنڍ_اظهار* nodep) {
        إذا (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            أرجع VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        إذا (ثابت AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            أرجع isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        إذا (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        إذا (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            أرجع isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        إذا (ثابت AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            أرجع isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        إذا (ثابت AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            أرجع isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        إذا (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            أرجع isSliceable(extp->lhsp());
        }
        إذا (ثابت AstNot* ثابت notp = VN_CAST(nodep, Not)) {  //
            أرجع isSliceable(notp->lhsp());
        }
        إذا (ثابت AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            أرجع isSliceable(repp->srcp());
        }
        أرجع isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ساكن Ast_ڳنڍ_اظهار* new_ٽڪرو(Ast_ڳنڍ_اظهار* nodep, صحيح lsb, صحيح ويڪر) {
        File_سٽ* ثابت flp = nodep->fileline();
        // A constant, as a new constant of the bits
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            V3_عدد انگ{nodep, ويڪر};
            انگ.opSel(constp->انگ(), lsb + ويڪر - 1, lsb);
            أرجع جديد AstConst{flp, انگ};
        }
        // A constant bit select, from the bits of what it selects from
        إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            إذا (ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                أرجع new_ٽڪرو(selp->fromp(), lsbp->منزل_SInt() + lsb, ويڪر);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        إذا (AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            ثابت صحيح r_ويڪر = catp->rhsp()->ويڪر();
            ثابت صحيح msb = lsb + ويڪر - 1;
            إذا (msb < r_ويڪر) أرجع new_ٽڪرو(catp->rhsp(), lsb, ويڪر);
            إذا (lsb >= r_ويڪر) أرجع new_ٽڪرو(catp->lhsp(), lsb - r_ويڪر, ويڪر);
            أرجع جديد AstConcat{flp, new_ٽڪرو(catp->lhsp(), 0, msb - r_ويڪر + 1),
                                 new_ٽڪرو(catp->rhsp(), lsb, r_ويڪر - lsb)};
        }
        إذا (AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع جديد AstCond{flp, condp->condp()->clone_وڻ_خالص(خطأ),
                               new_ٽڪرو(condp->thenp(), lsb, ويڪر),
                               new_ٽڪرو(condp->elsep(), lsb, ويڪر)};
        }
        إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            أرجع جديد AstAnd{flp, new_ٽڪرو(andp->lhsp(), lsb, ويڪر),
                              new_ٽڪرو(andp->rhsp(), lsb, ويڪر)};
        }
        إذا (AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            أرجع جديد AstOr{flp, new_ٽڪرو(orp->lhsp(), lsb, ويڪر),
                             new_ٽڪرو(orp->rhsp(), lsb, ويڪر)};
        }
        إذا (AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            أرجع جديد AstXor{flp, new_ٽڪرو(xorp->lhsp(), lsb, ويڪر),
                              new_ٽڪرو(xorp->rhsp(), lsb, ويڪر)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        إذا (AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            ثابت صحيح src_ويڪر = extp->lhsp()->ويڪر();
            ثابت صحيح msb = lsb + ويڪر - 1;
            إذا (msb < src_ويڪر) أرجع new_ٽڪرو(extp->lhsp(), lsb, ويڪر);
            إذا (lsb >= src_ويڪر) أرجع جديد AstConst{flp, AstConst::Widthed_قدر{}, ويڪر, 0};
            أرجع جديد AstConcat{
                flp, جديد AstConst{flp, AstConst::Widthed_قدر{}, msb - src_ويڪر + 1, 0},
                new_ٽڪرو(extp->lhsp(), lsb, src_ويڪر - lsb)};
        }
        إذا (AstNot* ثابت notp = VN_CAST(nodep, Not)) {
            أرجع جديد AstNot{flp, new_ٽڪرو(notp->lhsp(), lsb, ويڪر)};
        }
        // A replication, from the portions of the copies overlapping the bits
        إذا (AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            Ast_ڳنڍ_اظهار* ثابت srcp = repp->srcp();
            ثابت صحيح src_ويڪر = srcp->ويڪر();
            ثابت صحيح msb = lsb + ويڪر - 1;
            Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
            لكل (صحيح copyLsb = lsb / src_ويڪر * src_ويڪر; copyLsb <= msb; copyLsb += src_ويڪر) {
                ثابت صحيح partLsb = std::max(lsb, copyLsb) - copyLsb;
                ثابت صحيح partMsb = std::min(msb, copyLsb + src_ويڪر - 1) - copyLsb;
                Ast_ڳنڍ_اظهار* ثابت bitsp = new_ٽڪرو(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
            }
            أرجع resultp;
        }
        Ast_ڳنڍ_اظهار* ثابت clonep = nodep->clone_وڻ_خالص(خطأ);
        إذا (lsb == 0 && ويڪر == nodep->ويڪر()) أرجع clonep;
        أرجع جديد AstSel{flp, clonep, lsb, ويڪر};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::ويڪٽر<Ast_ڳنڍ_اظهار*> new_سونپيو_Rhsps(Ast_ڳنڍ_اظهار* nodep, Ast_ڳنڍ_DType* dtypep) {
        ثابت std::ويڪٽر<Component>& compsr = dtypeComponents(dtypep);
        std::ويڪٽر<Ast_ڳنڍ_اظهار*> valueps;
        valueps.reserve(compsr.سائيز());

        // If CReset, it is duplicated for each component
        إذا (AstCReset* ثابت cresetp = VN_CAST(nodep, CReset)) {
            لكل (ثابت Component& comp : compsr) {
                AstCReset* ثابت newp = cresetp->clone_وڻ(خطأ);
                newp->dtypep(comp.dtypep);
                valueps.ڌڪو_پٺ(newp);
            }
            أرجع valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        إذا (isPacked(dtypep)) {
            valueps.resize(compsr.سائيز(), خالي_اشارو);
            صحيح lsb = 0;  // LSB of the current term
            لكل (Ast_ڳنڍ_اظهار* ثابت termp : concatTerms(nodep)) {
                ثابت صحيح msb = lsb + termp->ويڪر() - 1;
                ثابت سائيز_t پهريون = component_اشاريو(dtypep, lsb);
                لكل (سائيز_t i = پهريون; i < compsr.سائيز() && compsr[i].lsb <= msb; ++i) {
                    ثابت Component& comp = compsr[i];
                    ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                    ثابت صحيح partMsb = std::min(msb, comp.msb);
                    File_سٽ* ثابت flp = termp->fileline();
                    Ast_ڳنڍ_اظهار* ثابت sp = new_ٽڪرو(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? جديد AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            أرجع valueps;
        }

        // Otherwise unpacked, so select each component
        لكل (سائيز_t i = 0; i < compsr.سائيز(); ++i) {
            valueps.ڌڪو_پٺ(newSelect(nodep->clone_وڻ(خطأ), dtypep, i));
        }
        أرجع valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ڳنڍ_اظهار* new_بٽ(File_سٽ* flp, ثابت رکو* placep, صحيح lsb, صحيح msb) {
        Ast_ڳنڍ_DType* ثابت dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->ورهايو, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        إذا (!placep->ورهايو) {
            AstVarRef* ثابت refp = newRef(flp, placep, VAccess::READ);
            إذا (lsb == 0 && msb == dtypep->ويڪر() - 1) أرجع refp;
            أرجع جديد AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ثابت std::ويڪٽر<Component>& comps = dtypeComponents(dtypep);
        Ast_ڳنڍ_اظهار* resultp = خالي_اشارو;
        ثابت سائيز_t پهريون = component_اشاريو(dtypep, lsb);
        لكل (سائيز_t i = پهريون; i < comps.سائيز() && comps[i].lsb <= msb; ++i) {
            ثابت Component& comp = comps[i];
            ثابت صحيح partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ثابت صحيح partMsb = std::min(msb, comp.msb) - comp.lsb;
            رکو* ثابت childp = placep->childrenp.at(i).get();
            Ast_ڳنڍ_اظهار* ثابت bitsp = new_بٽ(flp, childp, partLsb, partMsb);
            resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
        }
        أرجع resultp;
    }

    ساكن فراغ جوڙيو_New_سونپيو(Ast_ڳنڍ_سونپيو* origp, Ast_ڳنڍ_اظهار* lhsp, Ast_ڳنڍ_اظهار* rhsp) {
        origp->جوڙيو_هتي_This_As_ايندڙ(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    فراغ expandSS(Ast_ڳنڍ_سونپيو* origp, رکو* lPlacep, رکو* rPlacep) {
        File_سٽ* ثابت flp = origp->fileline();
        ثابت std::ويڪٽر<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        إذا (isPacked(rPlacep->dtypep)) {
            لكل (سائيز_t i = 0; i < compsr.سائيز(); ++i) {
                رکو* ثابت lChildp = lPlacep->childrenp.at(i).get();
                Ast_ڳنڍ_اظهار* ثابت rp = new_بٽ(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, خالي_اشارو, خالي_اشارو, rp);
            }
        } وإلا {
            لكل (سائيز_t i = 0; i < compsr.سائيز(); ++i) {
                رکو* ثابت lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, خالي_اشارو, rPlacep->childrenp.at(i).get(), خالي_اشارو);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    فراغ expandSN(Ast_ڳنڍ_سونپيو* origp, رکو* lPlacep, Ast_ڳنڍ_اظهار* rhsp) {
        ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*> valueps = new_سونپيو_Rhsps(rhsp, lPlacep->dtypep);
        لكل (سائيز_t i = 0; i < valueps.سائيز(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), خالي_اشارو, خالي_اشارو, valueps[i]);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    فراغ expandNS(Ast_ڳنڍ_سونپيو* origp, Ast_ڳنڍ_اظهار* lhsp, رکو* rPlacep) {
        File_سٽ* ثابت flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        إذا (isPacked(rPlacep->dtypep)) {
            جوڙيو_New_سونپيو(origp, lhsp, new_بٽ(flp, rPlacep, 0, rPlacep->dtypep->ويڪر() - 1));
            أرجع;
        }
        // Unpacked of the same shape, component-wise
        Ast_ڳنڍ_DType* ثابت dtypep = dtype_جو(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ثابت منطقي readsWrittenVar = !VN_IS(origp, سونپيو_Dly) && [&]() {
            ثابت VNUser2_اندريون_Use user2_اندريون_Use;
            أرجع lhsp->exists([](ثابت AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                إذا (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                إذا (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                أرجع vscp->user2() == 3;
            });
        }();
        إذا (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ثابت scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, خالي_اشارو, جديد AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, خالي_اشارو);
            جوڙيو_New_سونپيو(origp, lhsp, جديد AstVarRef{flp, vscp, VAccess::READ});
            أرجع;
        }
        ثابت سائيز_t سائيز = dtypeComponents(dtypep).سائيز();
        لكل (سائيز_t i = 0; i < سائيز; ++i) {
            Ast_ڳنڍ_اظهار* ثابت newLhsp = newSelect(lhsp->clone_وڻ(خطأ), dtypep, i);
            expand(origp, خالي_اشارو, newLhsp, rPlacep->childrenp.at(i).get(), خالي_اشارو);
        }
        VL_DO_DANGLING(ڌڪو_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    فراغ expand(Ast_ڳنڍ_سونپيو* origp,  //
                رکو* lPlacep, Ast_ڳنڍ_اظهار* lhsp,  //
                رکو* rPlacep, Ast_ڳنڍ_اظهار* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_سٽ* ثابت flp = origp->fileline();
        // A Place not split is referenced
        إذا (lPlacep && !lPlacep->ورهايو) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = خالي_اشارو;
        }
        إذا (rPlacep && !rPlacep->ورهايو) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = خالي_اشارو;
        }
        إذا (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } وإلا إذا (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } وإلا إذا (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } وإلا {
            جوڙيو_New_سونپيو(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    منطقي is_ورهايو_Between(ثابت رکو* placep, صحيح lsb, صحيح msb) {
        طالما (placep->ورهايو) {
            ثابت سائيز_t idx = component_اشاريو(placep->dtypep, lsb);
            إذا (idx != component_اشاريو(placep->dtypep, msb)) أرجع سچ;
            ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        أرجع خطأ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    فراغ hoistTerms(Ast_ڳنڍ_سونپيو* origp, ثابت رکو* lPlacep) {
        Ast_ڳنڍ_اظهار* ثابت rhsp = origp->rhsp();
        ثابت std::ويڪٽر<Ast_ڳنڍ_اظهار*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ثابت VNUser2_اندريون_Use user2_اندريون_Use;
        لكل (Ast_ڳنڍ_اظهار* ثابت termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
        صحيح lsb = 0;  // LSB of the current term
        لكل (Ast_ڳنڍ_اظهار* ثابت termp : termps) {
            ثابت صحيح tLsb = lsb;
            ثابت صحيح tMsb = lsb + termp->ويڪر() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            إذا (!termp->user2()) استمر;
            // Cheap to select from
            إذا (isCheap(termp)) {
                termp->user2(0);
                استمر;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            إذا (termp->is_خالص() && termp->user2() == 1 && !is_ورهايو_Between(lPlacep, tLsb, tMsb)) {
                استمر;
            }
            // Bits can be selected without evaluating it more than once
            إذا (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                استمر;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_سٽ* ثابت flp = termp->fileline();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            origp->جوڙيو_هتي_This_As_ايندڙ(جديد Ast_سونپيو{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ساكن فراغ markComponents(ثابت رکو* placep) {
        لكل (ثابت std::unique_ptr<رکو>& childp : placep->childrenp) {
            childp->vscp->user2(سچ);
            إذا (childp->ورهايو) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    فراغ rewriteAssignment(ثابت Assignment& assignment) {
        رکو* ثابت lPlacep = assignment.lPlacep;
        رکو* ثابت rPlacep = assignment.rPlacep;
        ثابت منطقي l_ورهايو = lPlacep && lPlacep->ورهايو;
        ثابت منطقي r_ورهايو = rPlacep && rPlacep->ورهايو;
        // Nothing to do if neither side is split
        إذا (!l_ورهايو && !r_ورهايو) أرجع;

        Ast_ڳنڍ_سونپيو* ثابت origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        إذا (l_ورهايو && !r_ورهايو && !VN_IS(origp, سونپيو_Dly)) {
            ثابت VNUser2_اندريون_Use user2_اندريون_Use;
            markComponents(lPlacep);
            Ast_ڳنڍ_اظهار* ثابت rhsp = origp->rhsp();
            إذا (rhsp->exists([](ثابت AstVarRef* refp) { أرجع refp->varScopep()->user2(); })) {
                File_سٽ* ثابت flp = rhsp->fileline();
                AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtype_جو(rhsp));
                rhsp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
                origp->جوڙيو_هتي_This_As_ايندڙ(جديد Ast_سونپيو{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        إذا (l_ورهايو && !r_ورهايو && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ڳنڍ_اظهار* ثابت lhsp = origp->lhsp()->unlink_Fr_پٺ();
        Ast_ڳنڍ_اظهار* ثابت rhsp = origp->rhsp()->unlink_Fr_پٺ();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_ورهايو ? lPlacep : خالي_اشارو, l_ورهايو ? خالي_اشارو : lhsp,  //
               r_ورهايو ? rPlacep : خالي_اشارو, r_ورهايو ? خالي_اشارو : rhsp);
        إذا (l_ورهايو) VL_DO_DANGLING(ڌڪو_Deletep(lhsp), lhsp);
        إذا (r_ورهايو) VL_DO_DANGLING(ڌڪو_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ڌڪو_Deletep(origp->unlink_Fr_پٺ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساكن Ast_سرگرم* combo_سرگرم(AstScope* scopep) {
        إذا (!scopep->user1p()) scopep->user1p(scopep->comboActivep(سچ));
        أرجع VN_AS(scopep->user1p(), سرگرم);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    فراغ driveFromComponents(رکو* placep, Ast_سرگرم* activep) {
        إذا (!placep->ورهايو) أرجع;
        AstVarScope* ثابت vscp = placep->vscp;
        File_سٽ* ثابت flp = vscp->fileline();
        لكل (سائيز_t i = 0; i < placep->childrenp.سائيز(); ++i) {
            رکو* ثابت childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ڳنڍ_اظهار* ثابت rhsp = جديد AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ڳنڍ_اظهار* ثابت lhsRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ڳنڍ_اظهار* ثابت lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->جوڙيو_Stmtsp(جديد AstAlways{جديد Ast_سونپيو_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    واضح DecomposeRewrite(State& state)
        : Decompose_بنياد{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        لكل (ثابت std::ويڪٽر<Select>& chain : m_state.chains) rewriteChain(chain);
        لكل (ثابت Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        لكل (رکو* ثابت placep : m_state.rootps) {
            إذا (!placep->ورهايو) استمر;
            ثابت AstVarScope* ثابت vscp = placep->vscp;
            // Only traced ones at this point
            إذا (!(vscp->varp()->isTrace() && vscp->isTrace())) استمر;
            Ast_سرگرم* ثابت activep = combo_سرگرم(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::جوڙيو_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::جوڙيو_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

فراغ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_بنياد::State state;
        Decompose_رڪارڊ::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_وڻ("decompose", 0, dump_وڻ_Either_Level() >= 3);
}
