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

رده DecomposeBase VL_NOT_FINAL {
محافظتشده:
    // TYPES

    // A component (element or member) of an aggregate type
    ساختار Component final {
        Ast_گره_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        صحیح declIdx;  // The declared index of the element, if an array
        صحیح lsb;  // LSB in the whole value, if packed
        صحیح msb;  // MSB in the whole value, if packed
    };

    ساختار Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ساختار Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        صحیح lsb;  // The first bit covered of this end
        صحیح otherLsb;  // The first bit covered of the other end
        صحیح width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ساختار Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_گره_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        منطقی splittable = درست;  // Can be split (there is no reason not to)
        منطقی wantsSplit = نادرست;  // Should be split (if possible, do split)
        منطقی split = نادرست;  // Place was split
        منطقی queued = نادرست;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ساختار Select final {
        Ast_گره_عبارت* exprp;  // The select
        Place* placep;  // The component
        صحیح lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ساختار Assignment final {
        Ast_گره_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

عمومی:
    // The state of the pass, shared by the steps
    رده State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ثابت VNUser4InUse m_user4InUse;

    عمومی:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_گره_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

محافظتشده:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ایستا منطقی isUnpacked(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (VN_IS(dtypep, UnpackArrayDType)) بازگشت درست;
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        بازگشت structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ایستا منطقی isPacked(ثابت Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        اگر (ثابت AstPackArrayDType* ثابت arrayp = VN_CAST(dtypep, PackArrayDType)) {
            بازگشت arrayp->subDTypep()->width() > 1;
        }
        ثابت AstStructDType* ثابت structp = VN_CAST(dtypep, StructDType);
        بازگشت structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ایستا منطقی isSplittableType(ثابت Ast_گره_DType* dtypep) {
        اگر (isPacked(dtypep)) بازگشت v3Global.opt.fDecomposePacked();
        اگر (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            اگر (ثابت AstStructDType* ثابت structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                برای (ثابت AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    اگر (mp->valuep()) بازگشت نادرست;
                }
            }
            بازگشت v3Global.opt.fDecomposeUnpacked();
        }
        بازگشت نادرست;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ایستا Ast_گره_DType* dtypeOf(ثابت Ast_گره_عبارت* nodep) {
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) بازگشت refp->varp()->dtypep();
        بازگشت nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ثابت std::vector<Component>& dtypeComponents(Ast_گره_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        اگر (compsr.empty()) {
            اگر (ثابت Ast_گره_Array_DType* ثابت arrayp = VN_CAST(dtypep, گره_Array_DType)) {
                ثابت منطقی packed = isPacked(dtypep);
                ثابت VNumRange range = arrayp->declRange();
                ثابت منطقی rev = packed && range.ascending();  // Only for naming components
                Ast_گره_DType* ثابت subp = arrayp->subDTypep();
                برای (صحیح i = 0; i < range.elements(); ++i) {
                    ثابت صحیح lsb = packed ? i * subp->width() : 0;
                    ثابت صحیح msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ثابت صحیح declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.افزودن_back({subp, nullptr, declIdx, lsb, msb});
                }
            } وگرنه {
                ثابت AstStructDType* ثابت structp = VN_AS(dtypep, StructDType);
                ثابت منطقی packed = isPacked(dtypep);
                برای (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ثابت صحیح lsb = packed ? mp->lsb() : 0;
                    ثابت صحیح msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.افزودن_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        بازگشت compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_گره_DType* dtypep, صحیح bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ثابت std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ثابت auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](ثابت Component& comp, صحیح b) {  //
                                             بازگشت b > comp.msb;
                                         });
        بازگشت it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        ثابت std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        اگر (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        اگر (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        بازگشت childpr.get();
    }

    // Is 'nodep' cheap to clone
    ایستا منطقی isCheap(ثابت Ast_گره_عبارت* nodep) {
        // Constants are cheap
        اگر (VN_IS(nodep, Const)) بازگشت درست;
        // So are resets
        اگر (VN_IS(nodep, CReset)) بازگشت درست;
        // Variable references are cheap for the most part
        اگر (ثابت AstVarRef* ثابت refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            اگر (refp->varp()->isForced()) بازگشت نادرست;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            اگر (refp->varp()->isSc()) بازگشت نادرست;
            // Otherwise the path rooted here is cheap
            بازگشت درست;
        }
        // Bit selects only with a constant LSB
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            بازگشت VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        اگر (ثابت AstArraySel* ثابت selp = VN_CAST(nodep, ArraySel)) {
            بازگشت isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        اگر (ثابت AstStructSel* ثابت selp = VN_CAST(nodep, StructSel)) {  //
            بازگشت isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        اگر (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {  //
            بازگشت isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        بازگشت نادرست;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

رده DecomposeRecord final : عمومی VNVisitorConst, عمومی DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ثابت VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_گره_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ایستا تهی warnNoSplit(AstVar* varp, ثابت Ast_گره* wherep, ثابت نویسه* reasonp) {
        // Only warn if user requested splitting
        اگر (!varp->attrSplitVar()) بازگشت;
        // A packed variable might still be split by V3BitBlast, which warns if not
        اگر (isPacked(varp->dtypep())) بازگشت;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, درست);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ایستا منطقی isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        اگر (!varp->user1()) {
            ثابت منطقی eligible = [&]() {
                // Check data type
                اگر (!isSplittableType(varp->dtypep())) بازگشت نادرست;
                // Check properties
                ثابت نویسه* ثابت reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                اگر (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                بازگشت !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        بازگشت varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(ثابت AstVarRef* refp) {
        AstVarScope* ثابت vscp = refp->varScopep();
        اگر (!isEligible(vscp->varp())) بازگشت nullptr;
        Place& place = m_state.places(vscp);
        اگر (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.افزودن_back(&place);
        }
        بازگشت &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ایستا تهی block(Place* placep, ثابت Ast_گره* wherep) {
        placep->splittable = نادرست;
        // Warn on the variable only, a component is split as far as possible, as requested
        اگر (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_گره_عبارت* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        اگر (AstArraySel* ثابت selp = VN_CAST(exprp, ArraySel)) {
            اگر (Place* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ثابت AstConst* ثابت bitp = VN_CAST(selp->bitp(), Const);
                اگر (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* ثابت childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = درست;
                    chain.افزودن_back({selp, childp, 0});
                    بازگشت childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            بازگشت nullptr;
        }
        // Struct selects correspond one to one with a component select
        اگر (AstStructSel* ثابت selp = VN_CAST(exprp, StructSel)) {
            اگر (Place* ثابت fromp = resolveChain(selp->fromp(), chain)) {
                اگر (!isUnpacked(fromp->dtypep)) بازگشت nullptr;
                AstStructDType* ثابت structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                اگر (!structp->user1SetOnce()) {
                    ثابت std::vector<Component>& compsr = dtypeComponents(structp);
                    برای (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                ثابت Ast_گره* ثابت memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* ثابت childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = درست;
                chain.افزودن_back({selp, childp, 0});
                بازگشت childp;
            }
            بازگشت nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        اگر (AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            اگر (!placep) {
                iterateConst(selp->lsbp());
                بازگشت nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const);
            اگر (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                بازگشت nullptr;
            }
            // Descend into the Place containing the bits, until exact
            صحیح lsb = lsbp->toSInt();
            صحیح msb = lsb + selp->widthConst() - 1;
            وقتی (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                اگر (!isPacked(placep->dtypep)) بازگشت nullptr;
                ثابت size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                اگر (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    بازگشت nullptr;
                }
                placep->wantsSplit = درست;
                ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.افزودن_back({selp, placep, lsb});
            }
            بازگشت placep;
        }
        // Base case
        اگر (AstVarRef* ثابت refp = VN_CAST(exprp, VarRef)) بازگشت placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        بازگشت nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    تهی referencedWhole(Ast_گره_عبارت* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        اگر (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            بازگشت;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    تهی visitSelect(Ast_گره_عبارت* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* ثابت placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        اگر (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        اگر (!chain.empty()) m_state.chains.افزودن_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ایستا منطقی hasReversedRange(ثابت Ast_گره_DType* aDTypep, ثابت Ast_گره_DType* bDTypep) {
        ثابت AstUnpackArrayDType* ثابت aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ثابت AstUnpackArrayDType* ثابت bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        اگر (!aArrayp || !bArrayp) بازگشت نادرست;
        اگر (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) بازگشت درست;
        بازگشت hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    تهی visitAssignment(Ast_گره_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ثابت منطقی reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        اگر (!m_assignp) بازگشت;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_گره_عبارت* ثابت lhsp = nodep->lhsp();
        Ast_گره_عبارت* ثابت rhsp = nodep->rhsp();
        Place* ثابت lPlacep = lhsp->user1u().to<Place*>();
        Place* ثابت rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        ثابت منطقی splittable = [&]() {
            // Assignment between two Places is splittable
            اگر (lPlacep && rPlacep) بازگشت درست;
            // If the LHS is a Place, the RHS must be a splittable expression
            اگر (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                اگر (isPacked(lPlacep->dtypep)) بازگشت درست;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                بازگشت isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            اگر (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                اگر (isPacked(rPlacep->dtypep)) بازگشت درست;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                بازگشت isCheap(lhsp);
            }
            // Otherwise not splittable
            بازگشت نادرست;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        اگر (!splittable) {
            اگر (rPlacep) block(rPlacep, rhsp);
            اگر (lPlacep) block(lPlacep, lhsp);
            بازگشت;
        }

        // If both sides are places, record the whole copies for the Decision phase
        اگر (lPlacep && rPlacep) {
            ثابت صحیح width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.افزودن_back({rPlacep, 0, 0, width});
            rPlacep->copies.افزودن_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.افزودن_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    تهی visit(Ast_گره_DType*) override {}  // No references in data types
    تهی visit(Ast_گره* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    تهی visit(AstArraySel* nodep) override { visitSelect(nodep); }
    تهی visit(AstStructSel* nodep) override { visitSelect(nodep); }
    تهی visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    تهی visit(AstAssign* nodep) override { visitAssignment(nodep); }
    تهی visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    تهی visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    تهی visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        اگر (Place* ثابت placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    تهی visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ثابت varp = nodep->varp();
        اگر (!isEligible(varp)) بازگشت;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        برای (Place* ثابت placep : m_state.rootps) {
            اگر (!isEligible(placep->vscp->varp())) placep->splittable = نادرست;
        }
    }

عمومی:
    ایستا تهی apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

رده DecomposeDecision final : عمومی DecomposeBase {
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
    ایستا منطقی canSplit(ثابت Place* placep) {
        // Not if not splittable
        اگر (!placep->splittable) بازگشت نادرست;
        // Not if already split
        اگر (placep->split) بازگشت نادرست;
        // Original variable or its parent was already split
        بازگشت !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    تهی enqueue(Place* placep) {
        اگر (placep->queued || !placep->wantsSplit || !canSplit(placep)) بازگشت;
        placep->queued = درست;
        m_worklist.افزودن_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    تهی createComponents(Place* placep) {
        AstVarScope* ثابت vscp = placep->vscp;
        AstVar* ثابت varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        اگر (varps.empty()) {
            ثابت منطقی packed = isPacked(varp->dtypep());
            اگر (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } وگرنه {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_سطر* ثابت flp = varp->fileline();
            ثابت VVarType varType = varp->varType();
            ثابت std::string name = varp->name();
            برای (ثابت Component& comp : dtypeComponents(varp->dtypep())) {
                ثابت std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_گره::encode_عدد(comp.declIdx) + "__KET__";
                AstVar* ثابت newp = جدید AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.افزودن_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ثابت scopep = vscp->scopep();
        File_سطر* ثابت flp = vscp->fileline();
        برای (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* ثابت newp = جدید AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    تهی addCopy(Place* placep, ثابت Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        اگر (!placep->splittable) بازگشت;
        // Add the copy to the Place
        placep->copies.افزودن_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        اگر (!isPacked(placep->dtypep)) بازگشت;
        اگر (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = درست;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    تهی newCopy(Place* ap, صحیح aLsb, Place* bp, صحیح bLsb, صحیح width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    تهی splitCopy(Place* placep, ثابت Copy& copy) {
        Place* ثابت otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        اگر (isPacked(placep->dtypep)) {
            ثابت std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ثابت صحیح lsb = copy.lsb;
            ثابت صحیح msb = copy.lsb + copy.width - 1;
            ثابت size_t first = componentIndex(placep->dtypep, lsb);
            برای (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                ثابت Component& comp = comps[i];
                ثابت صحیح partLsb = std::max(lsb, comp.lsb);
                ثابت صحیح partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            بازگشت;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = درست;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        ثابت size_t size = dtypeComponents(placep->dtypep).size();
        برای (size_t i = 0; i < size; ++i) {
            Place* ثابت ap = componentOf(placep, i);
            اگر (!ap->splittable) ادامه;
            Place* ثابت bp = componentOf(otherp, i);
            اگر (!bp->splittable) ادامه;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    تهی splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = درست;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ثابت size_t size = dtypeComponents(placep->dtypep).size();
        برای (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        برای (ثابت Copy& copy : placep->copies) {
            اگر (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        برای (Place* ثابت placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        وقتی (!m_worklist.empty()) {
            Place* ثابت placep = m_worklist.back();
            m_worklist.برداشت_back();
            placep->queued = نادرست;
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

عمومی:
    ایستا تهی apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

رده DecomposeRewrite final : عمومی VNDeleter, عمومی DecomposeBase {
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
    ایستا AstVarRef* newRef(File_سطر* flp, ثابت Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        بازگشت جدید AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    تهی rewriteChain(ثابت std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ثابت Select* selectp = nullptr;
        برای (ثابت Select& select : chain) {
            اگر (select.placep->parentp->split) selectp = &select;
        }
        اگر (!selectp) بازگشت;
        Ast_گره_عبارت* ثابت exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ثابت VAccess access = [&]() {
            ثابت Ast_گره_عبارت* rootp = chain.front().exprp;
            وقتی (درست) {
                اگر (ثابت AstArraySel* ثابت selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } وگرنه اگر (ثابت AstStructSel* ثابت selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } وگرنه اگر (ثابت AstSel* ثابت selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } وگرنه {
                    بازگشت VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_سطر* ثابت flp = exprp->fileline();
        Ast_گره_عبارت* newp = newRef(flp, selectp->placep, access);
        اگر (ثابت AstSel* ثابت selp = VN_CAST(exprp, Sel)) {
            ثابت صحیح width = selp->widthConst();
            اگر (width != newp->width()) newp = جدید AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(افزودن_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_گره_عبارت* newSelect(Ast_گره_عبارت* fromp, Ast_گره_DType* dtypep, size_t idx) {
        File_سطر* ثابت flp = fromp->fileline();
        ثابت Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        اگر (isPacked(dtypep)) {
            AstSel* ثابت selp = جدید AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            بازگشت selp;
        }
        // If unpacked array, it's an ArraySel
        اگر (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ثابت صحیح i = static_cast<صحیح>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            بازگشت جدید AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ثابت selp = جدید AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        بازگشت selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ایستا std::vector<Ast_گره_عبارت*> concatTerms(Ast_گره_عبارت* nodep) {
        std::vector<Ast_گره_عبارت*> termps;
        std::vector<Ast_گره_عبارت*> پشته{nodep};
        وقتی (!پشته.empty()) {
            Ast_گره_عبارت* ثابت exprp = پشته.back();
            پشته.برداشت_back();
            اگر (AstConcat* ثابت concatp = VN_CAST(exprp, Concat)) {
                پشته.افزودن_back(concatp->lhsp());
                پشته.افزودن_back(concatp->rhsp());
                ادامه;
            }
            اگر (AstReplicate* ثابت repp = VN_CAST(exprp, Replicate)) {
                ثابت AstConst* ثابت countp = VN_AS(repp->countp(), Const);
                برای (uint32_t i = 0; i < countp->toUInt(); ++i) پشته.افزودن_back(repp->srcp());
                ادامه;
            }
            termps.افزودن_back(exprp);
        }
        بازگشت termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ایستا منطقی isSliceable(ثابت Ast_گره_عبارت* nodep) {
        اگر (ثابت AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            بازگشت VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        اگر (ثابت AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            بازگشت isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        اگر (ثابت AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            بازگشت isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        اگر (ثابت AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            بازگشت isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        اگر (ثابت AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            بازگشت isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        اگر (ثابت AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            بازگشت isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        اگر (ثابت AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            بازگشت isSliceable(extp->lhsp());
        }
        اگر (ثابت AstNot* ثابت notp = VN_CAST(nodep, Not)) {  //
            بازگشت isSliceable(notp->lhsp());
        }
        اگر (ثابت AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            بازگشت isSliceable(repp->srcp());
        }
        بازگشت isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ایستا Ast_گره_عبارت* newSlice(Ast_گره_عبارت* nodep, صحیح lsb, صحیح width) {
        File_سطر* ثابت flp = nodep->fileline();
        // A constant, as a new constant of the bits
        اگر (ثابت AstConst* ثابت constp = VN_CAST(nodep, Const)) {
            V3_عدد رقم{nodep, width};
            رقم.opSel(constp->رقم(), lsb + width - 1, lsb);
            بازگشت جدید AstConst{flp, رقم};
        }
        // A constant bit select, from the bits of what it selects from
        اگر (AstSel* ثابت selp = VN_CAST(nodep, Sel)) {
            اگر (ثابت AstConst* ثابت lsbp = VN_CAST(selp->lsbp(), Const)) {
                بازگشت newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        اگر (AstConcat* ثابت catp = VN_CAST(nodep, Concat)) {
            ثابت صحیح rWidth = catp->rhsp()->width();
            ثابت صحیح msb = lsb + width - 1;
            اگر (msb < rWidth) بازگشت newSlice(catp->rhsp(), lsb, width);
            اگر (lsb >= rWidth) بازگشت newSlice(catp->lhsp(), lsb - rWidth, width);
            بازگشت جدید AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        اگر (AstCond* ثابت condp = VN_CAST(nodep, Cond)) {
            بازگشت جدید AstCond{flp, condp->condp()->clone_درخت_Pure(نادرست),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        اگر (AstAnd* ثابت andp = VN_CAST(nodep, And)) {
            بازگشت جدید AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        اگر (AstOr* ثابت orp = VN_CAST(nodep, Or)) {
            بازگشت جدید AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        اگر (AstXor* ثابت xorp = VN_CAST(nodep, Xor)) {
            بازگشت جدید AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        اگر (AstExtend* ثابت extp = VN_CAST(nodep, Extend)) {
            ثابت صحیح srcWidth = extp->lhsp()->width();
            ثابت صحیح msb = lsb + width - 1;
            اگر (msb < srcWidth) بازگشت newSlice(extp->lhsp(), lsb, width);
            اگر (lsb >= srcWidth) بازگشت جدید AstConst{flp, AstConst::Widthed_مقدار{}, width, 0};
            بازگشت جدید AstConcat{
                flp, جدید AstConst{flp, AstConst::Widthed_مقدار{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        اگر (AstNot* ثابت notp = VN_CAST(nodep, Not)) {
            بازگشت جدید AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        اگر (AstReplicate* ثابت repp = VN_CAST(nodep, Replicate)) {
            Ast_گره_عبارت* ثابت srcp = repp->srcp();
            ثابت صحیح srcWidth = srcp->width();
            ثابت صحیح msb = lsb + width - 1;
            Ast_گره_عبارت* resultp = nullptr;
            برای (صحیح copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                ثابت صحیح partLsb = std::max(lsb, copyLsb) - copyLsb;
                ثابت صحیح partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_گره_عبارت* ثابت bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? جدید AstConcat{flp, bitsp, resultp} : bitsp;
            }
            بازگشت resultp;
        }
        Ast_گره_عبارت* ثابت clonep = nodep->clone_درخت_Pure(نادرست);
        اگر (lsb == 0 && width == nodep->width()) بازگشت clonep;
        بازگشت جدید AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_گره_عبارت*> newAssignRhsps(Ast_گره_عبارت* nodep, Ast_گره_DType* dtypep) {
        ثابت std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_گره_عبارت*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        اگر (AstCReset* ثابت cresetp = VN_CAST(nodep, CReset)) {
            برای (ثابت Component& comp : compsr) {
                AstCReset* ثابت newp = cresetp->clone_درخت(نادرست);
                newp->dtypep(comp.dtypep);
                valueps.افزودن_back(newp);
            }
            بازگشت valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        اگر (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            صحیح lsb = 0;  // LSB of the current term
            برای (Ast_گره_عبارت* ثابت termp : concatTerms(nodep)) {
                ثابت صحیح msb = lsb + termp->width() - 1;
                ثابت size_t first = componentIndex(dtypep, lsb);
                برای (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    ثابت Component& comp = compsr[i];
                    ثابت صحیح partLsb = std::max(lsb, comp.lsb);
                    ثابت صحیح partMsb = std::min(msb, comp.msb);
                    File_سطر* ثابت flp = termp->fileline();
                    Ast_گره_عبارت* ثابت sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? جدید AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            بازگشت valueps;
        }

        // Otherwise unpacked, so select each component
        برای (size_t i = 0; i < compsr.size(); ++i) {
            valueps.افزودن_back(newSelect(nodep->clone_درخت(نادرست), dtypep, i));
        }
        بازگشت valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_گره_عبارت* newBits(File_سطر* flp, ثابت Place* placep, صحیح lsb, صحیح msb) {
        Ast_گره_DType* ثابت dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        اگر (!placep->split) {
            AstVarRef* ثابت refp = newRef(flp, placep, VAccess::READ);
            اگر (lsb == 0 && msb == dtypep->width() - 1) بازگشت refp;
            بازگشت جدید AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ثابت std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_گره_عبارت* resultp = nullptr;
        ثابت size_t first = componentIndex(dtypep, lsb);
        برای (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            ثابت Component& comp = comps[i];
            ثابت صحیح partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ثابت صحیح partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* ثابت childp = placep->childrenp.at(i).get();
            Ast_گره_عبارت* ثابت bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? جدید AstConcat{flp, bitsp, resultp} : bitsp;
        }
        بازگشت resultp;
    }

    ایستا تهی addNewAssign(Ast_گره_Assign* origp, Ast_گره_عبارت* lhsp, Ast_گره_عبارت* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    تهی expandSS(Ast_گره_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_سطر* ثابت flp = origp->fileline();
        ثابت std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        اگر (isPacked(rPlacep->dtypep)) {
            برای (size_t i = 0; i < compsr.size(); ++i) {
                Place* ثابت lChildp = lPlacep->childrenp.at(i).get();
                Ast_گره_عبارت* ثابت rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } وگرنه {
            برای (size_t i = 0; i < compsr.size(); ++i) {
                Place* ثابت lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    تهی expandSN(Ast_گره_Assign* origp, Place* lPlacep, Ast_گره_عبارت* rhsp) {
        ثابت std::vector<Ast_گره_عبارت*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        برای (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    تهی expandNS(Ast_گره_Assign* origp, Ast_گره_عبارت* lhsp, Place* rPlacep) {
        File_سطر* ثابت flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        اگر (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            بازگشت;
        }
        // Unpacked of the same shape, component-wise
        Ast_گره_DType* ثابت dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ثابت منطقی readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            ثابت VNUser2InUse user2InUse;
            بازگشت lhsp->exists([](ثابت AstVarRef* refp) {
                AstVarScope* ثابت vscp = refp->varScopep();
                اگر (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                اگر (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                بازگشت vscp->user2() == 3;
            });
        }();
        اگر (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ثابت scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, جدید AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, جدید AstVarRef{flp, vscp, VAccess::READ});
            بازگشت;
        }
        ثابت size_t size = dtypeComponents(dtypep).size();
        برای (size_t i = 0; i < size; ++i) {
            Ast_گره_عبارت* ثابت newLhsp = newSelect(lhsp->clone_درخت(نادرست), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    تهی expand(Ast_گره_Assign* origp,  //
                Place* lPlacep, Ast_گره_عبارت* lhsp,  //
                Place* rPlacep, Ast_گره_عبارت* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_سطر* ثابت flp = origp->fileline();
        // A Place not split is referenced
        اگر (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        اگر (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        اگر (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } وگرنه اگر (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } وگرنه اگر (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } وگرنه {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    منطقی isSplitBetween(ثابت Place* placep, صحیح lsb, صحیح msb) {
        وقتی (placep->split) {
            ثابت size_t idx = componentIndex(placep->dtypep, lsb);
            اگر (idx != componentIndex(placep->dtypep, msb)) بازگشت درست;
            ثابت Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        بازگشت نادرست;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    تهی hoistTerms(Ast_گره_Assign* origp, ثابت Place* lPlacep) {
        Ast_گره_عبارت* ثابت rhsp = origp->rhsp();
        ثابت std::vector<Ast_گره_عبارت*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ثابت VNUser2InUse user2InUse;
        برای (Ast_گره_عبارت* ثابت termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
        صحیح lsb = 0;  // LSB of the current term
        برای (Ast_گره_عبارت* ثابت termp : termps) {
            ثابت صحیح tLsb = lsb;
            ثابت صحیح tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            اگر (!termp->user2()) ادامه;
            // Cheap to select from
            اگر (isCheap(termp)) {
                termp->user2(0);
                ادامه;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            اگر (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                ادامه;
            }
            // Bits can be selected without evaluating it more than once
            اگر (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ادامه;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_سطر* ثابت flp = termp->fileline();
            AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(جدید AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ثابت tmpRefp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(جدید AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ایستا تهی markComponents(ثابت Place* placep) {
        برای (ثابت std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(درست);
            اگر (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    تهی rewriteAssignment(ثابت Assignment& assignment) {
        Place* ثابت lPlacep = assignment.lPlacep;
        Place* ثابت rPlacep = assignment.rPlacep;
        ثابت منطقی lSplit = lPlacep && lPlacep->split;
        ثابت منطقی rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        اگر (!lSplit && !rSplit) بازگشت;

        Ast_گره_Assign* ثابت origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        اگر (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            ثابت VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_گره_عبارت* ثابت rhsp = origp->rhsp();
            اگر (rhsp->exists([](ثابت AstVarRef* refp) { بازگشت refp->varScopep()->user2(); })) {
                File_سطر* ثابت flp = rhsp->fileline();
                AstScope* ثابت scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ثابت vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(جدید AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ثابت tmpRefp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(جدید AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        اگر (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_گره_عبارت* ثابت lhsp = origp->lhsp()->unlinkFrBack();
        Ast_گره_عبارت* ثابت rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        اگر (lSplit) VL_DO_DANGLING(افزودن_Deletep(lhsp), lhsp);
        اگر (rSplit) VL_DO_DANGLING(افزودن_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(افزودن_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    ایستا AstActive* comboActive(AstScope* scopep) {
        اگر (!scopep->user1p()) scopep->user1p(scopep->comboActivep(درست));
        بازگشت VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    تهی driveFromComponents(Place* placep, AstActive* activep) {
        اگر (!placep->split) بازگشت;
        AstVarScope* ثابت vscp = placep->vscp;
        File_سطر* ثابت flp = vscp->fileline();
        برای (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* ثابت childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_گره_عبارت* ثابت rhsp = جدید AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_گره_عبارت* ثابت lhsRefp = جدید AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_گره_عبارت* ثابت lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(جدید AstAlways{جدید AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        برای (ثابت std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        برای (ثابت Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        برای (Place* ثابت placep : m_state.rootps) {
            اگر (!placep->split) ادامه;
            ثابت AstVarScope* ثابت vscp = placep->vscp;
            // Only traced ones at this point
            اگر (!(vscp->varp()->isTrace() && vscp->isTrace())) ادامه;
            AstActive* ثابت activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

عمومی:
    ایستا تهی apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

تهی V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_درخت("decompose", 0, dump_درخت_Either_Level() >= 3);
}
