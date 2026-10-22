// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#అన్తర్భావ "V3PchAstMT.h"

#అన్తర్భావ "V3Control.h"

#అన్తర్భావ "V3InstrCount.h"
#అన్తర్భావ "V3Os.h"
#అన్తర్భావ "V3String.h"

#అన్తర్భావ <memory>
#అన్తర్భావ <set>
#అన్తర్భావ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
మూస <రకం_పేరు T>
తరగతి V3ControlWildcardResolver final {
    మార్చదగిన V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<స్థిరరాశి std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<స్థిరరాశి std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

బహిరంగ:
    V3ControlWildcardResolver() = అప్రమేయ;
    ~V3ControlWildcardResolver() = అప్రమేయ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    శూన్య update(స్థిరరాశి V3ControlWildcardResolver& other) VL_MT_సురక్షిత_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        స్థిరరాశి V3_తాళం_Guard other_తాళం{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : other.m_mapPatterns) m_mapPatterns[itr.మొదటి].update(itr.రెండవ);
    }

    // Access and create a pattern entry
    T& at(స్థిరరాశి string& name) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        తిరిగి m_mapPatterns[V3Os::filename_Slash_మార్గం(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(స్థిరరాశి string& name) VL_MT_సురక్షిత_EXCLUDES(m_mutex) {
        స్థిరరాశి V3_తాళం_Guard తాళం_2{m_mutex};
        // Lookup if it was resolved before, typically not
        స్థిరరాశి స్వయంచాలక pair = m_mapResolved.emplace(name, శూన్యసూచిక);
        std::unique_ptr<T>& entryr = pair.మొదటి->రెండవ;
        // Resolve entry when first requested, cache the result
        ఒకవేళ (pair.రెండవ) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            స్థిరరాశి string slashName = V3Os::filename_Slash_మార్గం(name);
            కొరకు (స్థిరరాశి స్వయంచాలక& patEnt : m_mapPatterns) {
                ఒకవేళ (VString::wildmatch(slashName, patEnt.మొదటి)) {
                    ఒకవేళ (!entryr) entryr.reset(కొత్త T{});
                    entryr->update(patEnt.రెండవ);
                }
            }
            // Perform final actions that needed all updates completed
            ఒకవేళ (entryr) entryr->updateFinalize();
        }
        తిరిగి entryr.get();
    }
};

// List of attributes for variables
తరగతి V3ControlVar final {
    std::సదిశ<VAttrType> m_attrs;  // The list of attributes
బహిరంగ:
    // Add new attribugte
    శూన్య కూడు(VAttrType attr) { m_attrs.emplace_వెనుక(attr); }
    // Update from other by copying all attributes
    శూన్య update(స్థిరరాశి V3ControlVar& other) {
        m_attrs.reserve(m_attrs.కొలత() + other.m_attrs.కొలత());
        m_attrs.చొప్పించు(m_attrs.చివర(), other.m_attrs.begin(), other.m_attrs.చివర());
    }
    శూన్య updateFinalize() {}
    // Apply all attributes to the variable
    శూన్య apply(AstVar* varp) స్థిరరాశి {
        కొరకు (స్థిరరాశి VAttrType attr : m_attrs) {
            varp->కూడు_Attrsp(కొత్త Ast_Attr_యొక్క{varp->fileline(), attr});
        }
    }
};

ఉపయోగించి V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

తరగతి WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<స్థిరరాశి std::string, తార్కిక> m_mapPatterns;  // Pattern match results
    std::deque<string> m_పంక్తులు;  // Source text lines

    // METHODS
    స్థిర WildcardContents& s() {  // Singleton
        స్థిర WildcardContents s_s;
        తిరిగి s_s;
    }
    శూన్య clearCacheImp() { m_mapPatterns.clear(); }
    శూన్య నెట్టు_పాఠ్యం_Imp(స్థిరరాశి string& పాఠ్యం) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        స్థిరరాశి string leftover = m_పంక్తులు.వెనుక() + పాఠ్యం;
        m_పంక్తులు.తీయి_వెనుక();

        // Insert line-by-line
        string::కొలత_type పంక్తి_ప్రారంభం = 0;
        అయితే (నిజం) {
            స్థిరరాశి string::కొలత_type పంక్తి_చివర = leftover.కనుగొను('\n', పంక్తి_ప్రారంభం);
            ఒకవేళ (పంక్తి_చివర != string::npos) {
                స్థిరరాశి string oneline(leftover, పంక్తి_ప్రారంభం, పంక్తి_చివర - పంక్తి_ప్రారంభం + 1);
                ఒకవేళ (oneline.కొలత() > 1) m_పంక్తులు.నెట్టు_వెనుక(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_పంక్తులు.కొలత() - 1) << "]: " << oneline);
                పంక్తి_ప్రారంభం = పంక్తి_చివర + 1;
            } లేకపోతే {
                ఆపు;
            }
        }
        // Keep leftover for next time
        m_పంక్తులు.emplace_వెనుక(string(leftover, పంక్తి_ప్రారంభం));  // Might be ""
        clearCacheImp();
    }

    తార్కిక resolveUncachedImp(స్థిరరాశి string& contentsRegexp) {
        కొరకు (స్థిరరాశి string& i : m_పంక్తులు) {
            ఒకవేళ (VString::wildmatch(i, contentsRegexp)) తిరిగి నిజం;
        }
        తిరిగి అబద్ధం;
    }
    తార్కిక resolveCachedImp(స్థిరరాశి string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ఒకవేళ (contentsRegexp.ఖాళీలు()) తిరిగి నిజం;
        స్థిరరాశి స్వయంచాలక pair = m_mapPatterns.emplace(contentsRegexp, అబద్ధం);
        తార్కిక& entryr = pair.మొదటి->రెండవ;
        // Resolve entry when first requested, cache the result
        ఒకవేళ (pair.రెండవ) entryr = resolveUncachedImp(contentsRegexp);
        తిరిగి entryr;
    }

బహిరంగ:
    WildcardContents() {
        m_పంక్తులు.emplace_వెనుక("");  // start with no leftover
    }
    ~WildcardContents() = అప్రమేయ;
    // Return true iff contentsRegexp in parsed contents
    స్థిర తార్కిక resolve(స్థిరరాశి string& contentsRegexp) {
        తిరిగి s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    స్థిర శూన్య నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం) { s().నెట్టు_పాఠ్యం_Imp(పాఠ్యం); }
};

//######################################################################
// Function or task: Have variables and properties

తరగతి V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    తార్కిక m_noinline = అబద్ధం;  // Don't inline function/task
    తార్కిక m_ప్రజా = అబద్ధం;  // Public function/task

