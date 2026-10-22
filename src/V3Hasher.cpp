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

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Hasher.h"

#అన్తర్భావ <functional>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that computes node hashes

తరగతి HasherVisitor final : బహిరంగ VNVisitorConst {
    // NODE STATE
    //  AstNode::user4() -> V3Hash.  Hash value of this node (hash of 0 is illegal)
    // VNUser4InUse     in V3Hasher.h

    // STATE
    V3Hash m_hash;  // Hash value accumulator
    స్థిరరాశి తార్కిక m_cache_లోపల_User4;  // Use user4 to cache each V3Hash?
    std::సమితి<Ast_కణుపు*> m_సందర్శించిన;  // Keeps track of some visited nodes to prevent
                                   // infinite recursion

    // METHODS

    శూన్య guardRecursion(Ast_కణుపు* స్థిరరాశి nodep, std::function<శూన్య()>&& f) {
        // Guard against infinite recursion if there's no caching
        // Otherwise caching does the same but faster
        ఒకవేళ (!m_cache_లోపల_User4) {
            ఒకవేళ (m_సందర్శించిన.కనుగొను(nodep) != m_సందర్శించిన.చివర()) {
                m_hash += V3Hash{nodep->name()};
                తిరిగి;
            }
            m_సందర్శించిన.చొప్పించు(nodep);
        }
        f();
    }

    V3Hash hash_కణుపు_And_Iterate(Ast_కణుపు* nodep, తార్కిక hashDType, తార్కిక hashChildren,
                              std::function<శూన్య()>&& f) {
        // See comments in visit(AstCFunc) about this breaking recursion
        ఒకవేళ (m_cache_లోపల_User4 && nodep->user4()) {
            తిరిగి V3Hash{static_cast<uint32_t>(nodep->user4())};
        } లేకపోతే {
            VL_RESTORER(m_hash);
            // Reset accumulator
            m_hash = V3Hash{nodep->type().hash()};  // Node type
            f();  // Node-specific hash
            ఒకవేళ (hashDType && nodep != nodep->dtypep())
                iterateConstNull(nodep->dtypep());  // Node dtype
            ఒకవేళ (hashChildren) iterateChildrenConst(nodep);  // Children
            ఒకవేళ (m_cache_లోపల_User4) nodep->user4(m_hash.విలువ());
            తిరిగి m_hash;
        }
    }

    // VISITORS

    constexpr స్థిర తార్కిక HASH_DTYPE = నిజం;
    constexpr స్థిర తార్కిక HASH_CHILDREN = నిజం;

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

    శూన్య సందర్శించు(Ast_కణుపు* nodep) override {
#ఒకవేళ VL_DEBUG
        UINFO(0, "%Warning: Hashing node as AstNode: " << nodep);
#అయితే_ముగింపు
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [=]() {});
    }

    //------------------------------------------------------------
    // AstNodeDType
    శూన్య సందర్శించు(Ast_కణుపు_Array_DType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            m_hash += nodep->ఎడమ();
            m_hash += nodep->కుడి();
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_UOr_Struct_DType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, అబద్ధం, [ఇది, nodep]() {  //
            m_hash += nodep->unique_అంకె();
        });
    }
    శూన్య సందర్శించు(AstParamTypeDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    శూన్య సందర్శించు(AstMemberDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstDefImplicitDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->unique_అంకె();
        });
    }
    శూన్య సందర్శించు(AstAssocArrayDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
            iterateConstNull(nodep->virtRefDType2p());
        });
    }
    శూన్య సందర్శించు(AstBracketArrayDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstDynArrayDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstUnsizedArrayDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstWildcardArrayDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstBasicDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->keyword();
            m_hash += nodep->numeric();
            m_hash += nodep->nrange().ఎడమ();
            m_hash += nodep->nrange().కుడి();
        });
    }
    శూన్య సందర్శించు(AstCDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstConstDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstClassRefDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->classp());
        });
    }
    శూన్య సందర్శించు(AstIfaceRefDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->cellp());
        });
    }
    శూన్య సందర్శించు(Ast_వరుస_DType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->virtRefDTypep());
        });
    }
    శూన్య సందర్శించు(AstRefDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->typedefp());
            iterateConstNull(nodep->refDTypep());
        });
    }
    శూన్య సందర్శించు(Ast_ప్రవాహం_DType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstVoidDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstEnumDType* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, అబద్ధం, [ఇది, nodep]() {  //
            m_hash += nodep->unique_అంకె();
        });
    }

    //------------------------------------------------------------
    // AstNodeExpr
    శూన్య సందర్శించు(Ast_కణుపు_సమాసం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstSel* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->వెడల్పు_Const();
        });
    }
    శూన్య సందర్శించు(AstConst* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->అంకె().గమ్యం_Hash();
        });
    }
    శూన్య సందర్శించు(AstNullCheck* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstCCast* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->కొలత();
        });
    }
    శూన్య సందర్శించు(AstVarRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            ఒకవేళ (nodep->varScopep()) {
                iterateConstNull(nodep->varScopep());
            } లేకపోతే {
                iterateConstNull(nodep->varp());
                m_hash += nodep->self_సూచిక().asString();
            }
        });
    }
    శూన్య సందర్శించు(AstVarXRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->varp());
            m_hash += nodep->dotted();
        });
    }
    శూన్య సందర్శించు(AstMemberSel* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstFScanF* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->పాఠ్యం();
        });
    }
    శూన్య సందర్శించు(AstSScanF* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->పాఠ్యం();
        });
    }
    శూన్య సందర్శించు(Ast_Addr_యొక్క_CFunc* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    శూన్య సందర్శించు(AstTriggered* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            కొరకు (స్థిరరాశి uint32_t సూచీ : nodep->indices()) m_hash += సూచీ;
        });
    }

    //------------------------------------------------------------
    // AstNodeStmt
    శూన్య సందర్శించు(Ast_కణుపు_Stmt* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(Ast_కణుపు_CCall* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->funcp());
        });
    }
    శూన్య సందర్శించు(Ast_Case_అంశం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask_Ref* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->taskp());
            iterateConstNull(nodep->classOrPackagep());
        });
    }
    శూన్య సందర్శించు(AstCMethodHard* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->పద్ధతి();
        });
    }
    శూన్య సందర్శించు(AstCAwait* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    శూన్య సందర్శించు(AstCLocalScope* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstCoverInc* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    శూన్య సందర్శించు(AstDisplay* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    శూన్య సందర్శించు(AstMonitorOff* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->off();
        });
    }
    శూన్య సందర్శించు(AstJumpGo* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, అబద్ధం, []() {});
    }
    శూన్య సందర్శించు(AstTraceInc* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->declp());
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cover_Or_Assert* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }

    //------------------------------------------------------------
    // AstNode direct descendants
    శూన్య సందర్శించు(Ast_కణుపు_Range* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, అబద్ధం, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_Pre_Sel* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstClassExtends* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(Ast_Foreach_శీర్షిక* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstDefParam* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstArg* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstParseRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstClassOrPackageRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->classOrPackageNodep());
        });
    }
    శూన్య సందర్శించు(Ast_Sen_అంశం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->అంచు_Type();
        });
    }
    శూన్య సందర్శించు(Ast_Sen_వృక్షం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstSFormatF* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->పాఠ్యం();
        });
    }
    శూన్య సందర్శించు(AstElabDisplay* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->displayType();
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_Cons_అంశం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstConsArray* nodep) override {
        // Hash unpacked array values by element, so equal values hash the same whether or not
        // they use a default
        స్థిరరాశి AstUnpackArrayDType* స్థిరరాశి dtypep
            = VN_AS(nodep->dtypep()->skipRefp(), UnpackArrayDType);
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, /* hashChildren: */ అబద్ధం,
                                     [ఇది, nodep, dtypep]() {
                                         స్థిరరాశి uint32_t కొలత = dtypep->elementsConst();
                                         కొరకు (uint32_t n = 0; n < కొలత; ++n) {  //
                                             iterateConstNull(nodep->valuep(n));
                                         }
                                     });
    }
    శూన్య సందర్శించు(AstPragma* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->pragType();
        });
    }
    శూన్య సందర్శించు(Ast_Attr_యొక్క* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->attrType();
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_File* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstCFunc* nodep) override {
        guardRecursion(nodep, [ఇది, nodep]() {  //
            m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
                // We might be in a recursive function, if so on *second* call
                // here we need to break what would be an infinite loop.
                ఒకవేళ (m_cache_లోపల_User4) nodep->user4(V3Hash{1}.విలువ());  // Set this "first" call
                // So that a second call will then exit hashNodeAndIterate
                // Having a constant in the hash just means the recursion will
                // end, it shouldn't change the CFunc having a unique hash itself.
                m_hash += nodep->isLoose();
            });
        });
    }
    శూన్య సందర్శించు(AstVar* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->varType();
        });
    }
    శూన్య సందర్శించు(AstScope* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, అబద్ధం, [ఇది, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->aboveScopep());
        });
    }
    శూన్య సందర్శించు(AstVarScope* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            iterateConstNull(nodep->varp());
            iterateConstNull(nodep->scopep());
        });
    }
    శూన్య సందర్శించు(Ast_Enum_అంశం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstTypedef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstTypedefFwd* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(Ast_చురుకైన* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            iterateConstNull(nodep->sentreep());
        });
    }
    శూన్య సందర్శించు(AstCell* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->modp());
        });
    }
    శూన్య సందర్శించు(AstCellInline* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN,
                                     [ఇది, nodep]() { m_hash += nodep->name(); });
    }
    శూన్య సందర్శించు(Ast_కణుపు_FTask* nodep) override {
        guardRecursion(nodep, [ఇది, nodep]() {  //
            m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
                m_hash += nodep->name();
                // See comments in AstCFunc
                ఒకవేళ (m_cache_లోపల_User4) nodep->user4(V3Hash{1}.విలువ());
            });
        });
    }
    శూన్య సందర్శించు(AstModport* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstModportVarRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->varp());
        });
    }
    శూన్య సందర్శించు(AstModportFTaskRef* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            iterateConstNull(nodep->ftaskp());
        });
    }
    శూన్య సందర్శించు(Ast_కణుపు_Procedure* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstBegin* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
        });
    }
    శూన్య సందర్శించు(AstFork* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->name();
            m_hash += nodep->joinType();
            m_hash += nodep->తక్షణ_ప్రారంభం();
        });
    }
    శూన్య సందర్శించు(AstPin* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, [ఇది, nodep]() {
            m_hash += nodep->name();
            m_hash += nodep->pin_అంకె();
        });
    }
    శూన్య సందర్శించు(Ast_పాఠ్యం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->పాఠ్యం();
        });
    }
    శూన్య సందర్శించు(Ast_పాఠ్యం_ఖండం* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, [ఇది, nodep]() {  //
            m_hash += nodep->ఉపసర్గ();
            m_hash += nodep->separator();
            m_hash += nodep->suffix();
        });
    }
    శూన్య సందర్శించు(AstCStmt* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstCStmtUser* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, అబద్ధం, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstCExpr* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstCExprUser* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }
    శూన్య సందర్శించు(AstWith* nodep) override {
        m_hash += hash_కణుపు_And_Iterate(nodep, HASH_DTYPE, HASH_CHILDREN, []() {});
    }

