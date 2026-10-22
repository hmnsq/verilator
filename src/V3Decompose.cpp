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

#അന്തര്ഭാവ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#അന്തര്ഭാവ "V3Decompose.h"

#അന്തര്ഭാവ "V3AstUserAllocator.h"
#അന്തര്ഭാവ "V3MemberMap.h"
#അന്തര്ഭാവ "V3SharedTmps.h"
#അന്തര്ഭാവ "V3Stats.h"

#അന്തര്ഭാവ <algorithm>
#അന്തര്ഭാവ <memory>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// 'DecomposeBase' is a mixin class for the steps in this pass, it holds
// a reference to the State shared between the steps, and provides shared
// methods. It is otherwise stateless and exists only to help separate
// the code of the steps into logically separate classes.

വർഗ്ഗം DecomposeBase VL_NOT_FINAL {
സംരക്ഷിത:
    // TYPES

    // A component (element or member) of an aggregate type
    ഘടന Component final {
        Ast_കണ്ണി_DType* dtypep;  // Type of the component
        AstMemberDType* memberp;  // The member, if a struct
        പൂർണ്ണസംഖ്യ declIdx;  // The declared index of the element, if an array
        പൂർണ്ണസംഖ്യ lsb;  // LSB in the whole value, if packed
        പൂർണ്ണസംഖ്യ msb;  // MSB in the whole value, if packed
    };

    ഘടന Place;  // A variable or a component of one, see below

    // A copy between two Places, built from assignments. For unpacked
    // Places, it is always a whole copy. For packed Places, it might be a
    // partial range copy. Only used during the Decision step to track
    // which Places want to be split.
    ഘടന Copy final {
        Place* otherp;  // The Place at the other end
        // Following only for packed places
        പൂർണ്ണസംഖ്യ lsb;  // The first bit covered of this end
        പൂർണ്ണസംഖ്യ otherLsb;  // The first bit covered of the other end
        പൂർണ്ണസംഖ്യ width;  // The number of bits covered at each end
    };

    // A 'place' represents an original variable that might be split, or one of its
    // components, recursively. See the file header for more details.
    ഘടന Place final {
        Place* parentp = nullptr;  // The Place this is a component of, nullptr if original
        Place* rootp = nullptr;  // The original variable (root of the tree), root points to itself
        Ast_കണ്ണി_DType* dtypep = nullptr;  // The data type of the variable or component
        // The AstVarScope for this Place, null for components of an unsplit Place
        AstVarScope* vscp = nullptr;
        // The component Places created so far, by index, sized on the first one created
        std::vector<std::unique_ptr<Place>> childrenp;
        std::vector<Copy> copies;  // The copies covering it, until split
        തർക്കം splittable = സത്യം;  // Can be split (there is no reason not to)
        തർക്കം wantsSplit = അസത്യം;  // Should be split (if possible, do split)
        തർക്കം split = അസത്യം;  // Place was split
        തർക്കം queued = അസത്യം;  // On the worklist
    };

    // A component selected by a select chain, e.g. one step in 's.a[1][2].b[3]'
    ഘടന Select final {
        Ast_കണ്ണി_വ്യഞ്ജകം* exprp;  // The select
        Place* placep;  // The component
        പൂർണ്ണസംഖ്യ lsb;  // The first bit selected from the component, for packed
    };

    // An assignment to expand if a side is split
    ഘടന Assignment final {
        Ast_കണ്ണി_Assign* assp;  // The assignment
        Place* lPlacep;  // The Place the LHS addresses exactly, if any
        Place* rPlacep;  // The Place the RHS addresses exactly, if any
    };

പൊതു:
    // The state of the pass, shared by the steps
    വർഗ്ഗം State final {
        // NODE STATE
        //  AstNodeDType::user4()       -> Components of the type, via dtypeComponents
        //  AstVarScope::user4()        -> Place: the original variable, via places
        സ്ഥിരം VNUser4InUse m_user4InUse;

    പൊതു:
        // The components of the aggregate types, see DecomposeBase::dtypeComponents
        AstUser4Allocator<Ast_കണ്ണി_DType, std::vector<Component>> dtypeComponents;
        // The Places of the original variables, see DecomposeRecord::placeOf
        AstUser4Allocator<AstVarScope, Place> places;
        std::vector<Place*> rootps;  // The original Places, in creation order
        std::vector<std::vector<Select>> chains;  // Select chains addressing components
        std::vector<Assignment> assignments;  // Assignments to expand if a side is split
    };

സംരക്ഷിത:
    // STATE
    State& m_state;  // The state of the pass

    // CONSTRUCTORS
    explicit DecomposeBase(State& state)
        : m_state{state} {}

    // METHODS

    // Splittable unpacked type:
    // - Unpacked struct
    // - Unpacked array
    സ്ഥിതം തർക്കം isUnpacked(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (VN_IS(dtypep, UnpackArrayDType)) തിരികെ സത്യം;
        സ്ഥിരം AstStructDType* സ്ഥിരം structp = VN_CAST(dtypep, StructDType);
        തിരികെ structp && !structp->packed();
    }

    // Splittable packed type:
    // - Packed struct
    // - Packed array of width > 1 elements (that is: a multi-dimensional array).
    //   Specifically excludes e.g.: 'logic [31:0], bit_t [31:0], logic [31:0][0:0]'
    സ്ഥിതം തർക്കം isPacked(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        എങ്കിൽ (സ്ഥിരം AstPackArrayDType* സ്ഥിരം arrayp = VN_CAST(dtypep, PackArrayDType)) {
            തിരികെ arrayp->subDTypep()->width() > 1;
        }
        സ്ഥിരം AstStructDType* സ്ഥിരം structp = VN_CAST(dtypep, StructDType);
        തിരികെ structp && structp->packed();
    }

    // Is the type an aggregate that can be split, as enabled by the options
    സ്ഥിതം തർക്കം isSplittableType(സ്ഥിരം Ast_കണ്ണി_DType* dtypep) {
        എങ്കിൽ (isPacked(dtypep)) തിരികെ v3Global.opt.fDecomposePacked();
        എങ്കിൽ (isUnpacked(dtypep)) {
            // Not splittable if has member default values
            എങ്കിൽ (സ്ഥിരം AstStructDType* സ്ഥിരം structp = VN_CAST(dtypep->skipRefp(), StructDType)) {
                വേണ്ടി (സ്ഥിരം AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    എങ്കിൽ (mp->valuep()) തിരികെ അസത്യം;
                }
            }
            തിരികെ v3Global.opt.fDecomposeUnpacked();
        }
        തിരികെ അസത്യം;
    }

    // Type of the expression. For a VarRef, this returns the type of the variable,
    // which might differ from the type of the VarRef itself after earlier optimizations.
    സ്ഥിതം Ast_കണ്ണി_DType* dtypeOf(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) തിരികെ refp->varp()->dtypep();
        തിരികെ nodep->dtypep();
    }

    // Components of an aggregate type. Indexed in storage order, that is:
    // - For packed arrays, component 0 is in the LSBs
    // - For packed structs, component 0 is the last declared member, in the LSBs
    // - For unpacked arrays, component 0 is in storage slot 0 at runtime (element at lo() index)
    // - For unpacked structs, component 0 is the last declared member, to match packed
    സ്ഥിരം std::vector<Component>& dtypeComponents(Ast_കണ്ണി_DType* dtypep) {
        dtypep = dtypep->skipRefp();
        UASSERT_OBJ(isUnpacked(dtypep) || isPacked(dtypep), dtypep,
                    "dtypeComponents of non-aggregate type");

        // Cached via user4
        std::vector<Component>& compsr = m_state.dtypeComponents(dtypep);

        // Compute on first lookup
        എങ്കിൽ (compsr.empty()) {
            എങ്കിൽ (സ്ഥിരം Ast_കണ്ണി_Array_DType* സ്ഥിരം arrayp = VN_CAST(dtypep, കണ്ണി_Array_DType)) {
                സ്ഥിരം തർക്കം packed = isPacked(dtypep);
                സ്ഥിരം VNumRange range = arrayp->declRange();
                സ്ഥിരം തർക്കം rev = packed && range.ascending();  // Only for naming components
                Ast_കണ്ണി_DType* സ്ഥിരം subp = arrayp->subDTypep();
                വേണ്ടി (പൂർണ്ണസംഖ്യ i = 0; i < range.elements(); ++i) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = packed ? i * subp->width() : 0;
                    സ്ഥിരം പൂർണ്ണസംഖ്യ msb = packed ? lsb + subp->width() - 1 : 0;
                    // The *declared* index of the element in slot i, only used for the name
                    സ്ഥിരം പൂർണ്ണസംഖ്യ declIdx = rev ? range.hi() - i : range.lo() + i;
                    compsr.തള്ളുക_back({subp, nullptr, declIdx, lsb, msb});
                }
            } അല്ലെങ്കിൽ {
                സ്ഥിരം AstStructDType* സ്ഥിരം structp = VN_AS(dtypep, StructDType);
                സ്ഥിരം തർക്കം packed = isPacked(dtypep);
                വേണ്ടി (AstMemberDType* mp = structp->membersp(); mp;
                     mp = VN_AS(mp->nextp(), MemberDType)) {
                    സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = packed ? mp->lsb() : 0;
                    സ്ഥിരം പൂർണ്ണസംഖ്യ msb = packed ? lsb + mp->width() - 1 : 0;
                    compsr.തള്ളുക_back({mp->subDTypep(), mp, 0, lsb, msb});
                }
                // Last member is first
                std::reverse(compsr.begin(), compsr.end());
            }
        }

        // The components of this data type
        തിരികെ compsr;
    }

    // Index of the component of a packed type containing bit 'bit'
    size_t componentIndex(Ast_കണ്ണി_DType* dtypep, പൂർണ്ണസംഖ്യ bit) {
        UASSERT_OBJ(isPacked(dtypep), dtypep, "componentIndex of non-packed type");
        സ്ഥിരം std::vector<Component>& compsr = dtypeComponents(dtypep);
        // The first component ending above the bit. This is O(log n) binary search.
        സ്ഥിരം auto it = std::lower_bound(compsr.begin(), compsr.end(), bit,
                                         [](സ്ഥിരം Component& comp, പൂർണ്ണസംഖ്യ b) {  //
                                             തിരികെ b > comp.msb;
                                         });
        തിരികെ it - compsr.begin();
    }

    // The Place for component 'idx' of 'placep', created if needed
    Place* componentOf(Place* placep, size_t idx) {
        സ്ഥിരം std::vector<Component>& comps = dtypeComponents(placep->dtypep);
        എങ്കിൽ (placep->childrenp.empty()) placep->childrenp.resize(comps.size());
        std::unique_ptr<Place>& childpr = placep->childrenp.at(idx);
        എങ്കിൽ (!childpr) {
            childpr = std::make_unique<Place>();
            childpr->parentp = placep;
            childpr->rootp = placep->rootp;
            childpr->dtypep = comps.at(idx).dtypep;
            childpr->splittable = isSplittableType(childpr->dtypep);
            childpr->wantsSplit = placep->rootp->vscp->varp()->attrSplitVar();
        }
        തിരികെ childpr.get();
    }

    // Is 'nodep' cheap to clone
    സ്ഥിതം തർക്കം isCheap(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Constants are cheap
        എങ്കിൽ (VN_IS(nodep, Const)) തിരികെ സത്യം;
        // So are resets
        എങ്കിൽ (VN_IS(nodep, CReset)) തിരികെ സത്യം;
        // Variable references are cheap for the most part
        എങ്കിൽ (സ്ഥിരം AstVarRef* സ്ഥിരം refp = VN_CAST(nodep, VarRef)) {
            // Not a forced variable (not cheap, but also trips several bugs in V3Force)
            എങ്കിൽ (refp->varp()->isForced()) തിരികെ അസത്യം;
            // Not a SystemC variable, which can only be accessed whole, not selected from.
            എങ്കിൽ (refp->varp()->isSc()) തിരികെ അസത്യം;
            // Otherwise the path rooted here is cheap
            തിരികെ സത്യം;
        }
        // Bit selects only with a constant LSB
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            തിരികെ VN_IS(selp->lsbp(), Const) && isCheap(selp->fromp());
        }
        // Other selects with cheap indices are cheap
        എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം selp = VN_CAST(nodep, ArraySel)) {
            തിരികെ isCheap(selp->bitp()) && isCheap(selp->fromp());
        }
        എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(nodep, StructSel)) {  //
            തിരികെ isCheap(selp->fromp());
        }
        // A zero extension of a cheap expression, e.g. of a narrow index
        എങ്കിൽ (സ്ഥിരം AstExtend* സ്ഥിരം extp = VN_CAST(nodep, Extend)) {  //
            തിരികെ isCheap(extp->lhsp());
        }
        // Other expressions are not cheap
        തിരികെ അസത്യം;
    }
};