బహిరంగ:
    V3ControlFTask() = అప్రమేయ;
    శూన్య update(స్థిరరాశి V3ControlFTask& f) {
        // Don't overwrite true with false
        ఒకవేళ (f.m_noinline) m_noinline = నిజం;
        ఒకవేళ (f.m_ప్రజా) m_ప్రజా = నిజం;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    శూన్య updateFinalize() {}

    V3ControlVarResolver& params() { తిరిగి m_params; }
    V3ControlVarResolver& ports() { తిరిగి m_ports; }
    V3ControlVarResolver& vars() { తిరిగి m_vars; }

    శూన్య సమితి_No_Inline(తార్కిక సమితి) { m_noinline = సమితి; }
    శూన్య సమితి_ప్రజా(తార్కిక సమితి) { m_ప్రజా = సమితి; }

    శూన్య apply(Ast_కణుపు_FTask* ftaskp) స్థిరరాశి {
        ఒకవేళ (m_noinline)
            ftaskp->కూడు_Stmtsp(కొత్త AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ఒకవేళ (m_ప్రజా)
            ftaskp->కూడు_Stmtsp(కొత్త AstPragma{ftaskp->fileline(), VPragmaType::ప్రజా_TASK});
    }
};

ఉపయోగించి V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

తరగతి V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_రిజిస్టరు_Wrapper m_fsm_రిజిస్టరు_Wrapper;  // FSM wrapper descriptor
    std::unordered_సమితి<std::string> m_coverage_Off_ఖండాలు;  // List of block names for coverage_off
    std::సమితి<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    తార్కిక m_has_Fsm_రిజిస్టరు_Wrapper = అబద్ధం;  // Whether descriptor has been set
    తార్కిక m_inline = అబద్ధం;  // Whether to force the inline
    తార్కిక m_inline_విలువ = అబద్ధం;  // The inline value (on/off)

బహిరంగ:
    V3ControlModule() = అప్రమేయ;

    శూన్య update(స్థిరరాశి V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ఒకవేళ (m.m_has_Fsm_రిజిస్టరు_Wrapper) {
            m_fsm_రిజిస్టరు_Wrapper = m.m_fsm_రిజిస్టరు_Wrapper;
            m_has_Fsm_రిజిస్టరు_Wrapper = నిజం;
        }
        కొరకు (స్థిరరాశి string& i : m.m_coverage_Off_ఖండాలు) m_coverage_Off_ఖండాలు.చొప్పించు(i);
        ఒకవేళ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_విలువ = m.m_inline_విలువ;
        }
        కొరకు (స్వయంచాలక it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.చొప్పించు(*it);
        }
    }
    శూన్య updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { తిరిగి m_tasks; }
    V3ControlVarResolver& params() { తిరిగి m_params; }
    V3ControlVarResolver& ports() { తిరిగి m_ports; }
    V3ControlVarResolver& vars() { తిరిగి m_vars; }

    శూన్య కూడు_Coverage_ఖండం_Off(స్థిరరాశి string& name) { m_coverage_Off_ఖండాలు.చొప్పించు(name); }
    శూన్య సమితి_Inline(తార్కిక సమితి) {
        m_inline = నిజం;
        m_inline_విలువ = సమితి;
    }
    శూన్య సమితి_Fsm_రిజిస్టరు_Wrapper(File_పంక్తి* fl, స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper& desc) {
        ఒకవేళ (m_has_Fsm_రిజిస్టరు_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_కణుపు::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_రిజిస్టరు_Wrapper = desc;
        m_has_Fsm_రిజిస్టరు_Wrapper = నిజం;
    }
    స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper* fsm_రిజిస్టరు_Wrapperp() స్థిరరాశి {
        తిరిగి m_has_Fsm_రిజిస్టరు_Wrapper ? &m_fsm_రిజిస్టరు_Wrapper : శూన్యసూచిక;
    }
    శూన్య కూడు_Module_Pragma(VPragmaType pragma) { m_modPragmas.చొప్పించు(pragma); }

    శూన్య apply(Ast_కణుపు_Module* modp) {
        ఒకవేళ (m_inline) {
            స్థిరరాశి VPragmaType type
                = m_inline_విలువ ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_కణుపు* స్థిరరాశి nodep = కొత్త AstPragma{modp->fileline(), type};
            modp->కూడు_Stmtsp(nodep);
        }
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ఒకవేళ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(నిజం);
            Ast_కణుపు* స్థిరరాశి nodep = కొత్త AstPragma{modp->fileline(), itr};
            modp->కూడు_Stmtsp(nodep);
        }
    }

    శూన్య apply_ఖండం(Ast_Gen_ఖండం* nodep) {
        స్థిరరాశి VPragmaType pragma = VPragmaType::COVERAGE_ఖండం_OFF;
        ఒకవేళ (!nodep->unnamed()) {
            కొరకు (స్థిరరాశి string& i : m_coverage_Off_ఖండాలు) {
                ఒకవేళ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->కూడు_Itemsp(కొత్త AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    శూన్య apply_ఖండం(Ast_కణుపు_ఖండం* nodep) {
        స్థిరరాశి VPragmaType pragma = VPragmaType::COVERAGE_ఖండం_OFF;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (!nodep->unnamed()) {
            కొరకు (స్థిరరాశి string& i : m_coverage_Off_ఖండాలు) {
                ఒకవేళ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->కూడు_Stmtsp(కొత్త AstStmtPragma{flp, కొత్త AstPragma{flp, pragma}});
                }
            }
        }
    }
};

ఉపయోగించి V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
తరగతి V3_Control_Ignores_పంక్తి final {
బహిరంగ:
    స్థిరరాశి పూర్ణాంకం m_పంక్తి_Min;  // Minimum line number to make change at
    స్థిరరాశి పూర్ణాంకం m_పంక్తి_Max;  // Maximum line number to make change at (inclusive)
    స్థిరరాశి V3_Error_సంకేతం m_సంకేతం;  // Error code
    స్థిరరాశి తార్కిక m_on;  // True to enable message
    V3_Control_Ignores_పంక్తి(V3_Error_సంకేతం సంకేతం_2, పూర్ణాంకం linemin, పూర్ణాంకం linemax, తార్కిక on)
        : m_పంక్తి_Min{linemin}
        , m_పంక్తి_Max{linemax}
        , m_సంకేతం{సంకేతం_2}
        , m_on{on} {}
    ~V3_Control_Ignores_పంక్తి() = అప్రమేయ;
    తార్కిక every_పంక్తి() స్థిరరాశి { తిరిగి m_పంక్తి_Min == 0 && m_పంక్తి_Max == 0; }
    తార్కిక లోపల_Range(పూర్ణాంకం lineno) స్థిరరాశి {
        తిరిగి (lineno >= m_పంక్తి_Min && lineno <= m_పంక్తి_Max) || every_పంక్తి();
    }
};
std::ostream& క్రియాకారకం<<(std::ostream& os, స్థిరరాశి V3_Control_Ignores_పంక్తి& rhs) {
    తిరిగి os << rhs.m_పంక్తి_Min << "-" << rhs.m_పంక్తి_Max << ", " << rhs.m_సంకేతం << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
స్థిర std::సదిశ<V3_Control_Ignores_పంక్తి> control_Ign_పంక్తులు;

ఉపయోగించి IgnIndices = std::సదిశ<uint32_t>;  // List of {s_ignLines indces}

తరగతి VInterval_వృక్షం final {
    నిర్మాణం నమోదు_3 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        పూర్ణాంకం m_center;  // Line number of central point
        std::unique_ptr<నమోదు_3> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<నమోదు_3> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        స్పష్టమైన నమోదు_3(పూర్ణాంకం center)
            : m_center{center} {}
    };

    std::unique_ptr<నమోదు_3> m_rootp;  // Root of interval tree
    IgnIndices m_every_పంక్తులు;  // All-line disables

అంతరంగ:
    std::unique_ptr<నమోదు_3> build_వృక్షం(స్థిరరాశి IgnIndices& points) {
        ఒకవేళ (points.ఖాళీలు()) తిరిగి శూన్యసూచిక;

        పూర్ణాంకం minval = std::numeric_limits<పూర్ణాంకం>::max();
        పూర్ణాంకం maxval = std::numeric_limits<పూర్ణాంకం>::min();
        కొరకు (స్థిరరాశి స్వయంచాలక& it : points) {
            స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
            ఒకవేళ (cign.every_పంక్తి()) {
                m_every_పంక్తులు.emplace_వెనుక(it);
                కొనసాగు;
            }
            minval = std::min(minval, cign.m_పంక్తి_Min);
            maxval = std::max(maxval, cign.m_పంక్తి_Max);
        }

        స్థిరరాశి పూర్ణాంకం center = minval + (maxval - minval) / 2;
        స్వయంచాలక entp = std::unique_ptr<నమోదు_3>(కొత్త నమోదు_3{center});

        IgnIndices ఎడమ_Points;
        IgnIndices కుడి_Points;
        కొరకు (స్థిరరాశి స్వయంచాలక& it : points) {
            స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
            ఒకవేళ (cign.every_పంక్తి()) కొనసాగు;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ఒకవేళ (cign.m_పంక్తి_Max < center)
                ఎడమ_Points.emplace_వెనుక(it);
            లేకపోతే ఒకవేళ (cign.m_పంక్తి_Min > center)
                కుడి_Points.emplace_వెనుక(it);
            లేకపోతే {
                entp->m_byMin.నెట్టు_వెనుక(it);
                entp->m_byMax.నెట్టు_వెనుక(it);
            }
        }

        std::క్రమీకరించు(entp->m_byMin.begin(), entp->m_byMin.చివర(), [](uint32_t a, uint32_t b) {
            తిరిగి control_Ign_పంక్తులు[a].m_పంక్తి_Min < control_Ign_పంక్తులు[b].m_పంక్తి_Min;
        });
        std::క్రమీకరించు(entp->m_byMax.begin(), entp->m_byMax.చివర(), [](uint32_t a, uint32_t b) {
            తిరిగి control_Ign_పంక్తులు[a].m_పంక్తి_Max > control_Ign_పంక్తులు[b].m_పంక్తి_Max;
        });

        entp->m_leftp = build_వృక్షం(ఎడమ_Points);
        entp->m_rightp = build_వృక్షం(కుడి_Points);
        తిరిగి entp;
    }

    శూన్య కనుగొను_వృక్షం(పూర్ణాంకం lineno, నమోదు_3* entp, IgnIndices& resultsr, పూర్ణాంకం& తదుపరి_Changer) స్థిరరాశి {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ఒకవేళ (lineno < entp->m_center) {
            తదుపరి_Changer = std::min(తదుపరి_Changer, entp->m_center);
            కొరకు (స్థిరరాశి స్వయంచాలక& it : entp->m_byMin) {
                స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
                ఒకవేళ (cign.m_పంక్తి_Min <= lineno) {
                    resultsr.emplace_వెనుక(it);
                } లేకపోతే {
                    తదుపరి_Changer = std::min(తదుపరి_Changer, cign.m_పంక్తి_Min);
                    ఆపు;
                }
            }
            ఒకవేళ (entp->m_leftp) కనుగొను_వృక్షం(lineno, entp->m_leftp.get(), resultsr, తదుపరి_Changer);
        } లేకపోతే ఒకవేళ (lineno > entp->m_center) {
            కొరకు (స్థిరరాశి స్వయంచాలక& it : entp->m_byMax) {
                స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
                ఒకవేళ (cign.m_పంక్తి_Max >= lineno) {
                    తదుపరి_Changer = std::min(తదుపరి_Changer, cign.m_పంక్తి_Max + 1);
                    resultsr.emplace_వెనుక(it);
                } లేకపోతే {
                    ఆపు;
                }
            }
            ఒకవేళ (entp->m_rightp) కనుగొను_వృక్షం(lineno, entp->m_rightp.get(), resultsr, తదుపరి_Changer);
        } లేకపోతే {
            కొరకు (స్థిరరాశి స్వయంచాలక& it : entp->m_byMin) {
                స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
                resultsr.emplace_వెనుక(it);
                ఒకవేళ (cign.m_పంక్తి_Max >= lineno)
                    తదుపరి_Changer = std::min(తదుపరి_Changer, cign.m_పంక్తి_Max + 1);
            }
            ఒకవేళ (entp->m_leftp) కనుగొను_వృక్షం(lineno, entp->m_leftp.get(), resultsr, తదుపరి_Changer);
            ఒకవేళ (entp->m_rightp) కనుగొను_వృక్షం(lineno, entp->m_rightp.get(), resultsr, తదుపరి_Changer);
        }
    }

బహిరంగ:
    // CONSTRUCTORS
    VInterval_వృక్షం() = అప్రమేయ;
    ~VInterval_వృక్షం() = అప్రమేయ;
    // METHODS
    శూన్య clear() { m_rootp = శూన్యసూచిక; }
    శూన్య build(స్థిరరాశి IgnIndices& points) {
        clear();
        m_rootp = build_వృక్షం(points);
    }
    శూన్య కనుగొను(పూర్ణాంకం lineno, IgnIndices& resultsr, పూర్ణాంకం& తదుపరి_Changer) స్థిరరాశి {
        resultsr = m_every_పంక్తులు;
        తదుపరి_Changer = std::numeric_limits<పూర్ణాంకం>::max();

        ఒకవేళ (m_rootp) కనుగొను_వృక్షం(lineno, m_rootp.get(), resultsr, తదుపరి_Changer);
        // Sort indices, so can process in parse order
        std::క్రమీకరించు(resultsr.begin(), resultsr.చివర());
    }

    స్థిర శూన్య selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices దత్తాంశం;
        std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        కొరకు (స్థిరరాశి స్వయంచాలక& it : points) {
            control_Ign_పంక్తులు.emplace_వెనుక(
                V3_Control_Ignores_పంక్తి{V3_Error_సంకేతం::I_LINT, it.మొదటి, it.రెండవ, నిజం});
            దత్తాంశం.emplace_వెనుక(static_cast<uint32_t>(control_Ign_పంక్తులు.కొలత() - 1));
        }
        VInterval_వృక్షం వృక్షం;
        వృక్షం.build(దత్తాంశం);

        IgnIndices results;
        పూర్ణాంకం తదుపరి_Change = 0;
        వృక్షం.కనుగొను(0, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 0);
        UASSERT_SELFTEST(తదుపరి_Change, 10);
        వృక్షం.కనుగొను(10, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(తదుపరి_Change, 11);
        వృక్షం.కనుగొను(11, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(తదుపరి_Change, 15);  // Center, or would be 20
        వృక్షం.కనుగొను(20, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(తదుపరి_Change, 21);
        వృక్షం.కనుగొను(21, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(తదుపరి_Change, 25);  // Center, or would be 30
        వృక్షం.కనుగొను(30, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(తదుపరి_Change, 31);
        వృక్షం.కనుగొను(40, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(తదుపరి_Change, 41);
        వృక్షం.కనుగొను(41, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 0);
        UASSERT_SELFTEST(తదుపరి_Change, std::numeric_limits<పూర్ణాంకం>::max());
        //
        points = {{0, 0}};
        కొరకు (స్థిరరాశి స్వయంచాలక& it : points) {
            control_Ign_పంక్తులు.emplace_వెనుక(
                V3_Control_Ignores_పంక్తి{V3_Error_సంకేతం::I_LINT, it.మొదటి, it.రెండవ, నిజం});
            దత్తాంశం.emplace_వెనుక(static_cast<uint32_t>(control_Ign_పంక్తులు.కొలత() - 1));
        }
        వృక్షం.build(దత్తాంశం);
        //
        వృక్షం.కనుగొను(50, results, తదుపరి_Change);
        UASSERT_SELFTEST(results.కొలత(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
ఉపయోగించి V3_Control_పంక్తి_Attribute = std::bitset<VPragmaType::ENUM_కొలత>;

తరగతి WaiverSetting final {
బహిరంగ:
    V3_Error_సంకేతం m_సంకేతం;  // Error code
    string m_contents;  // --contents regexp
    string m_సరిపోలిక;  // --match regexp
    WaiverSetting(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& contents, స్థిరరాశి string& సరిపోలిక)
        : m_సంకేతం{సంకేతం_2}
        , m_contents{contents}
        , m_సరిపోలిక{సరిపోలిక} {}
    ~WaiverSetting() = అప్రమేయ;
    WaiverSetting& క్రియాకారకం=(స్థిరరాశి WaiverSetting& rhs) = అప్రమేయ;
};

// File entity
తరగతి V3ControlFile final {
    ఉపయోగించి పంక్తి_Attr_Map = std::map<పూర్ణాంకం, V3_Control_పంక్తి_Attribute>;  // Map line->bitset of attributes
    ఉపయోగించి Waivers = std::సదిశ<WaiverSetting>;  // List of {code,wildcard string}

    పంక్తి_Attr_Map m_పంక్తి_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_వృక్షం m_interval_వృక్షం;  // Tree of indices to IgnLines

    నిర్మాణం {
        పూర్ణాంకం filenameno = -1;  // Last filename
        పూర్ణాంకం lineno = -1;  // Last linenumber
        పూర్ణాంకం తదుపరి_Change = -1;  // Line number of next change
    } m_చివరి_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    తార్కిక పంక్తి_సరిపోలిక(పూర్ణాంకం lineno, VPragmaType type) {
        ఒకవేళ (m_పంక్తి_Attrs.కనుగొను(0) != m_పంక్తి_Attrs.చివర() && m_పంక్తి_Attrs[0][type]) తిరిగి నిజం;
        ఒకవేళ (m_పంక్తి_Attrs.కనుగొను(lineno) == m_పంక్తి_Attrs.చివర()) తిరిగి అబద్ధం;
        తిరిగి m_పంక్తి_Attrs[lineno][type];
    }

బహిరంగ:
    V3ControlFile() = అప్రమేయ;
    శూన్య update(స్థిరరాశి V3ControlFile& file) {
        // Copy in all attributes and waivers
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : file.m_పంక్తి_Attrs) m_పంక్తి_Attrs[itr.మొదటి] |= itr.రెండవ;
        m_waivers.reserve(m_waivers.కొలత() + file.m_waivers.కొలత());
        m_waivers.చొప్పించు(m_waivers.చివర(), file.m_waivers.begin(), file.m_waivers.చివర());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.కొలత() + file.m_ignIndices.కొలత());
        m_ignIndices.చొప్పించు(m_ignIndices.చివర(), file.m_ignIndices.begin(),
                            file.m_ignIndices.చివర());
        // updateFinalize() will soon build tree
    }
    శూన్య updateFinalize() { m_interval_వృక్షం.build(m_ignIndices); }
    శూన్య కూడు_పంక్తి_Attribute(పూర్ణాంకం lineno, VPragmaType attr) { m_పంక్తి_Attrs[lineno].సమితి(attr); }
    శూన్య కూడు_Ignore(V3_Error_సంకేతం సంకేతం_2, పూర్ణాంకం min, పూర్ణాంకం max, తార్కిక on) {
        control_Ign_పంక్తులు.emplace_వెనుక(V3_Control_Ignores_పంక్తి{సంకేతం_2, min, max, on});
        m_ignIndices.emplace_వెనుక(static_cast<uint32_t>(control_Ign_పంక్తులు.కొలత() - 1));
        m_చివరి_Ignore.తదుపరి_Change = -1;
    }
    శూన్య కూడు_Ignore_సరిపోలిక(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& contents, స్థిరరాశి string& సరిపోలిక) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_సరిపోలిక = సరిపోలిక;
        ఒకవేళ (new_సరిపోలిక.ఖాళీలు() || new_సరిపోలిక.వెనుక() != '*') new_సరిపోలిక += '*';
        m_waivers.emplace_వెనుక(సంకేతం_2, contents, new_సరిపోలిక);
    }

    శూన్య apply_ఖండం(Ast_Gen_ఖండం* nodep) {
        // Apply to block at this line
        స్థిరరాశి VPragmaType pragma = VPragmaType::COVERAGE_ఖండం_OFF;
        ఒకవేళ (పంక్తి_సరిపోలిక(nodep->fileline()->lineno(), pragma)) {
            nodep->కూడు_Itemsp(కొత్త AstPragma{nodep->fileline(), pragma});
        }
    }
    శూన్య apply_ఖండం(Ast_కణుపు_ఖండం* nodep) {
        // Apply to block at this line
        స్థిరరాశి VPragmaType pragma = VPragmaType::COVERAGE_ఖండం_OFF;
        File_పంక్తి* స్థిరరాశి flp = nodep->fileline();
        ఒకవేళ (పంక్తి_సరిపోలిక(flp->lineno(), pragma)) {
            nodep->కూడు_Stmtsp(కొత్త AstStmtPragma{flp, కొత్త AstPragma{flp, pragma}});
        }
    }
    శూన్య applyCase(AstCase* nodep) {
        // Apply to this case at this line
        స్థిరరాశి పూర్ణాంకం lineno = nodep->fileline()->lineno();
        ఒకవేళ (పంక్తి_సరిపోలిక(lineno, VPragmaType::నిండిన_CASE)) nodep->నిండిన_Pragma(నిజం);
        ఒకవేళ (పంక్తి_సరిపోలిక(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(నిజం);
    }
    శూన్య applyIgnores(File_పంక్తి* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ఒకవేళ (filelinep->filenameno() == m_చివరి_Ignore.filenameno
            && filelinep->lineno() == m_చివరి_Ignore.lineno)
            తిరిగి;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ఒకవేళ (filelinep->filenameno() == m_చివరి_Ignore.filenameno
            && filelinep->lineno() < m_చివరి_Ignore.తదుపరి_Change) {
            m_చివరి_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            తిరిగి;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_చివరి_Ignore.filenameno = filelinep->filenameno();
        m_చివరి_Ignore.lineno = filelinep->lineno();
        m_చివరి_Ignore.తదుపరి_Change = std::numeric_limits<పూర్ణాంకం>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_వృక్షం.కనుగొను(filelinep->lineno(), results /*ref*/, m_చివరి_Ignore.తదుపరి_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_బిట్టు_సమితి bitset{VError_బిట్టు_సమితి::AllOnes{}};
        కొరకు (స్థిరరాశి స్వయంచాలక& it : results) {
            స్థిరరాశి V3_Control_Ignores_పంక్తి& cign = control_Ign_పంక్తులు[it];
            UASSERT(cign.లోపల_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_సంకేతం.forDelegateCodes(
                [&](V3_Error_సంకేతం subcode) { bitset.సమితి(subcode, cign.m_on); });
        }

        filelinep->warn_సమితి_Ctrl_బిట్టు_సమితి(bitset);
        m_చివరి_Ignore.తదుపరి_Change = std::max(m_చివరి_Ignore.తదుపరి_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_చివరి_Ignore.తదుపరి_Change
                                       << " Messages: " << bitset.ascii());
    }
    తార్కిక waive(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& సందేశం) {
        ఒకవేళ (సంకేతం_2.hardError()) తిరిగి అబద్ధం;
        కొరకు (స్థిరరాశి స్వయంచాలక& itr : m_waivers) {
            ఒకవేళ ((సంకేతం_2.isUnder(itr.m_సంకేతం) || (itr.m_సంకేతం == V3_Error_సంకేతం::I_LINT))
                && VString::wildmatch(సందేశం, itr.m_సరిపోలిక)
                && WildcardContents::resolve(itr.m_contents)) {
                తిరిగి నిజం;
            }
        }
        తిరిగి అబద్ధం;
    }
};

ఉపయోగించి V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

తరగతి V3_Control_Scope_Trace_నమోదు final {
బహిరంగ:
    స్థిరరాశి string m_scope;  // Scope or regexp to match
    స్థిరరాశి తార్కిక m_on;  // True to enable message
    పూర్ణాంకం m_స్థాయిలు = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_నమోదు(స్థిరరాశి string& scope, తార్కిక on, పూర్ణాంకం స్థాయిలు)
        : m_scope{scope}
        , m_on{on}
        , m_స్థాయిలు{స్థాయిలు} {}
    ~V3_Control_Scope_Trace_నమోదు() = అప్రమేయ;
    తార్కిక క్రియాకారకం<(స్థిరరాశి V3_Control_Scope_Trace_నమోదు& other) స్థిరరాశి {
        ఒకవేళ (m_on < other.m_on) తిరిగి నిజం;
        ఒకవేళ (m_on > other.m_on) తిరిగి అబద్ధం;
        ఒకవేళ (m_స్థాయిలు < other.m_స్థాయిలు) తిరిగి నిజం;
        ఒకవేళ (m_స్థాయిలు > other.m_స్థాయిలు) తిరిగి అబద్ధం;
        తిరిగి m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
తరగతి V3_Control_Scope_Trace_నమోదు_సరిపోలిక final {
బహిరంగ:
    స్థిరరాశి V3_Control_Scope_Trace_నమోదు* m_entryp;  // Entry the match refers to
    స్థిరరాశి string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_నమోదు_సరిపోలిక(స్థిరరాశి V3_Control_Scope_Trace_నమోదు* entryp, స్థిరరాశి string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    తార్కిక క్రియాకారకం<(స్థిరరాశి V3_Control_Scope_Trace_నమోదు_సరిపోలిక& other) స్థిరరాశి {
        ఒకవేళ (m_entryp < other.m_entryp) తిరిగి నిజం;
        ఒకవేళ (m_entryp > other.m_entryp) తిరిగి అబద్ధం;
        తిరిగి m_scopepart < other.m_scopepart;
    }
};

తరగతి V3ControlScopeTraceResolver final {
    std::సదిశ<V3_Control_Scope_Trace_నమోదు> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_నమోదు_సరిపోలిక, తార్కిక> m_సరిపోలిక_Cache;  // Matching entries for speed

బహిరంగ:
    శూన్య కూడు_Scope_Trace_On(తార్కిక on, స్థిరరాశి string& scope, పూర్ణాంకం స్థాయిలు) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << స్థాయిలు);
        m_entries.emplace_వెనుక(V3_Control_Scope_Trace_నమోదు{scope, on, స్థాయిలు});
        m_సరిపోలిక_Cache.clear();
    }

    తార్కిక get_నమోదు_సరిపోలిక(స్థిరరాశి V3_Control_Scope_Trace_నమోదు* entp, స్థిరరాశి string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        స్థిరరాశి V3_Control_Scope_Trace_నమోదు_సరిపోలిక కీలకం{entp, scopepart};
        స్థిరరాశి స్వయంచాలక pair = m_సరిపోలిక_Cache.emplace(కీలకం, అబద్ధం);
        ఒకవేళ (pair.రెండవ) pair.మొదటి->రెండవ = VString::wildmatch(scopepart, entp->m_scope);
        తిరిగి pair.మొదటి->రెండవ;
    }

    తార్కిక getScopeTraceOn(స్థిరరాశి string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        పూర్ణాంకం maxLevel = 1;
        కొరకు (స్థిరరాశి స్వయంచాలక& ch : scope) {
            ఒకవేళ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        తార్కిక enabled = నిజం;
        కొరకు (స్థిరరాశి స్వయంచాలక& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            పూర్ణాంకం partLevel = 1;
            కొరకు (string::కొలత_type part_చివర = 0; నిజం;) {
                part_చివర = scope.కనుగొను('.', part_చివర + 1);
                ఒకవేళ (part_చివర == string::npos) part_చివర = scope.నిడివి();
                స్థిరరాశి std::string scopepart = scope.substr(0, part_చివర);
                ఒకవేళ (get_నమోదు_సరిపోలిక(&ent, scopepart)) {
                    స్థిరరాశి తార్కిక level_సరిపోలిక
                        = !ent.m_స్థాయిలు || (ent.m_స్థాయిలు >= maxLevel - partLevel);
                    ఒకవేళ (level_సరిపోలిక) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_సరిపోలిక ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ఆపు;
                }
                ఒకవేళ (part_చివర == scope.నిడివి()) ఆపు;
                ++partLevel;
            }
        }
        తిరిగి enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

తరగతి V3_Control_Resolver_Hier_కార్మికుడు_నమోదు final {
    స్థిరరాశి పూర్ణాంకం m_workers;  // Number of workers requested for the hierarchical block
    File_పంక్తి* స్థిరరాశి m_flp;  // Location the worker count was specified at

బహిరంగ:
    స్పష్టమైన V3_Control_Resolver_Hier_కార్మికుడు_నమోదు(పూర్ణాంకం workers, File_పంక్తి* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    పూర్ణాంకం workers() స్థిరరాశి { తిరిగి m_workers; }
    File_పంక్తి* flp() స్థిరరాశి { తిరిగి m_flp; }
};

తరగతి V3ControlResolver final {
    గణన Profile_దత్తాంశం_బహుళకం : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_దత్తాంశం;  // Access to profile_data records
    uint8_t m_బహుళకం = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_కార్మికుడు_నమోదు> m_hierWorkers;
    File_పంక్తి* m_profile_File_పంక్తి = శూన్యసూచిక;  // Location profile_data was read from

    V3ControlResolver() = అప్రమేయ;
    ~V3ControlResolver() = అప్రమేయ;

బహిరంగ:
    స్థిర V3ControlResolver& s() {
        స్థిర V3ControlResolver s_singleton;
        తిరిగి s_singleton;
    }
    V3ControlModuleResolver& modules() { తిరిగి m_modules; }
    V3ControlFileResolver& files() { తిరిగి m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { తిరిగి m_scopeTraces; }

    శూన్య కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& hierDpi, uint64_t ఖర్చు) {
        // Empty key for hierarchical DPI wrapper costs.
        కూడు_Profile_దత్తాంశం(fl, hierDpi, "", ఖర్చు, HIER_DPI);
    }
    శూన్య కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& model, స్థిరరాశి string& కీలకం, uint64_t ఖర్చు,
                        Profile_దత్తాంశం_బహుళకం బహుళకం = MTASK) {
        ఒకవేళ (!m_profile_File_పంక్తి) m_profile_File_పంక్తి = fl;
        ఒకవేళ (ఖర్చు == 0) ఖర్చు = 1;  // Cost 0 means delete (or no data)
        ఒకవేళ (బహుళకం == MTASK) {
            m_profile_దత్తాంశం[model][కీలకం] += ఖర్చు;
        } లేకపోతే ఒకవేళ (బహుళకం == HIER_DPI) {
            m_profile_దత్తాంశం[model][కీలకం] = std::max(m_profile_దత్తాంశం[model][కీలకం], ఖర్చు);
        }
        m_బహుళకం |= బహుళకం;
    }
    తార్కిక కలిగియుంది_MTask_Profile_దత్తాంశం() స్థిరరాశి { తిరిగి m_బహుళకం & MTASK; }
    uint64_t get_Profile_దత్తాంశం(స్థిరరాశి string& hierDpi) స్థిరరాశి {
        // Empty key for hierarchical DPI wrapper costs.
        తిరిగి get_Profile_దత్తాంశం(hierDpi, "");
    }
    శూన్య కూడు_Hier_Workers(File_పంక్తి* flp, స్థిరరాశి string& model, పూర్ణాంకం workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    పూర్ణాంకం getHierWorkers(స్థిరరాశి string& model) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక mit = m_hierWorkers.కనుగొను(model);
        // Assign a single worker if no specified.
        తిరిగి mit != m_hierWorkers.cend() ? mit->రెండవ.workers() : 0;
    }
    File_పంక్తి* get_Hier_Workers_File_పంక్తి(స్థిరరాశి string& model) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక mit = m_hierWorkers.కనుగొను(model);
        తిరిగి mit != m_hierWorkers.cend() ? mit->రెండవ.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_దత్తాంశం(స్థిరరాశి string& model, స్థిరరాశి string& కీలకం) స్థిరరాశి {
        స్థిరరాశి స్వయంచాలక mit = m_profile_దత్తాంశం.కనుగొను(model);
        ఒకవేళ (mit == m_profile_దత్తాంశం.cend()) తిరిగి 0;
        స్థిరరాశి స్వయంచాలక it = mit->రెండవ.కనుగొను(కీలకం);
        ఒకవేళ (it == mit->రెండవ.cend()) తిరిగి 0;
        తిరిగి it->రెండవ;
    }
    File_పంక్తి* get_Profile_దత్తాంశం_File_పంక్తి() స్థిరరాశి { తిరిగి m_profile_File_పంక్తి; }  // Maybe null
    స్థిర uint64_t get_Current_Hier_ఖండం_ఖర్చు() {
        ఒకవేళ (uint64_t ఖర్చు = V3Control::get_Profile_దత్తాంశం(v3Global.opt.ఉపసర్గ())) {
            UINFO(9, "Fetching cost from profile info: " << ఖర్చు);
            తిరిగి ఖర్చు;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* స్థిరరాశి netlistp = v3Global.rootp();
        uint64_t ఖర్చు = 0;
        కొరకు (పూర్ణాంకం i = 0; i < VEval::ENUM_చివర; ++i) {
            స్థిరరాశి VEval eval{i};
            ఒకవేళ (eval.slow()) కొనసాగు;
            ఖర్చు += V3_Instr_లెక్క::లెక్క(netlistp->evalFuncp(eval), అబద్ధం);
        }
        UINFO(9, "Evaluating cost: " << ఖర్చు);
        తిరిగి ఖర్చు;
    }
};

//######################################################################
// V3Control

శూన్య V3Control::కూడు_Case_నిండిన(స్థిరరాశి string& filename, పూర్ణాంకం lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.కూడు_పంక్తి_Attribute(lineno, VPragmaType::నిండిన_CASE);
}
శూన్య V3Control::కూడు_Case_Parallel(స్థిరరాశి string& filename, పూర్ణాంకం lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.కూడు_పంక్తి_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
శూన్య V3Control::కూడు_Coverage_ఖండం_Off(స్థిరరాశి string& filename, పూర్ణాంకం lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.కూడు_పంక్తి_Attribute(lineno, VPragmaType::COVERAGE_ఖండం_OFF);
}
శూన్య V3Control::కూడు_Coverage_ఖండం_Off(స్థిరరాశి string& module, స్థిరరాశి string& blockname) {
    V3ControlResolver::s().modules().at(module).కూడు_Coverage_ఖండం_Off(blockname);
}

శూన్య V3Control::కూడు_Hier_Workers(File_పంక్తి* fl, స్థిరరాశి string& model, పూర్ణాంకం workers) {
    V3ControlResolver::s().కూడు_Hier_Workers(fl, model, workers);
}

శూన్య V3Control::కూడు_Fsm_రిజిస్టరు_Wrapper(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& d,
                                      స్థిరరాశి string& q, స్థిరరాశి string& clock, స్థిరరాశి string& reset,
                                      స్థిరరాశి string& reset_విలువ) {
    string missing;
    ఒకవేళ (module.ఖాళీలు()) missing += "-module";
    ఒకవేళ (d.ఖాళీలు()) missing = VString::dot(missing, ", ", "-d");
    ఒకవేళ (q.ఖాళీలు()) missing = VString::dot(missing, ", ", "-q");
    ఒకవేళ (clock.ఖాళీలు()) missing = VString::dot(missing, ", ", "-clock");
    ఒకవేళ (!missing.ఖాళీలు()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        తిరిగి;
    }

    స్థిరరాశి Fsm_రిజిస్టరు_Wrapper desc{module, d, q, clock, reset, reset_విలువ};
    V3ControlResolver::s().modules().at(module).సమితి_Fsm_రిజిస్టరు_Wrapper(fl, desc);
}

శూన్య V3Control::కూడు_Ignore(V3_Error_సంకేతం సంకేతం_2, తార్కిక on, స్థిరరాశి string& filename, పూర్ణాంకం min, పూర్ణాంకం max) {
    UINFO(9, "addIgnore " << సంకేతం_2 << " " << min << "-" << max << " fn=" << filename);
    ఒకవేళ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_పంక్తి::globalWarnOff(సంకేతం_2, !on);
    } లేకపోతే {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).కూడు_Ignore(సంకేతం_2, min, max, on);
    }
}
శూన్య V3Control::కూడు_Ignore_సరిపోలిక(V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& filename, స్థిరరాశి string& contents,
                               స్థిరరాశి string& సరిపోలిక) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).కూడు_Ignore_సరిపోలిక(సంకేతం_2, contents, సరిపోలిక);
}

శూన్య V3Control::కూడు_Inline(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& ftask, తార్కిక on) {
    ఒకవేళ (ftask.ఖాళీలు()) {
        V3ControlResolver::s().modules().at(module).సమితి_Inline(on);
    } లేకపోతే {
        ఒకవేళ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } లేకపోతే {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).సమితి_No_Inline(on);
        }
    }
}

శూన్య V3Control::కూడు_Module_Pragma(స్థిరరాశి string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).కూడు_Module_Pragma(pragma);
}

శూన్య V3Control::కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& hierDpi, uint64_t ఖర్చు) {
    V3ControlResolver::s().కూడు_Profile_దత్తాంశం(fl, hierDpi, ఖర్చు);
}
శూన్య V3Control::కూడు_Profile_దత్తాంశం(File_పంక్తి* fl, స్థిరరాశి string& model, స్థిరరాశి string& కీలకం,
                               uint64_t ఖర్చు) {
    V3ControlResolver::s().కూడు_Profile_దత్తాంశం(fl, model, కీలకం, ఖర్చు);
}

శూన్య V3Control::కూడు_Scope_Trace_On(తార్కిక on, స్థిరరాశి string& scope, పూర్ణాంకం స్థాయిలు) {
    V3ControlResolver::s().scopeTraces().కూడు_Scope_Trace_On(on, scope, స్థాయిలు);
}

శూన్య V3Control::కూడు_Var_Attr(File_పంక్తి* fl, స్థిరరాశి string& module, స్థిరరాశి string& ftask,
                           Var_Spec_రకం రకం, స్థిరరాశి string& నమూనా_2, VAttrType attr,
                           Ast_Sen_వృక్షం* sensep) {
    ఒకవేళ (sensep) {
        File_పంక్తి* స్థిరరాశి flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_వృక్షం(), sensep);
        // Used to be only accepted on public_flat_rw
        ఒకవేళ (attr != VAttrType::VAR_ప్రజా_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            తిరిగి;
        }
    }

    ఒకవేళ (రకం != Var_Spec_రకం::VAR) {
        ఎంపిక (attr) {
        స్థితి VAttrType::VAR_ప్రజా_FLAT:
        స్థితి VAttrType::VAR_ప్రజా_FLAT_RD:
        స్థితి VAttrType::VAR_ప్రజా_FLAT_RW: ఆపు;
        అప్రమేయ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            తిరిగి;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ఒకవేళ (నమూనా_2.ఖాళీలు()) {
        ఒకవేళ (attr == VAttrType::VAR_ప్రజా) {
            ఒకవేళ (ftask.ఖాళీలు()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).కూడు_Module_Pragma(
                    VPragmaType::ప్రజా_MODULE);
            } లేకపోతే {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).సమితి_ప్రజా(నిజం);
            }
        } లేకపోతే {
            fl->v3error("missing -var");
        }
    } లేకపోతే {
        ఒకవేళ (attr == VAttrType::VAR_FORCEABLE) {
            ఒకవేళ (module.ఖాళీలు()) {
                fl->v3error("forceable missing -module");
            } లేకపోతే ఒకవేళ (!ftask.ఖాళీలు()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } లేకపోతే {
                V3ControlResolver::s().modules().at(module).vars().at(నమూనా_2).కూడు(attr);
            }
        } లేకపోతే {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ఒకవేళ (ftask.ఖాళీలు()) {
                    ఒకవేళ (రకం == Var_Spec_రకం::PARAM) తిరిగి mod.params().at(నమూనా_2);
                    ఒకవేళ (రకం == Var_Spec_రకం::పోర్టు) తిరిగి mod.ports().at(నమూనా_2);
                    UASSERT_OBJ(రకం == Var_Spec_రకం::VAR, fl, "Unexpected VarSpecKind");
                    తిరిగి mod.vars().at(నమూనా_2);
                }
                ఒకవేళ (రకం == Var_Spec_రకం::PARAM) తిరిగి mod.ftasks().at(ftask).params().at(నమూనా_2);
                ఒకవేళ (రకం == Var_Spec_రకం::పోర్టు) తిరిగి mod.ftasks().at(ftask).ports().at(నమూనా_2);
                UASSERT_OBJ(రకం == Var_Spec_రకం::VAR, fl, "Unexpected VarSpecKind");
                తిరిగి mod.ftasks().at(ftask).vars().at(నమూనా_2);
            }();
            controlVar.కూడు(attr);
        }
    }
}

శూన్య V3Control::applyCase(AstCase* nodep) {
    స్థిరరాశి string& filename = nodep->fileline()->filename();
    V3ControlFile* స్థిరరాశి filep = V3ControlResolver::s().files().resolve(filename);
    ఒకవేళ (filep) filep->applyCase(nodep);
}

శూన్య V3Control::apply_Coverage_ఖండం(Ast_కణుపు_Module* modulep, AstBegin* nodep) {
    స్థిరరాశి string& filename = nodep->fileline()->filename();
    V3ControlFile* స్థిరరాశి filep = V3ControlResolver::s().files().resolve(filename);
    ఒకవేళ (filep) filep->apply_ఖండం(nodep);
    స్థిరరాశి string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* స్థిరరాశి modp = V3ControlResolver::s().modules().resolve(modname);
    ఒకవేళ (modp) modp->apply_ఖండం(nodep);
}
శూన్య V3Control::apply_Coverage_ఖండం(Ast_కణుపు_Module* modulep, Ast_Gen_ఖండం* nodep) {
    స్థిరరాశి string& filename = nodep->fileline()->filename();
    V3ControlFile* స్థిరరాశి filep = V3ControlResolver::s().files().resolve(filename);
    ఒకవేళ (filep) filep->apply_ఖండం(nodep);
    స్థిరరాశి string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* స్థిరరాశి modp = V3ControlResolver::s().modules().resolve(modname);
    ఒకవేళ (modp) modp->apply_ఖండం(nodep);
}

శూన్య V3Control::applyIgnores(File_పంక్తి* filelinep) {
    స్థిరరాశి string& filename = filelinep->filename();
    V3ControlFile* స్థిరరాశి filep = V3ControlResolver::s().files().resolve(filename);
    ఒకవేళ (filep) filep->applyIgnores(filelinep);
}

శూన్య V3Control::applyModule(Ast_కణుపు_Module* modulep) {
    స్థిరరాశి string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* స్థిరరాశి modp = V3ControlResolver::s().modules().resolve(modname);
    ఒకవేళ (modp) modp->apply(modulep);
}

శూన్య V3Control::applyFTask(Ast_కణుపు_Module* modulep, Ast_కణుపు_FTask* ftaskp) {
    స్థిరరాశి string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* స్థిరరాశి modp = V3ControlResolver::s().modules().resolve(modname);
    ఒకవేళ (!modp) తిరిగి;
    స్థిరరాశి V3ControlFTask* స్థిరరాశి ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ఒకవేళ (ftp) ftp->apply(ftaskp);
}

మూస <రకం_పేరు T_Resolver>
స్థిర శూన్య resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    స్థిరరాశి std::string name = varp->prettyDehashOrigOrName();
    ఒకవేళ (స్థిరరాశి V3ControlVar* స్థిరరాశి vp = resolver.vars().resolve(name)) vp->apply(varp);
    ఒకవేళ (varp->isParam()) {
        ఒకవేళ (స్థిరరాశి V3ControlVar* స్థిరరాశి vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ఒకవేళ (varp->isIO()) {
        ఒకవేళ (స్థిరరాశి V3ControlVar* స్థిరరాశి vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

శూన్య V3Control::applyVarAttr(స్థిరరాశి Ast_కణుపు_Module* modulep, స్థిరరాశి Ast_కణుపు_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* స్థిరరాశి modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ఒకవేళ (!modp) తిరిగి;
    ఒకవేళ (ftaskp) {
        V3ControlFTask* స్థిరరాశి ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ఒకవేళ (!ftp) తిరిగి;
        resolveThenApply(*ftp, varp);
    } లేకపోతే {
        resolveThenApply(*modp, varp);
    }
}

పూర్ణాంకం V3Control::getHierWorkers(స్థిరరాశి string& model) {
    తిరిగి V3ControlResolver::s().getHierWorkers(model);
}
File_పంక్తి* V3Control::get_Hier_Workers_File_పంక్తి(స్థిరరాశి string& model) {
    తిరిగి V3ControlResolver::s().get_Hier_Workers_File_పంక్తి(model);
}
స్థిరరాశి V3Control::Fsm_రిజిస్టరు_Wrapper* V3Control::get_Fsm_రిజిస్టరు_Wrapper(స్థిరరాశి string& module) {
    V3ControlModule* స్థిరరాశి modp = V3ControlResolver::s().modules().resolve(module);
    తిరిగి modp ? modp->fsm_రిజిస్టరు_Wrapperp() : శూన్యసూచిక;
}
uint64_t V3Control::get_Profile_దత్తాంశం(స్థిరరాశి string& hierDpi) {
    తిరిగి V3ControlResolver::s().get_Profile_దత్తాంశం(hierDpi);
}
uint64_t V3Control::get_Profile_దత్తాంశం(స్థిరరాశి string& model, స్థిరరాశి string& కీలకం) {
    తిరిగి V3ControlResolver::s().get_Profile_దత్తాంశం(model, కీలకం);
}
File_పంక్తి* V3Control::get_Profile_దత్తాంశం_File_పంక్తి() {
    తిరిగి V3ControlResolver::s().get_Profile_దత్తాంశం_File_పంక్తి();
}
తార్కిక V3Control::getScopeTraceOn(స్థిరరాశి string& scope) {
    తిరిగి V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

శూన్య V3Control::contents_నెట్టు_పాఠ్యం(స్థిరరాశి string& పాఠ్యం) { WildcardContents::నెట్టు_పాఠ్యం(పాఠ్యం); }

తార్కిక V3Control::కలిగియుంది_MTask_Profile_దత్తాంశం() {
    తిరిగి V3ControlResolver::s().కలిగియుంది_MTask_Profile_దత్తాంశం();
}
uint64_t V3Control::get_Current_Hier_ఖండం_ఖర్చు() {
    తిరిగి V3ControlResolver::s().get_Current_Hier_ఖండం_ఖర్చు();
}

తార్కిక V3Control::waive(స్థిరరాశి File_పంక్తి* filelinep, V3_Error_సంకేతం సంకేతం_2, స్థిరరాశి string& సందేశం) {
    V3ControlFile* స్థిరరాశి filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ఒకవేళ (!filep) తిరిగి అబద్ధం;
    తిరిగి filep->waive(సంకేతం_2, సందేశం);
}

శూన్య V3Control::selfTest() { VInterval_వృక్షం::selfTest(); }