బహిరంగ:
    // CONSTRUCTORS
    స్పష్టమైన HasherVisitor(Ast_కణుపు* nodep)
        : m_cache_లోపల_User4{నిజం} {
        iterateConst(nodep);
    }
    తరగతి Uncached {};
    HasherVisitor(స్థిరరాశి Ast_కణుపు* nodep, Uncached)
        : m_cache_లోపల_User4{అబద్ధం} {
        iterateConst(const_cast<Ast_కణుపు*>(nodep));
    }
    V3Hash finalHash() స్థిరరాశి { తిరిగి m_hash; }
    ~HasherVisitor() override = అప్రమేయ;
};

//######################################################################
// V3Hasher methods

V3Hash V3Hasher::క్రియాకారకం()(Ast_కణుపు* nodep) స్థిరరాశి {
    ఒకవేళ (!nodep->user4()) HasherVisitor{nodep};
    తిరిగి V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::rehash(Ast_కణుపు* nodep) స్థిరరాశి {
    nodep->user4(0);
    { HasherVisitor{nodep}; }
    తిరిగి V3Hash{static_cast<uint32_t>(nodep->user4())};
}

V3Hash V3Hasher::uncachedHash(స్థిరరాశి Ast_కణుపు* nodep) {
    స్థిరరాశి HasherVisitor visitor{nodep, HasherVisitor::Uncached{}};
    తిరిగి visitor.finalHash();
}

//######################################################################
// This is used by the std::hash specialization for VNRef.
// Declared separately to avoid a circular header dependency.

కొలత_t V3HasherUncachedHash(స్థిరరాశి Ast_కణుపు& కణుపు) {
    తిరిగి static_cast<కొలత_t>(V3Hasher::uncachedHash(&కణుపు).విలువ());
}
