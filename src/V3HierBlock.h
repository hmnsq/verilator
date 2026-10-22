// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Plan for Hierarchical Verilation
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

#ifndef VERILATOR_V3HIERBLOCK_H_
#define VERILATOR_V3HIERBLOCK_H_

#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Graph.h"

#अन्तर्भाव <map>
#अन्तर्भाव <set>
#अन्तर्भाव <string>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <unordered_set>
#अन्तर्भाव <utility>
#अन्तर्भाव <vector>

class AstNetlist;
class Ast_ग्रन्थिः_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : सार्वजनिक V3Graph {
सार्वजनिक:
    V3HierGraph() = स्वभाव;
    ~V3HierGraph() = स्वभाव;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    स्थिर VStringList sourceFiles(ध्रुव string& शिखरम्_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    शून्य writeCommandArgsFiles(bool forMkJson) ध्रुव VL_MT_DISABLED;
    शून्य writeParametersFiles() ध्रुव VL_MT_DISABLED;
    स्थिर string शिखरम्_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : सार्वजनिक V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    ध्रुव AstModule* ध्रुव m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    ध्रुव std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    ध्रुव std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    स्थिर StrGParams stringifyParams(ध्रुव std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

सार्वजनिक:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, ध्रुव AstModule* modp, ध्रुव std::vector<AstVar*>& params,
                ध्रुव std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = स्वभाव;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    ध्रुव AstModule* modp() ध्रुव { प्रत्यागम m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    स्थिर bool stringParamPassable(ध्रुव string& मानम्) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) ध्रुव VL_MT_DISABLED;
    VStringList hierBlockArgs() ध्रुव VL_MT_DISABLED;
    string hierPrefix() ध्रुव VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, ध्रुव वर्ण* prefix,
                            ध्रुव वर्ण* suffix) ध्रुव VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) ध्रुव VL_MT_DISABLED;
    string hierMkFilename(bool withDir) ध्रुव VL_MT_DISABLED;
    string hierLibFilename(bool withDir) ध्रुव VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) ध्रुव VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() ध्रुव VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    शून्य writeCommandArgsFile(bool forMkJson) ध्रुव VL_MT_DISABLED;
    शून्य writeParametersFile() ध्रुव VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) ध्रुव VL_MT_DISABLED;
    string typeParametersFilename() ध्रुव VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() ध्रुव override { प्रत्यागम m_modp->prettyNameQ(); }
    std::string dotShape() ध्रुव override { प्रत्यागम "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
सार्वजनिक:
    स्थिर शून्य createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
