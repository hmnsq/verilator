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
template <typename T>
वर्ग V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
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
        ध्रुव V3LockGuard lock{m_mutex};
        ध्रुव V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        प्रति (ध्रुव auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ध्रुव string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        प्रत्यागम m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ध्रुव string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ध्रुव auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        जइ (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ध्रुव string slashName = V3Os::filenameSlashPath(name);
            प्रति (ध्रुव auto& patEnt : m_mapPatterns) {
                जइ (VString::wildmatch(slashName, patEnt.first)) {
                    जइ (!entryr) entryr.reset(नूतन T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            जइ (entryr) entryr->updateFinalize();
        }
        प्रत्यागम entryr.get();
    }
};

// List of attributes for variables
वर्ग V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
सार्वजनिक:
    // Add new attribugte
    शून्य add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    शून्य update(ध्रुव V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.निवेशय(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    शून्य updateFinalize() {}
    // Apply all attributes to the variable
    शून्य apply(AstVar* varp) ध्रुव {
        प्रति (ध्रुव VAttrType attr : m_attrs) {
            varp->addAttrsp(नूतन AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

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
        ध्रुव string leftover = m_पङ्क्तयः.back() + text;
        m_पङ्क्तयः.निष्कासय_back();

        // Insert line-by-line
        string::size_type पङ्क्तिः_start = 0;
        जाव (सच्च) {
            ध्रुव string::size_type पङ्क्तिः_end = leftover.find('\n', पङ्क्तिः_start);
            जइ (पङ्क्तिः_end != string::npos) {
                ध्रुव string oneline(leftover, पङ्क्तिः_start, पङ्क्तिः_end - पङ्क्तिः_start + 1);
                जइ (oneline.size() > 1) m_पङ्क्तयः.प्रक्षिप_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_पङ्क्तयः.size() - 1) << "]: " << oneline);
                पङ्क्तिः_start = पङ्क्तिः_end + 1;
            } अण्णहा {
                भङ्ग;
            }
        }
        // Keep leftover for next time
        m_पङ्क्तयः.emplace_back(string(leftover, पङ्क्तिः_start));  // Might be ""
        clearCacheImp();
    }

    तार्किक resolveUncachedImp(ध्रुव string& contentsRegexp) {
        प्रति (ध्रुव string& i : m_पङ्क्तयः) {
            जइ (VString::wildmatch(i, contentsRegexp)) प्रत्यागम सच्च;
        }
        प्रत्यागम मिच्छ;
    }
    तार्किक resolveCachedImp(ध्रुव string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        जइ (contentsRegexp.empty()) प्रत्यागम सच्च;
        ध्रुव auto pair = m_mapPatterns.emplace(contentsRegexp, मिच्छ);
        तार्किक& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        जइ (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        प्रत्यागम entryr;
    }

सार्वजनिक:
    WildcardContents() {
        m_पङ्क्तयः.emplace_back("");  // start with no leftover
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
    तार्किक m_noinline = मिच्छ;  // Don't inline function/task
    तार्किक m_public = मिच्छ;  // Public function/task

सार्वजनिक:
    V3ControlFTask() = स्वभाव;
    शून्य update(ध्रुव V3ControlFTask& f) {
        // Don't overwrite true with false
        जइ (f.m_noinline) m_noinline = सच्च;
        जइ (f.m_public) m_public = सच्च;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    शून्य updateFinalize() {}

    V3ControlVarResolver& params() { प्रत्यागम m_params; }
    V3ControlVarResolver& ports() { प्रत्यागम m_ports; }
    V3ControlVarResolver& vars() { प्रत्यागम m_vars; }

    शून्य setNoInline(तार्किक set) { m_noinline = set; }
    शून्य setPublic(तार्किक set) { m_public = set; }

    शून्य apply(Ast_ग्रन्थिः_FTask* ftaskp) ध्रुव {
        जइ (m_noinline)
            ftaskp->addStmtsp(नूतन AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        जइ (m_public)
            ftaskp->addStmtsp(नूतन AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

वर्ग V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    तार्किक m_hasFsmRegisterWrapper = मिच्छ;  // Whether descriptor has been set
    तार्किक m_inline = मिच्छ;  // Whether to force the inline
    तार्किक m_inline_मानम् = मिच्छ;  // The inline value (on/off)

सार्वजनिक:
    V3ControlModule() = स्वभाव;

    शून्य update(ध्रुव V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        जइ (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = सच्च;
        }
        प्रति (ध्रुव string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.निवेशय(i);
        जइ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_मानम् = m.m_inline_मानम्;
        }
        प्रति (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.निवेशय(*it);
        }
    }
    शून्य updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { प्रत्यागम m_tasks; }
    V3ControlVarResolver& params() { प्रत्यागम m_params; }
    V3ControlVarResolver& ports() { प्रत्यागम m_ports; }
    V3ControlVarResolver& vars() { प्रत्यागम m_vars; }

    शून्य addCoverageBlockOff(ध्रुव string& name) { m_coverageOffBlocks.निवेशय(name); }
    शून्य setInline(तार्किक set) {
        m_inline = सच्च;
        m_inline_मानम् = set;
    }
    शून्य setFsmRegisterWrapper(File_पङ्क्तिः* fl, ध्रुव V3Control::FsmRegisterWrapper& desc) {
        जइ (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ग्रन्थिः::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = सच्च;
    }
    ध्रुव V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ध्रुव {
        प्रत्यागम m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    शून्य addModulePragma(VPragmaType pragma) { m_modPragmas.निवेशय(pragma); }

    शून्य apply(Ast_ग्रन्थिः_Module* modp) {
        जइ (m_inline) {
            ध्रुव VPragmaType type
                = m_inline_मानम् ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ग्रन्थिः* ध्रुव nodep = नूतन AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        प्रति (ध्रुव auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            जइ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(सच्च);
            Ast_ग्रन्थिः* ध्रुव nodep = नूतन AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    शून्य applyBlock(AstGenBlock* nodep) {
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        जइ (!nodep->unnamed()) {
            प्रति (ध्रुव string& i : m_coverageOffBlocks) {
                जइ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(नूतन AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    शून्य applyBlock(Ast_ग्रन्थिः_Block* nodep) {
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        जइ (!nodep->unnamed()) {
            प्रति (ध्रुव string& i : m_coverageOffBlocks) {
                जइ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(नूतन AstStmtPragma{flp, नूतन AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
वर्ग V3_Control_Ignores_पङ्क्तिः final {
सार्वजनिक:
    ध्रुव पूर्णाङ्क m_पङ्क्तिः_Min;  // Minimum line number to make change at
    ध्रुव पूर्णाङ्क m_पङ्क्तिः_Max;  // Maximum line number to make change at (inclusive)
    ध्रुव V3ErrorCode m_code;  // Error code
    ध्रुव तार्किक m_on;  // True to enable message
    V3_Control_Ignores_पङ्क्तिः(V3ErrorCode code, पूर्णाङ्क linemin, पूर्णाङ्क linemax, तार्किक on)
        : m_पङ्क्तिः_Min{linemin}
        , m_पङ्क्तिः_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_पङ्क्तिः() = स्वभाव;
    तार्किक every_पङ्क्तिः() ध्रुव { प्रत्यागम m_पङ्क्तिः_Min == 0 && m_पङ्क्तिः_Max == 0; }
    तार्किक inRange(पूर्णाङ्क lineno) ध्रुव {
        प्रत्यागम (lineno >= m_पङ्क्तिः_Min && lineno <= m_पङ्क्तिः_Max) || every_पङ्क्तिः();
    }
};
std::ostream& operator<<(std::ostream& os, ध्रुव V3_Control_Ignores_पङ्क्तिः& rhs) {
    प्रत्यागम os << rhs.m_पङ्क्तिः_Min << "-" << rhs.m_पङ्क्तिः_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
स्थिर std::vector<V3_Control_Ignores_पङ्क्तिः> control_Ign_पङ्क्तयः;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

वर्ग VInterval_वृक्षः final {
    संरचना Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        पूर्णाङ्क m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(पूर्णाङ्क center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_पङ्क्तयः;  // All-line disables

गुप्त:
    std::unique_ptr<Entry> build_वृक्षः(ध्रुव IgnIndices& points) {
        जइ (points.empty()) प्रत्यागम nullptr;

        पूर्णाङ्क minval = std::numeric_limits<पूर्णाङ्क>::max();
        पूर्णाङ्क maxval = std::numeric_limits<पूर्णाङ्क>::min();
        प्रति (ध्रुव auto& it : points) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            जइ (cign.every_पङ्क्तिः()) {
                m_every_पङ्क्तयः.emplace_back(it);
                अनुवर्त;
            }
            minval = std::min(minval, cign.m_पङ्क्तिः_Min);
            maxval = std::max(maxval, cign.m_पङ्क्तिः_Max);
        }

        ध्रुव पूर्णाङ्क center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(नूतन Entry{center});

        IgnIndices वामः_Points;
        IgnIndices दक्षिणः_Points;
        प्रति (ध्रुव auto& it : points) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            जइ (cign.every_पङ्क्तिः()) अनुवर्त;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            जइ (cign.m_पङ्क्तिः_Max < center)
                वामः_Points.emplace_back(it);
            अण्णहा जइ (cign.m_पङ्क्तिः_Min > center)
                दक्षिणः_Points.emplace_back(it);
            अण्णहा {
                entp->m_byMin.प्रक्षिप_back(it);
                entp->m_byMax.प्रक्षिप_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            प्रत्यागम control_Ign_पङ्क्तयः[a].m_पङ्क्तिः_Min < control_Ign_पङ्क्तयः[b].m_पङ्क्तिः_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            प्रत्यागम control_Ign_पङ्क्तयः[a].m_पङ्क्तिः_Max > control_Ign_पङ्क्तयः[b].m_पङ्क्तिः_Max;
        });

        entp->m_leftp = build_वृक्षः(वामः_Points);
        entp->m_rightp = build_वृक्षः(दक्षिणः_Points);
        प्रत्यागम entp;
    }

    शून्य find_वृक्षः(पूर्णाङ्क lineno, Entry* entp, IgnIndices& resultsr, पूर्णाङ्क& nextChanger) ध्रुव {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        जइ (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            प्रति (ध्रुव auto& it : entp->m_byMin) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                जइ (cign.m_पङ्क्तिः_Min <= lineno) {
                    resultsr.emplace_back(it);
                } अण्णहा {
                    nextChanger = std::min(nextChanger, cign.m_पङ्क्तिः_Min);
                    भङ्ग;
                }
            }
            जइ (entp->m_leftp) find_वृक्षः(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } अण्णहा जइ (lineno > entp->m_center) {
            प्रति (ध्रुव auto& it : entp->m_byMax) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                जइ (cign.m_पङ्क्तिः_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_पङ्क्तिः_Max + 1);
                    resultsr.emplace_back(it);
                } अण्णहा {
                    भङ्ग;
                }
            }
            जइ (entp->m_rightp) find_वृक्षः(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } अण्णहा {
            प्रति (ध्रुव auto& it : entp->m_byMin) {
                ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
                resultsr.emplace_back(it);
                जइ (cign.m_पङ्क्तिः_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_पङ्क्तिः_Max + 1);
            }
            जइ (entp->m_leftp) find_वृक्षः(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            जइ (entp->m_rightp) find_वृक्षः(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

सार्वजनिक:
    // CONSTRUCTORS
    VInterval_वृक्षः() = स्वभाव;
    ~VInterval_वृक्षः() = स्वभाव;
    // METHODS
    शून्य clear() { m_rootp = nullptr; }
    शून्य build(ध्रुव IgnIndices& points) {
        clear();
        m_rootp = build_वृक्षः(points);
    }
    शून्य find(पूर्णाङ्क lineno, IgnIndices& resultsr, पूर्णाङ्क& nextChanger) ध्रुव {
        resultsr = m_every_पङ्क्तयः;
        nextChanger = std::numeric_limits<पूर्णाङ्क>::max();

        जइ (m_rootp) find_वृक्षः(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    स्थिर शून्य selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        प्रति (ध्रुव auto& it : points) {
            control_Ign_पङ्क्तयः.emplace_back(
                V3_Control_Ignores_पङ्क्तिः{V3ErrorCode::I_LINT, it.first, it.second, सच्च});
            data.emplace_back(static_cast<uint32_t>(control_Ign_पङ्क्तयः.size() - 1));
        }
        VInterval_वृक्षः वृक्षः;
        वृक्षः.build(data);

        IgnIndices results;
        पूर्णाङ्क nextChange = 0;
        वृक्षः.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        वृक्षः.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        वृक्षः.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        वृक्षः.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        वृक्षः.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        वृक्षः.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        वृक्षः.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        वृक्षः.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<पूर्णाङ्क>::max());
        //
        points = {{0, 0}};
        प्रति (ध्रुव auto& it : points) {
            control_Ign_पङ्क्तयः.emplace_back(
                V3_Control_Ignores_पङ्क्तिः{V3ErrorCode::I_LINT, it.first, it.second, सच्च});
            data.emplace_back(static_cast<uint32_t>(control_Ign_पङ्क्तयः.size() - 1));
        }
        वृक्षः.build(data);
        //
        वृक्षः.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_पङ्क्तिः_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

वर्ग WaiverSetting final {
सार्वजनिक:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ध्रुव string& contents, ध्रुव string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = स्वभाव;
    WaiverSetting& operator=(ध्रुव WaiverSetting& rhs) = स्वभाव;
};

// File entity
वर्ग V3ControlFile final {
    using पङ्क्तिः_Attr_Map = std::map<पूर्णाङ्क, V3_Control_पङ्क्तिः_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    पङ्क्तिः_Attr_Map m_पङ्क्तिः_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_वृक्षः m_interval_वृक्षः;  // Tree of indices to IgnLines

    संरचना {
        पूर्णाङ्क filenameno = -1;  // Last filename
        पूर्णाङ्क lineno = -1;  // Last linenumber
        पूर्णाङ्क nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    तार्किक पङ्क्तिः_Match(पूर्णाङ्क lineno, VPragmaType type) {
        जइ (m_पङ्क्तिः_Attrs.find(0) != m_पङ्क्तिः_Attrs.end() && m_पङ्क्तिः_Attrs[0][type]) प्रत्यागम सच्च;
        जइ (m_पङ्क्तिः_Attrs.find(lineno) == m_पङ्क्तिः_Attrs.end()) प्रत्यागम मिच्छ;
        प्रत्यागम m_पङ्क्तिः_Attrs[lineno][type];
    }

सार्वजनिक:
    V3ControlFile() = स्वभाव;
    शून्य update(ध्रुव V3ControlFile& file) {
        // Copy in all attributes and waivers
        प्रति (ध्रुव auto& itr : file.m_पङ्क्तिः_Attrs) m_पङ्क्तिः_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.निवेशय(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.निवेशय(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    शून्य updateFinalize() { m_interval_वृक्षः.build(m_ignIndices); }
    शून्य add_पङ्क्तिः_Attribute(पूर्णाङ्क lineno, VPragmaType attr) { m_पङ्क्तिः_Attrs[lineno].set(attr); }
    शून्य addIgnore(V3ErrorCode code, पूर्णाङ्क min, पूर्णाङ्क max, तार्किक on) {
        control_Ign_पङ्क्तयः.emplace_back(V3_Control_Ignores_पङ्क्तिः{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_पङ्क्तयः.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    शून्य addIgnoreMatch(V3ErrorCode code, ध्रुव string& contents, ध्रुव string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        जइ (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    शून्य applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        जइ (पङ्क्तिः_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(नूतन AstPragma{nodep->fileline(), pragma});
        }
    }
    शून्य applyBlock(Ast_ग्रन्थिः_Block* nodep) {
        // Apply to block at this line
        ध्रुव VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        जइ (पङ्क्तिः_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(नूतन AstStmtPragma{flp, नूतन AstPragma{flp, pragma}});
        }
    }
    शून्य applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ध्रुव पूर्णाङ्क lineno = nodep->fileline()->lineno();
        जइ (पङ्क्तिः_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(सच्च);
        जइ (पङ्क्तिः_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(सच्च);
    }
    शून्य applyIgnores(File_पङ्क्तिः* filelinep) {
        // HOT routine, called each parsed token line of this filename
        जइ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            प्रत्यागम;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        जइ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            प्रत्यागम;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<पूर्णाङ्क>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_वृक्षः.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        प्रति (ध्रुव auto& it : results) {
            ध्रुव V3_Control_Ignores_पङ्क्तिः& cign = control_Ign_पङ्क्तयः[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    तार्किक waive(V3ErrorCode code, ध्रुव string& message) {
        जइ (code.hardError()) प्रत्यागम मिच्छ;
        प्रति (ध्रुव auto& itr : m_waivers) {
            जइ ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                प्रत्यागम सच्च;
            }
        }
        प्रत्यागम मिच्छ;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

वर्ग V3ControlScopeTraceEntry final {
सार्वजनिक:
    ध्रुव string m_scope;  // Scope or regexp to match
    ध्रुव तार्किक m_on;  // True to enable message
    पूर्णाङ्क m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ध्रुव string& scope, तार्किक on, पूर्णाङ्क levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = स्वभाव;
    तार्किक operator<(ध्रुव V3ControlScopeTraceEntry& other) ध्रुव {
        जइ (m_on < other.m_on) प्रत्यागम सच्च;
        जइ (m_on > other.m_on) प्रत्यागम मिच्छ;
        जइ (m_levels < other.m_levels) प्रत्यागम सच्च;
        जइ (m_levels > other.m_levels) प्रत्यागम मिच्छ;
        प्रत्यागम m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
वर्ग V3ControlScopeTraceEntryMatch final {
सार्वजनिक:
    ध्रुव V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ध्रुव string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ध्रुव V3ControlScopeTraceEntry* entryp, ध्रुव string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    तार्किक operator<(ध्रुव V3ControlScopeTraceEntryMatch& other) ध्रुव {
        जइ (m_entryp < other.m_entryp) प्रत्यागम सच्च;
        जइ (m_entryp > other.m_entryp) प्रत्यागम मिच्छ;
        प्रत्यागम m_scopepart < other.m_scopepart;
    }
};

वर्ग V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, तार्किक> m_matchCache;  // Matching entries for speed

सार्वजनिक:
    शून्य addScopeTraceOn(तार्किक on, ध्रुव string& scope, पूर्णाङ्क levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    तार्किक getEntryMatch(ध्रुव V3ControlScopeTraceEntry* entp, ध्रुव string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ध्रुव V3ControlScopeTraceEntryMatch कुञ्चिका{entp, scopepart};
        ध्रुव auto pair = m_matchCache.emplace(कुञ्चिका, मिच्छ);
        जइ (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        प्रत्यागम pair.first->second;
    }

    तार्किक getScopeTraceOn(ध्रुव string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        पूर्णाङ्क maxLevel = 1;
        प्रति (ध्रुव auto& ch : scope) {
            जइ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        तार्किक enabled = सच्च;
        प्रति (ध्रुव auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            पूर्णाङ्क partLevel = 1;
            प्रति (string::size_type partEnd = 0; सच्च;) {
                partEnd = scope.find('.', partEnd + 1);
                जइ (partEnd == string::npos) partEnd = scope.length();
                ध्रुव std::string scopepart = scope.substr(0, partEnd);
                जइ (getEntryMatch(&ent, scopepart)) {
                    ध्रुव तार्किक levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    जइ (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    भङ्ग;
                }
                जइ (partEnd == scope.length()) भङ्ग;
                ++partLevel;
            }
        }
        प्रत्यागम enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

वर्ग V3ControlResolverHierWorkerEntry final {
    ध्रुव पूर्णाङ्क m_workers;  // Number of workers requested for the hierarchical block
    File_पङ्क्तिः* ध्रुव m_flp;  // Location the worker count was specified at

सार्वजनिक:
    explicit V3ControlResolverHierWorkerEntry(पूर्णाङ्क workers, File_पङ्क्तिः* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    पूर्णाङ्क workers() ध्रुव { प्रत्यागम m_workers; }
    File_पङ्क्तिः* flp() ध्रुव { प्रत्यागम m_flp; }
};

वर्ग V3ControlResolver final {
    गणना ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_पङ्क्तिः* m_profile_File_पङ्क्तिः = nullptr;  // Location profile_data was read from

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

    शून्य addProfileData(File_पङ्क्तिः* fl, ध्रुव string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    शून्य addProfileData(File_पङ्क्तिः* fl, ध्रुव string& model, ध्रुव string& कुञ्चिका, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        जइ (!m_profile_File_पङ्क्तिः) m_profile_File_पङ्क्तिः = fl;
        जइ (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        जइ (mode == MTASK) {
            m_profileData[model][कुञ्चिका] += cost;
        } अण्णहा जइ (mode == HIER_DPI) {
            m_profileData[model][कुञ्चिका] = std::max(m_profileData[model][कुञ्चिका], cost);
        }
        m_mode |= mode;
    }
    तार्किक containsMTaskProfileData() ध्रुव { प्रत्यागम m_mode & MTASK; }
    uint64_t getProfileData(ध्रुव string& hierDpi) ध्रुव {
        // Empty key for hierarchical DPI wrapper costs.
        प्रत्यागम getProfileData(hierDpi, "");
    }
    शून्य addHierWorkers(File_पङ्क्तिः* flp, ध्रुव string& model, पूर्णाङ्क workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    पूर्णाङ्क getHierWorkers(ध्रुव string& model) ध्रुव {
        ध्रुव auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        प्रत्यागम mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_पङ्क्तिः* get_Hier_Workers_File_पङ्क्तिः(ध्रुव string& model) ध्रुव {
        ध्रुव auto mit = m_hierWorkers.find(model);
        प्रत्यागम mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ध्रुव string& model, ध्रुव string& कुञ्चिका) ध्रुव {
        ध्रुव auto mit = m_profileData.find(model);
        जइ (mit == m_profileData.cend()) प्रत्यागम 0;
        ध्रुव auto it = mit->second.find(कुञ्चिका);
        जइ (it == mit->second.cend()) प्रत्यागम 0;
        प्रत्यागम it->second;
    }
    File_पङ्क्तिः* get_Profile_Data_File_पङ्क्तिः() ध्रुव { प्रत्यागम m_profile_File_पङ्क्तिः; }  // Maybe null
    स्थिर uint64_t getCurrentHierBlockCost() {
        जइ (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            प्रत्यागम cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        uint64_t cost = 0;
        प्रति (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            जइ (eval.slow()) अनुवर्त;
            cost += V3_Instr_गणनम्::गणनम्(netlistp->evalFuncp(eval), मिच्छ);
        }
        UINFO(9, "Evaluating cost: " << cost);
        प्रत्यागम cost;
    }
};

//######################################################################
// V3Control

शून्य V3Control::addCaseFull(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पङ्क्तिः_Attribute(lineno, VPragmaType::FULL_CASE);
}
शून्य V3Control::addCaseParallel(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पङ्क्तिः_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
शून्य V3Control::addCoverageBlockOff(ध्रुव string& filename, पूर्णाङ्क lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_पङ्क्तिः_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
शून्य V3Control::addCoverageBlockOff(ध्रुव string& module, ध्रुव string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

शून्य V3Control::addHierWorkers(File_पङ्क्तिः* fl, ध्रुव string& model, पूर्णाङ्क workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

शून्य V3Control::addFsmRegisterWrapper(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& d,
                                      ध्रुव string& q, ध्रुव string& clock, ध्रुव string& reset,
                                      ध्रुव string& reset_मानम्) {
    string missing;
    जइ (module.empty()) missing += "-module";
    जइ (d.empty()) missing = VString::dot(missing, ", ", "-d");
    जइ (q.empty()) missing = VString::dot(missing, ", ", "-q");
    जइ (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    जइ (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        प्रत्यागम;
    }

    ध्रुव FsmRegisterWrapper desc{module, d, q, clock, reset, reset_मानम्};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

शून्य V3Control::addIgnore(V3ErrorCode code, तार्किक on, ध्रुव string& filename, पूर्णाङ्क min, पूर्णाङ्क max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    जइ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_पङ्क्तिः::globalWarnOff(code, !on);
    } अण्णहा {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
शून्य V3Control::addIgnoreMatch(V3ErrorCode code, ध्रुव string& filename, ध्रुव string& contents,
                               ध्रुव string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

शून्य V3Control::addInline(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask, तार्किक on) {
    जइ (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } अण्णहा {
        जइ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } अण्णहा {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

शून्य V3Control::addModulePragma(ध्रुव string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

शून्य V3Control::addProfileData(File_पङ्क्तिः* fl, ध्रुव string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
शून्य V3Control::addProfileData(File_पङ्क्तिः* fl, ध्रुव string& model, ध्रुव string& कुञ्चिका,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, कुञ्चिका, cost);
}

शून्य V3Control::addScopeTraceOn(तार्किक on, ध्रुव string& scope, पूर्णाङ्क levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

शून्य V3Control::addVarAttr(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask,
                           VarSpecKind kind, ध्रुव string& pattern, VAttrType attr,
                           Ast_Sen_वृक्षः* sensep) {
    जइ (sensep) {
        File_पङ्क्तिः* ध्रुव flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_वृक्षः(), sensep);
        // Used to be only accepted on public_flat_rw
        जइ (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            प्रत्यागम;
        }
    }

    जइ (kind != VarSpecKind::VAR) {
        विकल्प (attr) {
        पक्ष VAttrType::VAR_PUBLIC_FLAT:
        पक्ष VAttrType::VAR_PUBLIC_FLAT_RD:
        पक्ष VAttrType::VAR_PUBLIC_FLAT_RW: भङ्ग;
        स्वभाव:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            प्रत्यागम;
        }
    }

    // Semantics: Most of the attributes operate on signals
    जइ (pattern.empty()) {
        जइ (attr == VAttrType::VAR_PUBLIC) {
            जइ (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } अण्णहा {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(सच्च);
            }
        } अण्णहा {
            fl->v3error("missing -var");
        }
    } अण्णहा {
        जइ (attr == VAttrType::VAR_FORCEABLE) {
            जइ (module.empty()) {
                fl->v3error("forceable missing -module");
            } अण्णहा जइ (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } अण्णहा {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } अण्णहा {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                जइ (ftask.empty()) {
                    जइ (kind == VarSpecKind::PARAM) प्रत्यागम mod.params().at(pattern);
                    जइ (kind == VarSpecKind::PORT) प्रत्यागम mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    प्रत्यागम mod.vars().at(pattern);
                }
                जइ (kind == VarSpecKind::PARAM) प्रत्यागम mod.ftasks().at(ftask).params().at(pattern);
                जइ (kind == VarSpecKind::PORT) प्रत्यागम mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                प्रत्यागम mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

शून्य V3Control::applyCase(AstCase* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    जइ (filep) filep->applyCase(nodep);
}

शून्य V3Control::applyCoverageBlock(Ast_ग्रन्थिः_Module* modulep, AstBegin* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    जइ (filep) filep->applyBlock(nodep);
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    जइ (modp) modp->applyBlock(nodep);
}
शून्य V3Control::applyCoverageBlock(Ast_ग्रन्थिः_Module* modulep, AstGenBlock* nodep) {
    ध्रुव string& filename = nodep->fileline()->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    जइ (filep) filep->applyBlock(nodep);
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    जइ (modp) modp->applyBlock(nodep);
}

शून्य V3Control::applyIgnores(File_पङ्क्तिः* filelinep) {
    ध्रुव string& filename = filelinep->filename();
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filename);
    जइ (filep) filep->applyIgnores(filelinep);
}

शून्य V3Control::applyModule(Ast_ग्रन्थिः_Module* modulep) {
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    जइ (modp) modp->apply(modulep);
}

शून्य V3Control::applyFTask(Ast_ग्रन्थिः_Module* modulep, Ast_ग्रन्थिः_FTask* ftaskp) {
    ध्रुव string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(modname);
    जइ (!modp) प्रत्यागम;
    ध्रुव V3ControlFTask* ध्रुव ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    जइ (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
स्थिर शून्य resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ध्रुव std::string name = varp->prettyDehashOrigOrName();
    जइ (ध्रुव V3ControlVar* ध्रुव vp = resolver.vars().resolve(name)) vp->apply(varp);
    जइ (varp->isParam()) {
        जइ (ध्रुव V3ControlVar* ध्रुव vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    जइ (varp->isIO()) {
        जइ (ध्रुव V3ControlVar* ध्रुव vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

शून्य V3Control::applyVarAttr(ध्रुव Ast_ग्रन्थिः_Module* modulep, ध्रुव Ast_ग्रन्थिः_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ध्रुव modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    जइ (!modp) प्रत्यागम;
    जइ (ftaskp) {
        V3ControlFTask* ध्रुव ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        जइ (!ftp) प्रत्यागम;
        resolveThenApply(*ftp, varp);
    } अण्णहा {
        resolveThenApply(*modp, varp);
    }
}

पूर्णाङ्क V3Control::getHierWorkers(ध्रुव string& model) {
    प्रत्यागम V3ControlResolver::s().getHierWorkers(model);
}
File_पङ्क्तिः* V3Control::get_Hier_Workers_File_पङ्क्तिः(ध्रुव string& model) {
    प्रत्यागम V3ControlResolver::s().get_Hier_Workers_File_पङ्क्तिः(model);
}
ध्रुव V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ध्रुव string& module) {
    V3ControlModule* ध्रुव modp = V3ControlResolver::s().modules().resolve(module);
    प्रत्यागम modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ध्रुव string& hierDpi) {
    प्रत्यागम V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ध्रुव string& model, ध्रुव string& कुञ्चिका) {
    प्रत्यागम V3ControlResolver::s().getProfileData(model, कुञ्चिका);
}
File_पङ्क्तिः* V3Control::get_Profile_Data_File_पङ्क्तिः() {
    प्रत्यागम V3ControlResolver::s().get_Profile_Data_File_पङ्क्तिः();
}
तार्किक V3Control::getScopeTraceOn(ध्रुव string& scope) {
    प्रत्यागम V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

शून्य V3Control::contents_प्रक्षिप_Text(ध्रुव string& text) { WildcardContents::प्रक्षिप_Text(text); }

तार्किक V3Control::containsMTaskProfileData() {
    प्रत्यागम V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    प्रत्यागम V3ControlResolver::s().getCurrentHierBlockCost();
}

तार्किक V3Control::waive(ध्रुव File_पङ्क्तिः* filelinep, V3ErrorCode code, ध्रुव string& message) {
    V3ControlFile* ध्रुव filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    जइ (!filep) प्रत्यागम मिच्छ;
    प्रत्यागम filep->waive(code, message);
}

शून्य V3Control::selfTest() { VInterval_वृक्षः::selfTest(); }
