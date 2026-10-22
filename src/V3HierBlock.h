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

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Graph.h"

#அந்தர்பாவ <map>
#அந்தர்பாவ <set>
#அந்தர்பாவ <string>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

class AstNetlist;
class Ast_முனை_Module;
class AstParamTypeDType;
class AstVar;

//######################################################################

class V3HierGraph final : பொது V3Graph {
பொது:
    V3HierGraph() = இயல்பு;
    ~V3HierGraph() = இயல்பு;
    VL_UNCOPYABLE(V3HierGraph);
    VL_UNMOVABLE(V3HierGraph);

    // Explicit HDL inputs followed by the library-discovered top module file, if any.
    நிலையான VStringList sourceFiles(மாறிலி string& உச்சி_Module_File = "") VL_MT_DISABLED;
    // Write command line arguments to .f files for child Verilation run
    வெறுமை writeCommandArgsFiles(bool forMkJson) மாறிலி VL_MT_DISABLED;
    வெறுமை writeParametersFiles() மாறிலி VL_MT_DISABLED;
    நிலையான string உச்சி_Command_Args_Filename(bool forMkJson) VL_MT_DISABLED;
};

class V3HierBlock final : பொது V3GraphVertex {
    VL_RTTI_IMPL(V3HierBlock, V3GraphVertex)

    // TYPES
    // Parameter name, stringified value
    using StrGParam = std::pair<string, string>;
    using StrGParams = std::vector<StrGParam>;

    // MEMBERS
    மாறிலி AstModule* மாறிலி m_modp;  // Hierarchical block module
    // Value parameters that are overridden by #(.param(value)) syntax.
    மாறிலி std::vector<AstVar*> m_params;
    // Types parameters that are overridden by #(.param(value)) syntax.
    மாறிலி std::vector<AstParamTypeDType*> m_typeParams;

    // METHODS
    நிலையான StrGParams stringifyParams(மாறிலி std::vector<AstVar*>& params,
                                      bool forGOption) VL_MT_DISABLED;

பொது:
    // CONSTRUCTORs
    V3HierBlock(V3HierGraph* graphp, மாறிலி AstModule* modp, மாறிலி std::vector<AstVar*>& params,
                மாறிலி std::vector<AstParamTypeDType*>& typeParams)
        : V3GraphVertex{graphp}
        , m_modp{modp}
        , m_params{params}
        , m_typeParams{typeParams} {}
    ~V3HierBlock() VL_MT_DISABLED = இயல்பு;
    VL_UNCOPYABLE(V3HierBlock);
    VL_UNMOVABLE(V3HierBlock);

    மாறிலி AstModule* modp() மாறிலி { திரும்பு m_modp; }

    // Whether a string parameter value can be passed to a hierarchical block's Verilation
    நிலையான bool stringParamPassable(மாறிலி string& மதிப்பு) VL_PURE;

    // For emitting Makefile and build definition JSON
    VStringList commandArgs(bool forMkJson) மாறிலி VL_MT_DISABLED;
    VStringList hierBlockArgs() மாறிலி VL_MT_DISABLED;
    string hierPrefix() மாறிலி VL_MT_DISABLED;
    string hierSomeFilename(bool withDir, மாறிலி எழுத்து* prefix,
                            மாறிலி எழுத்து* suffix) மாறிலி VL_MT_DISABLED;
    string hierWrapperFilename(bool withDir) மாறிலி VL_MT_DISABLED;
    string hierMkFilename(bool withDir) மாறிலி VL_MT_DISABLED;
    string hierLibFilename(bool withDir) மாறிலி VL_MT_DISABLED;
    string hierGeneratedFilenames(bool withDir) மாறிலி VL_MT_DISABLED;
    // Returns the original HDL file if it is not included in v3Global.opt.vFiles().
    string vFileIfNecessary() மாறிலி VL_MT_DISABLED;
    // Write command line arguments to .f file for this hierarchical block
    வெறுமை writeCommandArgsFile(bool forMkJson) மாறிலி VL_MT_DISABLED;
    வெறுமை writeParametersFile() மாறிலி VL_MT_DISABLED;
    string commandArgsFilename(bool forMkJson) மாறிலி VL_MT_DISABLED;
    string typeParametersFilename() மாறிலி VL_MT_DISABLED;

    // For Graphviz dumps only
    std::string name() மாறிலி override { திரும்பு m_modp->prettyNameQ(); }
    std::string dotShape() மாறிலி override { திரும்பு "box"; }
};

//######################################################################
// Pass to create hierarcical block graph

class V3Hierarchical final {
பொது:
    நிலையான வெறுமை createGraph(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // guard
