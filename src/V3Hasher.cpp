// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: AstNode hash computation
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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstMT.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

ਵਰਗ HasherVisitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    ਅਚਲ ਤਾਰਕਿਕ m_cache_ਵਿੱਚ_User4;  // Use user4 to cache each V3Hash?
    std::ਸੈੱਟ<Ast_ਗੰਢ*> m_ਦੇਖਿਆ;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    ਖਾਲੀ guardRecursion(Ast_ਗੰਢ* ਅਚਲ nodep, std::function<ਖਾਲੀ()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        ਜੇ (!m_cache_ਵਿੱਚ_User4) {
            ਜੇ (m_ਦੇਖਿਆ.ਲੱਭੋ(nodep) != m_ਦੇਖਿਆ.ਸਿਰਾ()) {
                m_hash += V3Hash{nodep->name()};
                ਵਾਪਸ;
            }
            m_ਦੇਖਿਆ.ਪਾਓ(nodep);
        }
        f();
    }

    V3Hash hash_ਗੰਢ_And_Iterate(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ hashDType, ਤਾਰਕਿਕ hashChildren,
                              std::function<ਖਾਲੀ()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        ਜੇ (m_cache_ਵਿੱਚ_User4 && nodep->user4()) {
            ਵਾਪਸ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } ਨਹੀਂਤਾਂ {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            ਜੇ (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            ਜੇ (hashChildren) iterateChildrenConst(nodep);  // Children
            ਜੇ (m_cache_ਵਿੱਚ_User4) nodep->user4(m_hash.ਮੁੱਲ());
            ਵਾਪਸ m_hash;
        }
    }

    // VISITORS

    constexpr ਸਥਿਰ ਤਾਰਕਿਕ HASH_DTYPE = ਸੱਚ;
    constexpr ਸਥਿਰ ਤਾਰਕਿਕ HASH_CHILDREN = ਸੱਚ;

    // Each visitor below contributes to the hash any node-specific content
    // that is not dependent on either of the following, as these are
    // included by default by hashNode:
    // - Node type (as given by AstNode::type().hash())
    // - Node dtype (unless !hashDType)
    // - child nodes (unless !hashChildren)
    //
    // The hash must be stable, which means in particular it cannot rely on
    // pointer values, or any other value that might differ between separate
    // invocations of Verilator over the same design.
    //
    // Note there is a circularity problem where some child nodes can back
    // to their ancestral nodes via member pointers, which can lead to an
    // infinite traversal. To break this, nodes that are subject to such
    // referencing and represent code which can reasonably be assumed not to
    // be equivalent to any other code, are hashed either by name (e.g.:
    // AstNodeModule), or by unique identifier (e.g.: AstNodeUOrStructDType).

    //------------------------------------------------------------
    // AstNode - Warns to help find missing cases

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
#ਜੇ VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#ਜੇ_ਅੰਤ
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Array_DType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ਖੱਬੇ();
            m_hash += nodep->ਸੱਜੇ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_UOr_Struct_DType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, ਝੂਠ, [ਇਹ, nodep]() {  //
            m_hash += nodep->unique_ਅੰਕ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstParamTypeDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefImplicitDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->unique_ਅੰਕ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstAssocArrayDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstBracketArrayDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstDynArrayDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstBasicDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ਖੱਬੇ();
            m_hash += nodep->nrange().ਸੱਜੇ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstConstDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassRefDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstIfaceRefDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਕਤਾਰ_DType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstRefDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਧਾਰਾ_DType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstVoidDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstEnumDType* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, ਝੂਠ, [ਇਹ, nodep]() {  //
            m_hash += nodep->unique_ਅੰਕ();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_ਵਿਅੰਜਕ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstSel* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਚੌੜਾਈ_Const();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstConst* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਅੰਕ().ਮੰਜ਼ਿਲ_Hash();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstNullCheck* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstCCast* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਮਾਪ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            ਜੇ (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } ਨਹੀਂਤਾਂ {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_ਸੂਚਕ().asString();
            }
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarXRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstMemberSel* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstFScanF* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਪਾਠ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstSScanF* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਪਾਠ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Addr_ਦਾ_CFunc* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstTriggered* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            ਲਈ (ਅਚਲ uint32_t ਸੂਚਕਾਂਕ : nodep->indices()) m_hash += ਸੂਚਕਾਂਕ;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_CCall* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Case_ਵਸਤੂ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask_Ref* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCMethodHard* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਵਿਧੀ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCAwait* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCLocalScope* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverInc* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstDisplay* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstMonitorOff* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstJumpGo* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, ਝੂਠ, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstTraceInc* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cover_Or_Assert* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Range* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, ਝੂਠ, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Pre_Sel* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassExtends* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Foreach_ਸਿਰਲੇਖ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstDefParam* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstArg* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstParseRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਕਿਨਾਰਾ_Type();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstSFormatF* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਪਾਠ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstElabDisplay* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Cons_ਵਸਤੂ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        ਅਚਲ AstUnpackArrayDType* ਅਚਲ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ ਝੂਠ,
                                     [ਇਹ, nodep, dtypep]() {
                                         ਅਚਲ uint32_t ਮਾਪ = dtypep->elementsConst();
                                         ਲਈ (uint32_t n = 0; n < ਮਾਪ; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    ਖਾਲੀ ਦੇਖੋ(AstPragma* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Attr_ਦਾ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_File* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        guardRecursion(nodep, [ਇਹ, nodep]() {  //
            m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                ਜੇ (m_cache_ਵਿੱਚ_User4) nodep->user4(V3Hash{1}.ਮੁੱਲ());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, ਝੂਠ, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Enum_ਵਸਤੂ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstTypedefFwd* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCell* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCellInline* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ਇਹ, nodep]() { m_hash += nodep->name(); });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_FTask* nodep) override {
        guardRecursion(nodep, [ਇਹ, nodep]() {  //
            m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                ਜੇ (m_cache_ਵਿੱਚ_User4) nodep->user4(V3Hash{1}.ਮੁੱਲ());
            });
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstModport* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportVarRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstModportFTaskRef* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Procedure* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstBegin* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->ਤੁਰੰਤ_ਸ਼ੁਰੂ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstPin* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ਇਹ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_ਅੰਕ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਪਾਠ();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਪਾਠ_ਖੰਡ* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, [ਇਹ, nodep]() {  //
            m_hash += nodep->ਅਗੇਤਰ();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmt* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstCStmtUser* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, ਝੂਠ, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExpr* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstCExprUser* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    ਖਾਲੀ ਦੇਖੋ(AstWith* nodep) override {
        m_hash += hash_ਗੰਢ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ HasherVisitor(Ast_ਗੰਢ* nodep)
        : m_cache_ਵਿੱਚ_User4{ਸੱਚ} {
        iterateConst(nodep);
    }
    ਵਰਗ Uncached {};
    HasherVisitor(ਅਚਲ Ast_ਗੰਢ* nodep, Uncached)
        : m_cache_ਵਿੱਚ_User4{ਝੂਠ} {
        iterateConst(const_cast<Ast_ਗੰਢ*>(nodep));
    }
    V3Hash finalHash() ਅਚਲ { ਵਾਪਸ m_hash; }
    ~HasherVisitor() override = ਮੂਲ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::ਸੰਚਾਲਕ()(Ast_ਗੰਢ* nodep) ਅਚਲ {
    ਜੇ (!nodep->user4()) HasherVisitor{nodep};
    ਵਾਪਸ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ਗੰਢ* nodep) ਅਚਲ {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    ਵਾਪਸ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(ਅਚਲ Ast_ਗੰਢ* nodep) {
    ਅਚਲ HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    ਵਾਪਸ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

ਮਾਪ_t V3HasherUncachedHash(ਅਚਲ Ast_ਗੰਢ& ਗੰਢ) {
    ਵਾਪਸ static_cast<ਮਾਪ_t>(V3Hasher::uncachedHash(&ਗੰਢ).ਮੁੱਲ());
}
