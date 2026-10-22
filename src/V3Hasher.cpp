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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3Hasher.h"

#અન્તર્ભાવ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

વર્ગ HasherVisitor final : જાહેર VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    અચલ તાર્કિક m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_ગાંઠ*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    શૂન્ય guardRecursion(Ast_ગાંઠ* અચલ nodep, std::function<શૂન્ય()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        જો (!m_cacheInUser4) {
            જો (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                પાછા;
            }
            m_visited.ઉમેરો(nodep);
        }
        f();
    }

    V3Hash hash_ગાંઠ_And_Iterate(Ast_ગાંઠ* nodep, તાર્કિક hashDType, તાર્કિક hashChildren,
                              std::function<શૂન્ય()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        જો (m_cacheInUser4 && nodep->user4()) {
            પાછા V3Hash{static_cast<uint32_t>(nodep->user4())};
        } નહીંતર {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            જો (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            જો (hashChildren) iterateChildrenConst(nodep);  // Children
            જો (m_cacheInUser4) nodep->user4(m_hash.કિંમત());
            પાછા m_hash;
        }
    }

    // VISITORS

    constexpr સ્થિર તાર્કિક HASH_DTYPE = સાચું;
    constexpr સ્થિર તાર્કિક HASH_CHILDREN = સાચું;

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

    શૂન્ય visit(Ast_ગાંઠ* nodep) override {
#જો VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    શૂન્ય visit(Ast_ગાંઠ_Array_DType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ડાબું();
            m_hash += nodep->જમણું();
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_UOr_Struct_DType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, ખોટું, [આ, nodep]() {  //
            m_hash += nodep->unique_અંક();
        });
    }
    શૂન્ય visit(AstParamTypeDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    શૂન્ય visit(AstMemberDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->unique_અંક();
        });
    }
    શૂન્ય visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    શૂન્ય visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstDynArrayDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstBasicDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ડાબું();
            m_hash += nodep->nrange().જમણું();
        });
    }
    શૂન્ય visit(AstCDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstConstDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstClassRefDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    શૂન્ય visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    શૂન્ય visit(AstQueueDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    શૂન્ય visit(AstStreamDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstVoidDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstEnumDType* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, ખોટું, [આ, nodep]() {  //
            m_hash += nodep->unique_અંક();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    શૂન્ય visit(Ast_ગાંઠ_પદાવલિ* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstSel* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    શૂન્ય visit(AstConst* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->અંક().toHash();
        });
    }
    શૂન્ય visit(AstNullCheck* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstCCast* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    શૂન્ય visit(AstVarRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            જો (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } નહીંતર {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    શૂન્ય visit(AstVarXRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    શૂન્ય visit(AstMemberSel* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstFScanF* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    શૂન્ય visit(AstSScanF* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    શૂન્ય visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    શૂન્ય visit(AstTriggered* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            માટે (અચલ uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    શૂન્ય visit(Ast_ગાંઠ_Stmt* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(Ast_ગાંઠ_CCall* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    શૂન્ય visit(AstCaseItem* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask_Ref* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    શૂન્ય visit(AstCMethodHard* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    શૂન્ય visit(AstCAwait* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    શૂન્ય visit(AstCLocalScope* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstCoverInc* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    શૂન્ય visit(AstDisplay* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    શૂન્ય visit(AstMonitorOff* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    શૂન્ય visit(AstJumpGo* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, ખોટું, []() {});
    }
    શૂન્ય visit(AstTraceInc* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_Cover_Or_Assert* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    શૂન્ય visit(Ast_ગાંઠ_Range* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, ખોટું, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_Pre_Sel* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstClassExtends* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstForeachHeader* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstDefParam* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstArg* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstParseRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    શૂન્ય visit(AstSenItem* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    શૂન્ય visit(Ast_Sen_વૃક્ષ* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstSFormatF* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    શૂન્ય visit(AstElabDisplay* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_Cons_Item* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        અચલ AstUnpackArrayDType* અચલ dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ ખોટું,
                                     [આ, nodep, dtypep]() {
                                         અચલ uint32_t size = dtypep->elementsConst();
                                         માટે (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    શૂન્ય visit(AstPragma* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    શૂન્ય visit(AstAttrOf* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_File* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [આ, nodep]() {  //
            m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                જો (m_cacheInUser4) nodep->user4(V3Hash{1}.કિંમત());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    શૂન્ય visit(AstVar* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    શૂન્ય visit(AstScope* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, ખોટું, [આ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    શૂન્ય visit(AstEnumItem* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstTypedef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstTypedefFwd* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstActive* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    શૂન્ય visit(AstCell* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    શૂન્ય visit(AstCellInline* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [આ, nodep]() { m_hash += nodep->name(); });
    }
    શૂન્ય visit(Ast_ગાંઠ_FTask* nodep) override {
        guardRecursion(nodep, [આ, nodep]() {  //
            m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                જો (m_cacheInUser4) nodep->user4(V3Hash{1}.કિંમત());
            });
        });
    }
    શૂન્ય visit(AstModport* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstModportVarRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    શૂન્ય visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    શૂન્ય visit(Ast_ગાંઠ_Procedure* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstBegin* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    શૂન્ય visit(AstFork* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    શૂન્ય visit(AstPin* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [આ, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_અંક();
        });
    }
    શૂન્ય visit(AstText* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    શૂન્ય visit(AstTextBlock* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, [આ, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    શૂન્ય visit(AstCStmt* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstCStmtUser* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, ખોટું, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstCExpr* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstCExprUser* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    શૂન્ય visit(AstWith* nodep) override {
        m_hash += hash_ગાંઠ_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

જાહેર:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_ગાંઠ* nodep)
        : m_cacheInUser4{સાચું} {
        iterateConst(nodep);
    }
    વર્ગ Uncached {};
    HasherVisitor(અચલ Ast_ગાંઠ* nodep, Uncached)
        : m_cacheInUser4{ખોટું} {
        iterateConst(const_cast<Ast_ગાંઠ*>(nodep));
    }
    V3Hash finalHash() અચલ { પાછા m_hash; }
    ~HasherVisitor() override = મૂળભૂત;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::operator()(Ast_ગાંઠ* nodep) અચલ {
    જો (!nodep->user4()) HasherVisitor{nodep};
    પાછા V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_ગાંઠ* nodep) અચલ {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    પાછા V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(અચલ Ast_ગાંઠ* nodep) {
    અચલ HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    પાછા visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

size_t V3HasherUncachedHash(અચલ Ast_ગાંઠ& ગાંઠ) {
    પાછા static_cast<size_t>(V3Hasher::uncachedHash(&ગાંઠ).કિંમત());
}
