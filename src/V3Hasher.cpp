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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Hasher.h"

#अन्तर्भाव <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

वर्ग HasherVisitor final : सार्वजनिक VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    अचर तार्किक m_cacheInUser4;  // Use user4 to cache each V3Hash?
    std::set<Ast_गाँठो*> m_visited;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    शून्य guardRecursion(Ast_गाँठो* अचर nodep, std::function<शून्य()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        यदि (!m_cacheInUser4) {
            यदि (m_visited.find(nodep) != m_visited.end()) {
                m_hash += V3Hash{nodep->name()};
                फर्काऊ;
            }
            m_visited.घुसाउ(nodep);
        }
        f();
    }

    V3Hash hash_गाँठो_And_Iterate(Ast_गाँठो* nodep, तार्किक hashDType, तार्किक hashChildren,
                              std::function<शून्य()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        यदि (m_cacheInUser4 && nodep->user4()) {
            फर्काऊ V3Hash{static_cast<uint32_t>(nodep->user4())};
        } अन्यथा {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            यदि (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            यदि (hashChildren) iterateChildrenConst(nodep);  // Children
            यदि (m_cacheInUser4) nodep->user4(m_hash.मान());
            फर्काऊ m_hash;
        }
    }

    // VISITORS

    constexpr स्थिर तार्किक HASH_DTYPE = सत्य;
    constexpr स्थिर तार्किक HASH_CHILDREN = सत्य;

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

    शून्य visit(Ast_गाँठो* nodep) override {
#यदि VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#endif
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    शून्य visit(Ast_गाँठो_Array_DType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->बायाँ();
            m_hash += nodep->दायाँ();
        });
    }
    शून्य visit(Ast_गाँठो_UOr_Struct_DType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्क();
        });
    }
    शून्य visit(AstParamTypeDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    शून्य visit(AstMemberDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstDefImplicitDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्क();
        });
    }
    शून्य visit(AstAssocArrayDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    शून्य visit(AstBracketArrayDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstDynArrayDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstWildcardArrayDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstBasicDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().बायाँ();
            m_hash += nodep->nrange().दायाँ();
        });
    }
    शून्य visit(AstCDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstConstDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstClassRefDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    शून्य visit(AstIfaceRefDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    शून्य visit(AstQueueDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    शून्य visit(AstRefDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    शून्य visit(AstStreamDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstVoidDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstEnumDType* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->unique_अङ्क();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    शून्य visit(Ast_गाँठो_अभिव्यक्ति* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstSel* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->widthConst();
        });
    }
    शून्य visit(AstConst* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->अङ्क().toHash();
        });
    }
    शून्य visit(AstNullCheck* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstCCast* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->size();
        });
    }
    शून्य visit(AstVarRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            यदि (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } अन्यथा {
                iterateConstNull(nodep->varp());
                m_hash += nodep->selfPointer().asString();
            }
        });
    }
    शून्य visit(AstVarXRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    शून्य visit(AstMemberSel* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstFScanF* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य visit(AstSScanF* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य visit(AstAddrOfCFunc* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    शून्य visit(AstTriggered* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            लागि (अचर uint32_t index : nodep->indices()) m_hash += index;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    शून्य visit(Ast_गाँठो_Stmt* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(Ast_गाँठो_CCall* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    शून्य visit(AstCaseItem* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(Ast_गाँठो_FTask_Ref* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    शून्य visit(AstCMethodHard* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->method();
        });
    }
    शून्य visit(AstCAwait* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    शून्य visit(AstCLocalScope* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstCoverInc* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    शून्य visit(AstDisplay* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    शून्य visit(AstMonitorOff* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    शून्य visit(AstJumpGo* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, असत्य, []() {});
    }
    शून्य visit(AstTraceInc* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    शून्य visit(Ast_गाँठो_Cover_Or_Assert* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    शून्य visit(Ast_गाँठो_Range* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(Ast_गाँठो_Module* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, असत्य, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(Ast_गाँठो_Pre_Sel* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstClassExtends* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstForeachHeader* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstDefParam* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstArg* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstParseRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstClassOrPackageRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    शून्य visit(AstSenItem* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->edgeType();
        });
    }
    शून्य visit(Ast_Sen_रूख* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstSFormatF* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य visit(AstElabDisplay* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    शून्य visit(Ast_गाँठो_Cons_Item* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        अचर AstUnpackArrayDType* अचर dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ असत्य,
                                     [एतत्, nodep, dtypep]() {
                                         अचर uint32_t size = dtypep->elementsConst();
                                         लागि (uint32_t n = 0; n < size; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    शून्य visit(AstPragma* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    शून्य visit(AstAttrOf* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    शून्य visit(Ast_गाँठो_File* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstCFunc* nodep) override {
        guardRecursion(nodep, [एतत्, nodep]() {  //
            m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                यदि (m_cacheInUser4) nodep->user4(V3Hash{1}.मान());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    शून्य visit(AstVar* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    शून्य visit(AstScope* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, असत्य, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    शून्य visit(AstVarScope* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    शून्य visit(AstEnumItem* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstTypedef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstTypedefFwd* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstActive* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    शून्य visit(AstCell* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    शून्य visit(AstCellInline* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [एतत्, nodep]() { m_hash += nodep->name(); });
    }
    शून्य visit(Ast_गाँठो_FTask* nodep) override {
        guardRecursion(nodep, [एतत्, nodep]() {  //
            m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                यदि (m_cacheInUser4) nodep->user4(V3Hash{1}.मान());
            });
        });
    }
    शून्य visit(AstModport* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstModportVarRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    शून्य visit(AstModportFTaskRef* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    शून्य visit(Ast_गाँठो_Procedure* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstBegin* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    शून्य visit(AstFork* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->immediateStart();
        });
    }
    शून्य visit(AstPin* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [एतत्, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_अङ्क();
        });
    }
    शून्य visit(AstText* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->text();
        });
    }
    शून्य visit(AstTextBlock* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, [एतत्, nodep]() {  //
            m_hash += nodep->prefix();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    शून्य visit(AstCStmt* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstCStmtUser* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, असत्य, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstCExpr* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstCExprUser* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    शून्य visit(AstWith* nodep) override {
        m_hash += hash_गाँठो_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

सार्वजनिक:
    // CONSTRUCTORS
    explicit HasherVisitor(Ast_गाँठो* nodep)
        : m_cacheInUser4{सत्य} {
        iterateConst(nodep);
    }
    वर्ग Uncached {};
    HasherVisitor(अचर Ast_गाँठो* nodep, Uncached)
        : m_cacheInUser4{असत्य} {
        iterateConst(const_cast<Ast_गाँठो*>(nodep));
    }
    V3Hash finalHash() अचर { फर्काऊ m_hash; }
    ~HasherVisitor() override = स्वभाव;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::चालक()(Ast_गाँठो* nodep) अचर {
    यदि (!nodep->user4()) HasherVisitor{nodep};
    फर्काऊ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_गाँठो* nodep) अचर {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    फर्काऊ V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(अचर Ast_गाँठो* nodep) {
    अचर HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    फर्काऊ visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

माप_प्रकार V3HasherUncachedHash(अचर Ast_गाँठो& गाँठो) {
    फर्काऊ static_cast<माप_प्रकार>(V3Hasher::uncachedHash(&गाँठो).मान());
}
