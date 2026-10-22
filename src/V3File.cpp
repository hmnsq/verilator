// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#समावेश "V3PchAstMT.h"

#समावेश "V3File.h"

#समावेश "V3Os.h"
#समावेश "V3String.h"

#समावेश <cerrno>
#समावेश <cstdarg>
#समावेश <fcntl.h>
#समावेश <iomanip>
#समावेश <map>
#समावेश <memory>

#समावेश <sys/stat.h>
#समावेश <sys/types.h>

// clang-format off
#अगर defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#पूर्ण_अगर

#अगर_घोषित HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#अन्यथा
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#पूर्ण_अगर

#अगर_घोषित INFILTER_PIPE
# include <sys/wait.h>
#पूर्ण_अगर

#अगर defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#पूर्ण_अगर
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr पूर्णांक INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr पूर्णांक INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_आकार_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

श्रेणी V3FileDependImp final {
    // TYPES
    श्रेणी DependFile final {
        // A single file
        स्थिर bool m_target;  // True if write, else read
        bool m_exists = true;  // File exists
        स्थिर string m_filename;  // Filename
        काष्ठा stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    खुला:
        DependFile(स्थिर string& filename, bool target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = शेष;
        स्थिर string& filename() स्थिर { वापस m_filename; }
        bool target() स्थिर { वापस m_target; }
        bool exists() स्थिर { वापस m_exists; }
        off_t आकार() स्थिर { वापस m_stat.st_आकार; }
        ino_t ino() स्थिर { वापस m_stat.st_ino; }
        समय_प्रकार cstime() स्थिर { वापस m_stat.st_ctime; }  // Seconds
        समय_प्रकार cnstime() स्थिर { वापस VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        समय_प्रकार mstime() स्थिर { वापस m_stat.st_mtime; }  // Seconds
        समय_प्रकार mnstime() स्थिर { वापस VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_प्रतीक() {
            अटल VHashSha512 s_emptyHash;
            वापस m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        व्योम loadStats() {
            अगर (!m_stat.st_mtime) {
                स्थिर string fn = filename();
                स्थिर पूर्णांक err = stat(fn.c_str(), &m_stat);
                अगर (err != 0) {
                    std::स_रखो(&m_stat, 0, माप(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = false;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } अन्यथा {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    अगर (!target() && आकार() <= FILE_HASH_आकार_MAX) m_hash.जोड़ो_File(filename());
                }
            }
        }
        bool चालक<(स्थिर DependFile& rhs) स्थिर { वापस filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    अटल string stripQuotes(स्थिर string& in) {
        string pretty = in;
        string::आकार_type pos;
        जबतक ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        जबतक ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        वापस pretty;
    }

खुला:
    // ACCESSOR METHODS
    व्योम addSrcDepend(स्थिर string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिर V3LockGuard lock{m_mutex};
        स्थिर स्वतः itFoundPair = m_filenameSet.जोड़ो(filename);
        अगर (itFoundPair.second) {
            DependFile df{filename, false};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.जोड़ो(df);
        }
    }
    व्योम addTgtDepend(स्थिर string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        स्थिर V3LockGuard lock{m_mutex};
        स्थिर स्वतः itFoundPair = m_filenameSet.जोड़ो(filename);
        अगर (itFoundPair.second) m_filenameList.जोड़ो(DependFile{filename, true});
    }
    व्योम writeDepend(स्थिर string& filename);
    std::vector<string> getAllDeps() स्थिर;
    व्योम writeTimes(स्थिर string& filename, स्थिर string& cmdlineIn);
    bool checkTimes(स्थिर string& filename, स्थिर string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

व्योम V3FileDependImp::writeDepend(स्थिर string& filename) {
    स्थिर std::unique_ptr<std::बख_प्रवाह> ofp{V3File::new_ofstream(filename)};
    अगर (ofp->हार()) v3fatal("Can't write file: " << filename);

    क्रम (स्थिर DependFile& i : m_filenameList) {
        अगर (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    क्रम (स्थिर DependFile& i : m_filenameList) {
        अगर (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    अगर (v3Global.opt.makePhony()) {
        *ofp << '\n';
        क्रम (स्थिर DependFile& i : m_filenameList) {
            अगर (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() स्थिर {
    std::vector<string> r;
    क्रम (स्थिर स्वतः& itr : m_filenameList) {
        अगर (!itr.target() && itr.exists()) r.धकेलो_back(itr.filename());
    }
    वापस r;
}

व्योम V3FileDependImp::writeTimes(स्थिर string& filename, स्थिर string& cmdlineIn) {
    स्थिर std::unique_ptr<std::बख_प्रवाह> ofp{V3File::new_ofstream(filename)};
    अगर (ofp->हार()) v3fatal("Can't write file: " << filename);

    स्थिर string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    क्रम (स्वतः& it : m_filenameList) {
        DependFile* स्थिर depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t show_आकार = depfilep->आकार();
        ino_t showIno = depfilep->ino();
        अगर (depfilep->filename() == filename) {
            show_आकार = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::रखो_चौड़ाई(8) << show_आकार;
        *ofp << " " << std::रखो_चौड़ाई(8) << showIno;
        *ofp << " " << std::रखो_चौड़ाई(11) << depfilep->cstime();
        *ofp << " " << std::रखो_चौड़ाई(11) << depfilep->cnstime();
        *ofp << " " << std::रखो_चौड़ाई(11) << depfilep->mstime();
        *ofp << " " << std::रखो_चौड़ाई(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_प्रतीक() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

bool V3FileDependImp::checkTimes(स्थिर string& filename, स्थिर string& cmdlineIn) {
    स्थिर std::unique_ptr<std::अख_प्रवाह> ifp{V3File::new_ifstream_nodepend(filename)};
    अगर (ifp->हार()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        वापस false;
    }
    {
        स्थिर string ignore = V3Os::लो_पंक्ति(*ifp);
        अगर (ignore.empty()) { /*used*/
        }
    }
    {
        अक्षर chkDir;
        *ifp >> chkDir;
        अक्षर quote;
        *ifp >> quote;
        स्थिर string chkCmdline = V3Os::लो_पंक्ति(*ifp, '"');
        स्थिर string cmdline = stripQuotes(cmdlineIn);
        अगर (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            वापस false;
        }
    }

    स्थिर bool skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    जबतक (!ifp->खप()) {
        अक्षर chkDir;
        *ifp >> chkDir;
        off_t chk_आकार;
        *ifp >> chk_आकार;
        ino_t chkIno;
        *ifp >> chkIno;
        अगर (ifp->खप()) अवरोध;  // Needed to read final whitespace before found eof
        समय_प्रकार chkCstime;
        *ifp >> chkCstime;
        समय_प्रकार chkCnstime;
        *ifp >> chkCnstime;
        समय_प्रकार chkMstime;
        *ifp >> chkMstime;
        समय_प्रकार chkMnstime;
        *ifp >> chkMnstime;
        अक्षर quote;
        *ifp >> quote;
        स्थिर string chkHash = V3Os::लो_पंक्ति(*ifp, '"');
        *ifp >> quote;
        स्थिर string chkFilename = V3Os::लो_पंक्ति(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        काष्ठा stat curStat;
        स्थिर पूर्णांक err = stat(chkFilename.c_str(), &curStat);
        अगर (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            वापस false;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        अगर (filename == chkFilename) जारी;

        UINFO(9, " got d=" << chkDir << " s=" << chk_आकार << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_आकार << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        अगर (!(curStat.st_आकार >= chk_आकार && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_आकार << "=?" << chk_आकार << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            अगर (skipHashing) वापस false;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            अगर (chkHash == "unhashed") वापस false;

            VHashSha512 curHash;
            curHash.जोड़ो_File(chkFilename);
            अगर (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                वापस false;
            }
        }
    }
    वापस true;
}

//######################################################################
// V3File

व्योम V3File::addSrcDepend(स्थिर string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
व्योम V3File::addTgtDepend(स्थिर string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
व्योम V3File::writeDepend(स्थिर string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { वापस dependImp.getAllDeps(); }
व्योम V3File::writeTimes(स्थिर string& filename, स्थिर string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
bool V3File::checkTimes(स्थिर string& filename, स्थिर string& cmdlineIn) {
    वापस dependImp.checkTimes(filename, cmdlineIn);
}
व्योम V3File::createMakeDirFor(स्थिर string& filename) {
    अगर (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().लंबाई() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
व्योम V3File::createMakeDir() {
    अटल bool s_created = false;
    अगर (!s_created) {
        s_created = true;
        V3Os::createDir(v3Global.opt.makeDir());
        अगर (v3Global.opt.hier_शीर्ष()) V3Os::createDir(v3Global.opt.hier_शीर्ष_तथ्य_Dir());
    }
}

//######################################################################
// VInFilterImp

श्रेणी VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<स्थिर std::string, std::string> m_contentsMap;  // Cache of file contents
    bool m_readEof = false;  // Received EOF on read
#अगर_घोषित INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#अन्यथा
    पूर्णांक m_pid = 0;  // fork() process id - always zero as disabled
#पूर्ण_अगर
    bool m_pidExited = false;  //  waitpid() reaped the filter process
    पूर्णांक m_pidStatus = 0;  // Exit status of the filter process
    पूर्णांक m_writeFd = 0;  // File descriptor TO filter
    पूर्णांक m_readFd = 0;  // File descriptor FROM filter

निजी:
    // METHODS

    bool readContents(स्थिर string& filename, StrList& outl) {
        अगर (m_pid) {
            वापस readContentsFilter(filename, outl);
        } अन्यथा {
            वापस readContentsFile(filename, outl);
        }
    }
    bool readContentsFile(स्थिर string& filename, StrList& outl) {
        स्थिर पूर्णांक fd = खोलो(filename.c_str(), O_RDONLY);
        अगर (fd < 0) वापस false;
        m_readEof = false;
        readBlocks(fd, -1, outl);
        बंद(fd);
        वापस true;
    }
    // cppcheck-suppress constParameterReference
    bool readContentsFilter(स्थिर string& filename, StrList& outl) {
        (व्योम)filename;  // Prevent unused variable warning
        (व्योम)outl;  // Prevent unused variable warning
#अगर_घोषित INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        स्थिर string पंक्ति = read_Filter_पंक्ति();
        अगर (पंक्ति.find("Content-Length") != string::npos) {
            पूर्णांक len = 0;
            माला_पूछो(पंक्ति.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            वापस true;
        } अन्यथा {
            अगर (पंक्ति != "") v3error("--pipe-filter protocol error, unexpected: " << पंक्ति);
            वापस false;
        }
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
        वापस false;
#पूर्ण_अगर
    }

    // cppcheck-suppress functionConst
    व्योम checkFilter(bool hang) {
#अगर_घोषित INFILTER_PIPE
        अगर (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::म_त्रुटि(त्रुटि_सं));
            m_readEof = true;
            m_pidExited = true;
        }
#पूर्ण_अगर
    }

    व्योम readBlocks(पूर्णांक fd, पूर्णांक आकार, StrList& outl) {
        अक्षर buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        जबतक (!m_readEof && (आकार < 0 || आकार > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            अगर (आकार > 0 && आकार < todo) todo = आकार;
            त्रुटि_सं = 0;
            स्थिर ssize_t got = पढ़ो(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            अगर (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } अन्यथा अगर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#अगर_घोषित EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#पूर्ण_अगर
            ) {
                checkFilter(false);
                V3Os::u_sleep(1000);
                जारी;
            } अन्यथा {
                m_readEof = true;
                अवरोध;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_पंक्ति() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string पंक्ति;
        जबतक (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            स्थिर string onechar = listString(outl);
            पंक्ति += onechar;
            अगर (onechar == "\n") {
                अगर (पंक्ति == "\n") {
                    पंक्ति = "";
                    जारी;
                } अन्यथा {
                    अवरोध;
                }
            }
        }
        UINFO(6, "filter-line-in: " << पंक्ति);
        वापस पंक्ति;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    व्योम writeFilter(स्थिर string& out) {
        अगर (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            अगर (out[out.लंबाई() - 1] != '\n') अ_बाहर << '\n';
        }
        अगर (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = true;
            stop();
        }
        अचिन्हित offset = 0;
        जबतक (!m_readEof && out.लंबाई() > offset) {
            त्रुटि_सं = 0;
            स्थिर पूर्णांक got = ग_लिखो(m_writeFd, (out.c_str()) + offset, out.लंबाई() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            अगर (got > 0) {
                offset += got;
            } अन्यथा अगर (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#अगर_घोषित EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#पूर्ण_अगर
            ) {
                checkFilter(false);
                V3Os::u_sleep(1000);
                जारी;
            } अन्यथा {
                अवरोध;
            }
        }
    }

    // Start the filter
    व्योम start(स्थिर string& command) {
        अगर (command == "") {
            m_pid = 0;  // Disabled
        } अन्यथा {
            startFilter(command);
        }
    }
    व्योम startFilter(स्थिर string& command) {
        (व्योम)command;  // Prevent Unused variable warning
#अगर_घोषित INFILTER_PIPE
        पूर्णांक fd_stdin[2];  // Can't use std::array
        पूर्णांक fd_stdout[2];  // Can't use std::array
        constexpr पूर्णांक P_RD = 0;
        constexpr पूर्णांक P_WR = 1;

        अगर (VL_UNCOVERABLE(pipe(fd_stdin) != 0 || pipe(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        अगर (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        स्थिर pid_t pid = विभाजन();
        अगर (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        अगर (pid == 0) {  // Child
            UINFO(6, "In child");
            बंद(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            बंद(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<अक्षर*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            ख_लिखो(मानक_त्रुटि, "--pipe-filter: exec failed: %s\n", std::म_त्रुटि(त्रुटि_सं));
            _निकास(1);
        } अन्यथा {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = false;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = false;

            बंद(fd_stdin[P_RD]);
            बंद(fd_stdout[P_WR]);

            पूर्णांक flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#पूर्ण_अगर
    }

    व्योम stop() {
        अगर (m_pid) stopFilter();
    }
    व्योम stopFilter() {
        UINFO(6, "Stopping filter process");
#अगर_घोषित INFILTER_PIPE
        बंद(m_writeFd);
        checkFilter(true);
        अगर (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        बंद(m_readFd);
        UINFO(6, "Closed");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#पूर्ण_अगर
    }

रक्षित:
    मित्र श्रेणी VInFilter;
    // Read file contents and return it
    bool readWholefile(स्थिर string& filename, StrList& outl) {
        स्थिर स्वतः it = m_contentsMap.find(filename);
        अगर (it != m_contentsMap.end()) {
            outl.धकेलो_back(it->second);
            वापस true;
        }
        अगर (!readContents(filename, outl)) वापस false;
        अगर (list_आकार(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        वापस true;
    }
    अटल माप_प्रकार list_आकार(स्थिर StrList& sl) {
        माप_प्रकार परिणाम = 0;
        क्रम (स्थिर string& i : sl) परिणाम += i.लंबाई();
        वापस परिणाम;
    }
    अटल string listString(स्थिर StrList& sl) {
        string परिणाम;
        क्रम (स्थिर string& i : sl) परिणाम += i;
        वापस परिणाम;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(स्थिर string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(स्थिर string& command) { m_impp = नया VInFilterImp{command}; }
VInFilter::~VInFilter() {
    अगर (m_impp) VL_DO_CLEAR(मिटाओ m_impp, m_impp = nullptr);
}

bool VInFilter::readWholefile(स्थिर string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    वापस m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

अटल constexpr पूर्णांक MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
अटल स्थिर std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    क्रम (पूर्णांक i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<माप_प्रकार>(i), ' ');
    वापस table;
}();

स्थिर std::string& V3OutFormatter::indentSpaces(पूर्णांक अंक) {
    // Indent the specified number of spaces.
    वापस s_indentSpaces[std::max(0, std::min(अंक, MAXSPACE))];
}

bool V3OutFormatter::tokenMatch(स्थिर अक्षर* cp, स्थिर अक्षर* cmp) {
    जबतक (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    अगर (*cmp) वापस false;
    अगर (*cp && !std::है_खाली(*cp)) वापस false;
    वापस true;
}

bool V3OutFormatter::tokenStart(स्थिर अक्षर* cp) {
    वापस (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

bool V3OutFormatter::tokenEnd(स्थिर अक्षर* cp) {
    वापस (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

bool V3OutFormatter::tokenNotStart(स्थिर अक्षर* cp) {
    वापस (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

पूर्णांक V3OutFormatter::endLevels(स्थिर अक्षर* strg) {
    पूर्णांक levels = m_indentLevel;
    {
        स्थिर अक्षर* cp = strg;
        जबतक (std::है_खाली(*cp)) ++cp;
        चयन (*cp) {
        हाल '\n':  // Newlines.. No need for whitespace before it
            वापस 0;
        हाल '#':  // Preproc directive
            अगर (m_lang == LA_C) वापस 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            स्थिर अक्षर* mp = cp;
            क्रम (; std::है_अक्षर_अंक(*mp); ++mp) {}
            अगर (mp[0] == ':' && mp[1] != ':') वापस (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    क्रम (स्थिर अक्षर* cp = strg; *cp; ++cp) {
        चयन (*cp) {
        हाल '}':
        हाल ')': levels -= m_blockIndent; अवरोध;
        हाल '<':
            अगर (m_lang == LA_XML) {
                अगर (cp[1] == '/') levels -= m_blockIndent;
            }
            अवरोध;
        हाल 'e':
            अगर (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            अवरोध;
        हाल '\t':
        हाल ' ': अवरोध;  // Continue
        शेष: वापस levels;  // Letter
        }
    }
    वापस levels;
}

व्योम V3OutFormatter::putns(स्थिर Ast_गाँठ* nodep, स्थिर अक्षर* strg) {
    अगर (!v3Global.opt.decoration()) {
        putsOutput(strg);
        वापस;
    }

    स्थिर bool put_गाँठ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_पंक्ति::builtInFilename() != nodep->fileline()->filename();

    अगर (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_गाँठ_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = false;
    }

    अगर (put_गाँठ_Decoration) {
        स्थिर File_पंक्ति* स्थिर flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        स्थिर std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    bool notstart = false;
    bool wordstart = true;
    bool equalsForBracket = false;  // Looking for "= {"
    क्रम (स्थिर अक्षर* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        अगर (std::है_अक्षर(*cp)) {
            अगर (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = true;
            अगर (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            अगर (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        स्थिर bool wasInBackslash = m_inBackslash;
        m_inBackslash = false;
        चयन (*cp) {
        हाल '\n':
            ++m_lineno;
            wordstart = true;
            अगर (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = true;
            } अन्यथा {
                m_prependIndent = false;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            अवरोध;
        हाल ' ': wordstart = true; अवरोध;
        हाल '\t': wordstart = true; अवरोध;
        हाल '"':
            wordstart = false;
            अगर (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            अवरोध;
        हाल '\\':
            wordstart = false;
            अगर (!wasInBackslash) m_inBackslash = true;
            अवरोध;
        हाल '/':
            अगर (m_lang == LA_C || m_lang == LA_VERILOG) {
                अगर (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    जबतक (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    अगर (*cp) putcNoTracking(*cp);
                }
            }
            अवरोध;
        हाल '{':
            अगर (!m_inStringLiteral) {
                अगर (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.धकेलो(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            अवरोध;
        हाल '}':
            अगर (!m_inStringLiteral) {
                अगर (m_bracketLevel > 0) {
                    m_parenVec.निकालो();
                    --m_bracketLevel;
                }
                indentDec();
            }
            अवरोध;
        हाल '(':
            अगर (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.धकेलो(m_column);
            }
            अवरोध;
        हाल ')':
            अगर (!m_inStringLiteral) {
                अगर (!m_parenVec.empty()) m_parenVec.निकालो();
                indentDec();
            }
            अवरोध;
        हाल '<':
            अगर (m_lang == LA_XML) {
                अगर (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } अन्यथा अगर (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } अन्यथा {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            अवरोध;
        हाल '>':
            अगर (m_lang == LA_XML) {
                indentDec();
                अगर (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            अवरोध;
        शेष: wordstart = false; अवरोध;
        }

        चयन (*cp) {
        हाल '=': equalsForBracket = true; अवरोध;
        हाल ' ': अवरोध;
        शेष: equalsForBracket = false; अवरोध;
        }
    }
}

व्योम V3OutFormatter::put_Break_व्यंजक() {
    अगर (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
व्योम V3OutFormatter::putBreak() {
    अगर (!v3Global.opt.decoration()) वापस;
    अगर (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        अगर (exceededWidth()) {
            putcNoTracking('\n');
            अगर (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.शीर्ष()));
        }
    }
}

व्योम V3OutFormatter::putsQuoted(स्थिर string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    स्थिर string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    क्रम (स्थिर अक्षर c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    अगर (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
व्योम V3OutFormatter::putsNoTracking(स्थिर string& strg) {
    अगर (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        वापस;
    }
    // Don't track {}'s, probably because it's a $display format string
    क्रम (स्थिर अक्षर c : strg) putcNoTracking(c);
}

व्योम V3OutFormatter::putcNoTracking(अक्षर chr) {
    अगर (!v3Global.opt.decoration()) {
        putcOutput(chr);
        वापस;
    }
    चयन (chr) {
    हाल '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = true;
        अवरोध;
    हाल '\t': m_column = ((m_column + 9) / 8) * 8; अवरोध;
    हाल ' ':
    हाल '(':
    हाल '|':
    हाल '&': ++m_column; अवरोध;
    शेष:
        ++m_column;
        m_nobreak = false;
        अवरोध;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(स्थिर string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    अगर (lang == LA_XML) {
        // Encode chars into XML string
        क्रम (स्थिर अक्षर c : namein) {
            अगर (c == '"') {
                out += "&quot;"s;
            } अन्यथा अगर (c == '\'') {
                out += "&apos;"s;
            } अन्यथा अगर (c == '<') {
                out += "&lt;"s;
            } अन्यथा अगर (c == '>') {
                out += "&gt;"s;
            } अन्यथा अगर (c == '&') {
                out += "&amp;"s;
            } अन्यथा अगर (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                out += "&#"s + cvtToStr(static_cast<अचिन्हित पूर्णांक>(c & 0xff)) + ";";
            }
        }
    } अन्यथा {
        // Encode control chars into C style escapes
        क्रम (स्थिर अक्षर c : namein) {
            अगर (c == '\\' || c == '"') {
                out += "\\"s + c;
            } अन्यथा अगर (c == '\n') {
                out += "\\n";
            } अन्यथा अगर (c == '\r') {
                out += "\\r";
            } अन्यथा अगर (c == '\t') {
                out += "\\t";
            } अन्यथा अगर (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                // This will also cover \a etc
                स्थिर string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    वापस out;
}

//----------------------------------------------------------------------
// Simple wrappers

व्योम V3OutFormatter::म_लिखो(स्थिर अक्षर* fmt...) {
    constexpr माप_प्रकार bufsize = 5000;
    अक्षर sbuff[bufsize];
    बहु_सूची ap;
    बहु_शुरू(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    बहु_पूर्ण(ap);
    यह->माला_दो(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(स्थिर string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{नया std::array<अक्षर, WRITE_बफर_आकार_BYTES>{}} {
    अगर ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    अगर (m_fp) ख_बंद(m_fp);
    m_fp = nullptr;
}

व्योम V3OutFile::putsForceIncs() {
    स्थिर VStringList& forceIncs = v3Global.opt.forceIncs();
    क्रम (स्थिर string& i : forceIncs) माला_दो("#include \"" + i + "\"\n");
}

व्योम V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = true;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    क्रम (अक्षर& c : var) {
        अगर (!std::है_अक्षर_अंक(c)) c = '_';
    }
    माला_दो("\n#ifndef " + var + "\n");
    माला_दो("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ब_प्रवाह& ब_प्रवाह, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ब_प्रवाह} {}

//######################################################################
// VIdProtect

श्रेणी VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<स्थिर std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
रक्षित:
    // CONSTRUCTORS
    मित्र श्रेणी VIdProtect;
    अटल VIdProtectImp& singleton() VL_MT_SAFE {
        अटल VIdProtectImp s;
        वापस s;
    }

खुला:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = शेष;
    // METHODS
    string passthru(स्थिर string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        अगर (!v3Global.opt.protectIds()) वापस old;
        स्थिर V3LockGuard lock{m_mutex};
        स्थिर स्वतः it = m_nameMap.find(old);
        अगर (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } अन्यथा {
            m_nameMap.emplace(old, old);
            m_newIdSet.जोड़ो(old);
        }
        वापस old;
    }
    string protectIf(स्थिर string& old, bool doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        अगर (!v3Global.opt.protectIds() || old.empty() || !doIt) वापस old;
        स्थिर V3LockGuard lock{m_mutex};
        स्थिर स्वतः pair = m_nameMap.emplace(old, "");
        अगर (pair.second) {
            string out;
            अगर (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } अन्यथा {
                VHashSha512 digest{v3Global.opt.protect_कुंजी_Defaulted()};
                digest.जोड़ो(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                क्रम (माप_प्रकार len = 6; len < out.आकार() - 3; len += 3) {
                    स्थिर string tryout = out.substr(0, len);
                    अगर (m_newIdSet.जोड़ो(tryout).second) {
                        out = tryout;
                        अवरोध;
                    }
                }
            }
            pair.first->second = out;
        }
        वापस pair.first->second;
    }
    string protect_शब्दगण_If(स्थिर string& old, bool doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        अगर (!(doIt && v3Global.opt.protectIds())) वापस old;
        string out;
        string::आकार_type start = 0;
        // space, ., ->
        जबतक (true) {
            // When C++11, use find_if and lambda
            string::आकार_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            अगर (pos == string::npos) अवरोध;
            out += protectIf(old.substr(start, pos - start), true) + separator;
            start = pos + separator.लंबाई();
        }
        out += protectIf(old.substr(start), true);
        वापस out;
    }
    व्योम writeMapFile(स्थिर string& filename) स्थिर {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.माला_दो("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.माला_दो("<verilator_id_map>\n");
        {
            क्रम (स्थिर स्वतः& itr : m_nameMap) {
                of.माला_दो("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.माला_दो("</verilator_id_map>\n");
    }

निजी:
    व्योम trySep(स्थिर string& old, string::आकार_type start, स्थिर string& trySep,
                string::आकार_type& posr, string& separatorr) VL_PURE {
        स्थिर string::आकार_type trypos = old.find(trySep, start);
        अगर (trypos != string::npos) {
            अगर (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(स्थिर string& in) VL_MT_SAFE {
    वापस v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(स्थिर string& old, bool doIt) VL_MT_SAFE {
    वापस VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_शब्दगण_If(स्थिर string& old, bool doIt) VL_MT_SAFE {
    वापस VIdProtectImp::singleton().protect_शब्दगण_If(old, doIt);
}
व्योम VIdProtect::writeMapFile(स्थिर string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
