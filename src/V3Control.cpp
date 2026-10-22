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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3Control.h"

#अन्तर्भाव "V3InstrCount.h"
#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <memory>
#अन्तर्भाव <set>
#अन्तर्भाव <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
प्रतिरूपम् <प्रकारनाम T>
वर्ग V3ControlWildcardResolver final {
    परिवर्तनीय V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ध्रुव std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ध्रुव std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

सार्वजनिक:
    V3ControlWildcardResolver() = स्वभाव;
    ~V3ControlWildcardResolver() = स्वभाव;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    शून्य update(ध्रुव V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        ध्रुव V3_तालकम्_Guard other_तालकम्{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        प्रति (ध्रुव स्वचालित& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ध्रुव string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        प्रत्यागम m_mapPatterns[V3Os::filename_Slash_मार्गः(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ध्रुव string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3_तालकम्_Guard तालकम्_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ध्रुव स्वचालित pair = m_mapResolved.emplace(name, शून्यसूचकः);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        यदि (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ध्रुव string slashName = V3Os::filename_Slash_मार्गः(name);
            प्रति (ध्रुव स्वचालित& patEnt : m_mapPatterns) {
                यदि (VString::wildmatch(slashName, patEnt.first)) {
                    यदि (!entryr) entryr.reset(नूतन T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            यदि (entryr) entryr->updateFinalize();
        }
        प्रत्यागम entryr.get();
    }
};

// List of attributes for variables
वर्ग V3ControlVar final {
    std::सदिशम्<VAttrType> m_attrs;  // The list of attributes
सार्वजनिक:
    // Add new attribugte
    शून्य सङ्कलय(VAttrType attr) { m_attrs.emplace_पृष्ठभागः(attr); }
    // Update from other by copying all attributes
    शून्य update(ध्रुव V3ControlVar& other) {
        m_attrs.reserve(m_attrs.आकार() + other.m_attrs.आकार());
        m_attrs.निवेशय(m_attrs.अन्तः_2(), other.m_attrs.begin(), other.m_attrs.अन्तः_2());
    }
    शून्य updateFinalize() {}
    // Apply all attributes to the variable
    शून्य apply(AstVar* varp) ध्रुव {
        प्रति (ध्रुव VAttrType attr : m_attrs) {
            varp->सङ्कलय_Attrsp(नूतन Ast_Attr_सम्बन्धि{varp->fileline(), attr});
        }
    }
};

उपयुज्य V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

वर्ग WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ध्रुव std::string, तार्किक> m_mapPatterns;  // Pattern match results
    std::deque<string> m_पङ्क्तयः;  // Source text lines

    // METHODS
    स्थिर WildcardContents& s() {  // Singleton
        स्थिर WildcardContents s_s;
        प्रत्यागम s_s;
    }
    शून्य clearCacheImp() { m_mapPatterns.clear(); }
    शून्य प्रक्षिप_Text_Imp(ध्रुव string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ध्रुव string leftover = m_पङ्क्तयः.पृष्ठभागः() + text;
        m_पङ्क्तयः.निष्कासय_पृष्ठभागः();

        // Insert line-by-line
        string::आकार_type पङ्क्तिः_start = 0;
        यावत् (सत्य) {
            ध्रुव string::आकार_type पङ्क्तिः_अन्तः = leftover.find('\n', पङ्क्तिः_start);
            यदि (पङ्क्तिः_अन्तः != string::npos) {
                ध्रुव string oneline(leftover, पङ्क्तिः_start, पङ्क्तिः_अन्तः - पङ्क्तिः_start + 1);
                यदि (oneline.आकार() > 1) m_पङ्क्तयः.प्रक्षिप_पृष्ठभागः(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_पङ्क्तयः.आकार() - 1) << "]: " << oneline);
                पङ्क्तिः_start = पङ्क्तिः_अन्तः + 1;
            } अन्यथा {
                भङ्ग;
            }
        }
        // Keep leftover for next time
        m_पङ्क्तयः.emplace_पृष्ठभागः(string(leftover, पङ्क्तिः_start));  // Might be ""
        clearCacheImp();
    }

    तार्किक resolveUncachedImp(ध्रुव string& contentsRegexp) {
        प्रति (ध्रुव string& i : m_पङ्क्तयः) {
            यदि (VString::wildmatch(i, contentsRegexp)) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }
    तार्किक resolveCachedImp(ध्रुव string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        यदि (contentsRegexp.रिक्तानि()) प्रत्यागम सत्य;
        ध्रुव स्वचालित pair = m_mapPatterns.emplace(contentsRegexp, असत्य);
        तार्किक& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        यदि (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        प्रत्यागम entryr;
    }

सार्वजनिक:
    WildcardContents() {
        m_पङ्क्तयः.emplace_पृष्ठभागः("");  // start with no leftover
    }
    ~WildcardContents() = स्वभाव;
    // Return true iff contentsRegexp in parsed contents
    स्थिर तार्किक resolve(ध्रुव string& contentsRegexp) {
        प्रत्यागम s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    स्थिर शून्य प्रक्षिप_Text(ध्रुव string& text) { s().प्रक्षिप_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

वर्ग V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    तार्किक m_noinline = असत्य;  // Don't inline function/task
    तार्किक m_लौकिकम् = असत्य;  // Public function/task

सार्वजनिक:
    V3ControlFTask() = स्वभाव;
    शून्य update(ध्रुव V3ControlFTask& f) {
        // Don't overwrite true with false
        यदि (f.m_noinline) m_noinline = सत्य;
        यदि (f.m_लौकिकम्) m_लौकिकम् = सत्य;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    शून्य updateFinalize() {}

    V3ControlVarResolver& params() { प्रत्यागम m_params; }
    V3ControlVarResolver& ports() { प्रत्यागम m_ports; }
    V3ControlVarResolver& vars() { प्रत्यागम m_vars; }

    शून्य समुच्चयः_No_Inline(तार्किक समुच्चयः) { m_noinline = समुच्चयः; }
    शून्य समुच्चयः_लौकिकम्(तार्किक समुच्चयः) { m_लौकिकम् = समुच्चयः; }

    शून्य apply(Ast_ग्रन्थिः_FTask* ftaskp) ध्रुव {
        यदि (m_noinline)
            ftaskp->सङ्कलय_Stmtsp(नूतन AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        यदि (m_लौकिकम्)
            ftaskp->सङ्कलय_Stmtsp(नूतन AstPragma{ftaskp->fileline(), VPragmaType::लौकिकम्_TASK});
    }
};

उपयुज्य V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

वर्ग V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_पञ्जिकाकोशः_Wrapper m_fsm_पञ्जिकाकोशः_Wrapper;  // FSM wrapper descriptor
    std::unordered_समुच्चयः<std::string> m_coverage_Off_खण्डानि;  // List of block names for coverage_off
    std::समुच्चयः<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    तार्किक m_has_Fsm_पञ्जिकाकोशः_Wrapper = असत्य;  // Whether descriptor has been set
    तार्किक m_inline = असत्य;  // Whether to force the inline
    तार्किक m_inline_मानम् = असत्य;  // The inline value (on/off)

सार्वजनिक:
    V3ControlModule() = स्वभाव;

    शून्य update(ध्रुव V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        यदि (m.m_has_Fsm_पञ्जिकाकोशः_Wrapper) {
            m_fsm_पञ्जिकाकोशः_Wrapper = m.m_fsm_पञ्जिकाकोशः_Wrapper;
            m_has_Fsm_पञ्जिकाकोशः_Wrapper = सत्य;
        }
        प्रति (ध्रुव string& i : m.m_coverage_Off_खण्डानि) m_coverage_Off_खण्डानि.निवेशय(i);
        यदि (!m_inline) {
            m_inline = m.m_inline;
            m_inline_मानम् = m.m_inline_मानम्;
        }
        प्रति (स्वचालित it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.निवेशय(*it);
        }
    }
    शून्य updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { प्रत्यागम m_tasks; }
    V3ControlVarResolver& params() { प्रत्यागम m_params; }
    V3ControlVarResolver& ports() { प्रत्यागम m_ports; }
    V3ControlVarResolver& vars() { प्रत्यागम m_vars; }

    शून्य सङ्कलय_Coverage_खण्डम्_Off(ध्रुव string& name) { m_coverage_Off_खण्डानि.निवेशय(name); }
    शून्य समुच्चयः_Inline(तार्किक समुच्चयः) {
        m_inline = सत्य;
        m_inline_मानम् = समुच्चयः;
    }
    शून्य समुच्चयः_Fsm_पञ्जिकाकोशः_Wrapper(File_पङ्क्तिः* fl, ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper& desc) {
        यदि (m_has_Fsm_पञ्जिकाकोशः_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ग्रन्थिः::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_पञ्जिकाकोशः_Wrapper = desc;
        m_has_Fsm_पञ्जिकाकोशः_Wrapper = सत्य;
    }
    ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper* fsm_पञ्जिकाकोशः_Wrapperp() ध्रुव {
        प्रत्यागम m_has_Fsm_पञ्जिकाकोशः_Wrapper ? &m_fsm_पञ्जिकाकोशः_Wrapper : शून्यसूचकः;
    }
    शून्य सङ्कलय_Module_Pragma(VPragmaType pragma) { m_modPragmas.निवेशय(pragma); }

    शून्य apply(Ast_ग्रन्थिः_Module* modp) {
        यदि (m_inline) {
            ध्रुव VPragmaType type
                = m_inline_मानम् ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ग्रन्थिः* ध्रुव nodep = नूतन AstPragma{modp->fileline(), type};
            modp->सङ्कलय_Stmtsp(nodep);
        }
        प्रति (ध्रुव स्वचालित& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            यदि (itr == VPragmaType::HIER_PARAMS) modp->hierParams(सत्य);
            Ast_ग्रन्थिः* ध्रुव nodep = नूतन AstPragma{modp->fileline(), itr};
            modp->सङ्कलय_Stmtsp(nodep);
        }
    }

    शून्य apply_खण्डम्(Ast_Gen_खण्डम्* nodep) {
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_खण्डम्_OFF;
        यदि (!nodep->unnamed()) {
            प्रति (ध्रुव string& i : m_coverage_Off_खण्डानि) {
                यदि (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->सङ्कलय_Itemsp(नूतन AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    शून्य apply_खण्डम्(Ast_ग्रन्थिः_खण्डम्* nodep) {
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_खण्डम्_OFF;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (!nodep->unnamed()) {
            प्रति (ध्रुव string& i : m_coverage_Off_खण्डानि) {
                यदि (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->सङ्कलय_Stmtsp(नूतन AstStmtPragma{flp, नूतन AstPragma{flp, pragma}});
                }
            }
        }
    }
};

उपयुज्य V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
वर्ग V3_Control_Ignores_पङ्क्तिः final {
सार्वजनिक:
    ध्रुव पूर्णाङ्क m_पङ्क्तिः_Min;  // Minimum line number to make change at
    ध्रुव पूर्णाङ्क m_पङ्क्तिः_Max;  // Maximum line number to make change at (inclusive)
    ध्रुव V3_Error_संहिता m_संहिता;  // Error code
    ध्रुव तार्किक m_on;  // True to enable message
    V3_Control_Ignores_पङ्क्तिः(V3_Error_संहिता संहिता, पूर्णाङ्क linemin, पूर्णाङ्क linemax, तार्किक on)
        : m_पङ्क्तिः_Min{linemin}
        , m_पङ्क्तिः_Max{linemax}
        , m_संहिता{संहिता}
        , m_on{on} {}
    ~V3_Control_Ignores_पङ्क्तिः() = स्वभाव;
    तार्किक every_पङ्क्तिः() ध्रुव { प्रत्यागम m_पङ्क्तिः_Min == 0 && m_पङ्क्तिः_Max == 0; }
    तार्किक अन्तर्_Range(पूर्णाङ्क lineno) ध्रुव {
        प्रत्यागम (lineno >= m_पङ्क्तिः_Min && lineno <= m_पङ्क्तिः_Max) || every_पङ्क्तिः();
    }
};
std::ostream& संक्रियकः<<(std::ostream& os, ध्रुव V3_Control_Ignores_पङ्क्तिः& rhs) {
    प्रत्यागम os << rhs.m_पङ्क्तिः_Min << "-" << rhs.m_पङ्क्तिः_Max << ", " << rhs.m_संहिता << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
स्थिर std::सदिशम्<V3_Control_Ignores_पङ्क्तिः> control_Ign_पङ्क्तयः;

उपयुज्य IgnIndices = std::सदिशम्<uint32_t>;  // List of {s_ignLines indces}

वर्ग VInterval_वृक्षः final {
    संरचना प्रविष्टिः_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        पूर्णाङ्क m_center;  // Line number of central point
        std::unique_ptr<प्रविष्टिः_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<प्रविष्टिः_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        स्पष्ट प्रविष्टिः_2(पूर्णाङ्क center)
            : m_center{center} {}
    };

    std::unique_ptr<प्रविष्टिः_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_पङ्क्तयः;  // All-line disables

गुप्त:
    std::unique_ptr<प्रविष्टिः_2> build_वृक्षः(ध्रुव IgnIndices& points) {
        यदि (points.रिक्तानि()) प्रत्यागम शून्यसूचकः;

        पूर्णाङ्क minval = std::numeric_limits<पूर्णाङ्क>::max();
        पूर्णाङ्क maxval = std::numeric_limits<पूर्णाङ्क>::min();
        प्रति (ध्रुव स्वचालित& it : points) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            यदि (cign.every_पङ्क्तिः()) {
                m_every_पङ्क्तयः.emplace_पृष्ठभागः(it);
                अनुवर्त;
            }
            minval = std::min(minval, cign.m_पङ्क्तिः_Min);
            maxval = std::max(maxval, cign.m_पङ्क्तिः_Max);
        }

        ध्रुव पूर्णाङ्क center = minval + (maxval - minval) / 2;
        स्वचालित entp = std::unique_ptr<प्रविष्टिः_2>(नूतन प्रविष्टिः_2{center});

        IgnIndices वामः_Points;
        IgnIndices दक्षिणः_Points;
        प्रति (ध्रुव स्वचालित& it : points) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            यदि (cign.every_पङ्क्तिः()) अनुवर्त;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            यदि (cign.m_पङ्क्तिः_Max < center)
                वामः_Points.emplace_पृष्ठभागः(it);
            अन्यथा यदि (cign.m_पङ्क्तिः_Min > center)
                दक्षिणः_Points.emplace_पृष्ठभागः(it);
            अन्यथा {
                entp->m_byMin.प्रक्षिप_पृष्ठभागः(it);
                entp->m_byMax.प्रक्षिप_पृष्ठभागः(it);
            }
        }

        std::क्रमय(entp->m_byMin.begin(), entp->m_byMin.अन्तः_2(), [](uint32_t a, uint32_t b) {
            प्रत्यागम control_Ign_पङ्क्तयः[a].m_पङ्क्तिः_Min < control_Ign_पङ्क्तयः[b].m_पङ्क्तिः_Min;
        });
        std::क्रमय(entp->m_byMax.begin(), entp->m_byMax.अन्तः_2(), [](uint32_t a, uint32_t b) {
            प्रत्यागम control_Ign_पङ्क्तयः[a].m_पङ्क्तिः_Max > control_Ign_पङ्क्तयः[b].m_पङ्क्तिः_Max;
        });

        entp->m_leftp = build_वृक्षः(वामः_Points);
        entp->m_rightp = build_वृक्षः(दक्षिणः_Points);
        प्रत्यागम entp;
    }

    शून्य find_वृक्षः(पूर्णाङ्क lineno, प्रविष्टिः_2* entp, IgnIndices& resultsr, पूर्णाङ्क& अग्र_Changer) ध्रुव {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        यदि (lineno < entp->m_center) {
            अग्र_Changer = std::min(अग्र_Changer, entp->m_center);
            प्रति (ध्रुव स्वचालित& it : entp->m_byMin) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                यदि (cign.m_पङ्क्तिः_Min <= lineno) {
                    resultsr.emplace_पृष्ठभागः(it);
                } अन्यथा {
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पङ्क्तिः_Min);
                    भङ्ग;
                }
            }
            यदि (entp->m_leftp) find_वृक्षः(lineno, entp->m_leftp.get(), resultsr, अग्र_Changer);
        } अन्यथा यदि (lineno > entp->m_center) {
            प्रति (ध्रुव स्वचालित& it : entp->m_byMax) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                यदि (cign.m_पङ्क्तिः_Max >= lineno) {
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पङ्क्तिः_Max + 1);
                    resultsr.emplace_पृष्ठभागः(it);
                } अन्यथा {
                    भङ्ग;
                }
            }
            यदि (entp->m_rightp) find_वृक्षः(lineno, entp->m_rightp.get(), resultsr, अग्र_Changer);
        } अन्यथा {
            प्रति (ध्रुव स्वचालित& it : entp->m_byMin) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                resultsr.emplace_पृष्ठभागः(it);
                यदि (cign.m_पङ्क्तिः_Max >= lineno)
                    अग्र_Changer = std::min(अग्र_Changer, cign.m_पङ्क्तिः_Max + 1);
            }
            यदि (entp->m_leftp) find_वृक्षः(lineno, entp->m_leftp.get(), resultsr, अग्र_Changer);
            यदि (entp->m_rightp) find_वृक्षः(lineno, entp->m_rightp.get(), resultsr, अग्र_Changer);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    VInterval_वृक्षः() = स्वभाव;
    ~VInterval_वृक्षः() = स्वभाव;
    // METHODS
    शून्य clear() { m_rootp = शून्यसूचकः; }
    शून्य build(ध्रुव IgnIndices& points) {
        clear();
        m_rootp = build_वृक्षः(points);
    }
    शून्य find(पूर्णाङ्क lineno, IgnIndices& resultsr, पूर्णाङ्क& अग्र_Changer) ध्रुव {
        resultsr = m_every_पङ्क्तयः;
        अग्र_Changer = std::numeric_limits<पूर्णाङ्क>::max();

        यदि (m_rootp) find_वृक्षः(lineno, m_rootp.get(), resultsr, अग्र_Changer);
        // Sort indices, so can process in parse order
        std::क्रमय(resultsr.begin(), resultsr.अन्तः_2());
    }

    स्थिर शून्य selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices दत्त;
        std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        प्रति (ध्रुव स्वचालित& it : points) {
            control_Ign_पङ्क्तयः.emplace_पृष्ठभागः(
                V3_Control_Ignores_पङ्क्तिः{V3_Error_संहिता::I_LINT, it.first, it.second, सत्य});
            दत्त.emplace_पृष्ठभागः(static_cast<uint32_t>(control_Ign_पङ्क्तयः.आकार() - 1));
        }
        VInterval_वृक्षः वृक्षः;
        वृक्षः.build(दत्त);

        IgnIndices results;
        पूर्णाङ्क अग्र_Change = 0;
        वृक्षः.find(0, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 0);
        UASSERT_SELFTEST(अग्र_Change, 10);
        वृक्षः.find(10, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(अग्र_Change, 11);
        वृक्षः.find(11, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(अग्र_Change, 15);  // Center, or would be 20
        वृक्षः.find(20, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(अग्र_Change, 21);
        वृक्षः.find(21, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 25);  // Center, or would be 30
        वृक्षः.find(30, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 31);
        वृक्षः.find(40, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(अग्र_Change, 41);
        वृक्षः.find(41, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 0);
        UASSERT_SELFTEST(अग्र_Change, std::numeric_limits<पूर्णाङ्क>::max());
        //
        points = {{0, 0}};
        प्रति (ध्रुव स्वचालित& it : points) {
            control_Ign_पङ्क्तयः.emplace_पृष्ठभागः(
                V3_Control_Ignores_पङ्क्तिः{V3_Error_संहिता::I_LINT, it.first, it.second, सत्य});
            दत्त.emplace_पृष्ठभागः(static_cast<uint32_t>(control_Ign_पङ्क्तयः.आकार() - 1));
        }
        वृक्षः.build(दत्त);
        //
        वृक्षः.find(50, results, अग्र_Change);
        UASSERT_SELFTEST(results.आकार(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
उपयुज्य V3_Control_पङ्क्तिः_Attribute = std::bitset<VPragmaType::ENUM_आकार>;

वर्ग WaiverSetting final {
सार्वजनिक:
    V3_Error_संहिता m_संहिता;  // Error code
    string m_contents;  // --contents regexp
    string m_मेलनम्;  // --match regexp
    WaiverSetting(V3_Error_संहिता संहिता, ध्रुव string& contents, ध्रुव string& मेलनम्)
        : m_संहिता{संहिता}
        , m_contents{contents}
        , m_मेलनम्{मेलनम्} {}
    ~WaiverSetting() = स्वभाव;
    WaiverSetting& संक्रियकः=(ध्रुव WaiverSetting& rhs) = स्वभाव;
};

// File entity
वर्ग V3ControlFile final {
    उपयुज्य पङ्क्तिः_Attr_Map = std::map<पूर्णाङ्क, V3_Control_पङ्क्तिः_Attribute>;  // Map line->bitset of attributes
    उपयुज्य Waivers = std::सदिशम्<WaiverSetting>;  // List of {code,wildcard string}

    पङ्क्तिः_Attr_Map m_पङ्क्तिः_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_वृक्षः m_interval_वृक्षः;  // Tree of indices to IgnLines

    संरचना {
        पूर्णाङ्क filenameno = -1;  // Last filename
        पूर्णाङ्क lineno = -1;  // Last linenumber
        पूर्णाङ्क अग्र_Change = -1;  // Line number of next change
    } m_चरमः_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    तार्किक पङ्क्तिः_मेलनम्(पूर्णाङ्क lineno, VPragmaType type) {
        यदि (m_पङ्क्तिः_Attrs.find(0) != m_पङ्क्तिः_Attrs.अन्तः_2() && m_पङ्क्तिः_Attrs[0][type]) प्रत्यागम सत्य;
        यदि (m_पङ्क्तिः_Attrs.find(lineno) == m_पङ्क्तिः_Attrs.अन्तः_2()) प्रत्यागम असत्य;
        प्रत्यागम m_पङ्क्तिः_Attrs[lineno][type];
    }

सार्वजनिक:
    V3ControlFile() = स्वभाव;
    शून्य update(ध्रुव V3ControlFile& file) {
        // Copy in all attributes and waivers
        प्रति (ध्रुव स्वचालित& itr : file.m_पङ्क्तिः_Attrs) m_पङ्क्तिः_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.आकार() + file.m_waivers.आकार());
        m_waivers.निवेशय(m_waivers.अन्तः_2(), file.m_waivers.begin(), file.m_waivers.अन्तः_2());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.आकार() + file.m_ignIndices.आकार());
        m_ignIndices.निवेशय(m_ignIndices.अन्तः_2(), file.m_ignIndices.begin(),
                            file.m_ignIndices.अन्तः_2());
        // updateFinalize() will soon build tree
    }
    शून्य updateFinalize() { m_interval_वृक्षः.build(m_ignIndices); }
    शून्य सङ्कलय_पङ्क्तिः_Attribute(पूर्णाङ्क lineno, VPragmaType attr) { m_पङ्क्तिः_Attrs[lineno].समुच्चयः(attr); }
    शून्य सङ्कलय_Ignore(V3_Error_संहिता संहिता, पूर्णाङ्क min, पूर्णाङ्क max, तार्किक on) {
        control_Ign_पङ्क्तयः.emplace_पृष्ठभागः(V3_Control_Ignores_पङ्क्तिः{संहिता, min, max, on});
        m_ignIndices.emplace_पृष्ठभागः(static_cast<uint32_t>(control_Ign_पङ्क्तयः.आकार() - 1));
        m_चरमः_Ignore.अग्र_Change = -1;
    }
    शून्य सङ्कलय_Ignore_मेलनम्(V3_Error_संहिता संहिता, ध्रुव string& contents, ध्रुव string& मेलनम्) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_मेलनम् = मेलनम्;
        यदि (new_मेलनम्.रिक्तानि() || new_मेलनम्.पृष्ठभागः() != '*') new_मेलनम् += '*';
        m_waivers.emplace_पृष्ठभागः(संहिता, contents, new_मेलनम्);
    }

    शून्य apply_खण्डम्(Ast_Gen_खण्डम्* nodep) {
        // Apply to block at this line
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_खण्डम्_OFF;
        यदि (पङ्क्तिः_मेलनम्(nodep->fileline()->lineno(), pragma)) {
            nodep->सङ्कलय_Itemsp(नूतन AstPragma{nodep->fileline(), pragma});
        }
    }
    शून्य apply_खण्डम्(Ast_ग्रन्थिः_खण्डम्* nodep) {
        // Apply to block at this line
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_खण्डम्_OFF;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        यदि (पङ्क्तिः_मेलनम्(flp->lineno(), pragma)) {
            nodep->सङ्कलय_Stmtsp(नूतन AstStmtPragma{flp, नूतन AstPragma{flp, pragma}});
        }
    }
    शून्य applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ध्रुव पूर्णाङ्क lineno = nodep->fileline()->lineno();
        यदि (पङ्क्तिः_मेलनम्(lineno, VPragmaType::पूर्णम्_CASE)) nodep->पूर्णम्_Pragma(सत्य);
        यदि (पङ्क्तिः_मेलनम्(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(सत्य);
    }
    शून्य applyIgnores(File_पङ्क्तिः* filelinep) {
        // HOT routine, called each parsed token line of this filename
        यदि (filelinep->filenameno() == m_चरमः_Ignore.filenameno
            && filelinep->lineno() == m_चरमः_Ignore.lineno)
            प्रत्यागम;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        यदि (filelinep->filenameno() == m_चरमः_Ignore.filenameno
            && filelinep->lineno() < m_चरमः_Ignore.अग्र_Change) {
            m_चरमः_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            प्रत्यागम;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_चरमः_Ignore.filenameno = filelinep->filenameno();
        m_चरमः_Ignore.lineno = filelinep->lineno();
        m_चरमः_Ignore.अग्र_Change = std::numeric_limits<पूर्णाङ्क>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_वृक्षः.find(filelinep->lineno(), results /*ref*/, m_चरमः_Ignore.अग्र_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_Bit_समुच्चयः bitset{VError_Bit_समुच्चयः::AllOnes{}};
        प्रति (ध्रुव स्वचालित& it : results) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            UASSERT(cign.अन्तर्_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_संहिता.forDelegateCodes(
                [&](V3_Error_संहिता subcode) { bitset.समुच्चयः(subcode, cign.m_on); });
        }

        filelinep->warn_समुच्चयः_Ctrl_Bit_समुच्चयः(bitset);
        m_चरमः_Ignore.अग्र_Change = std::max(m_चरमः_Ignore.अग्र_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_चरमः_Ignore.अग्र_Change
                                       << " Messages: " << bitset.ascii());
    }
    तार्किक waive(V3_Error_संहिता संहिता, ध्रुव string& सन्देशः) {
        यदि (संहिता.hardError()) प्रत्यागम असत्य;
        प्रति (ध्रुव स्वचालित& itr : m_waivers) {
            यदि ((संहिता.isUnder(itr.m_संहिता) || (itr.m_संहिता == V3_Error_संहिता::I_LINT))
                && VString::wildmatch(सन्देशः, itr.m_मेलनम्)
                && WildcardContents::resolve(itr.m_contents)) {
                प्रत्यागम सत्य;
            }
        }
        प्रत्यागम असत्य;
    }
};

उपयुज्य V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

वर्ग V3_Control_Scope_Trace_प्रविष्टिः final {
सार्वजनिक:
    ध्रुव string m_scope;  // Scope or regexp to match
    ध्रुव तार्किक m_on;  // True to enable message
    पूर्णाङ्क m_स्तराः = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_प्रविष्टिः(ध्रुव string& scope, तार्किक on, पूर्णाङ्क स्तराः)
        : m_scope{scope}
        , m_on{on}
        , m_स्तराः{स्तराः} {}
    ~V3_Control_Scope_Trace_प्रविष्टिः() = स्वभाव;
    तार्किक संक्रियकः<(ध्रुव V3_Control_Scope_Trace_प्रविष्टिः& other) ध्रुव {
        यदि (m_on < other.m_on) प्रत्यागम सत्य;
        यदि (m_on > other.m_on) प्रत्यागम असत्य;
        यदि (m_स्तराः < other.m_स्तराः) प्रत्यागम सत्य;
        यदि (m_स्तराः > other.m_स्तराः) प्रत्यागम असत्य;
        प्रत्यागम m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
वर्ग V3_Control_Scope_Trace_प्रविष्टिः_मेलनम् final {
सार्वजनिक:
    ध्रुव V3_Control_Scope_Trace_प्रविष्टिः* m_entryp;  // Entry the match refers to
    ध्रुव string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_प्रविष्टिः_मेलनम्(ध्रुव V3_Control_Scope_Trace_प्रविष्टिः* entryp, ध्रुव string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    तार्किक संक्रियकः<(ध्रुव V3_Control_Scope_Trace_प्रविष्टिः_मेलनम्& other) ध्रुव {
        यदि (m_entryp < other.m_entryp) प्रत्यागम सत्य;
        यदि (m_entryp > other.m_entryp) प्रत्यागम असत्य;
        प्रत्यागम m_scopepart < other.m_scopepart;
    }
};

वर्ग V3ControlScopeTraceResolver final {
    std::सदिशम्<V3_Control_Scope_Trace_प्रविष्टिः> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_प्रविष्टिः_मेलनम्, तार्किक> m_मेलनम्_Cache;  // Matching entries for speed

सार्वजनिक:
    शून्य सङ्कलय_Scope_Trace_On(तार्किक on, ध्रुव string& scope, पूर्णाङ्क स्तराः) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << स्तराः);
        m_entries.emplace_पृष्ठभागः(V3_Control_Scope_Trace_प्रविष्टिः{scope, on, स्तराः});
        m_मेलनम्_Cache.clear();
    }

    तार्किक get_प्रविष्टिः_मेलनम्(ध्रुव V3_Control_Scope_Trace_प्रविष्टिः* entp, ध्रुव string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ध्रुव V3_Control_Scope_Trace_प्रविष्टिः_मेलनम् कुञ्चिका{entp, scopepart};
        ध्रुव स्वचालित pair = m_मेलनम्_Cache.emplace(कुञ्चिका, असत्य);
        यदि (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        प्रत्यागम pair.first->second;
    }

    तार्किक getScopeTraceOn(ध्रुव string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        पूर्णाङ्क maxLevel = 1;
        प्रति (ध्रुव स्वचालित& ch : scope) {
            यदि (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        तार्किक enabled = सत्य;
        प्रति (ध्रुव स्वचालित& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            पूर्णाङ्क partLevel = 1;
            प्रति (string::आकार_type part_अन्तः = 0; सत्य;) {
                part_अन्तः = scope.find('.', part_अन्तः + 1);
                यदि (part_अन्तः == string::npos) part_अन्तः = scope.दीर्घता_2();
                ध्रुव std::string scopepart = scope.substr(0, part_अन्तः);
                यदि (get_प्रविष्टिः_मेलनम्(&ent, scopepart)) {
                    ध्रुव तार्किक level_मेलनम्
                        = !ent.m_स्तराः || (ent.m_स्तराः >= maxLevel - partLevel);
                    यदि (level_मेलनम्) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_मेलनम् ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    भङ्ग;
                }
                यदि (part_अन्तः == scope.दीर्घता_2()) भङ्ग;
                ++partLevel;
            }
        }
        प्रत्यागम enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

वर्ग V3_Control_Resolver_Hier_कार्यकर्ता_प्रविष्टिः final {
    ध्रुव पूर्णाङ्क m_workers;  // Number of workers requested for the hierarchical block
    File_पङ्क्तिः* ध्रुव m_flp;  // Location the worker count was specified at

सार्वजनिक:
    स्पष्ट V3_Control_Resolver_Hier_कार्यकर्ता_प्रविष्टिः(पूर्णाङ्क workers, File_पङ्क्तिः* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    पूर्णाङ्क workers() ध्रुव { प्रत्यागम m_workers; }
    File_पङ्क्तिः* flp() ध्रुव { प्रत्यागम m_flp; }
};

वर्ग V3ControlResolver final {
    गणना Profile_दत्त_बहुलकम् : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_दत्त;  // Access to profile_data records
    uint8_t m_बहुलकम् = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_कार्यकर्ता_प्रविष्टिः> m_hierWorkers;
    File_पङ्क्तिः* m_profile_File_पङ्क्तिः = शून्यसूचकः;  // Location profile_data was read from

    V3ControlResolver() = स्वभाव;
    ~V3ControlResolver() = स्वभाव;

सार्वजनिक:
    स्थिर V3ControlResolver& s() {
        स्थिर V3ControlResolver s_singleton;
        प्रत्यागम s_singleton;
    }
    V3ControlModuleResolver& modules() { प्रत्यागम m_modules; }
    V3ControlFileResolver& files() { प्रत्यागम m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { प्रत्यागम m_scopeTraces; }

    शून्य सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        सङ्कलय_Profile_दत्त(fl, hierDpi, "", cost, HIER_DPI);
    }
    शून्य सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& model, ध्रुव string& कुञ्चिका, uint64_t cost,
                        Profile_दत्त_बहुलकम् बहुलकम् = MTASK) {
        यदि (!m_profile_File_पङ्क्तिः) m_profile_File_पङ्क्तिः = fl;
        यदि (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        यदि (बहुलकम् == MTASK) {
            m_profile_दत्त[model][कुञ्चिका] += cost;
        } अन्यथा यदि (बहुलकम् == HIER_DPI) {
            m_profile_दत्त[model][कुञ्चिका] = std::max(m_profile_दत्त[model][कुञ्चिका], cost);
        }
        m_बहुलकम् |= बहुलकम्;
    }
    तार्किक अन्तर्भवति_MTask_Profile_दत्त() ध्रुव { प्रत्यागम m_बहुलकम् & MTASK; }
    uint64_t get_Profile_दत्त(ध्रुव string& hierDpi) ध्रुव {
        // Empty key for hierarchical DPI wrapper costs.
        प्रत्यागम get_Profile_दत्त(hierDpi, "");
    }
    शून्य सङ्कलय_Hier_Workers(File_पङ्क्तिः* flp, ध्रुव string& model, पूर्णाङ्क workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    पूर्णाङ्क getHierWorkers(ध्रुव string& model) ध्रुव {
        ध्रुव स्वचालित mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        प्रत्यागम mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_पङ्क्तिः* get_Hier_Workers_File_पङ्क्तिः(ध्रुव string& model) ध्रुव {
        ध्रुव स्वचालित mit = m_hierWorkers.find(model);
        प्रत्यागम mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_दत्त(ध्रुव string& model, ध्रुव string& कुञ्चिका) ध्रुव {
        ध्रुव स्वचालित mit = m_profile_दत्त.find(model);
        यदि (mit == m_profile_दत्त.cend()) प्रत्यागम 0;
        ध्रुव स्वचालित it = mit->second.find(कुञ्चिका);
        यदि (it == mit->second.cend()) प्रत्यागम 0;
        प्रत्यागम it->second;
    }
    File_पङ्क्तिः* get_Profile_दत्त_File_पङ्क्तिः() ध्रुव { प्रत्यागम m_profile_File_पङ्क्तिः; }  // Maybe null
    स्थिर uint64_t get_Current_Hier_खण्डम्_Cost() {
        यदि (uint64_t cost = V3Control::get_Profile_दत्त(v3Global.opt.उपसर्गः())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            प्रत्यागम cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        uint64_t cost = 0;
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            यदि (eval.slow()) अनुवर्त;
            cost += V3_Instr_गणनम्::गणनम्(netlistp->evalFuncp(eval), असत्य);
        }
        UINFO(9, "Evaluating cost: " << cost);
        प्रत्यागम cost;
    }
};

//######################################################################
// V3Control

शून्य V3Control::सङ्कलय_Case_पूर्णम्(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.सङ्कलय_पङ्क्तिः_Attribute(lineno, VPragmaType::पूर्णम्_CASE);
}
शून्य V3Control::सङ्कलय_Case_Parallel(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.सङ्कलय_पङ्क्तिः_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
शून्य V3Control::सङ्कलय_Coverage_खण्डम्_Off(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.सङ्कलय_पङ्क्तिः_Attribute(lineno, VPragmaType::COVERAGE_खण्डम्_OFF);
}
शून्य V3Control::सङ्कलय_Coverage_खण्डम्_Off(ध्रुव string& module, ध्रुव string& blockname) {
    V3ControlResolver::s().modules().at(module).सङ्कलय_Coverage_खण्डम्_Off(blockname);
}

शून्य V3Control::सङ्कलय_Hier_Workers(File_पङ्क्तिः* fl, ध्रुव string& model, पूर्णाङ्क workers) {
    V3ControlResolver::s().सङ्कलय_Hier_Workers(fl, model, workers);
}

शून्य V3Control::सङ्कलय_Fsm_पञ्जिकाकोशः_Wrapper(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& d,
                                      ध्रुव string& q, ध्रुव string& clock, ध्रुव string& reset,
                                      ध्रुव string& reset_मानम्) {
    string missing;
    यदि (module.रिक्तानि()) missing += "-module";
    यदि (d.रिक्तानि()) missing = VString::dot(missing, ", ", "-d");
    यदि (q.रिक्तानि()) missing = VString::dot(missing, ", ", "-q");
    यदि (clock.रिक्तानि()) missing = VString::dot(missing, ", ", "-clock");
    यदि (!missing.रिक्तानि()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        प्रत्यागम;
    }

    ध्रुव Fsm_पञ्जिकाकोशः_Wrapper desc{module, d, q, clock, reset, reset_मानम्};
    V3ControlResolver::s().modules().at(module).समुच्चयः_Fsm_पञ्जिकाकोशः_Wrapper(fl, desc);
}

शून्य V3Control::सङ्कलय_Ignore(V3_Error_संहिता संहिता, तार्किक on, ध्रुव string& filename, पूर्णाङ्क min, पूर्णाङ्क max) {
    UINFO(9, "addIgnore " << संहिता << " " << min << "-" << max << " fn=" << filename);
    यदि (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_पङ्क्तिः::globalWarnOff(संहिता, !on);
    } अन्यथा {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).सङ्कलय_Ignore(संहिता, min, max, on);
    }
}
शून्य V3Control::सङ्कलय_Ignore_मेलनम्(V3_Error_संहिता संहिता, ध्रुव string& filename, ध्रुव string& contents,
                               ध्रुव string& मेलनम्) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).सङ्कलय_Ignore_मेलनम्(संहिता, contents, मेलनम्);
}

शून्य V3Control::सङ्कलय_Inline(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask, तार्किक on) {
    यदि (ftask.रिक्तानि()) {
        V3ControlResolver::s().modules().at(module).समुच्चयः_Inline(on);
    } अन्यथा {
        यदि (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } अन्यथा {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).समुच्चयः_No_Inline(on);
        }
    }
}

शून्य V3Control::सङ्कलय_Module_Pragma(ध्रुव string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).सङ्कलय_Module_Pragma(pragma);
}

शून्य V3Control::सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().सङ्कलय_Profile_दत्त(fl, hierDpi, cost);
}
शून्य V3Control::सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& model, ध्रुव string& कुञ्चिका,
                               uint64_t cost) {
    V3ControlResolver::s().सङ्कलय_Profile_दत्त(fl, model, कुञ्चिका, cost);
}

शून्य V3Control::सङ्कलय_Scope_Trace_On(तार्किक on, ध्रुव string& scope, पूर्णाङ्क स्तराः) {
    V3ControlResolver::s().scopeTraces().सङ्कलय_Scope_Trace_On(on, scope, स्तराः);
}

शून्य V3Control::सङ्कलय_Var_Attr(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask,
                           Var_Spec_विधा विधा, ध्रुव string& आदर्शः_2, VAttrType attr,
                           Ast_Sen_वृक्षः* sensep) {
    यदि (sensep) {
        File_पङ्क्तिः* ध्रुव flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_वृक्षः(), sensep);
        // Used to be only accepted on public_flat_rw
        यदि (attr != VAttrType::VAR_लौकिकम्_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            प्रत्यागम;
        }
    }

    यदि (विधा != Var_Spec_विधा::VAR) {
        विकल्प (attr) {
        पक्ष VAttrType::VAR_लौकिकम्_FLAT:
        पक्ष VAttrType::VAR_लौकिकम्_FLAT_RD:
        पक्ष VAttrType::VAR_लौकिकम्_FLAT_RW: भङ्ग;
        स्वभाव:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            प्रत्यागम;
        }
    }

    // Semantics: Most of the attributes operate on signals
    यदि (आदर्शः_2.रिक्तानि()) {
        यदि (attr == VAttrType::VAR_लौकिकम्) {
            यदि (ftask.रिक्तानि()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).सङ्कलय_Module_Pragma(
                    VPragmaType::लौकिकम्_MODULE);
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).समुच्चयः_लौकिकम्(सत्य);
            }
        } अन्यथा {
            fl->v3error("missing -var");
        }
    } अन्यथा {
        यदि (attr == VAttrType::VAR_FORCEABLE) {
            यदि (module.रिक्तानि()) {
                fl->v3error("forceable missing -module");
            } अन्यथा यदि (!ftask.रिक्तानि()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } अन्यथा {
                V3ControlResolver::s().modules().at(module).vars().at(आदर्शः_2).सङ्कलय(attr);
            }
        } अन्यथा {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                यदि (ftask.रिक्तानि()) {
                    यदि (विधा == Var_Spec_विधा::PARAM) प्रत्यागम mod.params().at(आदर्शः_2);
                    यदि (विधा == Var_Spec_विधा::पत्तनम्) प्रत्यागम mod.ports().at(आदर्शः_2);
                    UASSERT_OBJ(विधा == Var_Spec_विधा::VAR, fl, "Unexpected VarSpecKind");
                    प्रत्यागम mod.vars().at(आदर्शः_2);
                }
                यदि (विधा == Var_Spec_विधा::PARAM) प्रत्यागम mod.ftasks().at(ftask).params().at(आदर्शः_2);
                यदि (विधा == Var_Spec_विधा::पत्तनम्) प्रत्यागम mod.ftasks().at(ftask).ports().at(आदर्शः_2);
                UASSERT_OBJ(विधा == Var_Spec_विधा::VAR, fl, "Unexpected VarSpecKind");
                प्रत्यागम mod.ftasks().at(ftask).vars().at(आदर्शः_2);
            }();
            controlVar.सङ्कलय(attr);
        }
    }
}

शून्य V3Control::applyCase(AstCase* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    यदि (filep) filep->applyCase(nodep);
}

शून्य V3Control::apply_Coverage_खण्डम्(Ast_ग्रन्थिः_Module* modulep, AstBegin* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    यदि (filep) filep->apply_खण्डम्(nodep);
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    यदि (modp) modp->apply_खण्डम्(nodep);
}
शून्य V3Control::apply_Coverage_खण्डम्(Ast_ग्रन्थिः_Module* modulep, Ast_Gen_खण्डम्* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    यदि (filep) filep->apply_खण्डम्(nodep);
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    यदि (modp) modp->apply_खण्डम्(nodep);
}

शून्य V3Control::applyIgnores(File_पङ्क्तिः* filelinep) {
    ध्रुव string& filename = filelinep->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    यदि (filep) filep->applyIgnores(filelinep);
}

शून्य V3Control::applyModule(Ast_ग्रन्थिः_Module* modulep) {
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    यदि (modp) modp->apply(modulep);
}

शून्य V3Control::applyFTask(Ast_ग्रन्थिः_Module* modulep, Ast_ग्रन्थिः_FTask* ftaskp) {
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    यदि (!modp) प्रत्यागम;
    ध्रुव V3ControlFTask* ध्रुव ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    यदि (ftp) ftp->apply(ftaskp);
}

प्रतिरूपम् <प्रकारनाम T_Resolver>
स्थिर शून्य resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ध्रुव std::string name = varp->prettyDehashOrigOrName();
    यदि (ध्रुव V3ControlVar* ध्रुव vp = resolver.vars().resolve(name)) vp->apply(varp);
    यदि (varp->isParam()) {
        यदि (ध्रुव V3ControlVar* ध्रुव vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    यदि (varp->isIO()) {
        यदि (ध्रुव V3ControlVar* ध्रुव vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

शून्य V3Control::applyVarAttr(ध्रुव Ast_ग्रन्थिः_Module* modulep, ध्रुव Ast_ग्रन्थिः_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ध्रुव modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    यदि (!modp) प्रत्यागम;
    यदि (ftaskp) {
        V3ControlFTask* ध्रुव ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        यदि (!ftp) प्रत्यागम;
        resolveThenApply(*ftp, varp);
    } अन्यथा {
        resolveThenApply(*modp, varp);
    }
}

पूर्णाङ्क V3Control::getHierWorkers(ध्रुव string& model) {
    प्रत्यागम V3ControlResolver::s().getHierWorkers(model);
}
File_पङ्क्तिः* V3Control::get_Hier_Workers_File_पङ्क्तिः(ध्रुव string& model) {
    प्रत्यागम V3ControlResolver::s().get_Hier_Workers_File_पङ्क्तिः(model);
}
ध्रुव V3Control::Fsm_पञ्जिकाकोशः_Wrapper* V3Control::get_Fsm_पञ्जिकाकोशः_Wrapper(ध्रुव string& module) {
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(module);
    प्रत्यागम modp ? modp->fsm_पञ्जिकाकोशः_Wrapperp() : शून्यसूचकः;
}
uint64_t V3Control::get_Profile_दत्त(ध्रुव string& hierDpi) {
    प्रत्यागम V3ControlResolver::s().get_Profile_दत्त(hierDpi);
}
uint64_t V3Control::get_Profile_दत्त(ध्रुव string& model, ध्रुव string& कुञ्चिका) {
    प्रत्यागम V3ControlResolver::s().get_Profile_दत्त(model, कुञ्चिका);
}
File_पङ्क्तिः* V3Control::get_Profile_दत्त_File_पङ्क्तिः() {
    प्रत्यागम V3ControlResolver::s().get_Profile_दत्त_File_पङ्क्तिः();
}
तार्किक V3Control::getScopeTraceOn(ध्रुव string& scope) {
    प्रत्यागम V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

शून्य V3Control::contents_प्रक्षिप_Text(ध्रुव string& text) { WildcardContents::प्रक्षिप_Text(text); }

तार्किक V3Control::अन्तर्भवति_MTask_Profile_दत्त() {
    प्रत्यागम V3ControlResolver::s().अन्तर्भवति_MTask_Profile_दत्त();
}
uint64_t V3Control::get_Current_Hier_खण्डम्_Cost() {
    प्रत्यागम V3ControlResolver::s().get_Current_Hier_खण्डम्_Cost();
}

तार्किक V3Control::waive(ध्रुव File_पङ्क्तिः* filelinep, V3_Error_संहिता संहिता, ध्रुव string& सन्देशः) {
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    यदि (!filep) प्रत्यागम असत्य;
    प्रत्यागम filep->waive(संहिता, सन्देशः);
}

शून्य V3Control::selfTest() { VInterval_वृक्षः::selfTest(); }
