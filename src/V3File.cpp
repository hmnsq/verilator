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
#यदि defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#अन्यथा
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#यदि defined(_WIN32) || defined(__MINGW32__)
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

वर्ग V3FileDependImp final {
    // TYPES
    वर्ग DependFile final {
        // A single file
        अचर तार्किक m_target;  // True if write, else read
        तार्किक m_exists = सत्य;  // File exists
        अचर string m_filename;  // Filename
        संरचना stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    सार्वजनिक:
        DependFile(अचर string& filename, तार्किक target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = स्वभाव;
        अचर string& filename() अचर { फर्काऊ m_filename; }
        तार्किक target() अचर { फर्काऊ m_target; }
        तार्किक exists() अचर { फर्काऊ m_exists; }
        off_t size() अचर { फर्काऊ m_stat.st_size; }
        ino_t ino() अचर { फर्काऊ m_stat.st_ino; }
        समय_प्रकार cstime() अचर { फर्काऊ m_stat.st_ctime; }  // Seconds
        समय_प्रकार cnstime() अचर { फर्काऊ VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        समय_प्रकार mstime() अचर { फर्काऊ m_stat.st_mtime; }  // Seconds
        समय_प्रकार mnstime() अचर { फर्काऊ VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_चिह्न() {
            स्थिर VHashSha512 s_emptyHash;
            फर्काऊ m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        शून्य loadStats() {
            यदि (!m_stat.st_mtime) {
                अचर string fn = filename();
                अचर पूर्णाङ्क err = stat(fn.c_str(), &m_stat);
                यदि (err != 0) {
                    std::स_रखो(&m_stat, 0, परिमाण(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = असत्य;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } अन्यथा {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    यदि (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.घुसाउ_File(filename());
                }
            }
        }
        तार्किक चालक<(अचर DependFile& rhs) अचर { फर्काऊ filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    स्थिर string stripQuotes(अचर string& in) {
        string pretty = in;
        string::size_type pos;
        जबसम्म ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        जबसम्म ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        फर्काऊ pretty;
    }

सार्वजनिक:
    // ACCESSOR METHODS
    शून्य addSrcDepend(अचर string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        अचर V3LockGuard lock{m_mutex};
        अचर स्वतः itFoundPair = m_filenameSet.घुसाउ(filename);
        यदि (itFoundPair.second) {
            DependFile df{filename, असत्य};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.घुसाउ(df);
        }
    }
    शून्य addTgtDepend(अचर string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        अचर V3LockGuard lock{m_mutex};
        अचर स्वतः itFoundPair = m_filenameSet.घुसाउ(filename);
        यदि (itFoundPair.second) m_filenameList.घुसाउ(DependFile{filename, सत्य});
    }
    शून्य writeDepend(अचर string& filename);
    std::vector<string> getAllDeps() अचर;
    शून्य writeTimes(अचर string& filename, अचर string& cmdlineIn);
    तार्किक checkTimes(अचर string& filename, अचर string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

शून्य V3FileDependImp::writeDepend(अचर string& filename) {
    अचर std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    लागि (अचर DependFile& i : m_filenameList) {
        यदि (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    लागि (अचर DependFile& i : m_filenameList) {
        यदि (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    यदि (v3Global.opt.makePhony()) {
        *ofp << '\n';
        लागि (अचर DependFile& i : m_filenameList) {
            यदि (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() अचर {
    std::vector<string> r;
    लागि (अचर स्वतः& itr : m_filenameList) {
        यदि (!itr.target() && itr.exists()) r.थप_back(itr.filename());
    }
    फर्काऊ r;
}

शून्य V3FileDependImp::writeTimes(अचर string& filename, अचर string& cmdlineIn) {
    अचर std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    यदि (ofp->fail()) v3fatal("Can't write file: " << filename);

    अचर string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    लागि (स्वतः& it : m_filenameList) {
        DependFile* अचर depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        यदि (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hash_Digest_चिह्न() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

तार्किक V3FileDependImp::checkTimes(अचर string& filename, अचर string& cmdlineIn) {
    अचर std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    यदि (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        फर्काऊ असत्य;
    }
    {
        अचर string ignore = V3Os::getline(*ifp);
        यदि (ignore.empty()) { /*used*/
        }
    }
    {
        वर्ण chkDir;
        *ifp >> chkDir;
        वर्ण quote;
        *ifp >> quote;
        अचर string chkCmdline = V3Os::getline(*ifp, '"');
        अचर string cmdline = stripQuotes(cmdlineIn);
        यदि (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            फर्काऊ असत्य;
        }
    }

    अचर तार्किक skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    जबसम्म (!ifp->eof()) {
        वर्ण chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        यदि (ifp->eof()) रोक;  // Needed to read final whitespace before found eof
        समय_प्रकार chkCstime;
        *ifp >> chkCstime;
        समय_प्रकार chkCnstime;
        *ifp >> chkCnstime;
        समय_प्रकार chkMstime;
        *ifp >> chkMstime;
        समय_प्रकार chkMnstime;
        *ifp >> chkMnstime;
        वर्ण quote;
        *ifp >> quote;
        अचर string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        अचर string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        संरचना stat curStat;
        अचर पूर्णाङ्क err = stat(chkFilename.c_str(), &curStat);
        यदि (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            फर्काऊ असत्य;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        यदि (filename == chkFilename) जारी;

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
        यदि (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            यदि (skipHashing) फर्काऊ असत्य;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            यदि (chkHash == "unhashed") फर्काऊ असत्य;

            VHashSha512 curHash;
            curHash.घुसाउ_File(chkFilename);
            यदि (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                फर्काऊ असत्य;
            }
        }
    }
    फर्काऊ सत्य;
}

//######################################################################
// V3File

शून्य V3File::addSrcDepend(अचर string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
शून्य V3File::addTgtDepend(अचर string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
शून्य V3File::writeDepend(अचर string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { फर्काऊ dependImp.getAllDeps(); }
शून्य V3File::writeTimes(अचर string& filename, अचर string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
तार्किक V3File::checkTimes(अचर string& filename, अचर string& cmdlineIn) {
    फर्काऊ dependImp.checkTimes(filename, cmdlineIn);
}
शून्य V3File::createMakeDirFor(अचर string& filename) {
    यदि (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
शून्य V3File::createMakeDir() {
    स्थिर तार्किक s_created = असत्य;
    यदि (!s_created) {
        s_created = सत्य;
        V3Os::createDir(v3Global.opt.makeDir());
        यदि (v3Global.opt.hier_शीर्ष()) V3Os::createDir(v3Global.opt.hier_शीर्ष_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

वर्ग VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<अचर std::string, std::string> m_contentsMap;  // Cache of file contents
    तार्किक m_readEof = असत्य;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#अन्यथा
    पूर्णाङ्क m_pid = 0;  // fork() process id - always zero as disabled
#endif
    तार्किक m_pidExited = असत्य;  //  waitpid() reaped the filter process
    पूर्णाङ्क m_pidStatus = 0;  // Exit status of the filter process
    पूर्णाङ्क m_writeFd = 0;  // File descriptor TO filter
    पूर्णाङ्क m_readFd = 0;  // File descriptor FROM filter

गुप्त:
    // METHODS

    तार्किक readContents(अचर string& filename, StrList& outl) {
        यदि (m_pid) {
            फर्काऊ readContentsFilter(filename, outl);
        } अन्यथा {
            फर्काऊ readContentsFile(filename, outl);
        }
    }
    तार्किक readContentsFile(अचर string& filename, StrList& outl) {
        अचर पूर्णाङ्क fd = खोलो(filename.c_str(), O_RDONLY);
        यदि (fd < 0) फर्काऊ असत्य;
        m_readEof = असत्य;
        readBlocks(fd, -1, outl);
        बंद(fd);
        फर्काऊ सत्य;
    }
    // cppcheck-suppress constParameterReference
    तार्किक readContentsFilter(अचर string& filename, StrList& outl) {
        (शून्य)filename;  // Prevent unused variable warning
        (शून्य)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        अचर string पङ्क्ति = read_Filter_पङ्क्ति();
        यदि (पङ्क्ति.find("Content-Length") != string::npos) {
            पूर्णाङ्क len = 0;
            माला_पूछो(पङ्क्ति.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            फर्काऊ सत्य;
        } अन्यथा {
            यदि (पङ्क्ति != "") v3error("--pipe-filter protocol error, unexpected: " << पङ्क्ति);
            फर्काऊ असत्य;
        }
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
        फर्काऊ असत्य;
#endif
    }

    // cppcheck-suppress functionConst
    शून्य checkFilter(तार्किक hang) {
#ifdef INFILTER_PIPE
        यदि (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::म_त्रुटि(त्रुटि_सं));
            m_readEof = सत्य;
            m_pidExited = सत्य;
        }
#endif
    }

    शून्य readBlocks(पूर्णाङ्क fd, पूर्णाङ्क size, StrList& outl) {
        वर्ण buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        जबसम्म (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            यदि (size > 0 && size < todo) todo = size;
            त्रुटि_सं = 0;
            अचर ssize_t got = पठ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            यदि (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } अन्यथा यदि (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#ifdef EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#endif
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                जारी;
            } अन्यथा {
                m_readEof = सत्य;
                रोक;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_पङ्क्ति() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string पङ्क्ति;
        जबसम्म (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            अचर string onechar = listString(outl);
            पङ्क्ति += onechar;
            यदि (onechar == "\n") {
                यदि (पङ्क्ति == "\n") {
                    पङ्क्ति = "";
                    जारी;
                } अन्यथा {
                    रोक;
                }
            }
        }
        UINFO(6, "filter-line-in: " << पङ्क्ति);
        फर्काऊ पङ्क्ति;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    शून्य writeFilter(अचर string& out) {
        यदि (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            यदि (out[out.length() - 1] != '\n') cout << '\n';
        }
        यदि (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = सत्य;
            stop();
        }
        अचिन्हित offset = 0;
        जबसम्म (!m_readEof && out.length() > offset) {
            त्रुटि_सं = 0;
            अचर पूर्णाङ्क got = लिख(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            यदि (got > 0) {
                offset += got;
            } अन्यथा यदि (त्रुटि_सं == EINTR || त्रुटि_सं == EAGAIN
#ifdef EWOULDBLOCK
                       || त्रुटि_सं == EWOULDBLOCK
#endif
            ) {
                checkFilter(असत्य);
                V3Os::u_sleep(1000);
                जारी;
            } अन्यथा {
                रोक;
            }
        }
    }

    // Start the filter
    शून्य start(अचर string& command) {
        यदि (command == "") {
            m_pid = 0;  // Disabled
        } अन्यथा {
            startFilter(command);
        }
    }
    शून्य startFilter(अचर string& command) {
        (शून्य)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        पूर्णाङ्क fd_stdin[2];  // Can't use std::array
        पूर्णाङ्क fd_stdout[2];  // Can't use std::array
        constexpr पूर्णाङ्क P_RD = 0;
        constexpr पूर्णाङ्क P_WR = 1;

        यदि (VL_UNCOVERABLE(प्रणाल(fd_stdin) != 0 || प्रणाल(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        यदि (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        अचर pid_t pid = विभाजन();
        यदि (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::म_त्रुटि(त्रुटि_सं));  // LCOV_EXCL_LINE
        यदि (pid == 0) {  // Child
            UINFO(6, "In child");
            बंद(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            बंद(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<वर्ण*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            ख_लिखो(मानक_त्रुटि, "--pipe-filter: exec failed: %s\n", std::म_त्रुटि(त्रुटि_सं));
            _exit(1);
        } अन्यथा {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = असत्य;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = असत्य;

            बंद(fd_stdin[P_RD]);
            बंद(fd_stdout[P_WR]);

            पूर्णाङ्क flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    शून्य stop() {
        यदि (m_pid) stopFilter();
    }
    शून्य stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        बंद(m_writeFd);
        checkFilter(सत्य);
        यदि (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        बंद(m_readFd);
        UINFO(6, "Closed");
#अन्यथा
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

रक्षित:
    मित्र वर्ग VInFilter;
    // Read file contents and return it
    तार्किक readWholefile(अचर string& filename, StrList& outl) {
        अचर स्वतः it = m_contentsMap.find(filename);
        यदि (it != m_contentsMap.end()) {
            outl.थप_back(it->second);
            फर्काऊ सत्य;
        }
        यदि (!readContents(filename, outl)) फर्काऊ असत्य;
        यदि (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        फर्काऊ सत्य;
    }
    स्थिर माप_प्रकार listSize(अचर StrList& sl) {
        माप_प्रकार result = 0;
        लागि (अचर string& i : sl) result += i.length();
        फर्काऊ result;
    }
    स्थिर string listString(अचर StrList& sl) {
        string result;
        लागि (अचर string& i : sl) result += i;
        फर्काऊ result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(अचर string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(अचर string& command) { m_impp = नूतन VInFilterImp{command}; }
VInFilter::~VInFilter() {
    यदि (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

तार्किक VInFilter::readWholefile(अचर string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    फर्काऊ m_impp->readWholefile(filename, outl);
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
स्थिर अचर std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    लागि (पूर्णाङ्क i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<माप_प्रकार>(i), ' ');
    फर्काऊ table;
}();

अचर std::string& V3OutFormatter::indentSpaces(पूर्णाङ्क अङ्क) {
    // Indent the specified number of spaces.
    फर्काऊ s_indentSpaces[std::max(0, std::min(अङ्क, MAXSPACE))];
}

तार्किक V3OutFormatter::tokenMatch(अचर वर्ण* cp, अचर वर्ण* cmp) {
    जबसम्म (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    यदि (*cmp) फर्काऊ असत्य;
    यदि (*cp && !std::है_खाली(*cp)) फर्काऊ असत्य;
    फर्काऊ सत्य;
}

तार्किक V3OutFormatter::tokenStart(अचर वर्ण* cp) {
    फर्काऊ (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

तार्किक V3OutFormatter::tokenEnd(अचर वर्ण* cp) {
    फर्काऊ (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

तार्किक V3OutFormatter::tokenNotStart(अचर वर्ण* cp) {
    फर्काऊ (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

पूर्णाङ्क V3OutFormatter::endLevels(अचर वर्ण* strg) {
    पूर्णाङ्क levels = m_indentLevel;
    {
        अचर वर्ण* cp = strg;
        जबसम्म (std::है_खाली(*cp)) ++cp;
        विकल्प (*cp) {
        पक्ष '\n':  // Newlines.. No need for whitespace before it
            फर्काऊ 0;
        पक्ष '#':  // Preproc directive
            यदि (m_lang == LA_C) फर्काऊ 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            अचर वर्ण* mp = cp;
            लागि (; std::है_अक्षर_अंक(*mp); ++mp) {}
            यदि (mp[0] == ':' && mp[1] != ':') फर्काऊ (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    लागि (अचर वर्ण* cp = strg; *cp; ++cp) {
        विकल्प (*cp) {
        पक्ष '}':
        पक्ष ')': levels -= m_blockIndent; रोक;
        पक्ष '<':
            यदि (m_lang == LA_XML) {
                यदि (cp[1] == '/') levels -= m_blockIndent;
            }
            रोक;
        पक्ष 'e':
            यदि (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            रोक;
        पक्ष '\t':
        पक्ष ' ': रोक;  // Continue
        स्वभाव: फर्काऊ levels;  // Letter
        }
    }
    फर्काऊ levels;
}

शून्य V3OutFormatter::putns(अचर Ast_गाँठो* nodep, अचर वर्ण* strg) {
    यदि (!v3Global.opt.decoration()) {
        putsOutput(strg);
        फर्काऊ;
    }

    अचर तार्किक put_गाँठो_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_पङ्क्ति::builtInFilename() != nodep->fileline()->filename();

    यदि (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_गाँठो_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = असत्य;
    }

    यदि (put_गाँठो_Decoration) {
        अचर File_पङ्क्ति* अचर flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        अचर std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    तार्किक notstart = असत्य;
    तार्किक wordstart = सत्य;
    तार्किक equalsForBracket = असत्य;  // Looking for "= {"
    लागि (अचर वर्ण* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        यदि (std::है_अक्षर(*cp)) {
            यदि (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = सत्य;
            यदि (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            यदि (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        अचर तार्किक wasInBackslash = m_inBackslash;
        m_inBackslash = असत्य;
        विकल्प (*cp) {
        पक्ष '\n':
            ++m_lineno;
            wordstart = सत्य;
            यदि (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = सत्य;
            } अन्यथा {
                m_prependIndent = असत्य;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            रोक;
        पक्ष ' ': wordstart = सत्य; रोक;
        पक्ष '\t': wordstart = सत्य; रोक;
        पक्ष '"':
            wordstart = असत्य;
            यदि (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            रोक;
        पक्ष '\\':
            wordstart = असत्य;
            यदि (!wasInBackslash) m_inBackslash = सत्य;
            रोक;
        पक्ष '/':
            यदि (m_lang == LA_C || m_lang == LA_VERILOG) {
                यदि (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    जबसम्म (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    यदि (*cp) putcNoTracking(*cp);
                }
            }
            रोक;
        पक्ष '{':
            यदि (!m_inStringLiteral) {
                यदि (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.थप(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            रोक;
        पक्ष '}':
            यदि (!m_inStringLiteral) {
                यदि (m_bracketLevel > 0) {
                    m_parenVec.झिक();
                    --m_bracketLevel;
                }
                indentDec();
            }
            रोक;
        पक्ष '(':
            यदि (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.थप(m_column);
            }
            रोक;
        पक्ष ')':
            यदि (!m_inStringLiteral) {
                यदि (!m_parenVec.empty()) m_parenVec.झिक();
                indentDec();
            }
            रोक;
        पक्ष '<':
            यदि (m_lang == LA_XML) {
                यदि (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } अन्यथा यदि (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } अन्यथा {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            रोक;
        पक्ष '>':
            यदि (m_lang == LA_XML) {
                indentDec();
                यदि (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            रोक;
        स्वभाव: wordstart = असत्य; रोक;
        }

        विकल्प (*cp) {
        पक्ष '=': equalsForBracket = सत्य; रोक;
        पक्ष ' ': रोक;
        स्वभाव: equalsForBracket = असत्य; रोक;
        }
    }
}

शून्य V3OutFormatter::put_Break_अभिव्यक्ति() {
    यदि (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
शून्य V3OutFormatter::putBreak() {
    यदि (!v3Global.opt.decoration()) फर्काऊ;
    यदि (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        यदि (exceededWidth()) {
            putcNoTracking('\n');
            यदि (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.शीर्ष()));
        }
    }
}

शून्य V3OutFormatter::putsQuoted(अचर string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    अचर string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    लागि (अचर वर्ण c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    यदि (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
शून्य V3OutFormatter::putsNoTracking(अचर string& strg) {
    यदि (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        फर्काऊ;
    }
    // Don't track {}'s, probably because it's a $display format string
    लागि (अचर वर्ण c : strg) putcNoTracking(c);
}

शून्य V3OutFormatter::putcNoTracking(वर्ण chr) {
    यदि (!v3Global.opt.decoration()) {
        putcOutput(chr);
        फर्काऊ;
    }
    विकल्प (chr) {
    पक्ष '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = सत्य;
        रोक;
    पक्ष '\t': m_column = ((m_column + 9) / 8) * 8; रोक;
    पक्ष ' ':
    पक्ष '(':
    पक्ष '|':
    पक्ष '&': ++m_column; रोक;
    स्वभाव:
        ++m_column;
        m_nobreak = असत्य;
        रोक;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(अचर string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    यदि (lang == LA_XML) {
        // Encode chars into XML string
        लागि (अचर वर्ण c : namein) {
            यदि (c == '"') {
                out += "&quot;"s;
            } अन्यथा यदि (c == '\'') {
                out += "&apos;"s;
            } अन्यथा यदि (c == '<') {
                out += "&lt;"s;
            } अन्यथा यदि (c == '>') {
                out += "&gt;"s;
            } अन्यथा यदि (c == '&') {
                out += "&amp;"s;
            } अन्यथा यदि (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                out += "&#"s + cvtToStr(static_cast<अचिन्हित पूर्णाङ्क>(c & 0xff)) + ";";
            }
        }
    } अन्यथा {
        // Encode control chars into C style escapes
        लागि (अचर वर्ण c : namein) {
            यदि (c == '\\' || c == '"') {
                out += "\\"s + c;
            } अन्यथा यदि (c == '\n') {
                out += "\\n";
            } अन्यथा यदि (c == '\r') {
                out += "\\r";
            } अन्यथा यदि (c == '\t') {
                out += "\\t";
            } अन्यथा यदि (std::है_छाप(c)) {
                out += c;
            } अन्यथा {
                // This will also cover \a etc
                अचर string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    फर्काऊ out;
}

//----------------------------------------------------------------------
// Simple wrappers

शून्य V3OutFormatter::म_लिखो(अचर वर्ण* fmt...) {
    constexpr माप_प्रकार bufsize = 5000;
    वर्ण sbuff[bufsize];
    बहु_सूची ap;
    बहु_शुरू(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    बहु_पूर्ण(ap);
    एतत्->माला_दो(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(अचर string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{नूतन std::array<वर्ण, WRITE_BUFFER_SIZE_BYTES>{}} {
    यदि ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    यदि (m_fp) ख_बंद(m_fp);
    m_fp = nullptr;
}

शून्य V3OutFile::putsForceIncs() {
    अचर VStringList& forceIncs = v3Global.opt.forceIncs();
    लागि (अचर string& i : forceIncs) माला_दो("#include \"" + i + "\"\n");
}

शून्य V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = सत्य;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    लागि (वर्ण& c : var) {
        यदि (!std::है_अक्षर_अंक(c)) c = '_';
    }
    माला_दो("\n#ifndef " + var + "\n");
    माला_दो("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

वर्ग VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<अचर std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
रक्षित:
    // CONSTRUCTORS
    मित्र वर्ग VIdProtect;
    स्थिर VIdProtectImp& singleton() VL_MT_SAFE {
        स्थिर VIdProtectImp s;
        फर्काऊ s;
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
    string passthru(अचर string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        यदि (!v3Global.opt.protectIds()) फर्काऊ old;
        अचर V3LockGuard lock{m_mutex};
        अचर स्वतः it = m_nameMap.find(old);
        यदि (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } अन्यथा {
            m_nameMap.emplace(old, old);
            m_newIdSet.घुसाउ(old);
        }
        फर्काऊ old;
    }
    string protectIf(अचर string& old, तार्किक doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        यदि (!v3Global.opt.protectIds() || old.empty() || !doIt) फर्काऊ old;
        अचर V3LockGuard lock{m_mutex};
        अचर स्वतः pair = m_nameMap.emplace(old, "");
        यदि (pair.second) {
            string out;
            यदि (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } अन्यथा {
                VHashSha512 digest{v3Global.opt.protect_साँचो_Defaulted()};
                digest.घुसाउ(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                लागि (माप_प्रकार len = 6; len < out.size() - 3; len += 3) {
                    अचर string tryout = out.substr(0, len);
                    यदि (m_newIdSet.घुसाउ(tryout).second) {
                        out = tryout;
                        रोक;
                    }
                }
            }
            pair.first->second = out;
        }
        फर्काऊ pair.first->second;
    }
    string protect_शब्दहरू_If(अचर string& old, तार्किक doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        यदि (!(doIt && v3Global.opt.protectIds())) फर्काऊ old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        जबसम्म (सत्य) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            यदि (pos == string::npos) रोक;
            out += protectIf(old.substr(start, pos - start), सत्य) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), सत्य);
        फर्काऊ out;
    }
    शून्य writeMapFile(अचर string& filename) अचर {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.माला_दो("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.माला_दो("<verilator_id_map>\n");
        {
            लागि (अचर स्वतः& itr : m_nameMap) {
                of.माला_दो("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.माला_दो("</verilator_id_map>\n");
    }

गुप्त:
    शून्य trySep(अचर string& old, string::size_type start, अचर string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        अचर string::size_type trypos = old.find(trySep, start);
        यदि (trypos != string::npos) {
            यदि (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(अचर string& in) VL_MT_SAFE {
    फर्काऊ v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(अचर string& old, तार्किक doIt) VL_MT_SAFE {
    फर्काऊ VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_शब्दहरू_If(अचर string& old, तार्किक doIt) VL_MT_SAFE {
    फर्काऊ VIdProtectImp::singleton().protect_शब्दहरू_If(old, doIt);
}
शून्य VIdProtect::writeMapFile(अचर string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
