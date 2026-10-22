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

#அந்தர்பாவ "V3PchAstMT.h"

#அந்தர்பாவ "V3File.h"

#அந்தர்பாவ "V3Os.h"
#அந்தர்பாவ "V3String.h"

#அந்தர்பாவ <cerrno>
#அந்தர்பாவ <cstdarg>
#அந்தர்பாவ <fcntl.h>
#அந்தர்பாவ <iomanip>
#அந்தர்பாவ <map>
#அந்தர்பாவ <memory>

#அந்தர்பாவ <sys/stat.h>
#அந்தர்பாவ <sys/types.h>

// clang-format off
#எனில் defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#இல்லையெனில்
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#எனில் defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr முழு INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr முழு INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

வகுப்பு V3FileDependImp final {
    // TYPES
    வகுப்பு DependFile final {
        // A single file
        மாறிலி தருக்க m_target;  // True if write, else read
        தருக்க m_exists = உண்மை;  // File exists
        மாறிலி string m_filename;  // Filename
        கட்டமைப்பு stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    பொது:
        DependFile(மாறிலி string& filename, தருக்க target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = இயல்பு;
        மாறிலி string& filename() மாறிலி { திரும்பு m_filename; }
        தருக்க target() மாறிலி { திரும்பு m_target; }
        தருக்க exists() மாறிலி { திரும்பு m_exists; }
        off_t size() மாறிலி { திரும்பு m_stat.st_size; }
        ino_t ino() மாறிலி { திரும்பு m_stat.st_ino; }
        time_t cstime() மாறிலி { திரும்பு m_stat.st_ctime; }  // Seconds
        time_t cnstime() மாறிலி { திரும்பு VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() மாறிலி { திரும்பு m_stat.st_mtime; }  // Seconds
        time_t mnstime() மாறிலி { திரும்பு VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_குறியீடு() {
            நிலையான VHashSha512 s_emptyHash;
            திரும்பு m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        வெறுமை loadStats() {
            எனில் (!m_stat.st_mtime) {
                மாறிலி string fn = filename();
                மாறிலி முழு err = stat(fn.c_str(), &m_stat);
                எனில் (err != 0) {
                    std::memset(&m_stat, 0, அளவு(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = பொய்;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } இல்லையெனில் {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    எனில் (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.செருகு_File(filename());
                }
            }
        }
        தருக்க operator<(மாறிலி DependFile& rhs) மாறிலி { திரும்பு filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    நிலையான string stripQuotes(மாறிலி string& in) {
        string pretty = in;
        string::size_type pos;
        போது ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        போது ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        திரும்பு pretty;
    }

பொது:
    // ACCESSOR METHODS
    வெறுமை addSrcDepend(மாறிலி string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        மாறிலி V3LockGuard lock{m_mutex};
        மாறிலி auto itFoundPair = m_filenameSet.செருகு(filename);
        எனில் (itFoundPair.second) {
            DependFile df{filename, பொய்};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.செருகு(df);
        }
    }
    வெறுமை addTgtDepend(மாறிலி string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        மாறிலி V3LockGuard lock{m_mutex};
        மாறிலி auto itFoundPair = m_filenameSet.செருகு(filename);
        எனில் (itFoundPair.second) m_filenameList.செருகு(DependFile{filename, உண்மை});
    }
    வெறுமை writeDepend(மாறிலி string& filename);
    std::vector<string> getAllDeps() மாறிலி;
    வெறுமை writeTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn);
    தருக்க checkTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

வெறுமை V3FileDependImp::writeDepend(மாறிலி string& filename) {
    மாறிலி std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    எனில் (ofp->fail()) v3fatal("Can't write file: " << filename);

    க்கு (மாறிலி DependFile& i : m_filenameList) {
        எனில் (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    க்கு (மாறிலி DependFile& i : m_filenameList) {
        எனில் (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    எனில் (v3Global.opt.makePhony()) {
        *ofp << '\n';
        க்கு (மாறிலி DependFile& i : m_filenameList) {
            எனில் (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() மாறிலி {
    std::vector<string> r;
    க்கு (மாறிலி auto& itr : m_filenameList) {
        எனில் (!itr.target() && itr.exists()) r.தள்ளு_back(itr.filename());
    }
    திரும்பு r;
}

வெறுமை V3FileDependImp::writeTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn) {
    மாறிலி std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    எனில் (ofp->fail()) v3fatal("Can't write file: " << filename);

    மாறிலி string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    க்கு (auto& it : m_filenameList) {
        DependFile* மாறிலி depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        எனில் (depfilep->filename() == filename) {
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
        *ofp << " \"" << depfilep->hash_Digest_குறியீடு() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

தருக்க V3FileDependImp::checkTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn) {
    மாறிலி std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    எனில் (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        திரும்பு பொய்;
    }
    {
        மாறிலி string ignore = V3Os::getline(*ifp);
        எனில் (ignore.empty()) { /*used*/
        }
    }
    {
        எழுத்து chkDir;
        *ifp >> chkDir;
        எழுத்து quote;
        *ifp >> quote;
        மாறிலி string chkCmdline = V3Os::getline(*ifp, '"');
        மாறிலி string cmdline = stripQuotes(cmdlineIn);
        எனில் (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            திரும்பு பொய்;
        }
    }

    மாறிலி தருக்க skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    போது (!ifp->eof()) {
        எழுத்து chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        எனில் (ifp->eof()) நிறுத்து;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        எழுத்து quote;
        *ifp >> quote;
        மாறிலி string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        மாறிலி string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        கட்டமைப்பு stat curStat;
        மாறிலி முழு err = stat(chkFilename.c_str(), &curStat);
        எனில் (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            திரும்பு பொய்;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        எனில் (filename == chkFilename) தொடர்;

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
        எனில் (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            எனில் (skipHashing) திரும்பு பொய்;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            எனில் (chkHash == "unhashed") திரும்பு பொய்;

            VHashSha512 curHash;
            curHash.செருகு_File(chkFilename);
            எனில் (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                திரும்பு பொய்;
            }
        }
    }
    திரும்பு உண்மை;
}

//######################################################################
// V3File

வெறுமை V3File::addSrcDepend(மாறிலி string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
வெறுமை V3File::addTgtDepend(மாறிலி string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
வெறுமை V3File::writeDepend(மாறிலி string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { திரும்பு dependImp.getAllDeps(); }
வெறுமை V3File::writeTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
தருக்க V3File::checkTimes(மாறிலி string& filename, மாறிலி string& cmdlineIn) {
    திரும்பு dependImp.checkTimes(filename, cmdlineIn);
}
வெறுமை V3File::createMakeDirFor(மாறிலி string& filename) {
    எனில் (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
வெறுமை V3File::createMakeDir() {
    நிலையான தருக்க s_created = பொய்;
    எனில் (!s_created) {
        s_created = உண்மை;
        V3Os::createDir(v3Global.opt.makeDir());
        எனில் (v3Global.opt.hier_உச்சி()) V3Os::createDir(v3Global.opt.hier_உச்சி_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

வகுப்பு VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<மாறிலி std::string, std::string> m_contentsMap;  // Cache of file contents
    தருக்க m_readEof = பொய்;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#இல்லையெனில்
    முழு m_pid = 0;  // fork() process id - always zero as disabled
#endif
    தருக்க m_pidExited = பொய்;  //  waitpid() reaped the filter process
    முழு m_pidStatus = 0;  // Exit status of the filter process
    முழு m_writeFd = 0;  // File descriptor TO filter
    முழு m_readFd = 0;  // File descriptor FROM filter

தனிப்பட்ட:
    // METHODS

    தருக்க readContents(மாறிலி string& filename, StrList& outl) {
        எனில் (m_pid) {
            திரும்பு readContentsFilter(filename, outl);
        } இல்லையெனில் {
            திரும்பு readContentsFile(filename, outl);
        }
    }
    தருக்க readContentsFile(மாறிலி string& filename, StrList& outl) {
        மாறிலி முழு fd = open(filename.c_str(), O_RDONLY);
        எனில் (fd < 0) திரும்பு பொய்;
        m_readEof = பொய்;
        readBlocks(fd, -1, outl);
        close(fd);
        திரும்பு உண்மை;
    }
    // cppcheck-suppress constParameterReference
    தருக்க readContentsFilter(மாறிலி string& filename, StrList& outl) {
        (வெறுமை)filename;  // Prevent unused variable warning
        (வெறுமை)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        மாறிலி string வரி = read_Filter_வரி();
        எனில் (வரி.find("Content-Length") != string::npos) {
            முழு len = 0;
            sscanf(வரி.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            திரும்பு உண்மை;
        } இல்லையெனில் {
            எனில் (வரி != "") v3error("--pipe-filter protocol error, unexpected: " << வரி);
            திரும்பு பொய்;
        }
#இல்லையெனில்
        v3fatalSrc("--pipe-filter not implemented on this platform");
        திரும்பு பொய்;
#endif
    }

    // cppcheck-suppress functionConst
    வெறுமை checkFilter(தருக்க hang) {
#ifdef INFILTER_PIPE
        எனில் (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = உண்மை;
            m_pidExited = உண்மை;
        }
#endif
    }

    வெறுமை readBlocks(முழு fd, முழு size, StrList& outl) {
        எழுத்து buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        போது (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            எனில் (size > 0 && size < todo) todo = size;
            errno = 0;
            மாறிலி ssize_t got = பட(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            எனில் (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } இல்லையெனில் எனில் (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(பொய்);
                V3Os::u_sleep(1000);
                தொடர்;
            } இல்லையெனில் {
                m_readEof = உண்மை;
                நிறுத்து;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_வரி() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string வரி;
        போது (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            மாறிலி string onechar = listString(outl);
            வரி += onechar;
            எனில் (onechar == "\n") {
                எனில் (வரி == "\n") {
                    வரி = "";
                    தொடர்;
                } இல்லையெனில் {
                    நிறுத்து;
                }
            }
        }
        UINFO(6, "filter-line-in: " << வரி);
        திரும்பு வரி;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    வெறுமை writeFilter(மாறிலி string& out) {
        எனில் (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            எனில் (out[out.length() - 1] != '\n') cout << '\n';
        }
        எனில் (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = உண்மை;
            stop();
        }
        unsigned offset = 0;
        போது (!m_readEof && out.length() > offset) {
            errno = 0;
            மாறிலி முழு got = லிக(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            எனில் (got > 0) {
                offset += got;
            } இல்லையெனில் எனில் (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(பொய்);
                V3Os::u_sleep(1000);
                தொடர்;
            } இல்லையெனில் {
                நிறுத்து;
            }
        }
    }

    // Start the filter
    வெறுமை start(மாறிலி string& command) {
        எனில் (command == "") {
            m_pid = 0;  // Disabled
        } இல்லையெனில் {
            startFilter(command);
        }
    }
    வெறுமை startFilter(மாறிலி string& command) {
        (வெறுமை)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        முழு fd_stdin[2];  // Can't use std::array
        முழு fd_stdout[2];  // Can't use std::array
        constexpr முழு P_RD = 0;
        constexpr முழு P_WR = 1;

        எனில் (VL_UNCOVERABLE(ப்ரணால(fd_stdin) != 0 || ப்ரணால(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        எனில் (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        மாறிலி pid_t pid = fork();
        எனில் (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        எனில் (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<எழுத்து*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } இல்லையெனில் {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = பொய்;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = பொய்;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            முழு flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#இல்லையெனில்
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    வெறுமை stop() {
        எனில் (m_pid) stopFilter();
    }
    வெறுமை stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(உண்மை);
        எனில் (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#இல்லையெனில்
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

பாதுகாக்கப்பட்ட:
    friend வகுப்பு VInFilter;
    // Read file contents and return it
    தருக்க readWholefile(மாறிலி string& filename, StrList& outl) {
        மாறிலி auto it = m_contentsMap.find(filename);
        எனில் (it != m_contentsMap.end()) {
            outl.தள்ளு_back(it->second);
            திரும்பு உண்மை;
        }
        எனில் (!readContents(filename, outl)) திரும்பு பொய்;
        எனில் (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        திரும்பு உண்மை;
    }
    நிலையான size_t listSize(மாறிலி StrList& sl) {
        size_t result = 0;
        க்கு (மாறிலி string& i : sl) result += i.length();
        திரும்பு result;
    }
    நிலையான string listString(மாறிலி StrList& sl) {
        string result;
        க்கு (மாறிலி string& i : sl) result += i;
        திரும்பு result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(மாறிலி string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(மாறிலி string& command) { m_impp = புதிய VInFilterImp{command}; }
VInFilter::~VInFilter() {
    எனில் (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

தருக்க VInFilter::readWholefile(மாறிலி string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    திரும்பு m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

நிலையான constexpr முழு MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
நிலையான மாறிலி std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    க்கு (முழு i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    திரும்பு table;
}();

மாறிலி std::string& V3OutFormatter::indentSpaces(முழு இலக்கம்) {
    // Indent the specified number of spaces.
    திரும்பு s_indentSpaces[std::max(0, std::min(இலக்கம், MAXSPACE))];
}

தருக்க V3OutFormatter::tokenMatch(மாறிலி எழுத்து* cp, மாறிலி எழுத்து* cmp) {
    போது (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    எனில் (*cmp) திரும்பு பொய்;
    எனில் (*cp && !std::isspace(*cp)) திரும்பு பொய்;
    திரும்பு உண்மை;
}

தருக்க V3OutFormatter::tokenStart(மாறிலி எழுத்து* cp) {
    திரும்பு (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

தருக்க V3OutFormatter::tokenEnd(மாறிலி எழுத்து* cp) {
    திரும்பு (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

தருக்க V3OutFormatter::tokenNotStart(மாறிலி எழுத்து* cp) {
    திரும்பு (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

முழு V3OutFormatter::endLevels(மாறிலி எழுத்து* strg) {
    முழு levels = m_indentLevel;
    {
        மாறிலி எழுத்து* cp = strg;
        போது (std::isspace(*cp)) ++cp;
        தேர்வு (*cp) {
        நிலை '\n':  // Newlines.. No need for whitespace before it
            திரும்பு 0;
        நிலை '#':  // Preproc directive
            எனில் (m_lang == LA_C) திரும்பு 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            மாறிலி எழுத்து* mp = cp;
            க்கு (; std::isalnum(*mp); ++mp) {}
            எனில் (mp[0] == ':' && mp[1] != ':') திரும்பு (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    க்கு (மாறிலி எழுத்து* cp = strg; *cp; ++cp) {
        தேர்வு (*cp) {
        நிலை '}':
        நிலை ')': levels -= m_blockIndent; நிறுத்து;
        நிலை '<':
            எனில் (m_lang == LA_XML) {
                எனில் (cp[1] == '/') levels -= m_blockIndent;
            }
            நிறுத்து;
        நிலை 'e':
            எனில் (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            நிறுத்து;
        நிலை '\t':
        நிலை ' ': நிறுத்து;  // Continue
        இயல்பு: திரும்பு levels;  // Letter
        }
    }
    திரும்பு levels;
}

வெறுமை V3OutFormatter::putns(மாறிலி Ast_முனை* nodep, மாறிலி எழுத்து* strg) {
    எனில் (!v3Global.opt.decoration()) {
        putsOutput(strg);
        திரும்பு;
    }

    மாறிலி தருக்க put_முனை_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_வரி::builtInFilename() != nodep->fileline()->filename();

    எனில் (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_முனை_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = பொய்;
    }

    எனில் (put_முனை_Decoration) {
        மாறிலி File_வரி* மாறிலி flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        மாறிலி std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    தருக்க notstart = பொய்;
    தருக்க wordstart = உண்மை;
    தருக்க equalsForBracket = பொய்;  // Looking for "= {"
    க்கு (மாறிலி எழுத்து* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        எனில் (std::isalpha(*cp)) {
            எனில் (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = உண்மை;
            எனில் (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            எனில் (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        மாறிலி தருக்க wasInBackslash = m_inBackslash;
        m_inBackslash = பொய்;
        தேர்வு (*cp) {
        நிலை '\n':
            ++m_lineno;
            wordstart = உண்மை;
            எனில் (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = உண்மை;
            } இல்லையெனில் {
                m_prependIndent = பொய்;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            நிறுத்து;
        நிலை ' ': wordstart = உண்மை; நிறுத்து;
        நிலை '\t': wordstart = உண்மை; நிறுத்து;
        நிலை '"':
            wordstart = பொய்;
            எனில் (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            நிறுத்து;
        நிலை '\\':
            wordstart = பொய்;
            எனில் (!wasInBackslash) m_inBackslash = உண்மை;
            நிறுத்து;
        நிலை '/':
            எனில் (m_lang == LA_C || m_lang == LA_VERILOG) {
                எனில் (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    போது (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    எனில் (*cp) putcNoTracking(*cp);
                }
            }
            நிறுத்து;
        நிலை '{':
            எனில் (!m_inStringLiteral) {
                எனில் (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.தள்ளு(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            நிறுத்து;
        நிலை '}':
            எனில் (!m_inStringLiteral) {
                எனில் (m_bracketLevel > 0) {
                    m_parenVec.எடு();
                    --m_bracketLevel;
                }
                indentDec();
            }
            நிறுத்து;
        நிலை '(':
            எனில் (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.தள்ளு(m_column);
            }
            நிறுத்து;
        நிலை ')':
            எனில் (!m_inStringLiteral) {
                எனில் (!m_parenVec.empty()) m_parenVec.எடு();
                indentDec();
            }
            நிறுத்து;
        நிலை '<':
            எனில் (m_lang == LA_XML) {
                எனில் (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } இல்லையெனில் எனில் (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } இல்லையெனில் {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            நிறுத்து;
        நிலை '>':
            எனில் (m_lang == LA_XML) {
                indentDec();
                எனில் (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            நிறுத்து;
        இயல்பு: wordstart = பொய்; நிறுத்து;
        }

        தேர்வு (*cp) {
        நிலை '=': equalsForBracket = உண்மை; நிறுத்து;
        நிலை ' ': நிறுத்து;
        இயல்பு: equalsForBracket = பொய்; நிறுத்து;
        }
    }
}

வெறுமை V3OutFormatter::put_Break_கோவை() {
    எனில் (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
வெறுமை V3OutFormatter::putBreak() {
    எனில் (!v3Global.opt.decoration()) திரும்பு;
    எனில் (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        எனில் (exceededWidth()) {
            putcNoTracking('\n');
            எனில் (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.உச்சி()));
        }
    }
}

வெறுமை V3OutFormatter::putsQuoted(மாறிலி string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    மாறிலி string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    க்கு (மாறிலி எழுத்து c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    எனில் (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
வெறுமை V3OutFormatter::putsNoTracking(மாறிலி string& strg) {
    எனில் (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        திரும்பு;
    }
    // Don't track {}'s, probably because it's a $display format string
    க்கு (மாறிலி எழுத்து c : strg) putcNoTracking(c);
}

வெறுமை V3OutFormatter::putcNoTracking(எழுத்து chr) {
    எனில் (!v3Global.opt.decoration()) {
        putcOutput(chr);
        திரும்பு;
    }
    தேர்வு (chr) {
    நிலை '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = உண்மை;
        நிறுத்து;
    நிலை '\t': m_column = ((m_column + 9) / 8) * 8; நிறுத்து;
    நிலை ' ':
    நிலை '(':
    நிலை '|':
    நிலை '&': ++m_column; நிறுத்து;
    இயல்பு:
        ++m_column;
        m_nobreak = பொய்;
        நிறுத்து;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(மாறிலி string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    எனில் (lang == LA_XML) {
        // Encode chars into XML string
        க்கு (மாறிலி எழுத்து c : namein) {
            எனில் (c == '"') {
                out += "&quot;"s;
            } இல்லையெனில் எனில் (c == '\'') {
                out += "&apos;"s;
            } இல்லையெனில் எனில் (c == '<') {
                out += "&lt;"s;
            } இல்லையெனில் எனில் (c == '>') {
                out += "&gt;"s;
            } இல்லையெனில் எனில் (c == '&') {
                out += "&amp;"s;
            } இல்லையெனில் எனில் (std::isprint(c)) {
                out += c;
            } இல்லையெனில் {
                out += "&#"s + cvtToStr(static_cast<unsigned முழு>(c & 0xff)) + ";";
            }
        }
    } இல்லையெனில் {
        // Encode control chars into C style escapes
        க்கு (மாறிலி எழுத்து c : namein) {
            எனில் (c == '\\' || c == '"') {
                out += "\\"s + c;
            } இல்லையெனில் எனில் (c == '\n') {
                out += "\\n";
            } இல்லையெனில் எனில் (c == '\r') {
                out += "\\r";
            } இல்லையெனில் எனில் (c == '\t') {
                out += "\\t";
            } இல்லையெனில் எனில் (std::isprint(c)) {
                out += c;
            } இல்லையெனில் {
                // This will also cover \a etc
                மாறிலி string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    திரும்பு out;
}

//----------------------------------------------------------------------
// Simple wrappers

வெறுமை V3OutFormatter::printf(மாறிலி எழுத்து* fmt...) {
    constexpr size_t bufsize = 5000;
    எழுத்து sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    இது->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(மாறிலி string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{புதிய std::array<எழுத்து, WRITE_BUFFER_SIZE_BYTES>{}} {
    எனில் ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    எனில் (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

வெறுமை V3OutFile::putsForceIncs() {
    மாறிலி VStringList& forceIncs = v3Global.opt.forceIncs();
    க்கு (மாறிலி string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

வெறுமை V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = உண்மை;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    க்கு (எழுத்து& c : var) {
        எனில் (!std::isalnum(c)) c = '_';
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

வகுப்பு VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<மாறிலி std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
பாதுகாக்கப்பட்ட:
    // CONSTRUCTORS
    friend வகுப்பு VIdProtect;
    நிலையான VIdProtectImp& singleton() VL_MT_SAFE {
        நிலையான VIdProtectImp s;
        திரும்பு s;
    }

பொது:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = இயல்பு;
    // METHODS
    string passthru(மாறிலி string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        எனில் (!v3Global.opt.protectIds()) திரும்பு old;
        மாறிலி V3LockGuard lock{m_mutex};
        மாறிலி auto it = m_nameMap.find(old);
        எனில் (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } இல்லையெனில் {
            m_nameMap.emplace(old, old);
            m_newIdSet.செருகு(old);
        }
        திரும்பு old;
    }
    string protectIf(மாறிலி string& old, தருக்க doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        எனில் (!v3Global.opt.protectIds() || old.empty() || !doIt) திரும்பு old;
        மாறிலி V3LockGuard lock{m_mutex};
        மாறிலி auto pair = m_nameMap.emplace(old, "");
        எனில் (pair.second) {
            string out;
            எனில் (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } இல்லையெனில் {
                VHashSha512 digest{v3Global.opt.protect_சாவி_Defaulted()};
                digest.செருகு(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                க்கு (size_t len = 6; len < out.size() - 3; len += 3) {
                    மாறிலி string tryout = out.substr(0, len);
                    எனில் (m_newIdSet.செருகு(tryout).second) {
                        out = tryout;
                        நிறுத்து;
                    }
                }
            }
            pair.first->second = out;
        }
        திரும்பு pair.first->second;
    }
    string protect_சொற்கள்_If(மாறிலி string& old, தருக்க doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        எனில் (!(doIt && v3Global.opt.protectIds())) திரும்பு old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        போது (உண்மை) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            எனில் (pos == string::npos) நிறுத்து;
            out += protectIf(old.substr(start, pos - start), உண்மை) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), உண்மை);
        திரும்பு out;
    }
    வெறுமை writeMapFile(மாறிலி string& filename) மாறிலி {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            க்கு (மாறிலி auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

தனிப்பட்ட:
    வெறுமை trySep(மாறிலி string& old, string::size_type start, மாறிலி string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        மாறிலி string::size_type trypos = old.find(trySep, start);
        எனில் (trypos != string::npos) {
            எனில் (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(மாறிலி string& in) VL_MT_SAFE {
    திரும்பு v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(மாறிலி string& old, தருக்க doIt) VL_MT_SAFE {
    திரும்பு VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_சொற்கள்_If(மாறிலி string& old, தருக்க doIt) VL_MT_SAFE {
    திரும்பு VIdProtectImp::singleton().protect_சொற்கள்_If(old, doIt);
}
வெறுமை VIdProtect::writeMapFile(மாறிலி string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