//######################################################################
// Record the facts about the places, without changing the tree

വർഗ്ഗം DecomposeRecord final : പൊതു VNVisitorConst, പൊതു DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> int: bit 0: eligible, bit 1: evaluated, see isEligible
    //  AstStructDType::user1()     -> bool: members annotated with their indices
    //  AstMemberDType::user1()     -> uint64_t: component index of the member
    //  AstNodeExpr::user1u()       -> Place*: the one an assignment side addresses exactly
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE
    VMemberMap m_memberMap;  // Struct members by name
    Ast_കണ്ണി_Assign* m_assignp = nullptr;  // The visited assignment iff splittable

    // METHODS

    // Warn that splitting requested via split_var cannot be done, because of 'reasonp'
    സ്ഥിതം ശൂന്യം warnNoSplit(AstVar* varp, സ്ഥിരം Ast_കണ്ണി* wherep, സ്ഥിരം അക്ഷരം* reasonp) {
        // Only warn if user requested splitting
        എങ്കിൽ (!varp->attrSplitVar()) തിരികെ;
        // A packed variable might still be split by V3BitBlast, which warns if not
        എങ്കിൽ (isPacked(varp->dtypep())) തിരികെ;
        wherep->v3warn(SPLITVAR, varp->prettyNameQ()
                                     << " marked split_var but will not be split because "
                                     << reasonp << ".\n");
        wherep->fileline()->modifyWarnOff(V3ErrorCode::SPLITVAR, സത്യം);  // Warn only once
    }

    // Is the variable eligible for splitting by its type and properties
    // Note this might be overridden by a visit to an unsupported construct,
    // so it is not completely determined until the whole netlist is visited.
    സ്ഥിതം തർക്കം isEligible(AstVar* varp) {
        // Compute and cache eligibility on first encounter
        എങ്കിൽ (!varp->user1()) {
            സ്ഥിരം തർക്കം eligible = [&]() {
                // Check data type
                എങ്കിൽ (!isSplittableType(varp->dtypep())) തിരികെ അസത്യം;
                // Check properties
                സ്ഥിരം അക്ഷരം* സ്ഥിരം reasonp = varp->cannotSplitKindReason();
                // Warn that it cannot be split if user explicitly requested splitting
                എങ്കിൽ (reasonp) warnNoSplit(varp, varp, reasonp);
                // Eligible if no refusal reason returned
                തിരികെ !reasonp;
            }();
            varp->user1(2 | eligible);
        }
        // Return the cached result
        തിരികെ varp->user1() & 1;
    }

    // The root Place for the referenced VarScope, nullptr if not eligible
    Place* placeOf(സ്ഥിരം AstVarRef* refp) {
        AstVarScope* സ്ഥിരം vscp = refp->varScopep();
        എങ്കിൽ (!isEligible(vscp->varp())) തിരികെ nullptr;
        Place& place = m_state.places(vscp);
        എങ്കിൽ (!place.rootp) {
            place.rootp = &place;
            place.dtypep = vscp->varp()->dtypep();
            place.vscp = vscp;
            place.wantsSplit = vscp->varp()->attrSplitVar();
            m_state.rootps.തള്ളുക_back(&place);
        }
        തിരികെ &place;
    }

    // Block the Place from splitting, due to the construct in 'wherep' which prevents splitting
    സ്ഥിതം ശൂന്യം block(Place* placep, സ്ഥിരം Ast_കണ്ണി* wherep) {
        placep->splittable = അസത്യം;
        // Warn on the variable only, a component is split as far as possible, as requested
        എങ്കിൽ (!placep->parentp) {
            warnNoSplit(placep->vscp->varp(), wherep, "it is referenced in an unsupported way");
        }
    }

    // Resolve the select chain ending at 'exprp' to the Place it addresses exactly.
    // Returns nullptr if it does not address an eligible one exactly. A Place a component of
    // which is selected wants to be split, one used by a select that cannot be followed is
    // blocked. Iterates expressions not part of the select chain. The components selected are
    // appended to 'chain'.
    Place* resolveChain(Ast_കണ്ണി_വ്യഞ്ജകം* exprp, std::vector<Select>& chain) {
        // Array selects correspond one to one with a component select
        എങ്കിൽ (AstArraySel* സ്ഥിരം selp = VN_CAST(exprp, ArraySel)) {
            എങ്കിൽ (Place* സ്ഥിരം fromp = resolveChain(selp->fromp(), chain)) {
                UASSERT_OBJ(isUnpacked(fromp->dtypep), selp, "ArraySel of non-unpacked Place");
                // Index must be constant and in bounds
                സ്ഥിരം AstConst* സ്ഥിരം bitp = VN_CAST(selp->bitp(), Const);
                എങ്കിൽ (bitp && bitp->toUQuad() < dtypeComponents(fromp->dtypep).size()) {
                    Place* സ്ഥിരം childp = componentOf(fromp, bitp->toUQuad());
                    fromp->wantsSplit = സത്യം;
                    chain.തള്ളുക_back({selp, childp, 0});
                    തിരികെ childp;
                }
                // Otherwise block splitting of the Place
                block(fromp, selp->fromp());
            }
            // Must visit the non-constant index
            iterateConst(selp->bitp());
            തിരികെ nullptr;
        }
        // Struct selects correspond one to one with a component select
        എങ്കിൽ (AstStructSel* സ്ഥിരം selp = VN_CAST(exprp, StructSel)) {
            എങ്കിൽ (Place* സ്ഥിരം fromp = resolveChain(selp->fromp(), chain)) {
                എങ്കിൽ (!isUnpacked(fromp->dtypep)) തിരികെ nullptr;
                AstStructDType* സ്ഥിരം structp = VN_AS(fromp->dtypep->skipRefp(), StructDType);
                // Annotate member indices of the struct on first encounter
                എങ്കിൽ (!structp->user1SetOnce()) {
                    സ്ഥിരം std::vector<Component>& compsr = dtypeComponents(structp);
                    വേണ്ടി (size_t i = 0; i < compsr.size(); ++i) compsr[i].memberp->user1(i);
                }
                സ്ഥിരം Ast_കണ്ണി* സ്ഥിരം memberp = m_memberMap.findMember(structp, selp->name());
                UASSERT_OBJ(memberp, selp, "Struct member not found: " << selp->name());
                Place* സ്ഥിരം childp = componentOf(fromp, memberp->user1());
                fromp->wantsSplit = സത്യം;
                chain.തള്ളുക_back({selp, childp, 0});
                തിരികെ childp;
            }
            തിരികെ nullptr;
        }
        // A packed Sel corresponds to one or more component selects, depends on source dimensions
        // E.g. Sel(VarRef(a), 3) on 'logic [3:0][2:0][1:0]' a corresponds to a[0][1][1],
        // and will contribute 2 chain entries (the fastest varying dimension is not splittable)
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(exprp, Sel)) {
            Place* placep = resolveChain(selp->fromp(), chain);
            എങ്കിൽ (!placep) {
                iterateConst(selp->lsbp());
                തിരികെ nullptr;
            }
            // Not followed with a variable LSB, or if out of range, so the Place is used whole
            സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(selp->lsbp(), Const);
            എങ്കിൽ (!lsbp || lsbp->toSInt() + selp->widthConst() > placep->dtypep->width()) {
                block(placep, selp->fromp());
                iterateConst(selp->lsbp());
                തിരികെ nullptr;
            }
            // Descend into the Place containing the bits, until exact
            പൂർണ്ണസംഖ്യ lsb = lsbp->toSInt();
            പൂർണ്ണസംഖ്യ msb = lsb + selp->widthConst() - 1;
            ആകുമ്പോൾ (lsb != 0 || msb != placep->dtypep->width() - 1) {
                // Within a component of a type not split
                എങ്കിൽ (!isPacked(placep->dtypep)) തിരികെ nullptr;
                സ്ഥിരം size_t idx = componentIndex(placep->dtypep, lsb);
                // Crossing components prevents splitting
                എങ്കിൽ (idx != componentIndex(placep->dtypep, msb)) {
                    block(placep, selp->fromp());
                    തിരികെ nullptr;
                }
                placep->wantsSplit = സത്യം;
                സ്ഥിരം Component& comp = dtypeComponents(placep->dtypep).at(idx);
                lsb -= comp.lsb;
                msb -= comp.lsb;
                placep = componentOf(placep, idx);
                chain.തള്ളുക_back({selp, placep, lsb});
            }
            തിരികെ placep;
        }
        // Base case
        എങ്കിൽ (AstVarRef* സ്ഥിരം refp = VN_CAST(exprp, VarRef)) തിരികെ placeOf(refp);
        // Not a select chain
        iterateConst(exprp);
        തിരികെ nullptr;
    }

    // 'nodep' references the Place 'placep' whole
    ശൂന്യം referencedWhole(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, Place* placep) {
        // If the reference is one of the sides of a splittable assignment, it can stay
        എങ്കിൽ (m_assignp && (nodep == m_assignp->lhsp() || nodep == m_assignp->rhsp())
            && isSplittableType(placep->dtypep)) {
            // Mark it for visitAssignment
            nodep->user1p(placep);
            തിരികെ;
        }

        // Otherwise must block splitting of this Place
        block(placep, nodep);
    }

    // Visit a select expression
    ശൂന്യം visitSelect(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        // Resolve the select chain to the Place it addresses exactly, and also capture the chain
        std::vector<Select> chain;
        Place* സ്ഥിരം placep = resolveChain(nodep, chain);
        // If the select addresses a Place, it references it whole, mark it as such
        എങ്കിൽ (placep) referencedWhole(nodep, placep);
        // Record the chain for the rewriting phase
        എങ്കിൽ (!chain.empty()) m_state.chains.തള്ളുക_back(std::move(chain));
    }

    // Do unpacked arrays of the types, at any depth, have ranges of opposite directions.
    // Assigning them pairs elements in reverse order (IEEE 1800-2023 7.6), not supported.
    സ്ഥിതം തർക്കം hasReversedRange(സ്ഥിരം Ast_കണ്ണി_DType* aDTypep, സ്ഥിരം Ast_കണ്ണി_DType* bDTypep) {
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം aArrayp = VN_CAST(aDTypep->skipRefp(), UnpackArrayDType);
        സ്ഥിരം AstUnpackArrayDType* സ്ഥിരം bArrayp = VN_CAST(bDTypep->skipRefp(), UnpackArrayDType);
        എങ്കിൽ (!aArrayp || !bArrayp) തിരികെ അസത്യം;
        എങ്കിൽ (aArrayp->declRange().ascending() != bArrayp->declRange().ascending()) തിരികെ സത്യം;
        തിരികെ hasReversedRange(aArrayp->subDTypep(), bArrayp->subDTypep());
    }

    // Assignment visitor, shared by the assignment types that can be split
    ശൂന്യം visitAssignment(Ast_കണ്ണി_Assign* nodep) {
        VL_RESTORER(m_assignp);
        // Not with timing control, or arrays of opposite directions, but always needs visiting
        സ്ഥിരം തർക്കം reversed = hasReversedRange(dtypeOf(nodep->lhsp()), dtypeOf(nodep->rhsp()));
        m_assignp = nodep->timingControlp() || reversed ? nullptr : nodep;
        iterateChildrenConst(nodep);
        എങ്കിൽ (!m_assignp) തിരികെ;

        // Note overlapping LHS/RHS handled in Rewrite

        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = nodep->lhsp();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = nodep->rhsp();
        Place* സ്ഥിരം lPlacep = lhsp->user1u().to<Place*>();
        Place* സ്ഥിരം rPlacep = rhsp->user1u().to<Place*>();

        // Is it a splittable assignment
        സ്ഥിരം തർക്കം splittable = [&]() {
            // Assignment between two Places is splittable
            എങ്കിൽ (lPlacep && rPlacep) തിരികെ സത്യം;
            // If the LHS is a Place, the RHS must be a splittable expression
            എങ്കിൽ (lPlacep) {
                // Packed values are always splittable, potentially through hoisting
                എങ്കിൽ (isPacked(lPlacep->dtypep)) തിരികെ സത്യം;
                // Otherwise each component is assigned a select of the RHS, so it must be cheap
                തിരികെ isCheap(rhsp);
            }
            // If the RHS is a Place, the assignment must be expandable component-wise
            എങ്കിൽ (rPlacep) {
                // If packed, the LHS is assigned once, as LHS = { components of RHS }
                എങ്കിൽ (isPacked(rPlacep->dtypep)) തിരികെ സത്യം;
                // Otherwise each component is assigned to a select of the LHS, so it must be cheap
                തിരികെ isCheap(lhsp);
            }
            // Otherwise not splittable
            തിരികെ അസത്യം;
        }();

        // If not splittable, the sides use their Places whole, so they are blocked
        എങ്കിൽ (!splittable) {
            എങ്കിൽ (rPlacep) block(rPlacep, rhsp);
            എങ്കിൽ (lPlacep) block(lPlacep, lhsp);
            തിരികെ;
        }

        // If both sides are places, record the whole copies for the Decision phase
        എങ്കിൽ (lPlacep && rPlacep) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = lPlacep->dtypep->width();  // Unused when unpacked
            lPlacep->copies.തള്ളുക_back({rPlacep, 0, 0, width});
            rPlacep->copies.തള്ളുക_back({lPlacep, 0, 0, width});
        }
        // Record for the rewriting phase
        m_state.assignments.തള്ളുക_back({nodep, lPlacep, rPlacep});
    }

    // VISITORS
    ശൂന്യം visit(Ast_കണ്ണി_DType*) override {}  // No references in data types
    ശൂന്യം visit(Ast_കണ്ണി* nodep) override { iterateChildrenConst(nodep); }
    // VISITORS - Selects that can cause splitting
    ശൂന്യം visit(AstArraySel* nodep) override { visitSelect(nodep); }
    ശൂന്യം visit(AstStructSel* nodep) override { visitSelect(nodep); }
    ശൂന്യം visit(AstSel* nodep) override { visitSelect(nodep); }
    // VISITORS - Assignments recordable as Copy
    ശൂന്യം visit(AstAssign* nodep) override { visitAssignment(nodep); }
    ശൂന്യം visit(AstAssignW* nodep) override { visitAssignment(nodep); }
    ശൂന്യം visit(AstAssignDly* nodep) override { visitAssignment(nodep); }
    // VISITORS - References to Places
    ശൂന്യം visit(AstVarRef* nodep) override {
        // Reference not handled explicitly in splittable constructs is to the
        // whole of the Place it references, mark as such.
        എങ്കിൽ (Place* സ്ഥിരം placep = placeOf(nodep)) referencedWhole(nodep, placep);
    }
    ശൂന്യം visit(AstMemberSel* nodep) override {
        iterateChildrenConst(nodep);
        AstVar* സ്ഥിരം varp = nodep->varp();
        എങ്കിൽ (!isEligible(varp)) തിരികെ;
        varp->user1(2);  // Mark not eligible
        warnNoSplit(varp, nodep, "it is accessed indirectly");
    }

    // CONSTRUCTORS
    DecomposeRecord(State& state, AstNetlist* netlistp)
        : DecomposeBase{state} {
        iterateConst(netlistp);
        // Block the original Places that became ineligible during traversal only
        വേണ്ടി (Place* സ്ഥിരം placep : m_state.rootps) {
            എങ്കിൽ (!isEligible(placep->vscp->varp())) placep->splittable = അസത്യം;
        }
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(State& state, AstNetlist* netlistp) { DecomposeRecord{state, netlistp}; }
};

