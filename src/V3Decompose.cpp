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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Decompose.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3AstUserAllocator.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3MemberMap.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3SharedTmps.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Stats.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <algorithm>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

ਵਰਗ Decompose_ਅਧਾਰ VL_NOT_FINAL {
ਸੁਰੱਖਿਅਤ:
    // TYPES

    // A component (element or member) of an aggregate type
    ਬਣਤਰ Component final {
        Ast_ਗੰਢ_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        ਪੂਰਨ declIdx;  // The declared index of the element, if an array
        ਪੂਰਨ lsb;  // LSB in the whole value, if packed
        ਪੂਰਨ msb;  // MSB in the whole value, if packed
    };

    ਬਣਤਰ ਰੱਖੋ;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ਬਣਤਰ Copy final {
        ਰੱਖੋ* otherp;  // The Place at the other end
        // Following only for packed places
        ਪੂਰਨ lsb;  // The first bit covered of this end
        ਪੂਰਨ otherLsb;  // The first bit covered of the other end
        ਪੂਰਨ ਚੌੜਾਈ;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ਬਣਤਰ ਰੱਖੋ final {
        ਰੱਖੋ* parentp = ਖਾਲੀ_ਸੂਚਕ;  // The Place this is a component of, nullptr if original
        ਰੱਖੋ* rootp = ਖਾਲੀ_ਸੂਚਕ;  // The original variable (root of the tree), root points to itself
        Ast_ਗੰਢ_DType* dtypep = ਖਾਲੀ_ਸੂਚਕ;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = ਖਾਲੀ_ਸੂਚਕ;
        // The component Places created so far, by index, sized on the first one created
        std::ਸਦਿਸ਼<std::unique_ptr<ਰੱਖੋ>> childrenp;
        std::ਸਦਿਸ਼<Copy> copies;  // The copies covering it, until split
        ਤਾਰਕਿਕ splittable = ਸੱਚ;  // Can be split (there is no reason not to)
        ਤਾਰਕਿਕ ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਝੂਠ;  // Should be split (if possible, do split)
        ਤਾਰਕਿਕ ਵੰਡੋ = ਝੂਠ;  // Place was split
        ਤਾਰਕਿਕ queued = ਝੂਠ;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ਬਣਤਰ Select final {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp;  // The select
        ਰੱਖੋ* placep;  // The component
        ਪੂਰਨ lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ਬਣਤਰ Assignment final {
        Ast_ਗੰਢ_ਸੌਂਪੋ* assp;  // The assignment
        ਰੱਖੋ* lPlacep;  // The Place the LHS addresses exactly, if any
        ਰੱਖੋ* rPlacep;  // The Place the RHS addresses exactly, if any
    };

ਜਨਤਕ:
    // The state of the pass, shared by the steps
    ਵਰਗ State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        ਅਚਲ VNUser4_ਵਿੱਚ_Use m_user4_ਵਿੱਚ_Use;

    ਜਨਤਕ:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_ਗੰਢ_DType, std::ਸਦਿਸ਼<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, ਰੱਖੋ> places;
        std::ਸਦਿਸ਼<ਰੱਖੋ*> rootps;  // The original Places, in creation order
        std::ਸਦਿਸ਼<std::ਸਦਿਸ਼<Select>> chains;  // Select chains addressing components
        std::ਸਦਿਸ਼<Assignment> assignments;  // Assignments to expand if a side is split
    };

ਸੁਰੱਖਿਅਤ:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    ਸਪਸ਼ਟ Decompose_ਅਧਾਰ(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    ਸਥਿਰ ਤਾਰਕਿਕ isUnpacked(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (VN_IS(dtypep, UnpackArrayDType)) ਵਾਪਸ ਸੱਚ;
        ਅਚਲ AstStructDType* ਅਚਲ structp = VN_CAST(dtypep, StructDType);
        ਵਾਪਸ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    ਸਥਿਰ ਤਾਰਕਿਕ isPacked(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        ਜੇ (ਅਚਲ AstPackArrayDType* ਅਚਲ arrayp = VN_CAST(dtypep, PackArrayDType)) {
            ਵਾਪਸ arrayp->subDTypep()->ਚੌੜਾਈ() > 1;
        }
        ਅਚਲ AstStructDType* ਅਚਲ structp = VN_CAST(dtypep, StructDType);
        ਵਾਪਸ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    ਸਥਿਰ ਤਾਰਕਿਕ isSplittableType(ਅਚਲ Ast_ਗੰਢ_DType* dtypep) {
        ਜੇ (isPacked(dtypep)) ਵਾਪਸ v3Global.opt.fDecomposePacked();
        ਜੇ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            ਜੇ (ਅਚਲ AstStructDType* ਅਚਲ structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                ਲਈ (ਅਚਲ AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ਜੇ (mp->valuep()) ਵਾਪਸ ਝੂਠ;
                }
            }
            ਵਾਪਸ v3Global.opt.fDecomposeUnpacked();
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    ਸਥਿਰ Ast_ਗੰਢ_DType* dtype_ਦਾ(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) ਵਾਪਸ refp->varp()->dtypep();
        ਵਾਪਸ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    ਅਚਲ std::ਸਦਿਸ਼<Component>& dtypeComponents(Ast_ਗੰਢ_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::ਸਦਿਸ਼<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        ਜੇ (compsr.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ arrayp = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                ਅਚਲ ਤਾਰਕਿਕ packed = isPacked(dtypep);
                ਅਚਲ VNumRange range = arrayp->declRange();
                ਅਚਲ ਤਾਰਕਿਕ rev = packed && range.ascending();  // Only for naming components
                Ast_ਗੰਢ_DType* ਅਚਲ subp = arrayp->subDTypep();
                ਲਈ (ਪੂਰਨ i = 0; i < range.elements(); ++i) {
                    ਅਚਲ ਪੂਰਨ lsb = packed ? i * subp->ਚੌੜਾਈ() : 0;
                    ਅਚਲ ਪੂਰਨ msb = packed ? lsb + subp->ਚੌੜਾਈ() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    ਅਚਲ ਪੂਰਨ declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.ਧੱਕੋ_ਪਿੱਠ({subp, ਖਾਲੀ_ਸੂਚਕ, declIdx, lsb, msb});
                }
            } ਨਹੀਂਤਾਂ {
                ਅਚਲ AstStructDType* ਅਚਲ structp = VN_AS(dtypep, StructDType);
                ਅਚਲ ਤਾਰਕਿਕ packed = isPacked(dtypep);
                ਲਈ (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    ਅਚਲ ਪੂਰਨ lsb = packed ? mp->lsb() : 0;
                    ਅਚਲ ਪੂਰਨ msb = packed ? lsb + mp->ਚੌੜਾਈ() - 1 : 0;
                    compsr.ਧੱਕੋ_ਪਿੱਠ({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::ਉਲਟ(compsr.begin(), compsr.ਸਿਰਾ());
            }
        }

        // The components of this data type
        ਵਾਪਸ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    ਮਾਪ_t component_ਸੂਚਕਾਂਕ(Ast_ਗੰਢ_DType* dtypep, ਪੂਰਨ ਦੋਆਧਾਰੀ_ਅੰਕ_2) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        ਅਚਲ std::ਸਦਿਸ਼<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = std::ਹੇਠਲਾ_bound(compsr.begin(), compsr.ਸਿਰਾ(), ਦੋਆਧਾਰੀ_ਅੰਕ_2,
                                         [](ਅਚਲ Component& comp, ਪੂਰਨ b) {  //
                                             ਵਾਪਸ b > comp.msb;
                                         });
        ਵਾਪਸ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    ਰੱਖੋ* component_ਦਾ(ਰੱਖੋ* placep, ਮਾਪ_t idx) {
        ਅਚਲ std::ਸਦਿਸ਼<Component>& comps = dtypeComponents(placep->dtypep);
        ਜੇ (placep->childrenp.ਖਾਲੀ_ਥਾਵਾਂ()) placep->childrenp.resize(comps.ਮਾਪ());
        std::unique_ptr<ਰੱਖੋ>& childpr = placep->childrenp.at(idx);
        ਜੇ (!childpr) {
            childpr = std::make_unique<ਰੱਖੋ>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->ਚਾਹੁੰਦਾ_ਵੰਡੋ = placep->rootp->vscp->varp()->attr_ਵੰਡੋ_Var();
        }
        ਵਾਪਸ childpr.get();
    }

    // Is 'nodep' cheap to clone
    ਸਥਿਰ ਤਾਰਕਿਕ isCheap(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Constants are cheap
        ਜੇ (VN_IS(nodep, Const)) ਵਾਪਸ ਸੱਚ;
        // So are resets
        ਜੇ (VN_IS(nodep, CReset)) ਵਾਪਸ ਸੱਚ;
        // Variable references are cheap for the most part
        ਜੇ (ਅਚਲ AstVarRef* ਅਚਲ refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            ਜੇ (refp->varp()->isForced()) ਵਾਪਸ ਝੂਠ;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            ਜੇ (refp->varp()->isSc()) ਵਾਪਸ ਝੂਠ;
            // Otherwise the path rooted here is cheap
            ਵਾਪਸ ਸੱਚ;
        }
        // Bit selects only with a constant LSB
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਵਾਪਸ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ selp = VN_CAST(nodep, ArraySel)) {
            ਵਾਪਸ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(nodep, StructSel)) {  //
            ਵਾਪਸ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        ਜੇ (ਅਚਲ AstExtend* ਅਚਲ extp = VN_CAST(nodep, Extend)) {  //
            ਵਾਪਸ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        ਵਾਪਸ ਝੂਠ;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

ਵਰਗ Decompose_ਰਿਕਾਰਡ final : ਜਨਤਕ VNVisitorConst, ਜਨਤਕ Decompose_ਅਧਾਰ {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_ਗੰਢ_ਸੌਂਪੋ* m_assignp = ਖਾਲੀ_ਸੂਚਕ;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    ਸਥਿਰ ਖਾਲੀ warn_No_ਵੰਡੋ(AstVar* varp, ਅਚਲ Ast_ਗੰਢ* wherep, ਅਚਲ ਅੱਖਰ* reasonp) {
        // Only warn if user requested splitting
        ਜੇ (!varp->attr_ਵੰਡੋ_Var()) ਵਾਪਸ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        ਜੇ (isPacked(varp->dtypep())) ਵਾਪਸ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3_Error_ਕੋਡ::SPLITVAR, ਸੱਚ);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    ਸਥਿਰ ਤਾਰਕਿਕ isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        ਜੇ (!varp->user1()) {
            ਅਚਲ ਤਾਰਕਿਕ eligible = [&]() {
                // Check data type
                ਜੇ (!isSplittableType(varp->dtypep())) ਵਾਪਸ ਝੂਠ;
                // Check properties
                ਅਚਲ ਅੱਖਰ* ਅਚਲ reasonp = varp->cannot_ਵੰਡੋ_ਕਿਸਮ_Reason();
                // Warn that it cannot be split if user explicitly requested splitting
                ਜੇ (reasonp) warn_No_ਵੰਡੋ(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                ਵਾਪਸ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        ਵਾਪਸ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    ਰੱਖੋ* ਰੱਖੋ_ਦਾ(ਅਚਲ AstVarRef* refp) {
        AstVarScope* ਅਚਲ vscp = refp->varScopep();
        ਜੇ (!isEligible(vscp->varp())) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        ਰੱਖੋ& ਰੱਖੋ_2 = m_state.places(vscp);
        ਜੇ (!ਰੱਖੋ_2.rootp) {
            ਰੱਖੋ_2.rootp = &ਰੱਖੋ_2;
            ਰੱਖੋ_2.dtypep = vscp->varp()->dtypep();
            ਰੱਖੋ_2.vscp = vscp;
            ਰੱਖੋ_2.ਚਾਹੁੰਦਾ_ਵੰਡੋ = vscp->varp()->attr_ਵੰਡੋ_Var();
            m_state.rootps.ਧੱਕੋ_ਪਿੱਠ(&ਰੱਖੋ_2);
        }
        ਵਾਪਸ &ਰੱਖੋ_2;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    ਸਥਿਰ ਖਾਲੀ ਖੰਡ(ਰੱਖੋ* placep, ਅਚਲ Ast_ਗੰਢ* wherep) {
        placep->splittable = ਝੂਠ;
        // Warn on the variable only, a component is split as far as possible, as requested
        ਜੇ (!placep->parentp) {
            warn_No_ਵੰਡੋ(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    ਰੱਖੋ* resolveChain(Ast_ਗੰਢ_ਵਿਅੰਜਕ* exprp, std::ਸਦਿਸ਼<Select>& chain) {
        // Array selects correspond one to one with a component select
        ਜੇ (AstArraySel* ਅਚਲ selp = VN_CAST(exprp, ArraySel)) {
            ਜੇ (ਰੱਖੋ* ਅਚਲ fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                ਅਚਲ AstConst* ਅਚਲ bitp = VN_CAST(selp->bitp(), Const);
                ਜੇ (bitp && bitp->ਮੰਜ਼ਿਲ_UQuad() < dtypeComponents(fromp->dtypep).ਮਾਪ()) {
                    ਰੱਖੋ* ਅਚਲ childp = component_ਦਾ(fromp, bitp->ਮੰਜ਼ਿਲ_UQuad());
                    fromp->ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਸੱਚ;
                    chain.ਧੱਕੋ_ਪਿੱਠ({selp, childp, 0});
                    ਵਾਪਸ childp;
                }
                // Otherwise block splitting of the Place
                ਖੰਡ(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // Struct selects correspond one to one with a component select
        ਜੇ (AstStructSel* ਅਚਲ selp = VN_CAST(exprp, StructSel)) {
            ਜੇ (ਰੱਖੋ* ਅਚਲ fromp = resolveChain(selp->fromp(), chain)) {
                ਜੇ (!isUnpacked(fromp->dtypep)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                AstStructDType* ਅਚਲ structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                ਜੇ (!structp->user1_ਸੈੱਟ_Once()) {
                    ਅਚਲ std::ਸਦਿਸ਼<Component>& compsr = dtypeComponents(structp);
                    ਲਈ (ਮਾਪ_t i = 0; i < compsr.ਮਾਪ(); ++i) compsr[i].memberp->user1(i);
                }
                ਅਚਲ Ast_ਗੰਢ* ਅਚਲ memberp = m_memberMap.ਲੱਭੋ_Member(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                ਰੱਖੋ* ਅਚਲ childp = component_ਦਾ(fromp, memberp->user1());
                fromp->ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਸੱਚ;
                chain.ਧੱਕੋ_ਪਿੱਠ({selp, childp, 0});
                ਵਾਪਸ childp;
            }
            ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(exprp, Sel)) {
            ਰੱਖੋ* placep = resolveChain(selp->fromp(), chain);
            ਜੇ (!placep) {
                iterateConst(selp->lsbp());
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(selp->lsbp(), Const);
            ਜੇ (!lsbp || lsbp->ਮੰਜ਼ਿਲ_SInt() + selp->ਚੌੜਾਈ_Const() > placep->dtypep->ਚੌੜਾਈ()) {
                ਖੰਡ(placep, selp->fromp());
                iterateConst(selp->lsbp());
                ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
            }
            // Descend into the Place containing the bits, until exact
            ਪੂਰਨ lsb = lsbp->ਮੰਜ਼ਿਲ_SInt();
            ਪੂਰਨ msb = lsb + selp->ਚੌੜਾਈ_Const() - 1;
            ਜਦੋਂ (lsb != 0 || msb != placep->dtypep->ਚੌੜਾਈ() - 1) {
                // Within a component of a type not split
                ਜੇ (!isPacked(placep->dtypep)) ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                ਅਚਲ ਮਾਪ_t idx = component_ਸੂਚਕਾਂਕ(placep->dtypep, lsb);
                // Crossing components prevents splitting
                ਜੇ (idx != component_ਸੂਚਕਾਂਕ(placep->dtypep, msb)) {
                    ਖੰਡ(placep, selp->fromp());
                    ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
                }
                placep->ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਸੱਚ;
                ਅਚਲ Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = component_ਦਾ(placep, idx);
                chain.ਧੱਕੋ_ਪਿੱਠ({selp, placep, lsb});
            }
            ਵਾਪਸ placep;
        }
        // Base case
        ਜੇ (AstVarRef* ਅਚਲ refp = VN_CAST(exprp, VarRef)) ਵਾਪਸ ਰੱਖੋ_ਦਾ(refp);
        // Not a select chain
        iterateConst(exprp);
        ਵਾਪਸ ਖਾਲੀ_ਸੂਚਕ;
    }

    // 'nodep' references the Place 'placep' whole
    ਖਾਲੀ referenced_ਸਾਰਾ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਰੱਖੋ* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        ਜੇ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            ਵਾਪਸ;
        }

        // Otherwise must block splitting of this Place
        ਖੰਡ(placep, nodep);
    }

    // Visit a select expression
    ਖਾਲੀ ਦੇਖੋ_Select(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::ਸਦਿਸ਼<Select> chain;
        ਰੱਖੋ* ਅਚਲ placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        ਜੇ (placep) referenced_ਸਾਰਾ(nodep, placep);
        // Record the chain for the rewriting phase
        ਜੇ (!chain.ਖਾਲੀ_ਥਾਵਾਂ()) m_state.chains.ਧੱਕੋ_ਪਿੱਠ(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    ਸਥਿਰ ਤਾਰਕਿਕ hasReversedRange(ਅਚਲ Ast_ਗੰਢ_DType* aDTypep, ਅਚਲ Ast_ਗੰਢ_DType* bDTypep) {
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        ਜੇ (!aArrayp || !bArrayp) ਵਾਪਸ ਝੂਠ;
        ਜੇ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) ਵਾਪਸ ਸੱਚ;
        ਵਾਪਸ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    ਖਾਲੀ ਦੇਖੋ_Assignment(Ast_ਗੰਢ_ਸੌਂਪੋ* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        ਅਚਲ ਤਾਰਕਿਕ reversed = hasReversedRange(dtype_ਦਾ(nodep->lhsp()), dtype_ਦਾ(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? ਖਾਲੀ_ਸੂਚਕ : nodep;
        iterateChildrenConst(nodep);
        ਜੇ (!m_assignp) ਵਾਪਸ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = nodep->lhsp();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = nodep->rhsp();
        ਰੱਖੋ* ਅਚਲ lPlacep = lhsp->user1u().ਮੰਜ਼ਿਲ<ਰੱਖੋ*>();
        ਰੱਖੋ* ਅਚਲ rPlacep = rhsp->user1u().ਮੰਜ਼ਿਲ<ਰੱਖੋ*>();

        // Is it a splittable assignment
        ਅਚਲ ਤਾਰਕਿਕ splittable = [&]() {
            // Assignment between two Places is splittable
            ਜੇ (lPlacep && rPlacep) ਵਾਪਸ ਸੱਚ;
            // If the LHS is a Place, the RHS must be a splittable expression
            ਜੇ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                ਜੇ (isPacked(lPlacep->dtypep)) ਵਾਪਸ ਸੱਚ;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                ਵਾਪਸ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            ਜੇ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                ਜੇ (isPacked(rPlacep->dtypep)) ਵਾਪਸ ਸੱਚ;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                ਵਾਪਸ isCheap(lhsp);
            }
            // Otherwise not splittable
            ਵਾਪਸ ਝੂਠ;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        ਜੇ (!splittable) {
            ਜੇ (rPlacep) ਖੰਡ(rPlacep, rhsp);
            ਜੇ (lPlacep) ਖੰਡ(lPlacep, lhsp);
            ਵਾਪਸ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        ਜੇ (lPlacep && rPlacep) {
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = lPlacep->dtypep->ਚੌੜਾਈ();  // Unused when unpacked
            lPlacep->copies.ਧੱਕੋ_ਪਿੱਠ({rPlacep, 0, 0, ਚੌੜਾਈ});
            rPlacep->copies.ਧੱਕੋ_ਪਿੱਠ({lPlacep, 0, 0, ਚੌੜਾਈ});
        }
        // Record for the rewriting phase
        m_state.assignments.ਧੱਕੋ_ਪਿੱਠ({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_DType*) override {}  // No references in data types
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    ਖਾਲੀ ਦੇਖੋ(AstArraySel* nodep) override { ਦੇਖੋ_Select(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstStructSel* nodep) override { ਦੇਖੋ_Select(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override { ਦੇਖੋ_Select(nodep); }
    // VISITORS - Assignments recordable as Copy
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_W* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override { ਦੇਖੋ_Assignment(nodep); }
    // VISITORS - References to Places
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        ਜੇ (ਰੱਖੋ* ਅਚਲ placep = ਰੱਖੋ_ਦਾ(nodep)) referenced_ਸਾਰਾ(nodep, placep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (!isEligible(varp)) ਵਾਪਸ;
        varp->user1(2);  // Mark not eligible
        warn_No_ਵੰਡੋ(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    Decompose_ਰਿਕਾਰਡ(State& state, AstNetlist* netlistp)
        : Decompose_ਅਧਾਰ{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        ਲਈ (ਰੱਖੋ* ਅਚਲ placep : m_state.rootps) {
            ਜੇ (!isEligible(placep->vscp->varp())) placep->splittable = ਝੂਠ;
        }
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(State& state, AstNetlist* netlistp) { Decompose_ਰਿਕਾਰਡ{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

ਵਰਗ DecomposeDecision final : ਜਨਤਕ Decompose_ਅਧਾਰ {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    std::ਸਦਿਸ਼<ਰੱਖੋ*> m_worklist;  // Places to consider splitting
    // The component AstVars of the AstVars, see the node state
    AstUser1Allocator<AstVar, std::ਸਦਿਸ਼<AstVar*>> m_ਵੰਡੋ_Varps;
    VDouble0 m_stat_ਵੰਡੋ_Unpacked_Orig;  // Original AstVars of unpacked type split
    VDouble0 m_stat_ਵੰਡੋ_Packed_Orig;  // Original AstVars of packed type split
    VDouble0 m_stat_ਵੰਡੋ_Unpacked_Comp;  // Component AstVars of unpacked type split
    VDouble0 m_stat_ਵੰਡੋ_Packed_Comp;  // Component AstVars of packed type split

    // METHODS

    // Can the Place be split, as far as known
    ਸਥਿਰ ਤਾਰਕਿਕ can_ਵੰਡੋ(ਅਚਲ ਰੱਖੋ* placep) {
        // Not if not splittable
        ਜੇ (!placep->splittable) ਵਾਪਸ ਝੂਠ;
        // Not if already split
        ਜੇ (placep->ਵੰਡੋ) ਵਾਪਸ ਝੂਠ;
        // Original variable or its parent was already split
        ਵਾਪਸ !placep->parentp || placep->parentp->ਵੰਡੋ;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    ਖਾਲੀ ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(ਰੱਖੋ* placep) {
        ਜੇ (placep->queued || !placep->ਚਾਹੁੰਦਾ_ਵੰਡੋ || !can_ਵੰਡੋ(placep)) ਵਾਪਸ;
        placep->queued = ਸੱਚ;
        m_worklist.ਧੱਕੋ_ਪਿੱਠ(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    ਖਾਲੀ createComponents(ਰੱਖੋ* placep) {
        AstVarScope* ਅਚਲ vscp = placep->vscp;
        AstVar* ਅਚਲ varp = vscp->varp();
        std::ਸਦਿਸ਼<AstVar*>& varps = m_ਵੰਡੋ_Varps(varp);
        // Create the component AstVars on first encounter
        ਜੇ (varps.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਅਚਲ ਤਾਰਕਿਕ packed = isPacked(varp->dtypep());
            ਜੇ (placep->parentp) {
                ++(packed ? m_stat_ਵੰਡੋ_Packed_Comp : m_stat_ਵੰਡੋ_Unpacked_Comp);
            } ਨਹੀਂਤਾਂ {
                ++(packed ? m_stat_ਵੰਡੋ_Packed_Orig : m_stat_ਵੰਡੋ_Unpacked_Orig);
            }
            File_ਸਤਰ* ਅਚਲ flp = varp->fileline();
            ਅਚਲ VVarType varType = varp->varType();
            ਅਚਲ std::string name = varp->name();
            ਲਈ (ਅਚਲ Component& comp : dtypeComponents(varp->dtypep())) {
                ਅਚਲ std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_ਗੰਢ::encode_ਸੰਖਿਆ(comp.declIdx) + "__KET__";
                AstVar* ਅਚਲ newp = ਨਵਾਂ AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagate_ਵੰਡੋ_Attr_From(varp);
                varps.ਧੱਕੋ_ਪਿੱਠ(newp);
                varp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* ਅਚਲ scopep = vscp->scopep();
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        ਲਈ (ਮਾਪ_t i = 0; i < varps.ਮਾਪ(); ++i) {
            AstVarScope* ਅਚਲ newp = ਨਵਾਂ AstVarScope{flp, scopep, varps[i]};
            component_ਦਾ(placep, i)->vscp = newp;
            vscp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    ਖਾਲੀ ਜੋੜੋ_Copy(ਰੱਖੋ* placep, ਅਚਲ Copy& copy) {
        UASSERT(!placep->ਵੰਡੋ, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        ਜੇ (!placep->splittable) ਵਾਪਸ;
        // Add the copy to the Place
        placep->copies.ਧੱਕੋ_ਪਿੱਠ(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        ਜੇ (!isPacked(placep->dtypep)) ਵਾਪਸ;
        ਜੇ (copy.lsb != 0 || copy.ਚੌੜਾਈ != placep->dtypep->ਚੌੜਾਈ()) {
            placep->ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਸੱਚ;
            ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    ਖਾਲੀ newCopy(ਰੱਖੋ* ap, ਪੂਰਨ aLsb, ਰੱਖੋ* bp, ਪੂਰਨ bLsb, ਪੂਰਨ ਚੌੜਾਈ) {
        UASSERT(ap != bp, "Copy within a Place");
        ਜੋੜੋ_Copy(ap, {bp, aLsb, bLsb, ਚੌੜਾਈ});
        ਜੋੜੋ_Copy(bp, {ap, bLsb, aLsb, ਚੌੜਾਈ});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    ਖਾਲੀ ਵੰਡੋ_Copy(ਰੱਖੋ* placep, ਅਚਲ Copy& copy) {
        ਰੱਖੋ* ਅਚਲ otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        ਜੇ (isPacked(placep->dtypep)) {
            ਅਚਲ std::ਸਦਿਸ਼<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            ਅਚਲ ਪੂਰਨ lsb = copy.lsb;
            ਅਚਲ ਪੂਰਨ msb = copy.lsb + copy.ਚੌੜਾਈ - 1;
            ਅਚਲ ਮਾਪ_t ਪਹਿਲਾ = component_ਸੂਚਕਾਂਕ(placep->dtypep, lsb);
            ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ; i < comps.ਮਾਪ() && comps[i].lsb <= msb; ++i) {
                ਅਚਲ Component& comp = comps[i];
                ਅਚਲ ਪੂਰਨ partLsb = std::max(lsb, comp.lsb);
                ਅਚਲ ਪੂਰਨ partMsb = std::min(msb, comp.msb);
                newCopy(component_ਦਾ(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            ਵਾਪਸ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->ਚਾਹੁੰਦਾ_ਵੰਡੋ = ਸੱਚ;
        ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(otherp);
        // The sides are the same shape, so pairwise whole copies
        ਅਚਲ ਮਾਪ_t ਮਾਪ = dtypeComponents(placep->dtypep).ਮਾਪ();
        ਲਈ (ਮਾਪ_t i = 0; i < ਮਾਪ; ++i) {
            ਰੱਖੋ* ਅਚਲ ap = component_ਦਾ(placep, i);
            ਜੇ (!ap->splittable) ਜਾਰੀ;
            ਰੱਖੋ* ਅਚਲ bp = component_ਦਾ(otherp, i);
            ਜੇ (!bp->splittable) ਜਾਰੀ;
            newCopy(ap, 0, bp, 0, ap->dtypep->ਚੌੜਾਈ());
        }
    }

    // Split the Place
    ਖਾਲੀ ਵੰਡੋ_ਰੱਖੋ(ਰੱਖੋ* placep) {
        UASSERT(!placep->ਵੰਡੋ, "Place split twice");
        // It is now split
        placep->ਵੰਡੋ = ਸੱਚ;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        ਅਚਲ ਮਾਪ_t ਮਾਪ = dtypeComponents(placep->dtypep).ਮਾਪ();
        ਲਈ (ਮਾਪ_t i = 0; i < ਮਾਪ; ++i) ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(component_ਦਾ(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        ਲਈ (ਅਚਲ Copy& copy : placep->copies) {
            ਜੇ (!copy.otherp->ਵੰਡੋ) ਵੰਡੋ_Copy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ DecomposeDecision(State& state)
        : Decompose_ਅਧਾਰ{state} {
        // Enqueue the original variables
        ਲਈ (ਰੱਖੋ* ਅਚਲ placep : m_state.rootps) ਕਤਾਰ_ਵਿੱਚ_ਪਾਓ(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ਜਦੋਂ (!m_worklist.ਖਾਲੀ_ਥਾਵਾਂ()) {
            ਰੱਖੋ* ਅਚਲ placep = m_worklist.ਪਿੱਠ();
            m_worklist.ਕੱਢੋ_ਪਿੱਠ();
            placep->queued = ਝੂਠ;
            ਵੰਡੋ_ਰੱਖੋ(placep);
        }

        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, unpacked variables split",
                         m_stat_ਵੰਡੋ_Unpacked_Orig);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, packed variables split",
                         m_stat_ਵੰਡੋ_Packed_Orig);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, unpacked components split further",
                         m_stat_ਵੰਡੋ_Unpacked_Comp);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, packed components split further",
                         m_stat_ਵੰਡੋ_Packed_Comp);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

ਵਰਗ DecomposeRewrite final : ਜਨਤਕ VNDeleter, ਜਨਤਕ Decompose_ਅਧਾਰ {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_user1_ਵਿੱਚ_Use;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    ਸਥਿਰ AstVarRef* newRef(File_ਸਤਰ* flp, ਅਚਲ ਰੱਖੋ* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        ਵਾਪਸ ਨਵਾਂ AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    ਖਾਲੀ rewriteChain(ਅਚਲ std::ਸਦਿਸ਼<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        ਅਚਲ Select* selectp = ਖਾਲੀ_ਸੂਚਕ;
        ਲਈ (ਅਚਲ Select& select : chain) {
            ਜੇ (select.placep->parentp->ਵੰਡੋ) selectp = &select;
        }
        ਜੇ (!selectp) ਵਾਪਸ;
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        ਅਚਲ VAccess access = [&]() {
            ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* rootp = chain.front().exprp;
            ਜਦੋਂ (ਸੱਚ) {
                ਜੇ (ਅਚਲ AstArraySel* ਅਚਲ selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstStructSel* ਅਚਲ selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } ਨਹੀਂਤਾਂ ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } ਨਹੀਂਤਾਂ {
                    ਵਾਪਸ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_ਸਤਰ* ਅਚਲ flp = exprp->fileline();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* newp = newRef(flp, selectp->placep, access);
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(exprp, Sel)) {
            ਅਚਲ ਪੂਰਨ ਚੌੜਾਈ = selp->ਚੌੜਾਈ_Const();
            ਜੇ (ਚੌੜਾਈ != newp->ਚੌੜਾਈ()) newp = ਨਵਾਂ AstSel{flp, newp, selectp->lsb, ਚੌੜਾਈ};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* newSelect(Ast_ਗੰਢ_ਵਿਅੰਜਕ* fromp, Ast_ਗੰਢ_DType* dtypep, ਮਾਪ_t idx) {
        File_ਸਤਰ* ਅਚਲ flp = fromp->fileline();
        ਅਚਲ Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        ਜੇ (isPacked(dtypep)) {
            AstSel* ਅਚਲ selp = ਨਵਾਂ AstSel{flp, fromp, comp.lsb, comp.dtypep->ਚੌੜਾਈ()};
            selp->dtypep(comp.dtypep);
            ਵਾਪਸ selp;
        }
        // If unpacked array, it's an ArraySel
        ਜੇ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            ਅਚਲ ਪੂਰਨ i = static_cast<ਪੂਰਨ>(idx);
            UASSERT_OBJ(static_cast<ਮਾਪ_t>(i) == idx, fromp, "Index overflow");
            ਵਾਪਸ ਨਵਾਂ AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* ਅਚਲ selp = ਨਵਾਂ AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        ਵਾਪਸ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    ਸਥਿਰ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> concatTerms(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> termps;
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> ਢੇਰ{nodep};
        ਜਦੋਂ (!ਢੇਰ.ਖਾਲੀ_ਥਾਵਾਂ()) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ exprp = ਢੇਰ.ਪਿੱਠ();
            ਢੇਰ.ਕੱਢੋ_ਪਿੱਠ();
            ਜੇ (AstConcat* ਅਚਲ concatp = VN_CAST(exprp, Concat)) {
                ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(concatp->lhsp());
                ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(concatp->rhsp());
                ਜਾਰੀ;
            }
            ਜੇ (AstReplicate* ਅਚਲ repp = VN_CAST(exprp, Replicate)) {
                ਅਚਲ AstConst* ਅਚਲ countp = VN_AS(repp->countp(), Const);
                ਲਈ (uint32_t i = 0; i < countp->ਮੰਜ਼ਿਲ_UInt(); ++i) ਢੇਰ.ਧੱਕੋ_ਪਿੱਠ(repp->srcp());
                ਜਾਰੀ;
            }
            termps.ਧੱਕੋ_ਪਿੱਠ(exprp);
        }
        ਵਾਪਸ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    ਸਥਿਰ ਤਾਰਕਿਕ isSliceable(ਅਚਲ Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) {
        ਜੇ (ਅਚਲ AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਵਾਪਸ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        ਜੇ (ਅਚਲ AstConcat* ਅਚਲ catp = VN_CAST(nodep, Concat)) {
            ਵਾਪਸ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        ਜੇ (ਅਚਲ AstCond* ਅਚਲ condp = VN_CAST(nodep, Cond)) {
            ਵਾਪਸ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        ਜੇ (ਅਚਲ AstAnd* ਅਚਲ andp = VN_CAST(nodep, And)) {
            ਵਾਪਸ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        ਜੇ (ਅਚਲ AstOr* ਅਚਲ orp = VN_CAST(nodep, Or)) {
            ਵਾਪਸ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        ਜੇ (ਅਚਲ AstXor* ਅਚਲ xorp = VN_CAST(nodep, Xor)) {
            ਵਾਪਸ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        ਜੇ (ਅਚਲ AstExtend* ਅਚਲ extp = VN_CAST(nodep, Extend)) {
            ਵਾਪਸ isSliceable(extp->lhsp());
        }
        ਜੇ (ਅਚਲ AstNot* ਅਚਲ notp = VN_CAST(nodep, Not)) {  //
            ਵਾਪਸ isSliceable(notp->lhsp());
        }
        ਜੇ (ਅਚਲ AstReplicate* ਅਚਲ repp = VN_CAST(nodep, Replicate)) {
            ਵਾਪਸ isSliceable(repp->srcp());
        }
        ਵਾਪਸ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    ਸਥਿਰ Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਟੁਕੜਾ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, ਪੂਰਨ lsb, ਪੂਰਨ ਚੌੜਾਈ) {
        File_ਸਤਰ* ਅਚਲ flp = nodep->fileline();
        // A constant, as a new constant of the bits
        ਜੇ (ਅਚਲ AstConst* ਅਚਲ constp = VN_CAST(nodep, Const)) {
            V3_ਸੰਖਿਆ ਅੰਕ{nodep, ਚੌੜਾਈ};
            ਅੰਕ.opSel(constp->ਅੰਕ(), lsb + ਚੌੜਾਈ - 1, lsb);
            ਵਾਪਸ ਨਵਾਂ AstConst{flp, ਅੰਕ};
        }
        // A constant bit select, from the bits of what it selects from
        ਜੇ (AstSel* ਅਚਲ selp = VN_CAST(nodep, Sel)) {
            ਜੇ (ਅਚਲ AstConst* ਅਚਲ lsbp = VN_CAST(selp->lsbp(), Const)) {
                ਵਾਪਸ new_ਟੁਕੜਾ(selp->fromp(), lsbp->ਮੰਜ਼ਿਲ_SInt() + lsb, ਚੌੜਾਈ);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        ਜੇ (AstConcat* ਅਚਲ catp = VN_CAST(nodep, Concat)) {
            ਅਚਲ ਪੂਰਨ r_ਚੌੜਾਈ = catp->rhsp()->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
            ਜੇ (msb < r_ਚੌੜਾਈ) ਵਾਪਸ new_ਟੁਕੜਾ(catp->rhsp(), lsb, ਚੌੜਾਈ);
            ਜੇ (lsb >= r_ਚੌੜਾਈ) ਵਾਪਸ new_ਟੁਕੜਾ(catp->lhsp(), lsb - r_ਚੌੜਾਈ, ਚੌੜਾਈ);
            ਵਾਪਸ ਨਵਾਂ AstConcat{flp, new_ਟੁਕੜਾ(catp->lhsp(), 0, msb - r_ਚੌੜਾਈ + 1),
                                 new_ਟੁਕੜਾ(catp->rhsp(), lsb, r_ਚੌੜਾਈ - lsb)};
        }
        ਜੇ (AstCond* ਅਚਲ condp = VN_CAST(nodep, Cond)) {
            ਵਾਪਸ ਨਵਾਂ AstCond{flp, condp->condp()->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ),
                               new_ਟੁਕੜਾ(condp->thenp(), lsb, ਚੌੜਾਈ),
                               new_ਟੁਕੜਾ(condp->elsep(), lsb, ਚੌੜਾਈ)};
        }
        ਜੇ (AstAnd* ਅਚਲ andp = VN_CAST(nodep, And)) {
            ਵਾਪਸ ਨਵਾਂ AstAnd{flp, new_ਟੁਕੜਾ(andp->lhsp(), lsb, ਚੌੜਾਈ),
                              new_ਟੁਕੜਾ(andp->rhsp(), lsb, ਚੌੜਾਈ)};
        }
        ਜੇ (AstOr* ਅਚਲ orp = VN_CAST(nodep, Or)) {
            ਵਾਪਸ ਨਵਾਂ AstOr{flp, new_ਟੁਕੜਾ(orp->lhsp(), lsb, ਚੌੜਾਈ),
                             new_ਟੁਕੜਾ(orp->rhsp(), lsb, ਚੌੜਾਈ)};
        }
        ਜੇ (AstXor* ਅਚਲ xorp = VN_CAST(nodep, Xor)) {
            ਵਾਪਸ ਨਵਾਂ AstXor{flp, new_ਟੁਕੜਾ(xorp->lhsp(), lsb, ਚੌੜਾਈ),
                              new_ਟੁਕੜਾ(xorp->rhsp(), lsb, ਚੌੜਾਈ)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        ਜੇ (AstExtend* ਅਚਲ extp = VN_CAST(nodep, Extend)) {
            ਅਚਲ ਪੂਰਨ src_ਚੌੜਾਈ = extp->lhsp()->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
            ਜੇ (msb < src_ਚੌੜਾਈ) ਵਾਪਸ new_ਟੁਕੜਾ(extp->lhsp(), lsb, ਚੌੜਾਈ);
            ਜੇ (lsb >= src_ਚੌੜਾਈ) ਵਾਪਸ ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, ਚੌੜਾਈ, 0};
            ਵਾਪਸ ਨਵਾਂ AstConcat{
                flp, ਨਵਾਂ AstConst{flp, AstConst::Widthed_ਮੁੱਲ{}, msb - src_ਚੌੜਾਈ + 1, 0},
                new_ਟੁਕੜਾ(extp->lhsp(), lsb, src_ਚੌੜਾਈ - lsb)};
        }
        ਜੇ (AstNot* ਅਚਲ notp = VN_CAST(nodep, Not)) {
            ਵਾਪਸ ਨਵਾਂ AstNot{flp, new_ਟੁਕੜਾ(notp->lhsp(), lsb, ਚੌੜਾਈ)};
        }
        // A replication, from the portions of the copies overlapping the bits
        ਜੇ (AstReplicate* ਅਚਲ repp = VN_CAST(nodep, Replicate)) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ srcp = repp->srcp();
            ਅਚਲ ਪੂਰਨ src_ਚੌੜਾਈ = srcp->ਚੌੜਾਈ();
            ਅਚਲ ਪੂਰਨ msb = lsb + ਚੌੜਾਈ - 1;
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
            ਲਈ (ਪੂਰਨ copyLsb = lsb / src_ਚੌੜਾਈ * src_ਚੌੜਾਈ; copyLsb <= msb; copyLsb += src_ਚੌੜਾਈ) {
                ਅਚਲ ਪੂਰਨ partLsb = std::max(lsb, copyLsb) - copyLsb;
                ਅਚਲ ਪੂਰਨ partMsb = std::min(msb, copyLsb + src_ਚੌੜਾਈ - 1) - copyLsb;
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bitsp = new_ਟੁਕੜਾ(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? ਨਵਾਂ AstConcat{flp, bitsp, resultp} : bitsp;
            }
            ਵਾਪਸ resultp;
        }
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ clonep = nodep->clone_ਰੁੱਖ_ਸ਼ੁੱਧ(ਝੂਠ);
        ਜੇ (lsb == 0 && ਚੌੜਾਈ == nodep->ਚੌੜਾਈ()) ਵਾਪਸ clonep;
        ਵਾਪਸ ਨਵਾਂ AstSel{flp, clonep, lsb, ਚੌੜਾਈ};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> new_ਸੌਂਪੋ_Rhsps(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep, Ast_ਗੰਢ_DType* dtypep) {
        ਅਚਲ std::ਸਦਿਸ਼<Component>& compsr = dtypeComponents(dtypep);
        std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> valueps;
        valueps.reserve(compsr.ਮਾਪ());

        // If CReset, it is duplicated for each component
        ਜੇ (AstCReset* ਅਚਲ cresetp = VN_CAST(nodep, CReset)) {
            ਲਈ (ਅਚਲ Component& comp : compsr) {
                AstCReset* ਅਚਲ newp = cresetp->clone_ਰੁੱਖ(ਝੂਠ);
                newp->dtypep(comp.dtypep);
                valueps.ਧੱਕੋ_ਪਿੱਠ(newp);
            }
            ਵਾਪਸ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        ਜੇ (isPacked(dtypep)) {
            valueps.resize(compsr.ਮਾਪ(), ਖਾਲੀ_ਸੂਚਕ);
            ਪੂਰਨ lsb = 0;  // LSB of the current term
            ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp : concatTerms(nodep)) {
                ਅਚਲ ਪੂਰਨ msb = lsb + termp->ਚੌੜਾਈ() - 1;
                ਅਚਲ ਮਾਪ_t ਪਹਿਲਾ = component_ਸੂਚਕਾਂਕ(dtypep, lsb);
                ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ; i < compsr.ਮਾਪ() && compsr[i].lsb <= msb; ++i) {
                    ਅਚਲ Component& comp = compsr[i];
                    ਅਚਲ ਪੂਰਨ partLsb = std::max(lsb, comp.lsb);
                    ਅਚਲ ਪੂਰਨ partMsb = std::min(msb, comp.msb);
                    File_ਸਤਰ* ਅਚਲ flp = termp->fileline();
                    Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ sp = new_ਟੁਕੜਾ(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? ਨਵਾਂ AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            ਵਾਪਸ valueps;
        }

        // Otherwise unpacked, so select each component
        ਲਈ (ਮਾਪ_t i = 0; i < compsr.ਮਾਪ(); ++i) {
            valueps.ਧੱਕੋ_ਪਿੱਠ(newSelect(nodep->clone_ਰੁੱਖ(ਝੂਠ), dtypep, i));
        }
        ਵਾਪਸ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_ਬਿੱਟ(File_ਸਤਰ* flp, ਅਚਲ ਰੱਖੋ* placep, ਪੂਰਨ lsb, ਪੂਰਨ msb) {
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->ਵੰਡੋ, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        ਜੇ (!placep->ਵੰਡੋ) {
            AstVarRef* ਅਚਲ refp = newRef(flp, placep, VAccess::READ);
            ਜੇ (lsb == 0 && msb == dtypep->ਚੌੜਾਈ() - 1) ਵਾਪਸ refp;
            ਵਾਪਸ ਨਵਾਂ AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        ਅਚਲ std::ਸਦਿਸ਼<Component>& comps = dtypeComponents(dtypep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* resultp = ਖਾਲੀ_ਸੂਚਕ;
        ਅਚਲ ਮਾਪ_t ਪਹਿਲਾ = component_ਸੂਚਕਾਂਕ(dtypep, lsb);
        ਲਈ (ਮਾਪ_t i = ਪਹਿਲਾ; i < comps.ਮਾਪ() && comps[i].lsb <= msb; ++i) {
            ਅਚਲ Component& comp = comps[i];
            ਅਚਲ ਪੂਰਨ partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            ਅਚਲ ਪੂਰਨ partMsb = std::min(msb, comp.msb) - comp.lsb;
            ਰੱਖੋ* ਅਚਲ childp = placep->childrenp.at(i).get();
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ bitsp = new_ਬਿੱਟ(flp, childp, partLsb, partMsb);
            resultp = resultp ? ਨਵਾਂ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        ਵਾਪਸ resultp;
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_New_ਸੌਂਪੋ(Ast_ਗੰਢ_ਸੌਂਪੋ* origp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        origp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    ਖਾਲੀ expandSS(Ast_ਗੰਢ_ਸੌਂਪੋ* origp, ਰੱਖੋ* lPlacep, ਰੱਖੋ* rPlacep) {
        File_ਸਤਰ* ਅਚਲ flp = origp->fileline();
        ਅਚਲ std::ਸਦਿਸ਼<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        ਜੇ (isPacked(rPlacep->dtypep)) {
            ਲਈ (ਮਾਪ_t i = 0; i < compsr.ਮਾਪ(); ++i) {
                ਰੱਖੋ* ਅਚਲ lChildp = lPlacep->childrenp.at(i).get();
                Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rp = new_ਬਿੱਟ(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, rp);
            }
        } ਨਹੀਂਤਾਂ {
            ਲਈ (ਮਾਪ_t i = 0; i < compsr.ਮਾਪ(); ++i) {
                ਰੱਖੋ* ਅਚਲ lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, ਖਾਲੀ_ਸੂਚਕ, rPlacep->childrenp.at(i).get(), ਖਾਲੀ_ਸੂਚਕ);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    ਖਾਲੀ expandSN(Ast_ਗੰਢ_ਸੌਂਪੋ* origp, ਰੱਖੋ* lPlacep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> valueps = new_ਸੌਂਪੋ_Rhsps(rhsp, lPlacep->dtypep);
        ਲਈ (ਮਾਪ_t i = 0; i < valueps.ਮਾਪ(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), ਖਾਲੀ_ਸੂਚਕ, ਖਾਲੀ_ਸੂਚਕ, valueps[i]);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    ਖਾਲੀ expandNS(Ast_ਗੰਢ_ਸੌਂਪੋ* origp, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp, ਰੱਖੋ* rPlacep) {
        File_ਸਤਰ* ਅਚਲ flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        ਜੇ (isPacked(rPlacep->dtypep)) {
            ਜੋੜੋ_New_ਸੌਂਪੋ(origp, lhsp, new_ਬਿੱਟ(flp, rPlacep, 0, rPlacep->dtypep->ਚੌੜਾਈ() - 1));
            ਵਾਪਸ;
        }
        // Unpacked of the same shape, component-wise
        Ast_ਗੰਢ_DType* ਅਚਲ dtypep = dtype_ਦਾ(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        ਅਚਲ ਤਾਰਕਿਕ readsWrittenVar = !VN_IS(origp, ਸੌਂਪੋ_Dly) && [&]() {
            ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
            ਵਾਪਸ lhsp->exists([](ਅਚਲ AstVarRef* refp) {
                AstVarScope* ਅਚਲ vscp = refp->varScopep();
                ਜੇ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                ਜੇ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                ਵਾਪਸ vscp->user2() == 3;
            });
        }();
        ਜੇ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* ਅਚਲ scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* ਅਚਲ vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, ਖਾਲੀ_ਸੂਚਕ, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, ਖਾਲੀ_ਸੂਚਕ);
            ਜੋੜੋ_New_ਸੌਂਪੋ(origp, lhsp, ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
            ਵਾਪਸ;
        }
        ਅਚਲ ਮਾਪ_t ਮਾਪ = dtypeComponents(dtypep).ਮਾਪ();
        ਲਈ (ਮਾਪ_t i = 0; i < ਮਾਪ; ++i) {
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ newLhsp = newSelect(lhsp->clone_ਰੁੱਖ(ਝੂਠ), dtypep, i);
            expand(origp, ਖਾਲੀ_ਸੂਚਕ, newLhsp, rPlacep->childrenp.at(i).get(), ਖਾਲੀ_ਸੂਚਕ);
        }
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    ਖਾਲੀ expand(Ast_ਗੰਢ_ਸੌਂਪੋ* origp,  //
                ਰੱਖੋ* lPlacep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* lhsp,  //
                ਰੱਖੋ* rPlacep, Ast_ਗੰਢ_ਵਿਅੰਜਕ* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_ਸਤਰ* ਅਚਲ flp = origp->fileline();
        // A Place not split is referenced
        ਜੇ (lPlacep && !lPlacep->ਵੰਡੋ) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (rPlacep && !rPlacep->ਵੰਡੋ) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = ਖਾਲੀ_ਸੂਚਕ;
        }
        ਜੇ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } ਨਹੀਂਤਾਂ ਜੇ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } ਨਹੀਂਤਾਂ ਜੇ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_New_ਸੌਂਪੋ(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    ਤਾਰਕਿਕ is_ਵੰਡੋ_Between(ਅਚਲ ਰੱਖੋ* placep, ਪੂਰਨ lsb, ਪੂਰਨ msb) {
        ਜਦੋਂ (placep->ਵੰਡੋ) {
            ਅਚਲ ਮਾਪ_t idx = component_ਸੂਚਕਾਂਕ(placep->dtypep, lsb);
            ਜੇ (idx != component_ਸੂਚਕਾਂਕ(placep->dtypep, msb)) ਵਾਪਸ ਸੱਚ;
            ਅਚਲ Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        ਵਾਪਸ ਝੂਠ;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    ਖਾਲੀ hoistTerms(Ast_ਗੰਢ_ਸੌਂਪੋ* origp, ਅਚਲ ਰੱਖੋ* lPlacep) {
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = origp->rhsp();
        ਅਚਲ std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* ਅਚਲ scopep = lPlacep->rootp->vscp->scopep();
        ਪੂਰਨ lsb = 0;  // LSB of the current term
        ਲਈ (Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ termp : termps) {
            ਅਚਲ ਪੂਰਨ tLsb = lsb;
            ਅਚਲ ਪੂਰਨ tMsb = lsb + termp->ਚੌੜਾਈ() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            ਜੇ (!termp->user2()) ਜਾਰੀ;
            // Cheap to select from
            ਜੇ (isCheap(termp)) {
                termp->user2(0);
                ਜਾਰੀ;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            ਜੇ (termp->is_ਸ਼ੁੱਧ() && termp->user2() == 1 && !is_ਵੰਡੋ_Between(lPlacep, tLsb, tMsb)) {
                ਜਾਰੀ;
            }
            // Bits can be selected without evaluating it more than once
            ਜੇ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                ਜਾਰੀ;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_ਸਤਰ* ਅਚਲ flp = termp->fileline();
            AstVarScope* ਅਚਲ vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* ਅਚਲ tmpRefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
            origp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    ਸਥਿਰ ਖਾਲੀ markComponents(ਅਚਲ ਰੱਖੋ* placep) {
        ਲਈ (ਅਚਲ std::unique_ptr<ਰੱਖੋ>& childp : placep->childrenp) {
            childp->vscp->user2(ਸੱਚ);
            ਜੇ (childp->ਵੰਡੋ) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    ਖਾਲੀ rewriteAssignment(ਅਚਲ Assignment& assignment) {
        ਰੱਖੋ* ਅਚਲ lPlacep = assignment.lPlacep;
        ਰੱਖੋ* ਅਚਲ rPlacep = assignment.rPlacep;
        ਅਚਲ ਤਾਰਕਿਕ l_ਵੰਡੋ = lPlacep && lPlacep->ਵੰਡੋ;
        ਅਚਲ ਤਾਰਕਿਕ r_ਵੰਡੋ = rPlacep && rPlacep->ਵੰਡੋ;
        // Nothing to do if neither side is split
        ਜੇ (!l_ਵੰਡੋ && !r_ਵੰਡੋ) ਵਾਪਸ;

        Ast_ਗੰਢ_ਸੌਂਪੋ* ਅਚਲ origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        ਜੇ (l_ਵੰਡੋ && !r_ਵੰਡੋ && !VN_IS(origp, ਸੌਂਪੋ_Dly)) {
            ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
            markComponents(lPlacep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = origp->rhsp();
            ਜੇ (rhsp->exists([](ਅਚਲ AstVarRef* refp) { ਵਾਪਸ refp->varScopep()->user2(); })) {
                File_ਸਤਰ* ਅਚਲ flp = rhsp->fileline();
                AstScope* ਅਚਲ scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* ਅਚਲ vscp = m_tmps.make(flp, scopep, dtype_ਦਾ(rhsp));
                rhsp->replaceWith(ਨਵਾਂ AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* ਅਚਲ tmpRefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
                origp->ਜੋੜੋ_ਇੱਥੇ_This_As_ਅਗਲਾ(ਨਵਾਂ Ast_ਸੌਂਪੋ{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        ਜੇ (l_ਵੰਡੋ && !r_ਵੰਡੋ && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = origp->lhsp()->unlink_Fr_ਪਿੱਠ();
        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = origp->rhsp()->unlink_Fr_ਪਿੱਠ();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               l_ਵੰਡੋ ? lPlacep : ਖਾਲੀ_ਸੂਚਕ, l_ਵੰਡੋ ? ਖਾਲੀ_ਸੂਚਕ : lhsp,  //
               r_ਵੰਡੋ ? rPlacep : ਖਾਲੀ_ਸੂਚਕ, r_ਵੰਡੋ ? ਖਾਲੀ_ਸੂਚਕ : rhsp);
        ਜੇ (l_ਵੰਡੋ) VL_DO_DANGLING(ਧੱਕੋ_Deletep(lhsp), lhsp);
        ਜੇ (r_ਵੰਡੋ) VL_DO_DANGLING(ਧੱਕੋ_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(origp->unlink_Fr_ਪਿੱਠ()), origp);
    }

    // The combinational AstActive of the scope, cached
    ਸਥਿਰ Ast_ਸਰਗਰਮ* combo_ਸਰਗਰਮ(AstScope* scopep) {
        ਜੇ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(ਸੱਚ));
        ਵਾਪਸ VN_AS(scopep->user1p(), ਸਰਗਰਮ);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    ਖਾਲੀ driveFromComponents(ਰੱਖੋ* placep, Ast_ਸਰਗਰਮ* activep) {
        ਜੇ (!placep->ਵੰਡੋ) ਵਾਪਸ;
        AstVarScope* ਅਚਲ vscp = placep->vscp;
        File_ਸਤਰ* ਅਚਲ flp = vscp->fileline();
        ਲਈ (ਮਾਪ_t i = 0; i < placep->childrenp.ਮਾਪ(); ++i) {
            ਰੱਖੋ* ਅਚਲ childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ rhsp = ਨਵਾਂ AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsRefp = ਨਵਾਂ AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->ਜੋੜੋ_Stmtsp(ਨਵਾਂ AstAlways{ਨਵਾਂ Ast_ਸੌਂਪੋ_W{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    ਸਪਸ਼ਟ DecomposeRewrite(State& state)
        : Decompose_ਅਧਾਰ{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        ਲਈ (ਅਚਲ std::ਸਦਿਸ਼<Select>& chain : m_state.chains) rewriteChain(chain);
        ਲਈ (ਅਚਲ Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        ਲਈ (ਰੱਖੋ* ਅਚਲ placep : m_state.rootps) {
            ਜੇ (!placep->ਵੰਡੋ) ਜਾਰੀ;
            ਅਚਲ AstVarScope* ਅਚਲ vscp = placep->vscp;
            // Only traced ones at this point
            ਜੇ (!(vscp->varp()->isTrace() && vscp->isTrace())) ਜਾਰੀ;
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = combo_ਸਰਗਰਮ(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::ਜੋੜੋ_Stat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

ਜਨਤਕ:
    ਸਥਿਰ ਖਾਲੀ apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

ਖਾਲੀ V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        Decompose_ਅਧਾਰ::State state;
        Decompose_ਰਿਕਾਰਡ::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_ਰੁੱਖ("decompose", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
