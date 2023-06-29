#ifndef UNIDIFF_H
#define UNIDIFF_H

#include <regex>
#include <fstream>
#include <string>

namespace diff {
    std::regex RE_SOURCE_FILENAME("^--- (?<filename>[^\t\n]+)(?:\t(?<timestamp>[^\n]+))?");
    std::regex RE_TARGET_FILENAME("^\\+\\+\\+ (?<filename>[^\t\n]+)(?:\t(?<timestamp>[^\n]+))?");

    // check diff git line for git renamed files support
    std::regex RE_DIFF_GIT_HEADER("^diff --git (?<source>a/[^\t\n]+) (?<target>b/[^\t\n]+)");
    std::regex RE_DIFF_GIT_HEADER_URI_LIKE("^diff --git (?<source>.*://[^\t\n]+) (?<target>.*://[^\t\n]+)");
    std::regex RE_DIFF_GIT_HEADER_NO_PREFIX("^diff --git (?<source>[^\t\n]+) (?<target>[^\t\n]+)");

    // check diff git new file marker `deleted file mode 100644`
    std::regex RE_DIFF_GIT_DELETED_FILE("^deleted file mode \\d+$");

    // check diff git new file marker `new file mode 100644`
    std::regex RE_DIFF_GIT_NEW_FILE("^new file mode \\d+$");

    // @@ (source offset, length) (target offset, length) @@ (section header)
    std::regex RE_HUNK_HEADER("^@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@[ ]?(.*)");

    /*
        kept line (context)
     \n empty line (treat like context)
     +  added line
     -  deleted line
     \  No newline case
    */
    std::regex RE_HUNK_BODY_LINE("^(?<line_type>[- \\+\\])(?<value>[.\n]*)");
    std::regex RE_HUNK_EMPTY_BODY_LINE("^(?<line_type>[- \\+\\]?)(?<value>[\r\n]{1,2})");

    std::regex RE_NO_NEWLINE_MARKER("^\\ No newline at end of file");

    std::string DEFAULT_ENCODING("UTF-8");
    std::string DEV_NULL("/dev/null");
    std::string LINE_TYPE_ADDED("+");
    std::string LINE_TYPE_REMOVED("-");
    std::string LINE_TYPE_CONTENT(" ");
    std::string LINE_TYPE_EMPTY("");
    std::string LINE_TYPE_NO_NEWLINE("\\\\");
    std::string LINE_VALUE_NO_NEWLINE(" No newline at end of file");


    class Line {
        public:
            std::string line_type;
            std::string value;
            int source_line_no, target_line_no, diff_line_no; //optional

            Line(std::string value, std::string line_type, int source_line_no = 0,
                int target_line_no = 0, int diff_line_no = 0);
            ~Line();
            bool is_added();
            bool is_removed();
            bool is_content();
    };

    class PatchInfo {
        public:
            std::vector<std::string> *info;

            PatchInfo();
            ~PatchInfo();
            std::string print();
    };

    class Hunk {
        public:
            std::vector<Line*> *lines;
            int source_start, source_length;
            int target_start, target_length;
            std::string section_header;

            Hunk(int source_start, int source_length, int target_start,
                int target_length, std::string section_header = "");
            ~Hunk();
    };

    class PatchedFile {
        public:
            std::vector<Hunk*> *hunks;
            PatchInfo *patch_info;
            std::string source_file, source_timestamp;
            std::string target_file, target_timestamp;

            PatchedFile(std::string source_file, std::string target_file,
                        std::string source_timestamp, std::string target_timestamp,
                        PatchInfo *patch_info = nullptr);
            ~PatchedFile();
            void parse_hunk(std::string line, std::ifstream& patch);
            void add_no_newline_marker_to_last_hunk();
            void append_trailing_empty_line();
    };

    class PatchSet {
        public:
            std::vector<PatchedFile*> *patched_files;

            PatchSet();
            ~PatchSet();
            void parser_patch_file(std::string path);
    };
}

#endif //UNIDIFF_H
