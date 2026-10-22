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

#अन्तर्भाव "V3PchAstMT.h"

#अन्तर्भाव "V3File.h"

#अन्तर्भाव "V3Os.h"
#अन्तर्भाव "V3String.h"

#अन्तर्भाव <cerrno>
#अन्तर्भाव <cstdarg>
#अन्तर्भाव <fcntl.h>
#अन्तर्भाव <iomanip>
#अन्तर्भाव <map>
#अन्तर्भाव <memory>

#अन्तर्भाव <sys/stat.h>
#अन्तर्भाव <sys/types.h>

// clang-format off
#सचे defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#अञ्ञथा
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#सचे defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr पूर्णाङ्क INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr पूर्णाङ्क INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

वग्ग V3FileDependImp final {
    // TYPES
    वग्ग DependFile final {
        // A single file
        ध्रुव तार्किक m_target;  // True if write, else read
        तार्किक m_exists = सच्च;  // File exists
        ध्रुव string m_filename;  // Filename
        संरचना stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    सार्वजनिक:
        DependFile(ध्रुव string& filename, तार्किक target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = स्वभाव;
        ध्रुव string& filename() ध्रुव { पटिनिवत्त m_filename; }
        तार्किक target() ध्रुव { पटिनिवत्त m_target; }
        तार्किक exists() ध्रुव { पटिनिवत्त m_exists; }
        off_t size() ध्रुव { पटिनिवत्त m_stat.st_size; }
        ino_t ino() ध्रुव { पटिनिवत्त m_stat.st_ino; }
        time_t cstime() ध्रुव { पटिनिवत्त m_stat.st_ctime; }  // Seconds
        time_t cnstime() ध्रुव { पटिनिवत्त VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() ध्रुव { पटिनिवत्त m_stat.st_mtime; }  // Seconds
        time_t mnstime() ध्रुव { पटिनिवत्त VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_चिह्नम्() {
            स्थिर VHashSha512 s_emptyHash;
            पटिनिवत्त m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        शून्य loadStats() {
            सचे (!m_stat.st_mtime) {
                ध्रुव string fn = filename();
                ध्रुव पूर्णाङ्क err = stat(fn.c_str(), &m_stat);
                सचे (err != 0) {
                    std::memset(&m_stat, 0, परिमाण(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = मिच्छा;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } अञ्ञथा {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    सचे (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.निवेशय_File(filename());
                }
            }
        }
        तार्किक operator<(ध्रुव DependFile& rhs) ध्रुव { पटिनिवत्त filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    स्थिर string stripQuotes(ध्रुव string& in) {
        string pretty = in;
        string::size_type pos;
        याव ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        याव ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        पटिनिवत्त pretty;
    }

सार्वजनिक:
    // ACCESSOR METHODS
    शून्य addSrcDepend(ध्रुव string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3LockGuard lock{m_mutex};
        ध्रुव auto itFoundPair = m_filenameSet.निवेशय(filename);
        सचे (itFoundPair.second) {
            DependFile df{filename, मिच्छा};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.निवेशय(df);
        }
    }
    शून्य addTgtDepend(ध्रुव string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ध्रुव V3LockGuard lock{m_mutex};
        ध्रुव auto itFoundPair = m_filenameSet.निवेशय(filename);
        सचे (itFoundPair.second) m_filenameList.निवेशय(DependFile{filename, सच्च});
    }
    शून्य writeDepend(ध्रुव string& filename);
    std::vector<string> getAllDeps() ध्रुव;
    शून्य writeTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn);
    तार्किक checkTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

शून्य V3FileDependImp::writeDepend(ध्रुव string& filename) {
    ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    सचे (ofp->fail()) v3fatal("Can't write file: " << filename);

    अत्थाय (ध्रुव DependFile& i : m_filenameList) {
        सचे (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    अत्थाय (ध्रुव DependFile& i : m_filenameList) {
        सचे (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    सचे (v3Global.opt.makePhony()) {
        *ofp << '\n';
        अत्थाय (ध्रुव DependFile& i : m_filenameList) {
            सचे (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() ध्रुव {
    std::vector<string> r;
    अत्थाय (ध्रुव auto& itr : m_filenameList) {
        सचे (!itr.target() && itr.exists()) r.प्रक्षिप_back(itr.filename());
    }
    पटिनिवत्त r;
}

शून्य V3FileDependImp::writeTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn) {
    ध्रुव std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    सचे (ofp->fail()) v3fatal("Can't write file: " << filename);

    ध्रुव string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    अत्थाय (auto& it : m_filenameList) {
        DependFile* ध्रुव depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        सचे (depfilep->filename() == filename) {
            showSize = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << showSize;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_चिह्नम्() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

तार्किक V3FileDependImp::checkTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn) {
    ध्रुव std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    सचे (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        पटिनिवत्त मिच्छा;
    }
    {
        ध्रुव string ignore = V3Os::getline(*ifp);
        सचे (ignore.empty()) { /*used*/
        }
    }
    {
        वर्ण chkDir;
        *ifp >> chkDir;
        वर्ण quote;
        *ifp >> quote;
        ध्रुव string chkCmdline = V3Os::getline(*ifp, '"');
        ध्रुव string cmdline = stripQuotes(cmdlineIn);
        सचे (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            पटिनिवत्त मिच्छा;
        }
    }

    ध्रुव तार्किक skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    याव (!ifp->eof()) {
        वर्ण chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        सचे (ifp->eof()) भङ्ग;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        वर्ण quote;
        *ifp >> quote;
        ध्रुव string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ध्रुव string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        संरचना stat curStat;
        ध्रुव पूर्णाङ्क err = stat(chkFilename.c_str(), &curStat);
        सचे (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            पटिनिवत्त मिच्छा;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        सचे (filename == chkFilename) अनुवर्त;

        UINFO(9, " got d=" << chkDir << " s=" << chkSize << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_size << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        सचे (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            सचे (skipHashing) पटिनिवत्त मिच्छा;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            सचे (chkHash == "unhashed") पटिनिवत्त मिच्छा;

            VHashSha512 curHash;
            curHash.निवेशय_File(chkFilename);
            सचे (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                पटिनिवत्त मिच्छा;
            }
        }
    }
    पटिनिवत्त सच्च;
}

//######################################################################
// V3File

शून्य V3File::addSrcDepend(ध्रुव string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
शून्य V3File::addTgtDepend(ध्रुव string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
शून्य V3File::writeDepend(ध्रुव string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { पटिनिवत्त dependImp.getAllDeps(); }
शून्य V3File::writeTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
तार्किक V3File::checkTimes(ध्रुव string& filename, ध्रुव string& cmdlineIn) {
    पटिनिवत्त dependImp.checkTimes(filename, cmdlineIn);
}
शून्य V3File::createMakeDirFor(ध्रुव string& filename) {
    सचे (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
शून्य V3File::createMakeDir() {
    स्थिर तार्किक s_created = मिच्छा;
    सचे (!s_created) {
        s_created = सच्च;
        V3Os::createDir(v3Global.opt.makeDir());
        सचे (v3Global.opt.hier_शिखरम्()) V3Os::createDir(v3Global.opt.hier_शिखरम्_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

वग्ग VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<ध्रुव std::string, std::string> m_contentsMap;  // Cache of file contents
    तार्किक m_readEof = मिच्छा;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#अञ्ञथा
    पूर्णाङ्क m_pid = 0;  // fork() process id - always zero as disabled
#endif
    तार्किक m_pidExited = मिच्छा;  //  waitpid() reaped the filter process
    पूर्णाङ्क m_pidStatus = 0;  // Exit status of the filter process
    पूर्णाङ्क m_writeFd = 0;  // File descriptor TO filter
    पूर्णाङ्क m_readFd = 0;  // File descriptor FROM filter

गुप्त:
    // METHODS

    तार्किक readContents(ध्रुव string& filename, StrList& outl) {
        सचे (m_pid) {
            पटिनिवत्त readContentsFilter(filename, outl);
        } अञ्ञथा {
            पटिनिवत्त readContentsFile(filename, outl);
        }
    }
    तार्किक readContentsFile(ध्रुव string& filename, StrList& outl) {
        ध्रुव पूर्णाङ्क fd = open(filename.c_str(), O_RDONLY);
        सचे (fd < 0) पटिनिवत्त मिच्छा;
        m_readEof = मिच्छा;
        readBlocks(fd, -1, outl);
        close(fd);
        पटिनिवत्त सच्च;
    }
    // cppcheck-suppress constParameterReference
    तार्किक readContentsFilter(ध्रुव string& filename, StrList& outl) {
        (शून्य)filename;  // Prevent unused variable warning
        (शून्य)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ध्रुव string पङ्क्तिः = read_Filter_पङ्क्तिः();
        सचे (पङ्क्तिः.find("Content-Length") != string::npos) {
            पूर्णाङ्क len = 0;
            sscanf(पङ्क्तिः.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            पटिनिवत्त सच्च;
        } अञ्ञथा {
            सचे (पङ्क्तिः != "") v3error("--pipe-filter protocol error, unexpected: " << पङ्क्तिः);
            पटिनिवत्त मिच्छा;
        }
#अञ्ञथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
        पटिनिवत्त मिच्छा;
#endif
    }

    // cppcheck-suppress functionConst
    शून्य checkFilter(तार्किक hang) {
#ifdef INFILTER_PIPE
        सचे (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = सच्च;
            m_pidExited = सच्च;
        }
#endif
    }

    शून्य readBlocks(पूर्णाङ्क fd, पूर्णाङ्क size, StrList& outl) {
        वर्ण buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        याव (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            सचे (size > 0 && size < todo) todo = size;
            errno = 0;
            ध्रुव ssize_t got = पठ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            सचे (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } अञ्ञथा सचे (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(मिच्छा);
                V3Os::u_sleep(1000);
                अनुवर्त;
            } अञ्ञथा {
                m_readEof = सच्च;
                भङ्ग;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_पङ्क्तिः() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string पङ्क्तिः;
        याव (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            ध्रुव string onechar = listString(outl);
            पङ्क्तिः += onechar;
            सचे (onechar == "\n") {
                सचे (पङ्क्तिः == "\n") {
                    पङ्क्तिः = "";
                    अनुवर्त;
                } अञ्ञथा {
                    भङ्ग;
                }
            }
        }
        UINFO(6, "filter-line-in: " << पङ्क्तिः);
        पटिनिवत्त पङ्क्तिः;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    शून्य writeFilter(ध्रुव string& out) {
        सचे (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            सचे (out[out.length() - 1] != '\n') cout << '\n';
        }
        सचे (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = सच्च;
            stop();
        }
        unsigned offset = 0;
        याव (!m_readEof && out.length() > offset) {
            errno = 0;
            ध्रुव पूर्णाङ्क got = लिख(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            सचे (got > 0) {
                offset += got;
            } अञ्ञथा सचे (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(मिच्छा);
                V3Os::u_sleep(1000);
                अनुवर्त;
            } अञ्ञथा {
                भङ्ग;
            }
        }
    }

    // Start the filter
    शून्य start(ध्रुव string& command) {
        सचे (command == "") {
            m_pid = 0;  // Disabled
        } अञ्ञथा {
            startFilter(command);
        }
    }
    शून्य startFilter(ध्रुव string& command) {
        (शून्य)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        पूर्णाङ्क fd_stdin[2];  // Can't use std::array
        पूर्णाङ्क fd_stdout[2];  // Can't use std::array
        constexpr पूर्णाङ्क P_RD = 0;
        constexpr पूर्णाङ्क P_WR = 1;

        सचे (VL_UNCOVERABLE(प्रणाल(fd_stdin) != 0 || प्रणाल(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        सचे (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ध्रुव pid_t pid = fork();
        सचे (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        सचे (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<वर्ण*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } अञ्ञथा {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = मिच्छा;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = मिच्छा;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            पूर्णाङ्क flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#अञ्ञथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    शून्य stop() {
        सचे (m_pid) stopFilter();
    }
    शून्य stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(सच्च);
        सचे (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#अञ्ञथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

रक्षित:
    friend वग्ग VInFilter;
    // Read file contents and return it
    तार्किक readWholefile(ध्रुव string& filename, StrList& outl) {
        ध्रुव auto it = m_contentsMap.find(filename);
        सचे (it != m_contentsMap.end()) {
            outl.प्रक्षिप_back(it->second);
            पटिनिवत्त सच्च;
        }
        सचे (!readContents(filename, outl)) पटिनिवत्त मिच्छा;
        सचे (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        पटिनिवत्त सच्च;
    }
    स्थिर size_t listSize(ध्रुव StrList& sl) {
        size_t result = 0;
        अत्थाय (ध्रुव string& i : sl) result += i.length();
        पटिनिवत्त result;
    }
    स्थिर string listString(ध्रुव StrList& sl) {
        string result;
        अत्थाय (ध्रुव string& i : sl) result += i;
        पटिनिवत्त result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(ध्रुव string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ध्रुव string& command) { m_impp = नूतन VInFilterImp{command}; }
VInFilter::~VInFilter() {
    सचे (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

तार्किक VInFilter::readWholefile(ध्रुव string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    पटिनिवत्त m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

स्थिर constexpr पूर्णाङ्क MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
स्थिर ध्रुव std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    अत्थाय (पूर्णाङ्क i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    पटिनिवत्त table;
}();

ध्रुव std::string& V3OutFormatter::indentSpaces(पूर्णाङ्क अङ्कः) {
    // Indent the specified number of spaces.
    पटिनिवत्त s_indentSpaces[std::max(0, std::min(अङ्कः, MAXSPACE))];
}

तार्किक V3OutFormatter::tokenMatch(ध्रुव वर्ण* cp, ध्रुव वर्ण* cmp) {
    याव (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    सचे (*cmp) पटिनिवत्त मिच्छा;
    सचे (*cp && !std::isspace(*cp)) पटिनिवत्त मिच्छा;
    पटिनिवत्त सच्च;
}

तार्किक V3OutFormatter::tokenStart(ध्रुव वर्ण* cp) {
    पटिनिवत्त (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

तार्किक V3OutFormatter::tokenEnd(ध्रुव वर्ण* cp) {
    पटिनिवत्त (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

तार्किक V3OutFormatter::tokenNotStart(ध्रुव वर्ण* cp) {
    पटिनिवत्त (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

पूर्णाङ्क V3OutFormatter::endLevels(ध्रुव वर्ण* strg) {
    पूर्णाङ्क levels = m_indentLevel;
    {
        ध्रुव वर्ण* cp = strg;
        याव (std::isspace(*cp)) ++cp;
        विकल्प (*cp) {
        पक्ष '\n':  // Newlines.. No need for whitespace before it
            पटिनिवत्त 0;
        पक्ष '#':  // Preproc directive
            सचे (m_lang == LA_C) पटिनिवत्त 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ध्रुव वर्ण* mp = cp;
            अत्थाय (; std::isalnum(*mp); ++mp) {}
            सचे (mp[0] == ':' && mp[1] != ':') पटिनिवत्त (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    अत्थाय (ध्रुव वर्ण* cp = strg; *cp; ++cp) {
        विकल्प (*cp) {
        पक्ष '}':
        पक्ष ')': levels -= m_blockIndent; भङ्ग;
        पक्ष '<':
            सचे (m_lang == LA_XML) {
                सचे (cp[1] == '/') levels -= m_blockIndent;
            }
            भङ्ग;
        पक्ष 'e':
            सचे (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            भङ्ग;
        पक्ष '\t':
        पक्ष ' ': भङ्ग;  // Continue
        स्वभाव: पटिनिवत्त levels;  // Letter
        }
    }
    पटिनिवत्त levels;
}

शून्य V3OutFormatter::putns(ध्रुव Ast_ग्रन्थिः* nodep, ध्रुव वर्ण* strg) {
    सचे (!v3Global.opt.decoration()) {
        putsOutput(strg);
        पटिनिवत्त;
    }

    ध्रुव तार्किक put_ग्रन्थिः_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_पङ्क्तिः::builtInFilename() != nodep->fileline()->filename();

    सचे (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ग्रन्थिः_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = मिच्छा;
    }

    सचे (put_ग्रन्थिः_Decoration) {
        ध्रुव File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        ध्रुव std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    तार्किक notstart = मिच्छा;
    तार्किक wordstart = सच्च;
    तार्किक equalsForBracket = मिच्छा;  // Looking for "= {"
    अत्थाय (ध्रुव वर्ण* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        सचे (std::isalpha(*cp)) {
            सचे (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = सच्च;
            सचे (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            सचे (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        ध्रुव तार्किक wasInBackslash = m_inBackslash;
        m_inBackslash = मिच्छा;
        विकल्प (*cp) {
        पक्ष '\n':
            ++m_lineno;
            wordstart = सच्च;
            सचे (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = सच्च;
            } अञ्ञथा {
                m_prependIndent = मिच्छा;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            भङ्ग;
        पक्ष ' ': wordstart = सच्च; भङ्ग;
        पक्ष '\t': wordstart = सच्च; भङ्ग;
        पक्ष '"':
            wordstart = मिच्छा;
            सचे (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            भङ्ग;
        पक्ष '\\':
            wordstart = मिच्छा;
            सचे (!wasInBackslash) m_inBackslash = सच्च;
            भङ्ग;
        पक्ष '/':
            सचे (m_lang == LA_C || m_lang == LA_VERILOG) {
                सचे (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    याव (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    सचे (*cp) putcNoTracking(*cp);
                }
            }
            भङ्ग;
        पक्ष '{':
            सचे (!m_inStringLiteral) {
                सचे (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.प्रक्षिप(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            भङ्ग;
        पक्ष '}':
            सचे (!m_inStringLiteral) {
                सचे (m_bracketLevel > 0) {
                    m_parenVec.निष्कासय();
                    --m_bracketLevel;
                }
                indentDec();
            }
            भङ्ग;
        पक्ष '(':
            सचे (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.प्रक्षिप(m_column);
            }
            भङ्ग;
        पक्ष ')':
            सचे (!m_inStringLiteral) {
                सचे (!m_parenVec.empty()) m_parenVec.निष्कासय();
                indentDec();
            }
            भङ्ग;
        पक्ष '<':
            सचे (m_lang == LA_XML) {
                सचे (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } अञ्ञथा सचे (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } अञ्ञथा {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            भङ्ग;
        पक्ष '>':
            सचे (m_lang == LA_XML) {
                indentDec();
                सचे (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            भङ्ग;
        स्वभाव: wordstart = मिच्छा; भङ्ग;
        }

        विकल्प (*cp) {
        पक्ष '=': equalsForBracket = सच्च; भङ्ग;
        पक्ष ' ': भङ्ग;
        स्वभाव: equalsForBracket = मिच्छा; भङ्ग;
        }
    }
}

शून्य V3OutFormatter::put_Break_व्यञ्जकम्() {
    सचे (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
शून्य V3OutFormatter::putBreak() {
    सचे (!v3Global.opt.decoration()) पटिनिवत्त;
    सचे (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        सचे (exceededWidth()) {
            putcNoTracking('\n');
            सचे (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.शिखरम्()));
        }
    }
}

शून्य V3OutFormatter::putsQuoted(ध्रुव string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ध्रुव string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    अत्थाय (ध्रुव वर्ण c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    सचे (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
शून्य V3OutFormatter::putsNoTracking(ध्रुव string& strg) {
    सचे (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        पटिनिवत्त;
    }
    // Don't track {}'s, probably because it's a $display format string
    अत्थाय (ध्रुव वर्ण c : strg) putcNoTracking(c);
}

शून्य V3OutFormatter::putcNoTracking(वर्ण chr) {
    सचे (!v3Global.opt.decoration()) {
        putcOutput(chr);
        पटिनिवत्त;
    }
    विकल्प (chr) {
    पक्ष '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = सच्च;
        भङ्ग;
    पक्ष '\t': m_column = ((m_column + 9) / 8) * 8; भङ्ग;
    पक्ष ' ':
    पक्ष '(':
    पक्ष '|':
    पक्ष '&': ++m_column; भङ्ग;
    स्वभाव:
        ++m_column;
        m_nobreak = मिच्छा;
        भङ्ग;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(ध्रुव string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    सचे (lang == LA_XML) {
        // Encode chars into XML string
        अत्थाय (ध्रुव वर्ण c : namein) {
            सचे (c == '"') {
                out += "&quot;"s;
            } अञ्ञथा सचे (c == '\'') {
                out += "&apos;"s;
            } अञ्ञथा सचे (c == '<') {
                out += "&lt;"s;
            } अञ्ञथा सचे (c == '>') {
                out += "&gt;"s;
            } अञ्ञथा सचे (c == '&') {
                out += "&amp;"s;
            } अञ्ञथा सचे (std::isprint(c)) {
                out += c;
            } अञ्ञथा {
                out += "&#"s + cvtToStr(static_cast<unsigned पूर्णाङ्क>(c & 0xff)) + ";";
            }
        }
    } अञ्ञथा {
        // Encode control chars into C style escapes
        अत्थाय (ध्रुव वर्ण c : namein) {
            सचे (c == '\\' || c == '"') {
                out += "\\"s + c;
            } अञ्ञथा सचे (c == '\n') {
                out += "\\n";
            } अञ्ञथा सचे (c == '\r') {
                out += "\\r";
            } अञ्ञथा सचे (c == '\t') {
                out += "\\t";
            } अञ्ञथा सचे (std::isprint(c)) {
                out += c;
            } अञ्ञथा {
                // This will also cover \a etc
                ध्रुव string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    पटिनिवत्त out;
}

//----------------------------------------------------------------------
// Simple wrappers

शून्य V3OutFormatter::printf(ध्रुव वर्ण* fmt...) {
    constexpr size_t bufsize = 5000;
    वर्ण sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    एतत्->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(ध्रुव string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{नूतन std::array<वर्ण, WRITE_BUFFER_SIZE_BYTES>{}} {
    सचे ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    सचे (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

शून्य V3OutFile::putsForceIncs() {
    ध्रुव VStringList& forceIncs = v3Global.opt.forceIncs();
    अत्थाय (ध्रुव string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

शून्य V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = सच्च;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    अत्थाय (वर्ण& c : var) {
        सचे (!std::isalnum(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

वग्ग VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ध्रुव std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
रक्षित:
    // CONSTRUCTORS
    friend वग्ग VIdProtect;
    स्थिर VIdProtectImp& singleton() VL_MT_SAFE {
        स्थिर VIdProtectImp s;
        पटिनिवत्त s;
    }

सार्वजनिक:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = स्वभाव;
    // METHODS
    string passthru(ध्रुव string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        सचे (!v3Global.opt.protectIds()) पटिनिवत्त old;
        ध्रुव V3LockGuard lock{m_mutex};
        ध्रुव auto it = m_nameMap.find(old);
        सचे (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } अञ्ञथा {
            m_nameMap.emplace(old, old);
            m_newIdSet.निवेशय(old);
        }
        पटिनिवत्त old;
    }
    string protectIf(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        सचे (!v3Global.opt.protectIds() || old.empty() || !doIt) पटिनिवत्त old;
        ध्रुव V3LockGuard lock{m_mutex};
        ध्रुव auto pair = m_nameMap.emplace(old, "");
        सचे (pair.second) {
            string out;
            सचे (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } अञ्ञथा {
                VHashSha512 digest{v3Global.opt.protect_कुञ्चिका_Defaulted()};
                digest.निवेशय(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                अत्थाय (size_t len = 6; len < out.size() - 3; len += 3) {
                    ध्रुव string tryout = out.substr(0, len);
                    सचे (m_newIdSet.निवेशय(tryout).second) {
                        out = tryout;
                        भङ्ग;
                    }
                }
            }
            pair.first->second = out;
        }
        पटिनिवत्त pair.first->second;
    }
    string protect_शब्दाः_If(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        सचे (!(doIt && v3Global.opt.protectIds())) पटिनिवत्त old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        याव (सच्च) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            सचे (pos == string::npos) भङ्ग;
            out += protectIf(old.substr(start, pos - start), सच्च) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), सच्च);
        पटिनिवत्त out;
    }
    शून्य writeMapFile(ध्रुव string& filename) ध्रुव {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            अत्थाय (ध्रुव auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

गुप्त:
    शून्य trySep(ध्रुव string& old, string::size_type start, ध्रुव string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        ध्रुव string::size_type trypos = old.find(trySep, start);
        सचे (trypos != string::npos) {
            सचे (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ध्रुव string& in) VL_MT_SAFE {
    पटिनिवत्त v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
    पटिनिवत्त VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_शब्दाः_If(ध्रुव string& old, तार्किक doIt) VL_MT_SAFE {
    पटिनिवत्त VIdProtectImp::singleton().protect_शब्दाः_If(old, doIt);
}
शून्य VIdProtect::writeMapFile(ध्रुव string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