//######################################################################
// Decision, which places are split, creating their components

വർഗ്ഗം DecomposeDecision final : പൊതു DecomposeBase {
    // NODE STATE
    //  AstVar::user1()             -> Split components, via m_splitVarps
    സ്ഥിരം VNUser1InUse m_user1InUse;

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
    സ്ഥിതം തർക്കം canSplit(സ്ഥിരം Place* placep) {
        // Not if not splittable
        എങ്കിൽ (!placep->splittable) തിരികെ അസത്യം;
        // Not if already split
        എങ്കിൽ (placep->split) തിരികെ അസത്യം;
        // Original variable or its parent was already split
        തിരികെ !placep->parentp || placep->parentp->split;
    }

    // Put the Place on the worklist, if it wants to be and can be split
    ശൂന്യം enqueue(Place* placep) {
        എങ്കിൽ (placep->queued || !placep->wantsSplit || !canSplit(placep)) തിരികെ;
        placep->queued = സത്യം;
        m_worklist.തള്ളുക_back(placep);
    }

    // Create the component AstVarScopes of the split Place, and set them on its components.
    // The component AstVars are shared by all scopes of the AstVar, created when first split.
    ശൂന്യം createComponents(Place* placep) {
        AstVarScope* സ്ഥിരം vscp = placep->vscp;
        AstVar* സ്ഥിരം varp = vscp->varp();
        std::vector<AstVar*>& varps = m_splitVarps(varp);
        // Create the component AstVars on first encounter
        എങ്കിൽ (varps.empty()) {
            സ്ഥിരം തർക്കം packed = isPacked(varp->dtypep());
            എങ്കിൽ (placep->parentp) {
                ++(packed ? m_statSplitPackedComp : m_statSplitUnpackedComp);
            } അല്ലെങ്കിൽ {
                ++(packed ? m_statSplitPackedOrig : m_statSplitUnpackedOrig);
            }
            File_വരി* സ്ഥിരം flp = varp->fileline();
            സ്ഥിരം VVarType varType = varp->varType();
            സ്ഥിരം std::string name = varp->name();
            വേണ്ടി (സ്ഥിരം Component& comp : dtypeComponents(varp->dtypep())) {
                സ്ഥിരം std::string suffix
                    = comp.memberp ? "__DOT__" + comp.memberp->name()
                                   : "__BRA__" + Ast_കണ്ണി::encode_സംഖ്യ(comp.declIdx) + "__KET__";
                AstVar* സ്ഥിരം newp = പുതിയ AstVar{flp, varType, name + suffix, comp.dtypep};
                newp->propagateSplitAttrFrom(varp);
                varps.തള്ളുക_back(newp);
                varp->addHereThisAsNext(newp);
            }
        }
        // Create the component AstVarScopes
        AstScope* സ്ഥിരം scopep = vscp->scopep();
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        വേണ്ടി (size_t i = 0; i < varps.size(); ++i) {
            AstVarScope* സ്ഥിരം newp = പുതിയ AstVarScope{flp, scopep, varps[i]};
            componentOf(placep, i)->vscp = newp;
            vscp->addHereThisAsNext(newp);
        }
    }

    // Add the copy to the unsplit Place at its end
    ശൂന്യം addCopy(Place* placep, സ്ഥിരം Copy& copy) {
        UASSERT(!placep->split, "Copy added to a split Place");
        // If a Place is not splittable, only the other end matters, no need to attach
        എങ്കിൽ (!placep->splittable) തിരികെ;
        // Add the copy to the Place
        placep->copies.തള്ളുക_back(copy);
        // Covering only a portion of a packed Place makes it want to be split,
        // so the copy becomes copies within its components, aligning both ends
        എങ്കിൽ (!isPacked(placep->dtypep)) തിരികെ;
        എങ്കിൽ (copy.lsb != 0 || copy.width != placep->dtypep->width()) {
            placep->wantsSplit = സത്യം;
            enqueue(placep);
        }
    }

    // A new copy between 'width' bits of Place 'ap' from 'aLsb', and of 'bp' from 'bLsb',
    // or between the whole of them if unpacked
    ശൂന്യം newCopy(Place* ap, പൂർണ്ണസംഖ്യ aLsb, Place* bp, പൂർണ്ണസംഖ്യ bLsb, പൂർണ്ണസംഖ്യ width) {
        UASSERT(ap != bp, "Copy within a Place");
        addCopy(ap, {bp, aLsb, bLsb, width});
        addCopy(bp, {ap, bLsb, aLsb, width});
    }

    // Split the copy of split Place 'placep' into copies between its components, and for
    // packed, the corresponding bits of the other end, or for unpacked, the components of the
    // other end, which then wants to be split too
    ശൂന്യം splitCopy(Place* placep, സ്ഥിരം Copy& copy) {
        Place* സ്ഥിരം otherp = copy.otherp;

        // If packed, split the copy based on the components of this place which is being split
        എങ്കിൽ (isPacked(placep->dtypep)) {
            സ്ഥിരം std::vector<Component>& comps = dtypeComponents(placep->dtypep);
            // For each component of this place covered by the copy, add copy with other side
            സ്ഥിരം പൂർണ്ണസംഖ്യ lsb = copy.lsb;
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = copy.lsb + copy.width - 1;
            സ്ഥിരം size_t first = componentIndex(placep->dtypep, lsb);
            വേണ്ടി (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
                സ്ഥിരം Component& comp = comps[i];
                സ്ഥിരം പൂർണ്ണസംഖ്യ partLsb = std::max(lsb, comp.lsb);
                സ്ഥിരം പൂർണ്ണസംഖ്യ partMsb = std::min(msb, comp.msb);
                newCopy(componentOf(placep, i), partLsb - comp.lsb, otherp,
                        copy.otherLsb + partLsb - lsb, partMsb - partLsb + 1);
            }
            തിരികെ;
        }

        // Unpacked copy of a split place: the other end wants to be split too
        otherp->wantsSplit = സത്യം;
        enqueue(otherp);
        // The sides are the same shape, so pairwise whole copies
        സ്ഥിരം size_t size = dtypeComponents(placep->dtypep).size();
        വേണ്ടി (size_t i = 0; i < size; ++i) {
            Place* സ്ഥിരം ap = componentOf(placep, i);
            എങ്കിൽ (!ap->splittable) തുടരുക;
            Place* സ്ഥിരം bp = componentOf(otherp, i);
            എങ്കിൽ (!bp->splittable) തുടരുക;
            newCopy(ap, 0, bp, 0, ap->dtypep->width());
        }
    }

    // Split the Place
    ശൂന്യം splitPlace(Place* placep) {
        UASSERT(!placep->split, "Place split twice");
        // It is now split
        placep->split = സത്യം;
        // Create its components
        createComponents(placep);
        // Enqueue the components that want to be split
        സ്ഥിരം size_t size = dtypeComponents(placep->dtypep).size();
        വേണ്ടി (size_t i = 0; i < size; ++i) enqueue(componentOf(placep, i));
        // Split the copies covering it, unless the other end is split, which split it already
        വേണ്ടി (സ്ഥിരം Copy& copy : placep->copies) {
            എങ്കിൽ (!copy.otherp->split) splitCopy(placep, copy);
        }
        placep->copies.clear();
    }

    // CONSTRUCTORS
    explicit DecomposeDecision(State& state)
        : DecomposeBase{state} {
        // Enqueue the original variables
        വേണ്ടി (Place* സ്ഥിരം placep : m_state.rootps) enqueue(placep);
        // Split enqueued places, which might make others splittable, repeat until done
        ആകുമ്പോൾ (!m_worklist.empty()) {
            Place* സ്ഥിരം placep = m_worklist.back();
            m_worklist.എടുക്കുക_back();
            placep->queued = അസത്യം;
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

പൊതു:
    സ്ഥിതം ശൂന്യം apply(State& state) { DecomposeDecision{state}; }
};

//######################################################################
// Rewrite, replacing the references to the split places

വർഗ്ഗം DecomposeRewrite final : പൊതു VNDeleter, പൊതു DecomposeBase {
    // NODE STATE
    //  AstScope::user1p()          -> AstActive*: combinational active, see comboActive
    //  AstNodeExpr::user2()        -> uint64_t: count of a term, (only in hoistTerms)
    //  AstVarScope::user2()        -> bool: component of the LHS, (only in rewriteAssignment)
    //  AstVarScope::user2()        -> int: bit 0: written, bit 1: read, (only in expandNS)
    സ്ഥിരം VNUser1InUse m_user1InUse;

    // STATE
    V3SharedTmps m_tmps{"__VdecompHoisted", VVarType::MODULETEMP};  // Temporaries of hoisted terms
    VDouble0 m_statHoisted;  // Terms assigned to temporaries
    VDouble0 m_statSliced;  // Terms sliced instead of hoisted
    VDouble0 m_statRhsReadsLhs;  // RHSs reading the LHS assigned to temporaries
    VDouble0 m_statLhsReadsLhs;  // Values for LHSs reading their own variable assembled

    // METHODS

    // Reference to the AstVarScope of the Place
    സ്ഥിതം AstVarRef* newRef(File_വരി* flp, സ്ഥിരം Place* placep, VAccess access) {
        UASSERT(placep->vscp, "Place without AstVarScope");
        തിരികെ പുതിയ AstVarRef{flp, placep->vscp, access};
    }

    // Replace the longest prefix of the select chain addressing a
    // component of a split Place with a reference to the component
    ശൂന്യം rewriteChain(സ്ഥിരം std::vector<Select>& chain) {
        // The last component selected of a split Place, so all before it are too
        സ്ഥിരം Select* selectp = nullptr;
        വേണ്ടി (സ്ഥിരം Select& select : chain) {
            എങ്കിൽ (select.placep->parentp->split) selectp = &select;
        }
        എങ്കിൽ (!selectp) തിരികെ;
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = selectp->exprp;
        // The access of the variable reference at the root of the chain, under the first select
        സ്ഥിരം VAccess access = [&]() {
            സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* rootp = chain.front().exprp;
            ആകുമ്പോൾ (സത്യം) {
                എങ്കിൽ (സ്ഥിരം AstArraySel* സ്ഥിരം selp = VN_CAST(rootp, ArraySel)) {
                    rootp = selp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstStructSel* സ്ഥിരം selp = VN_CAST(rootp, StructSel)) {
                    rootp = selp->fromp();
                } അല്ലെങ്കിൽ എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(rootp, Sel)) {
                    rootp = selp->fromp();
                } അല്ലെങ്കിൽ {
                    തിരികെ VN_AS(rootp, VarRef)->access();
                }
            }
        }();
        File_വരി* സ്ഥിരം flp = exprp->fileline();
        Ast_കണ്ണി_വ്യഞ്ജകം* newp = newRef(flp, selectp->placep, access);
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(exprp, Sel)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ width = selp->widthConst();
            എങ്കിൽ (width != newp->width()) newp = പുതിയ AstSel{flp, newp, selectp->lsb, width};
        }
        exprp->replaceWith(newp);
        VL_DO_DANGLING(തള്ളുക_Deletep(exprp), exprp);
    }

    // Select component 'idx' of 'fromp', with the components of aggregate 'dtypep'
    Ast_കണ്ണി_വ്യഞ്ജകം* newSelect(Ast_കണ്ണി_വ്യഞ്ജകം* fromp, Ast_കണ്ണി_DType* dtypep, size_t idx) {
        File_വരി* സ്ഥിരം flp = fromp->fileline();
        സ്ഥിരം Component& comp = dtypeComponents(dtypep).at(idx);
        // If Packed, it's a Sel of the bits
        എങ്കിൽ (isPacked(dtypep)) {
            AstSel* സ്ഥിരം selp = പുതിയ AstSel{flp, fromp, comp.lsb, comp.dtypep->width()};
            selp->dtypep(comp.dtypep);
            തിരികെ selp;
        }
        // If unpacked array, it's an ArraySel
        എങ്കിൽ (VN_IS(dtypep->skipRefp(), UnpackArrayDType)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ i = static_cast<പൂർണ്ണസംഖ്യ>(idx);
            UASSERT_OBJ(static_cast<size_t>(i) == idx, fromp, "Index overflow");
            തിരികെ പുതിയ AstArraySel{flp, fromp, i};
        }
        // Otherwise must be an unpacked struct, so a StructSel
        AstStructSel* സ്ഥിരം selp = പുതിയ AstStructSel{flp, fromp, comp.memberp->name()};
        selp->dtypep(comp.dtypep);
        തിരികെ selp;
    }

    // Leaf terms of a Concat/Replicate tree, in LSB order, or the expression itself if neither
    സ്ഥിതം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> concatTerms(Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> termps;
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> അട്ടി{nodep};
        ആകുമ്പോൾ (!അട്ടി.empty()) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം exprp = അട്ടി.back();
            അട്ടി.എടുക്കുക_back();
            എങ്കിൽ (AstConcat* സ്ഥിരം concatp = VN_CAST(exprp, Concat)) {
                അട്ടി.തള്ളുക_back(concatp->lhsp());
                അട്ടി.തള്ളുക_back(concatp->rhsp());
                തുടരുക;
            }
            എങ്കിൽ (AstReplicate* സ്ഥിരം repp = VN_CAST(exprp, Replicate)) {
                സ്ഥിരം AstConst* സ്ഥിരം countp = VN_AS(repp->countp(), Const);
                വേണ്ടി (uint32_t i = 0; i < countp->toUInt(); ++i) അട്ടി.തള്ളുക_back(repp->srcp());
                തുടരുക;
            }
            termps.തള്ളുക_back(exprp);
        }
        തിരികെ termps;
    }

    // Can bits of 'nodep' be selected without evaluating it more than once, see newSlice
    സ്ഥിതം തർക്കം isSliceable(സ്ഥിരം Ast_കണ്ണി_വ്യഞ്ജകം* nodep) {
        എങ്കിൽ (സ്ഥിരം AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            തിരികെ VN_IS(selp->lsbp(), Const) && isSliceable(selp->fromp());
        }
        എങ്കിൽ (സ്ഥിരം AstConcat* സ്ഥിരം catp = VN_CAST(nodep, Concat)) {
            തിരികെ isSliceable(catp->lhsp()) && isSliceable(catp->rhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstCond* സ്ഥിരം condp = VN_CAST(nodep, Cond)) {
            തിരികെ isCheap(condp->condp()) && isSliceable(condp->thenp())
                   && isSliceable(condp->elsep());
        }
        എങ്കിൽ (സ്ഥിരം AstAnd* സ്ഥിരം andp = VN_CAST(nodep, And)) {
            തിരികെ isSliceable(andp->lhsp()) && isSliceable(andp->rhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstOr* സ്ഥിരം orp = VN_CAST(nodep, Or)) {
            തിരികെ isSliceable(orp->lhsp()) && isSliceable(orp->rhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstXor* സ്ഥിരം xorp = VN_CAST(nodep, Xor)) {
            തിരികെ isSliceable(xorp->lhsp()) && isSliceable(xorp->rhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstExtend* സ്ഥിരം extp = VN_CAST(nodep, Extend)) {
            തിരികെ isSliceable(extp->lhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstNot* സ്ഥിരം notp = VN_CAST(nodep, Not)) {  //
            തിരികെ isSliceable(notp->lhsp());
        }
        എങ്കിൽ (സ്ഥിരം AstReplicate* സ്ഥിരം repp = VN_CAST(nodep, Replicate)) {
            തിരികെ isSliceable(repp->srcp());
        }
        തിരികെ isCheap(nodep);
    }

    // Select 'width' bits of 'nodep' from 'lsb', pushing the select into the operands where the
    // operation allows
    സ്ഥിതം Ast_കണ്ണി_വ്യഞ്ജകം* newSlice(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ width) {
        File_വരി* സ്ഥിരം flp = nodep->fileline();
        // A constant, as a new constant of the bits
        എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം constp = VN_CAST(nodep, Const)) {
            V3_സംഖ്യ അക്കം{nodep, width};
            അക്കം.opSel(constp->അക്കം(), lsb + width - 1, lsb);
            തിരികെ പുതിയ AstConst{flp, അക്കം};
        }
        // A constant bit select, from the bits of what it selects from
        എങ്കിൽ (AstSel* സ്ഥിരം selp = VN_CAST(nodep, Sel)) {
            എങ്കിൽ (സ്ഥിരം AstConst* സ്ഥിരം lsbp = VN_CAST(selp->lsbp(), Const)) {
                തിരികെ newSlice(selp->fromp(), lsbp->toSInt() + lsb, width);
            }
        }
        // A concatenation, from the RHS for the low bits, and the LHS for the high bits
        എങ്കിൽ (AstConcat* സ്ഥിരം catp = VN_CAST(nodep, Concat)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ rWidth = catp->rhsp()->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
            എങ്കിൽ (msb < rWidth) തിരികെ newSlice(catp->rhsp(), lsb, width);
            എങ്കിൽ (lsb >= rWidth) തിരികെ newSlice(catp->lhsp(), lsb - rWidth, width);
            തിരികെ പുതിയ AstConcat{flp, newSlice(catp->lhsp(), 0, msb - rWidth + 1),
                                 newSlice(catp->rhsp(), lsb, rWidth - lsb)};
        }
        എങ്കിൽ (AstCond* സ്ഥിരം condp = VN_CAST(nodep, Cond)) {
            തിരികെ പുതിയ AstCond{flp, condp->condp()->clone_വൃക്ഷം_Pure(അസത്യം),
                               newSlice(condp->thenp(), lsb, width),
                               newSlice(condp->elsep(), lsb, width)};
        }
        എങ്കിൽ (AstAnd* സ്ഥിരം andp = VN_CAST(nodep, And)) {
            തിരികെ പുതിയ AstAnd{flp, newSlice(andp->lhsp(), lsb, width),
                              newSlice(andp->rhsp(), lsb, width)};
        }
        എങ്കിൽ (AstOr* സ്ഥിരം orp = VN_CAST(nodep, Or)) {
            തിരികെ പുതിയ AstOr{flp, newSlice(orp->lhsp(), lsb, width),
                             newSlice(orp->rhsp(), lsb, width)};
        }
        എങ്കിൽ (AstXor* സ്ഥിരം xorp = VN_CAST(nodep, Xor)) {
            തിരികെ പുതിയ AstXor{flp, newSlice(xorp->lhsp(), lsb, width),
                              newSlice(xorp->rhsp(), lsb, width)};
        }
        // A zero extension, from the operand for its bits, and zeros above
        എങ്കിൽ (AstExtend* സ്ഥിരം extp = VN_CAST(nodep, Extend)) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ srcWidth = extp->lhsp()->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
            എങ്കിൽ (msb < srcWidth) തിരികെ newSlice(extp->lhsp(), lsb, width);
            എങ്കിൽ (lsb >= srcWidth) തിരികെ പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, width, 0};
            തിരികെ പുതിയ AstConcat{
                flp, പുതിയ AstConst{flp, AstConst::Widthed_മൂല്യം{}, msb - srcWidth + 1, 0},
                newSlice(extp->lhsp(), lsb, srcWidth - lsb)};
        }
        എങ്കിൽ (AstNot* സ്ഥിരം notp = VN_CAST(nodep, Not)) {
            തിരികെ പുതിയ AstNot{flp, newSlice(notp->lhsp(), lsb, width)};
        }
        // A replication, from the portions of the copies overlapping the bits
        എങ്കിൽ (AstReplicate* സ്ഥിരം repp = VN_CAST(nodep, Replicate)) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം srcp = repp->srcp();
            സ്ഥിരം പൂർണ്ണസംഖ്യ srcWidth = srcp->width();
            സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + width - 1;
            Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
            വേണ്ടി (പൂർണ്ണസംഖ്യ copyLsb = lsb / srcWidth * srcWidth; copyLsb <= msb; copyLsb += srcWidth) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ partLsb = std::max(lsb, copyLsb) - copyLsb;
                സ്ഥിരം പൂർണ്ണസംഖ്യ partMsb = std::min(msb, copyLsb + srcWidth - 1) - copyLsb;
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitsp = newSlice(srcp, partLsb, partMsb - partLsb + 1);
                // Higher bits go to the left
                resultp = resultp ? പുതിയ AstConcat{flp, bitsp, resultp} : bitsp;
            }
            തിരികെ resultp;
        }
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം clonep = nodep->clone_വൃക്ഷം_Pure(അസത്യം);
        എങ്കിൽ (lsb == 0 && width == nodep->width()) തിരികെ clonep;
        തിരികെ പുതിയ AstSel{flp, clonep, lsb, width};
    }

    // Values of the components of 'nodep', with the components of 'dtypep': for a reset cloned
    // from it, for packed assembled from the portions of its terms, otherwise selected from it
    std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> newAssignRhsps(Ast_കണ്ണി_വ്യഞ്ജകം* nodep, Ast_കണ്ണി_DType* dtypep) {
        സ്ഥിരം std::vector<Component>& compsr = dtypeComponents(dtypep);
        std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> valueps;
        valueps.reserve(compsr.size());

        // If CReset, it is duplicated for each component
        എങ്കിൽ (AstCReset* സ്ഥിരം cresetp = VN_CAST(nodep, CReset)) {
            വേണ്ടി (സ്ഥിരം Component& comp : compsr) {
                AstCReset* സ്ഥിരം newp = cresetp->clone_വൃക്ഷം(അസത്യം);
                newp->dtypep(comp.dtypep);
                valueps.തള്ളുക_back(newp);
            }
            തിരികെ valueps;
        }

        // If packed, assemble each component from the portions of the terms overlapping it.
        // This avoids quadratic cloning then folding if the RHS is a concatenation.
        എങ്കിൽ (isPacked(dtypep)) {
            valueps.resize(compsr.size(), nullptr);
            പൂർണ്ണസംഖ്യ lsb = 0;  // LSB of the current term
            വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp : concatTerms(nodep)) {
                സ്ഥിരം പൂർണ്ണസംഖ്യ msb = lsb + termp->width() - 1;
                സ്ഥിരം size_t first = componentIndex(dtypep, lsb);
                വേണ്ടി (size_t i = first; i < compsr.size() && compsr[i].lsb <= msb; ++i) {
                    സ്ഥിരം Component& comp = compsr[i];
                    സ്ഥിരം പൂർണ്ണസംഖ്യ partLsb = std::max(lsb, comp.lsb);
                    സ്ഥിരം പൂർണ്ണസംഖ്യ partMsb = std::min(msb, comp.msb);
                    File_വരി* സ്ഥിരം flp = termp->fileline();
                    Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം sp = newSlice(termp, partLsb - lsb, partMsb - partLsb + 1);
                    valueps[i] = valueps[i] ? പുതിയ AstConcat{flp, sp, valueps[i]} : sp;
                }
                lsb = msb + 1;
            }
            തിരികെ valueps;
        }

        // Otherwise unpacked, so select each component
        വേണ്ടി (size_t i = 0; i < compsr.size(); ++i) {
            valueps.തള്ളുക_back(newSelect(nodep->clone_വൃക്ഷം(അസത്യം), dtypep, i));
        }
        തിരികെ valueps;
    }

    // Return the expression holding bits 'lsb' to 'msb' of the packed Place
    Ast_കണ്ണി_വ്യഞ്ജകം* newBits(File_വരി* flp, സ്ഥിരം Place* placep, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb) {
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = placep->dtypep;
        // Only splittable packed Places are split
        UASSERT_OBJ(isPacked(dtypep) || !placep->split, dtypep, "Bits of non-packed Place");
        // If not split, return the bits selected out of a reference
        എങ്കിൽ (!placep->split) {
            AstVarRef* സ്ഥിരം refp = newRef(flp, placep, VAccess::READ);
            എങ്കിൽ (lsb == 0 && msb == dtypep->width() - 1) തിരികെ refp;
            തിരികെ പുതിയ AstSel{flp, refp, lsb, msb - lsb + 1};
        }
        // Otherwise, assemble the bits from the components via concatenations
        സ്ഥിരം std::vector<Component>& comps = dtypeComponents(dtypep);
        Ast_കണ്ണി_വ്യഞ്ജകം* resultp = nullptr;
        സ്ഥിരം size_t first = componentIndex(dtypep, lsb);
        വേണ്ടി (size_t i = first; i < comps.size() && comps[i].lsb <= msb; ++i) {
            സ്ഥിരം Component& comp = comps[i];
            സ്ഥിരം പൂർണ്ണസംഖ്യ partLsb = std::max(lsb, comp.lsb) - comp.lsb;
            സ്ഥിരം പൂർണ്ണസംഖ്യ partMsb = std::min(msb, comp.msb) - comp.lsb;
            Place* സ്ഥിരം childp = placep->childrenp.at(i).get();
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം bitsp = newBits(flp, childp, partLsb, partMsb);
            resultp = resultp ? പുതിയ AstConcat{flp, bitsp, resultp} : bitsp;
        }
        തിരികെ resultp;
    }

    സ്ഥിതം ശൂന്യം addNewAssign(Ast_കണ്ണി_Assign* origp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        origp->addHereThisAsNext(origp->cloneType(lhsp, rhsp));
    }

    // Expand the assignment between two split Places
    ശൂന്യം expandSS(Ast_കണ്ണി_Assign* origp, Place* lPlacep, Place* rPlacep) {
        File_വരി* സ്ഥിരം flp = origp->fileline();
        സ്ഥിരം std::vector<Component>& compsr = dtypeComponents(lPlacep->dtypep);
        എങ്കിൽ (isPacked(rPlacep->dtypep)) {
            വേണ്ടി (size_t i = 0; i < compsr.size(); ++i) {
                Place* സ്ഥിരം lChildp = lPlacep->childrenp.at(i).get();
                Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rp = newBits(flp, rPlacep, compsr[i].lsb, compsr[i].msb);
                expand(origp, lChildp, nullptr, nullptr, rp);
            }
        } അല്ലെങ്കിൽ {
            വേണ്ടി (size_t i = 0; i < compsr.size(); ++i) {
                Place* സ്ഥിരം lChildp = lPlacep->childrenp.at(i).get();
                expand(origp, lChildp, nullptr, rPlacep->childrenp.at(i).get(), nullptr);
            }
        }
    }

    // Expand the assignment between the split 'lPlacep' and non split 'rhsp'
    ശൂന്യം expandSN(Ast_കണ്ണി_Assign* origp, Place* lPlacep, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> valueps = newAssignRhsps(rhsp, lPlacep->dtypep);
        വേണ്ടി (size_t i = 0; i < valueps.size(); ++i) {
            expand(origp, lPlacep->childrenp.at(i).get(), nullptr, nullptr, valueps[i]);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
    }

    // Expand the assignment between the non split 'lhsp' and the split 'rPlacep'
    ശൂന്യം expandNS(Ast_കണ്ണി_Assign* origp, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp, Place* rPlacep) {
        File_വരി* സ്ഥിരം flp = origp->fileline();
        // If RHS is packed, assign the whole value assembled from the components
        എങ്കിൽ (isPacked(rPlacep->dtypep)) {
            addNewAssign(origp, lhsp, newBits(flp, rPlacep, 0, rPlacep->dtypep->width() - 1));
            തിരികെ;
        }
        // Unpacked of the same shape, component-wise
        Ast_കണ്ണി_DType* സ്ഥിരം dtypep = dtypeOf(lhsp);
        // If the LHS reads its own variable, e.g. 'a[a[0].k] = s', assigning a component could
        // change where the others go, so assemble the value in a temporary, and assign it once.
        // Not for NBAs, as the writes commit after all reads.
        സ്ഥിരം തർക്കം readsWrittenVar = !VN_IS(origp, AssignDly) && [&]() {
            സ്ഥിരം VNUser2InUse user2InUse;
            തിരികെ lhsp->exists([](സ്ഥിരം AstVarRef* refp) {
                AstVarScope* സ്ഥിരം vscp = refp->varScopep();
                എങ്കിൽ (refp->access().isWriteOrRW()) vscp->user2(vscp->user2() | 1);
                എങ്കിൽ (refp->access().isReadOrRW()) vscp->user2(vscp->user2() | 2);
                തിരികെ vscp->user2() == 3;
            });
        }();
        എങ്കിൽ (readsWrittenVar) {
            ++m_statLhsReadsLhs;
            AstScope* സ്ഥിരം scopep = rPlacep->rootp->vscp->scopep();
            AstVarScope* സ്ഥിരം vscp = m_tmps.make(flp, scopep, dtypep);
            expand(origp, nullptr, പുതിയ AstVarRef{flp, vscp, VAccess::WRITE}, rPlacep, nullptr);
            addNewAssign(origp, lhsp, പുതിയ AstVarRef{flp, vscp, VAccess::READ});
            തിരികെ;
        }
        സ്ഥിരം size_t size = dtypeComponents(dtypep).size();
        വേണ്ടി (size_t i = 0; i < size; ++i) {
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം newLhsp = newSelect(lhsp->clone_വൃക്ഷം(അസത്യം), dtypep, i);
            expand(origp, nullptr, newLhsp, rPlacep->childrenp.at(i).get(), nullptr);
        }
        VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
    }

    // Expand the assignment to the LHS from the RHS, into assignments of the type of 'origp',
    // inserted before it. Each side is either a Place, a component of a split one if not
    // split itself, or an expression, the other being nullptr.
    // Takes ownership of 'lhsp' and 'rhsp'.
    ശൂന്യം expand(Ast_കണ്ണി_Assign* origp,  //
                Place* lPlacep, Ast_കണ്ണി_വ്യഞ്ജകം* lhsp,  //
                Place* rPlacep, Ast_കണ്ണി_വ്യഞ്ജകം* rhsp) {
        UASSERT_OBJ(!lPlacep != !lhsp, origp, "Exactly one of lPlacep or lhsp must be non-null");
        UASSERT_OBJ(!rPlacep != !rhsp, origp, "Exactly one of rPlacep or rhsp must be non-null");
        File_വരി* സ്ഥിരം flp = origp->fileline();
        // A Place not split is referenced
        എങ്കിൽ (lPlacep && !lPlacep->split) {
            lhsp = newRef(flp, lPlacep, VAccess::WRITE);
            lPlacep = nullptr;
        }
        എങ്കിൽ (rPlacep && !rPlacep->split) {
            rhsp = newRef(flp, rPlacep, VAccess::READ);
            rPlacep = nullptr;
        }
        എങ്കിൽ (lPlacep && rPlacep) {
            expandSS(origp, lPlacep, rPlacep);
        } അല്ലെങ്കിൽ എങ്കിൽ (lPlacep) {
            expandSN(origp, lPlacep, rhsp);
        } അല്ലെങ്കിൽ എങ്കിൽ (rPlacep) {
            expandNS(origp, lhsp, rPlacep);
        } അല്ലെങ്കിൽ {
            addNewAssign(origp, lhsp, rhsp);
        }
    }

    // Will bits 'lsb' to 'msb' of the Place be in different variables after splitting it?
    തർക്കം isSplitBetween(സ്ഥിരം Place* placep, പൂർണ്ണസംഖ്യ lsb, പൂർണ്ണസംഖ്യ msb) {
        ആകുമ്പോൾ (placep->split) {
            സ്ഥിരം size_t idx = componentIndex(placep->dtypep, lsb);
            എങ്കിൽ (idx != componentIndex(placep->dtypep, msb)) തിരികെ സത്യം;
            സ്ഥിരം Component& comp = dtypeComponents(placep->dtypep).at(idx);
            lsb -= comp.lsb;
            msb -= comp.lsb;
            placep = placep->childrenp.at(idx).get();
        }
        തിരികെ അസത്യം;
    }

    // Assign the RHS terms of 'origp' that expanding it to the split 'lPlacep' would
    // evaluate more than once, or that are impure, to temporaries before 'origp'
    ശൂന്യം hoistTerms(Ast_കണ്ണി_Assign* origp, സ്ഥിരം Place* lPlacep) {
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = origp->rhsp();
        സ്ഥിരം std::vector<Ast_കണ്ണി_വ്യഞ്ജകം*> termps = concatTerms(rhsp);
        // A replicated term appears multiple times, so would be evaluated once for each
        സ്ഥിരം VNUser2InUse user2InUse;
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp : termps) termp->user2Inc();
        // Hoist the terms, each once, before the assignment
        AstScope* സ്ഥിരം scopep = lPlacep->rootp->vscp->scopep();
        പൂർണ്ണസംഖ്യ lsb = 0;  // LSB of the current term
        വേണ്ടി (Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം termp : termps) {
            സ്ഥിരം പൂർണ്ണസംഖ്യ tLsb = lsb;
            സ്ഥിരം പൂർണ്ണസംഖ്യ tMsb = lsb + termp->width() - 1;
            lsb = tMsb + 1;

            // Already decided, for a replicated term
            എങ്കിൽ (!termp->user2()) തുടരുക;
            // Cheap to select from
            എങ്കിൽ (isCheap(termp)) {
                termp->user2(0);
                തുടരുക;
            }
            // No need to hoist if pure, appears once, and lands in one assignment
            എങ്കിൽ (termp->isPure() && termp->user2() == 1 && !isSplitBetween(lPlacep, tLsb, tMsb)) {
                തുടരുക;
            }
            // Bits can be selected without evaluating it more than once
            എങ്കിൽ (isSliceable(termp)) {
                termp->user2(0);
                ++m_statSliced;
                തുടരുക;
            }

            // Hoist this term to a temporary assignment before the original
            termp->user2(0);
            ++m_statHoisted;
            File_വരി* സ്ഥിരം flp = termp->fileline();
            AstVarScope* സ്ഥിരം vscp = m_tmps.make(flp, scopep, termp->dtypep());
            termp->replaceWith(പുതിയ AstVarRef{flp, vscp, VAccess::READ});
            AstVarRef* സ്ഥിരം tmpRefp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
            origp->addHereThisAsNext(പുതിയ AstAssign{flp, tmpRefp, termp});
        }
    }

    // Mark the AstVarScopes of the components of the split 'placep', at any depth
    സ്ഥിതം ശൂന്യം markComponents(സ്ഥിരം Place* placep) {
        വേണ്ടി (സ്ഥിരം std::unique_ptr<Place>& childp : placep->childrenp) {
            childp->vscp->user2(സത്യം);
            എങ്കിൽ (childp->split) markComponents(childp.get());
        }
    }

    // Expand the assignment, if a side is split
    ശൂന്യം rewriteAssignment(സ്ഥിരം Assignment& assignment) {
        Place* സ്ഥിരം lPlacep = assignment.lPlacep;
        Place* സ്ഥിരം rPlacep = assignment.rPlacep;
        സ്ഥിരം തർക്കം lSplit = lPlacep && lPlacep->split;
        സ്ഥിരം തർക്കം rSplit = rPlacep && rPlacep->split;
        // Nothing to do if neither side is split
        എങ്കിൽ (!lSplit && !rSplit) തിരികെ;

        Ast_കണ്ണി_Assign* സ്ഥിരം origp = assignment.assp;

        // If the RHS expression reads components of the LHS, e.g. 's = {s.b, s.a}', expanding
        // would read components already written, so assign it to a temporary first. Not for
        // NBAs, as the writes commit after all reads. The LHS reading the RHS is fine, e.g.
        // 'a[s.i] = s', as the expansion only writes the LHS, so the RHS reads the same values
        // in every expanded assignment, unless they overlap, but then the RHS reads the LHS.
        എങ്കിൽ (lSplit && !rSplit && !VN_IS(origp, AssignDly)) {
            സ്ഥിരം VNUser2InUse user2InUse;
            markComponents(lPlacep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = origp->rhsp();
            എങ്കിൽ (rhsp->exists([](സ്ഥിരം AstVarRef* refp) { തിരികെ refp->varScopep()->user2(); })) {
                File_വരി* സ്ഥിരം flp = rhsp->fileline();
                AstScope* സ്ഥിരം scopep = lPlacep->rootp->vscp->scopep();
                AstVarScope* സ്ഥിരം vscp = m_tmps.make(flp, scopep, dtypeOf(rhsp));
                rhsp->replaceWith(പുതിയ AstVarRef{flp, vscp, VAccess::READ});
                AstVarRef* സ്ഥിരം tmpRefp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
                origp->addHereThisAsNext(പുതിയ AstAssign{flp, tmpRefp, rhsp});
                ++m_statRhsReadsLhs;
            }
        }
        // Hoist the terms of a packed RHS that would be evaluated more than once
        എങ്കിൽ (lSplit && !rSplit && isPacked(lPlacep->dtypep)) hoistTerms(origp, lPlacep);
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = origp->lhsp()->unlinkFrBack();
        Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = origp->rhsp()->unlinkFrBack();
        // A side is used if not split, otherwise its components are
        expand(origp,  //
               lSplit ? lPlacep : nullptr, lSplit ? nullptr : lhsp,  //
               rSplit ? rPlacep : nullptr, rSplit ? nullptr : rhsp);
        എങ്കിൽ (lSplit) VL_DO_DANGLING(തള്ളുക_Deletep(lhsp), lhsp);
        എങ്കിൽ (rSplit) VL_DO_DANGLING(തള്ളുക_Deletep(rhsp), rhsp);
        VL_DO_DANGLING(തള്ളുക_Deletep(origp->unlinkFrBack()), origp);
    }

    // The combinational AstActive of the scope, cached
    സ്ഥിതം AstActive* comboActive(AstScope* scopep) {
        എങ്കിൽ (!scopep->user1p()) scopep->user1p(scopep->comboActivep(സത്യം));
        തിരികെ VN_AS(scopep->user1p(), Active);
    }

    // Drive the split Place 'placep' from its components, recursively,
    // so the original variable has its value available.
    ശൂന്യം driveFromComponents(Place* placep, AstActive* activep) {
        എങ്കിൽ (!placep->split) തിരികെ;
        AstVarScope* സ്ഥിരം vscp = placep->vscp;
        File_വരി* സ്ഥിരം flp = vscp->fileline();
        വേണ്ടി (size_t i = 0; i < placep->childrenp.size(); ++i) {
            Place* സ്ഥിരം childp = placep->childrenp[i].get();
            driveFromComponents(childp, activep);
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം rhsp = പുതിയ AstVarRef{flp, childp->vscp, VAccess::READ};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsRefp = പുതിയ AstVarRef{flp, vscp, VAccess::WRITE};
            Ast_കണ്ണി_വ്യഞ്ജകം* സ്ഥിരം lhsp = newSelect(lhsRefp, placep->dtypep, i);
            activep->addStmtsp(പുതിയ AstAlways{പുതിയ AstAssignW{flp, lhsp, rhsp}});
        }
    }

    // CONSTRUCTORS
    explicit DecomposeRewrite(State& state)
        : DecomposeBase{state} {
        // Rewrite the references: the select chains, then expand the assignments, so the clones
        // of their sides are rewritten already
        വേണ്ടി (സ്ഥിരം std::vector<Select>& chain : m_state.chains) rewriteChain(chain);
        വേണ്ടി (സ്ഥിരം Assignment& assignment : m_state.assignments) rewriteAssignment(assignment);

        // Drive variables that must be kept from their components
        വേണ്ടി (Place* സ്ഥിരം placep : m_state.rootps) {
            എങ്കിൽ (!placep->split) തുടരുക;
            സ്ഥിരം AstVarScope* സ്ഥിരം vscp = placep->vscp;
            // Only traced ones at this point
            എങ്കിൽ (!(vscp->varp()->isTrace() && vscp->isTrace())) തുടരുക;
            AstActive* സ്ഥിരം activep = comboActive(vscp->scopep());
            driveFromComponents(placep, activep);
        }

        V3Stats::addStat("Optimizations, Decompose, terms hoisted", m_statHoisted);
        V3Stats::addStat("Optimizations, Decompose, terms sliced", m_statSliced);
        V3Stats::addStat("Optimizations, Decompose, RHS reading LHS hoisted", m_statRhsReadsLhs);
        V3Stats::addStat("Optimizations, Decompose, LHS reading LHS assembled", m_statLhsReadsLhs);
    }

പൊതു:
    സ്ഥിതം ശൂന്യം apply(State& state) { DecomposeRewrite{state}; }
};

//######################################################################
// V3Decompose class functions

ശൂന്യം V3Decompose::decomposeAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    {
        DecomposeBase::State state;
        DecomposeRecord::apply(state, nodep);
        DecomposeDecision::apply(state);
        DecomposeRewrite::apply(state);
    }  // Destruct before checking
    V3Global::dump_Check_Global_വൃക്ഷം("decompose", 0, dump_വൃക്ഷം_Either_Level() >= 3);
}
