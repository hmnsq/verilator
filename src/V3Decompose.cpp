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

صنف DecomposeBase VL_NOT_FINAL {
محمي:
    // TYPES

    // A component (element or member) of an aggregate type
    بنية Component final {
        Ast_نود_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        صحيح declIdx;  // The declared index of the element, if an array
        صحيح lsb;  // LSB in the whole value, if packed
        صحيح msb;  // MSB in the whole value, if packed
    };

    بنية Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    بنية Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        صحيح lsb;  // The first bit covered of this end
        صحيح otherLsb;  // The first bit covered of the other end
        صحيح width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    بنية Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_نود_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        منطقي splittable = true;  // Can be split (there is no reason not to)
        منطقي wantsSplit = خطأ;  // Should be split (if possible, do split)
        منطقي split = خطأ;  // Place was split
        منطقي queued = خطأ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    بنية Select final {
        Ast_نود_اوڠکاڤن* exprp;  // The select
        Place* placep;  // The component
        صحيح lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    بنية Assignment final {
        Ast_نود_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

عام:
    // The state of the pass, shared by the steps
    صنف State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ثابت VNUser4InUse m_user4InUse;

    عام:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_نود_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

محمي:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ساكن منطقي isUnpacked(ثابت Ast_نود_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (VN_IS(dtypep, UnpackArrayDType)) أرجع true;
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ساكن منطقي isPacked(ثابت Ast_نود_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        إذا (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            أرجع arrayp->subDTypep()->width() > 1;
        }
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        أرجع structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ساكن منطقي isSplittableType(ثابت Ast_نود_DType* dtypep) {
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
    ساكن Ast_نود_DType* dtypeOf(ثابت Ast_نود_اوڠکاڤن* nodep) {
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) أرجع refp->varp()->dtypep();
        أرجع nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ثابت std::vector<Component>& dtypeComponents(Ast_نود_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        إذا (compsr.empty()) {
            إذا (ثابت Ast_نود_Array_DType* ثابت arrayp = VN_CAST(dtypep, نود_Array_DType)) {
                ثابت منطقي packed = isPacked(dtypep);
                ثابت VNumRange range = arrayp->declRange();
                ثابت منطقي rev = packed && range.ascending();  // Only for naming components
                Ast_نود_DType* ثابت subp = arrayp->subDTypep();
                لكل (صحيح i = 0; i < range.elements(); ++i) {
                    ثابت صحيح lsb = packed ? i * subp->width() : 0;
                    ثابت صحيح msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ثابت صحيح declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.تولق_back({subp, nullptr, declIdx, lsb, msb});
                }
            } وإلا {
                ثابت AstStructDType* ثابت structp = VN_AS(dtypep, StructDType);
                ثابت منطقي packed = isPacked(dtypep);
                لكل (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ثابت صحيح lsb = packed ? mp->lsb() : 0;
                    ثابت صحيح msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.تولق_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        أرجع compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_نود_DType* dtypep, صحيح bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ثابت std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ثابت auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ثابت Component& comp, صحيح b) {  //
                                             أرجع b > comp.msb;
                                         });
        أرجع it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ثابت std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        إذا (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        إذا (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        أرجع childpr.get();
    }

    // Is 'nodep' cheap to clone
    ساكن منطقي isCheap(ثابت Ast_نود_اوڠکاڤن* nodep) {
        // Constants are cheap
        إذا (VN_IS(nodep, Const)) أرجع true;
        // So are resets
        إذا (VN_IS(nodep, CReset)) أرجع true;
        // Variable references are cheap for the most part
        إذا (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            إذا (refp->varp()->isForced()) أرجع خطأ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            إذا (refp->varp()->isSc()) أرجع خطأ;
            // Otherwise the path rooted here is cheap
            أرجع true;
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

صنف DecomposeRecord final : عام VNVisitorConst, عام DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_نود_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ساكن فراغ warnNoSplit(AstVar* varp, ثابت Ast_نود* wherep, ثابت حرف* reasonp) {
        // Only warn if user requested splitting
        إذا (!varp->attrSplitVar()) أرجع;
        // A packed variable might still be split by V3BitBlast, which warns if not
        إذا (isPacked(varp->dtypep())) أرجع;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, true);  // Warn only once
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
                ثابت حرف* ثابت reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                إذا (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                أرجع !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        أرجع varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        إذا (!isEligible(vscp->varp())) أرجع nullptr;
        Place& place = m_state.places(vscp);
        إذا (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.تولق_back(&place);
        }
        أرجع &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ساكن فراغ block(Place* placep, ثابت Ast_نود* wherep) {
        placep->splittable = خطأ;
        // Warn on the variable only, a component is split as far as possible, as requested
        إذا (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_نود_اوڠکاڤن* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        إذا (AstArraySel* ثابت selp = VN_CAST(exprp, ArraySel)) {
            إذا (Place* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ثابت AstConst* ثابت bitp = VN_CAST(selp->bitp(), Const);
                إذا (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ثابت childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = true;
                    chain.تولق_back({selp, childp, 0});
                    أرجع childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            أرجع nullptr;
        }
        // Struct selects correspond one to one with a component select
        إذا (AstStructSel* ثابت selp = VN_CAST(exprp, StructSel)) {
            إذا (Place* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                إذا (!isUnpacked(fromp->dtypep)) أرجع nullptr;
                AstStructDType* ثابت structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                إذا (!structp->user1SetOnce()) {
                    ثابت std::vector<Component>& compsr = dtypeComponents(structp);
                    لكل (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ثابت Ast_نود* ثابت memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ثابت childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = true;
                chain.تولق_back({selp, childp, 0});
                أرجع childp;
            }
            أرجع nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        إذا (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            إذا (!placep) {
                iterateConst(selp->lsbp());
                أرجع nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            إذا (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                أرجع nullptr;
            }
            // Descend into the Place containing the bits, until exact
            صحيح lsb = lsbp->toSInt();
            صحيح msb = lsb + selp->widthConst() - 1;
            طالما (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                إذا (!isPacked(placep->dtypep)) أرجع nullptr;
                ثابت size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                إذا (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    أرجع nullptr;
                }
                placep->wantsSplit = true;
                ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.تولق_back({selp, placep, lsb});
            }
            أرجع placep;
        }
        // Base case
        إذا (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) أرجع placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        أرجع nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    فراغ referencedWhole(Ast_نود_اوڠکاڤن* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        إذا (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            أرجع;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    فراغ visitSelect(Ast_نود_اوڠکاڤن* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ثابت placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        إذا (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        إذا (!chain.empty()) m_state.chains.تولق_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ساكن منطقي hasReversedRange(ثابت Ast_نود_DType* aDTypep, ثابت Ast_نود_DType* bDTypep) {
        ثابت AstUnpackArrayDType* ثابت aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        إذا (!aArrayp || !bArrayp) أرجع خطأ;
        إذا (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) أرجع true;
        أرجع hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    فراغ visitAssignment(Ast_نود_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ثابت منطقي reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        إذا (!m_assignp) أرجع;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_نود_اوڠکاڤن* ثابت lhsp = nodep->lhsp();
        Ast_نود_اوڠکاڤن* ثابت rhsp = nodep->rhsp();
        Place* ثابت lPlacep = lhsp->user1u().to<Place*>();
        Place* ثابت rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ثابت منطقي splittable = [&]() {
            // Assignment between two Places is splittable
            إذا (lPlacep && rPlacep) أرجع true;
            // If the LHS is a Place, the RHS must be a splittable expression
            إذا (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                إذا (isPacked(lPlacep->dtypep)) أرجع true;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                أرجع isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            إذا (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                إذا (isPacked(rPlacep->dtypep)) أرجع true;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                أرجع isCheap(lhsp);
            }
            // Otherwise not splittable
            أرجع خطأ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        إذا (!splittable) {
            إذا (rPlacep) block(rPlacep, rhsp);
            إذا (lPlacep) block(lPlacep, lhsp);
            أرجع;
        }

        // If both sides are places, record the whole copies for the Decision phase
        إذا (lPlacep && rPlacep) {
            ثابت صحيح width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.تولق_back({rPlacep, 0, 0, width});
            rPlacep->copies.تولق_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.تولق_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    فراغ visit(Ast_نود_DType*) override {}  // No references in data types
    فراغ visit(Ast_نود* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    فراغ visit(AstArraySel* nodep) override { visitSelect(nodep); }
    فراغ visit(AstStructSel* nodep) override { visitSelect(nodep); }
    فراغ visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    فراغ visit(AstAssign* nodep) override { visitAssignment(nodep); }
    فراغ visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    فراغ visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    فراغ visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        إذا (Place* ثابت placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    فراغ visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ثابت varp = nodep->varp();
        إذا (!isEligible(varp)) أرجع;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        لكل (Place* ثابت placep : m_state.rootps) {
            إذا (!isEligible(placep->vscp->varp())) placep->splittable = خطأ;
        }
    }

عام:
    ساكن فراغ apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

صنف DecomposeDecision final : عام DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    std::vector<Place*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::vector<AstVar*>> m_splitVarps;
    VDouble0 m_statSplitUnpackedOrig;  // Original AstVars of unpacked type split
    VDouble0 m_statSplitPackedOrig;  // Original AstVars of packed type split
    VDouble0 m_statSplitUnpackedComp;  // Component AstVars of unpacked type split
    VDouble0 m_statSplitPackedComp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ساكن منطقي canSplit(ثابت Place* placep) {
        // Not if not splittable
        إذا (!placep->splittable) أرجع خطأ;
        // Not if already split
        إذا (placep->split) أرجع خطأ;
        // Original variable or its parent was already split
        أرجع !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    فراغ enqueue(Place* placep) {
        إذا (placep->queued || !placep->wantsSplit || !canSplit(placep)) أرجع;
        placep->queued = true;
        m_worklist.تولق_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    فراغ createComponents(Place* placep) {
        AstVarScope* ثابت vscp = placep->vscp;
        AstVar* ثابت varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        إذا (varps.empty()) {
            ثابت منطقي packed = isPacked(varp->dtypep());
            إذا (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } وإلا {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_باريس* ثابت flp = varp->fileline();
            ثابت VVarType varType = varp->varType();
            ثابت std::string name = varp->name();
            لكل (ثابت Component& comp : dtypeComponents(varp->dtypep())) {
                ثابت std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_نود::encode_نومبور(comp.declIdx) + "__KET__";
                AstVar* ثابت newp = جديد AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.تولق_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ثابت scopep = vscp->scopep();
        File_باريس* ثابت flp = vscp->fileline();
        لكل (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ثابت newp = جديد AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    فراغ addCopy(Place* placep, ثابت Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        إذا (!placep->splittable) أرجع;
        // Add the copy to the Place
        placep->copies.تولق_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        إذا (!isPacked(placep->dtypep)) أرجع;
        إذا (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = true;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    فراغ newCopy(Place* ap, صحيح aLsb, Place* bp, صحيح bLsb, صحيح width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    فراغ splitCopy(Place* placep, ثابت Copy& copy) {
        Place* ثابت otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        إذا (isPacked(placep->dtypep)) {
            ثابت std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ثابت صحيح lsb = copy.lsb;
            ثابت صحيح msb = copy.lsb + copy.width - 1;
            ثابت size_t first = componentIndex(placep->dtypep, lsb);
            لكل (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ثابت Component& comp = comps[i];
                ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                ثابت صحيح partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            أرجع;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = true;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ثابت size_t size = dtypeComponents(placep->dtypep).size();
        لكل (size_t i = 0; i < size; ++i) {
            Place* ثابت ap = componentOf(placep, i);
            إذا (!ap->splittable) استمر;
            Place* ثابت bp = componentOf(otherp, i);
            إذا (!bp->splittable) استمر;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    فراغ splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = true;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ثابت size_t size = dtypeComponents(placep->dtypep).size();
        لكل (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        لكل (ثابت Copy& copy : placep->copies) {
            إذا (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        لكل (Place* ثابت placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        طالما (!m_worklist.empty()) {
            Place* ثابت placep = m_worklist.back();
            m_worklist.کلوار_back();
            placep->queued = خطأ;
            splitPlace(placep);
        }

        V3Stats::addStat("Optimizations, Decompose, unpacked variables split",
                         m_statSplitUnpackedOrig);
        V3Stats::addStat("Optimizations, Decompose, packed variables split",
                         m_statSplitPackedOrig);
        V3Stats::addStat("Optimizations, Decompose, unpacked components split further",
                         m_statSplitUnpackedComp);
        V3Stats::addStat("Optimizations, Decompose, packed components split further",
                         m_statSplitPackedComp);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

صنف DecomposeRewrite final : عام VNDeleter, عام DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ساكن AstVarRef* newRef(File_باريس* flp, ثابت Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        أرجع جديد AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    فراغ rewriteChain(ثابت std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ثابت Select* selectp = nullptr;
        لكل (ثابت Select& select : chain) {
            إذا (select.placep->parentp->split) selectp = &select;
        }
        إذا (!selectp) أرجع;
        Ast_نود_اوڠکاڤن* ثابت exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ثابت VAccess access = [&]() {
            ثابت Ast_نود_اوڠکاڤن* rootp = chain.front().exprp;
            طالما (true) {
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
        File_باريس* ثابت flp = exprp->fileline();
        Ast_نود_اوڠکاڤن* newp = newRef(flp, selectp->placep, access);
        إذا (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ثابت صحيح width = selp->widthConst();
            إذا (width != newp->width()) newp = جديد AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(تولق_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_نود_اوڠکاڤن* newSelect(Ast_نود_اوڠکاڤن* fromp, Ast_نود_DType* dtypep, size_t idx) {
        File_باريس* ثابت flp = fromp->fileline();
        ثابت Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        إذا (isPacked(dtypep)) {
            AstSel* ثابت selp = جديد AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            أرجع selp;
        }
        // If unpacked array, it's an ArraySel
        إذا (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ثابت صحيح i = static_cast<صحيح>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            أرجع جديد AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ثابت selp = جديد AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        أرجع selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ساكن std::vector<Ast_نود_اوڠکاڤن*> concatTerms(Ast_نود_اوڠکاڤن* nodep) {
        std::vector<Ast_نود_اوڠکاڤن*> termps;
        std::vector<Ast_نود_اوڠکاڤن*> تيندنن{nodep};
        طالما (!تيندنن.empty()) {
            Ast_نود_اوڠکاڤن* ثابت exprp = تيندنن.back();
            تيندنن.کلوار_back();
            إذا (AstConcat* ثابت concatp = VN_CAST(exprp, Concat)) {
                تيندنن.تولق_back(concatp->lhsp());
                تيندنن.تولق_back(concatp->rhsp());
                استمر;
            }
            إذا (AstReplicate* ثابت repp = VN_CAST(exprp, Replicate)) {
                ثابت AstConst* ثابت countp = VN_AS(repp->countp(), Const);
                لكل (uint32_t i = 0; i < countp->toUInt(); ++i) تيندنن.تولق_back(repp->srcp());
                استمر;
            }
            termps.تولق_back(exprp);
        }
        أرجع termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ساكن منطقي isSliceable(ثابت Ast_نود_اوڠکاڤن* nodep) {
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
    ساكن Ast_نود_اوڠکاڤن* newSlice(Ast_نود_اوڠکاڤن* nodep, صحيح lsb, صحيح width) {
        File_باريس* ثابت flp = nodep->fileline();
        // A constant, as a new constant of the bits
        إذا (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            V3_نومبور اڠک{nodep, width};
            اڠک.opSel(constp->اڠک(), lsb + width - 1, lsb);
            أرجع جديد AstConst{flp, اڠک};
        }
        // A constant bit select, from the bits of what it selects from
        إذا (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            إذا (ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                أرجع newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        إذا (AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            ثابت صحيح rWidth = catp->rhsp()->width();
            ثابت صحيح msb = lsb + width - 1;
            إذا (msb < rWidth) أرجع newSlice(catp->rhsp(), lsb, width);
            إذا (lsb >= rWidth) أرجع newSlice(catp->lhsp(), lsb - rWidth, width);
            أرجع جديد AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        إذا (AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            أرجع جديد AstCond{flp, condp->condp()->clone_ڤوکوق_Pure(خطأ),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        إذا (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            أرجع جديد AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        إذا (AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            أرجع جديد AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        إذا (AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            أرجع جديد AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        إذا (AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            ثابت صحيح srcWidth = extp->lhsp()->width();
            ثابت صحيح msb = lsb + width - 1;
            إذا (msb < srcWidth) أرجع newSlice(extp->lhsp(), lsb, width);
            إذا (lsb >= srcWidth) أرجع جديد AstConst{flp, AstConst::Widthed_نيلاي{}, width, 0};
            أرجع جديد AstConcat{
                flp, جديد AstConst{flp, AstConst::Widthed_نيلاي{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        إذا (AstNot* ثابت notp = VN_CAST(nodep, Not)) {
            أرجع جديد AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        إذا (AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            Ast_نود_اوڠکاڤن* ثابت srcp = repp->srcp();
            ثابت صحيح srcWidth = srcp->width();
            ثابت صحيح msb = lsb + width - 1;
            Ast_نود_اوڠکاڤن* resultp = nullptr;
            لكل (صحيح copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ثابت صحيح partLsb = std::max(lsb, copyLsb) - copyLsb;
                ثابت صحيح partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_نود_اوڠکاڤن* ثابت bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
            }
            أرجع resultp;
        }
        Ast_نود_اوڠکاڤن* ثابت clonep = nodep->clone_ڤوکوق_Pure(خطأ);
        إذا (lsb == 0 && width == nodep->width()) أرجع clonep;
        أرجع جديد AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_نود_اوڠکاڤن*> newAssignRhsps(Ast_نود_اوڠکاڤن* nodep, Ast_نود_DType* dtypep) {
        ثابت std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_نود_اوڠکاڤن*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        إذا (AstCReset* ثابت cresetp = VN_CAST(nodep, CReset)) {
            لكل (ثابت Component& comp : compsr) {
                AstCReset* ثابت newp = cresetp->clone_ڤوکوق(خطأ);
                newp->dtypep(comp.dtypep);
                valueps.تولق_back(newp);
            }
            أرجع valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        إذا (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            صحيح lsb = 0;  // LSB of the current term
            لكل (Ast_نود_اوڠکاڤن* ثابت termp : concatTerms(nodep)) {
                ثابت صحيح msb = lsb + termp->width() - 1;
                ثابت size_t first = componentIndex(dtypep, lsb);
                لكل (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ثابت Component& comp = compsr[i];
                    ثابت صحيح partLsb = std::max(lsb, comp.lsb);
                    ثابت صحيح partMsb = std::min(msb, comp.msb);
                    File_باريس* ثابت flp = termp->fileline();
                    Ast_نود_اوڠکاڤن* ثابت sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? جديد AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            أرجع valueps;
        }

        // Otherwise unpacked, so select each component
        لكل (size_t i = 0; i < compsr.size(); ++i) {
            valueps.تولق_back(newSelect(nodep->clone_ڤوکوق(خطأ), dtypep, i));
        }
        أرجع valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_نود_اوڠکاڤن* newBits(File_باريس* flp, ثابت Place* placep, صحيح lsb, صحيح msb) {
        Ast_نود_DType* ثابت dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        إذا (!placep->split) {
            AstVarRef* ثابت refp = newRef(flp, placep, VAccess::READ);
            إذا (lsb == 0 && msb == dtypep->width() - 1) أرجع refp;
            أرجع جديد AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ثابت std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_نود_اوڠکاڤن* resultp = nullptr;
        ثابت size_t first = componentIndex(dtypep, lsb);
        لكل (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ثابت Component& comp = comps[i];
            ثابت صحيح partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ثابت صحيح partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ثابت childp = placep->childrenp.at(i).get();
            Ast_نود_اوڠکاڤن* ثابت bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? جديد AstConcat{flp, bitsp, resultp} : bitsp;
        }
        أرجع resultp;
    }

    ساكن فراغ addNewAssign(Ast_نود_Assign* origp, Ast_نود_اوڠکاڤن* lhsp, Ast_نود_اوڠکاڤن* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    فراغ expandSS(Ast_نود_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_باريس* ثابت flp = origp->fileline();
        ثابت std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        إذا (isPacked(rPlacep->dtypep)) {
            لكل (size_t i = 0; i < compsr.size(); ++i) {
                Place* ثابت lChildp = lPlacep->childrenp.at(i).get();
                Ast_نود_اوڠکاڤن* ثابت rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } وإلا {
            لكل (size_t i = 0; i < compsr.size(); ++i) {
                Place* ثابت lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    فراغ expandSN(Ast_نود_Assign* origp, Place* lPlacep, Ast_نود_اوڠکاڤن* rhsp) {
        ثابت std::vector<Ast_نود_اوڠکاڤن*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        لكل (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(تولق_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    فراغ expandNS(Ast_نود_Assign* origp, Ast_نود_اوڠکاڤن* lhsp, Place* rPlacep) {
        File_باريس* ثابت flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        إذا (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            أرجع;
        }
        // Unpacked of the same shape, component-wise
        Ast_نود_DType* ثابت dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ثابت منطقي readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ثابت VNUser2InUse user2InUse;
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
            expand(origp, nullptr, جديد AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, جديد AstVarRef{flp, vscp, VAccess::READ});
            أرجع;
        }
        ثابت size_t size = dtypeComponents(dtypep).size();
        لكل (size_t i = 0; i < size; ++i) {
            Ast_نود_اوڠکاڤن* ثابت newLhsp = newSelect(lhsp->clone_ڤوکوق(خطأ), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(تولق_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    فراغ expand(Ast_نود_Assign* origp,  //
                Place* lPlacep, Ast_نود_اوڠکاڤن* lhsp,  //
                Place* rPlacep, Ast_نود_اوڠکاڤن* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_باريس* ثابت flp = origp->fileline();
        // A Place not split is referenced
        إذا (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        إذا (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        إذا (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } وإلا إذا (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } وإلا إذا (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } وإلا {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    منطقي isSplitBetween(ثابت Place* placep, صحيح lsb, صحيح msb) {
        طالما (placep->split) {
            ثابت size_t idx = componentIndex(placep->dtypep, lsb);
            إذا (idx != componentIndex(placep->dtypep, msb)) أرجع true;
            ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        أرجع خطأ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    فراغ hoistTerms(Ast_نود_Assign* origp, ثابت Place* lPlacep) {
        Ast_نود_اوڠکاڤن* ثابت rhsp = origp->rhsp();
        ثابت std::vector<Ast_نود_اوڠکاڤن*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ثابت VNUser2InUse user2InUse;
        لكل (Ast_نود_اوڠکاڤن* ثابت termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
        صحيح lsb = 0;  // LSB of the current term
        لكل (Ast_نود_اوڠکاڤن* ثابت termp : termps) {
            ثابت صحيح tLsb = lsb;
            ثابت صحيح tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            إذا (!termp->user2()) استمر;
            // Cheap to select from
            إذا (isCheap(termp)) {
                termp->user2(0);
                استمر;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            إذا (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
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
            File_باريس* ثابت flp = termp->fileline();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(جديد AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ساكن فراغ markComponents(ثابت Place* placep) {
        لكل (ثابت std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(true);
            إذا (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    فراغ rewriteAssignment(ثابت Assignment& assignment) {
        Place* ثابت lPlacep = assignment.lPlacep;
        Place* ثابت rPlacep = assignment.rPlacep;
        ثابت منطقي lSplit = lPlacep && lPlacep->split;
        ثابت منطقي rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        إذا (!lSplit && !rSplit) أرجع;

        Ast_نود_Assign* ثابت origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        إذا (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ثابت VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_نود_اوڠکاڤن* ثابت rhsp = origp->rhsp();
            إذا (rhsp->exists([](ثابت AstVarRef* refp) { أرجع refp->varScopep()->user2(); })) {
                File_باريس* ثابت flp = rhsp->fileline();
                AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(جديد AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ثابت tmpRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(جديد AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        إذا (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_نود_اوڠکاڤن* ثابت lhsp = origp->lhsp()->unlinkFrBack();
        Ast_نود_اوڠکاڤن* ثابت rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        إذا (lSplit) VL_DO_DANGLING(تولق_Deletep(lhsp), lhsp);
        إذا (rSplit) VL_DO_DANGLING(تولق_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(تولق_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ساكن AstActive* comboActive(AstScope* scopep) {
        إذا (!scopep->user1p()) scopep->user1p(scopep->comboActivep(true));
        أرجع VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    فراغ driveFromComponents(Place* placep, AstActive* activep) {
        إذا (!placep->split) أرجع;
        AstVarScope* ثابت vscp = placep->vscp;
        File_باريس* ثابت flp = vscp->fileline();
        لكل (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ثابت childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_نود_اوڠکاڤن* ثابت rhsp = جديد AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_نود_اوڠکاڤن* ثابت lhsRefp = جديد AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_نود_اوڠکاڤن* ثابت lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(جديد AstAlways{جديد AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        لكل (ثابت std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        لكل (ثابت Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        لكل (Place* ثابت placep : m_state.rootps) {
            إذا (!placep->split) استمر;
            ثابت AstVarScope* ثابت vscp = placep->vscp;
            // Only traced ones at this point
            إذا (!(vscp->varp()->isTrace() && vscp->isTrace())) استمر;
            AstActive* ثابت activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

عام:
    ساكن فراغ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

فراغ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ڤوکوق("decompose", 0, dump_ڤوکوق_Either_Level() >= 3);
}
