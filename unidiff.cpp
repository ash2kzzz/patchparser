#include "unidiff.h"
#include <iostream>

using namespace diff;

// class Line
bool Line::is_added() {return this->line_type == LINE_TYPE_ADDED;}
bool Line::is_removed() {return this->line_type == LINE_TYPE_REMOVED;}
bool Line::is_content() {return this->line_type == LINE_TYPE_CONTENT;}

Line::Line(std::string value, std::string line_type, int source_line_no,
        int target_line_no, int diff_line_no)
{
    this->value = value;
    this->line_type = line_type;
    this->source_line_no = source_line_no;
    this->target_line_no = target_line_no;
    this->diff_line_no =diff_line_no;
}

Line::~Line() {/*do nothing*/}

// class PatchInfo
PatchInfo::PatchInfo()
{
    this->info = new std::vector<std::string>;
}

PatchInfo::~PatchInfo()
{
    delete this->info;
}

std::string PatchInfo::print()
{
    std::string information("");
    for (auto it = this->info->begin(); it != this->info->end(); it++)
        information += *it;
    return information;
}

// class Hunk
Hunk::Hunk(int source_start, int source_length, int target_start,
            int target_length, std::string section_header)
{
    this->lines = &(std::vector<Line*>());
    this->source_start = source_start;
    this->target_start = target_start;
    this->source_length = source_length;
    this->target_length = target_length;
    this->section_header = section_header;
}

Hunk::~Hunk()
{
    this->lines = nullptr;
}

// class PatchedFile
PatchedFile::PatchedFile(std::string source_file, std::string target_file,
                        std::string source_timestamp, std::string target_timestamp,
                        PatchInfo *patch_info)
{
    this->hunks = &(std::vector<Hunk*>());
    this->source_file = source_file;
    this->target_file = target_file;
    this->source_timestamp = source_timestamp;
    this->target_timestamp = target_timestamp;
    this->patch_info = patch_info;
}

PatchedFile::~PatchedFile()
{
    this->hunks = nullptr;
}

void PatchedFile::parse_hunk(std::string line, std::ifstream& patch)
{
    std::smatch result;
    std::string line;
    std::regex_match(line, result, RE_HUNK_HEADER);
    Hunk hunk = Hunk(std::stoi(std::string(result[1])), std::stoi(std::string(result[2])),
                    std::stoi(std::string(result[3])), std::stoi(std::string(result[4])),
                    std::string(result[5]));
    int source_line_no = hunk.source_start;
    int target_line_no = hunk.target_start;
    int expected_source_end = source_line_no + hunk.source_length;
    int expected_target_end = target_line_no + hunk.target_length;
    int added = 0;
    int reomved = 0;
    while (patch.good() && std::getline(patch, line))
    {
        if (!std::regex_match(line, result, RE_HUNK_BODY_LINE))
            if (!std::regex_match(line, result, RE_HUNK_EMPTY_BODY_LINE))
            {
                std::cout << "Hunk diff line expected: " << line << std::endl;
                return;
            }
        std::string line_type = std::string(result[1]);
        if (line_type == LINE_TYPE_EMPTY)
            line_type = LINE_TYPE_CONTENT;
        std::string value = std::string(result[2]);
        Line *orginal_line = &Line(value, line_type);
        if (line_type == LINE_TYPE_ADDED)
        {
            orginal_line->target_line_no = target_line_no;
            target_line_no++;
        } else if (line_type == LINE_TYPE_REMOVED)
        {
            orginal_line->source_line_no = source_line_no;
            source_line_no++;
        } else if (line_type == LINE_TYPE_CONTENT)
        {
            orginal_line->source_line_no = source_line_no;
            orginal_line->target_line_no = target_line_no;
            source_line_no++;
            target_line_no++;
        } else if (line_type == LINE_TYPE_NO_NEWLINE);
        else orginal_line = nullptr;

        if (source_line_no > expected_source_end || target_line_no > expected_target_end)
        {
            std::cout << "Hunk is longer than expected" << std::endl;
            return;
        }
        if (orginal_line)
        {
            // orginal_line->diff_line_no = diff_line_no
            hunk.lines->push_back(orginal_line);
        }
        if (source_line_no == expected_source_end && target_line_no == expected_target_end)
            break;
    }
    if (source_line_no < expected_source_end || target_line_no < expected_target_end)
    {
        std::cout << "Hunk is shorter than expected" << std::endl;
            return;
    }
    this->hunks->push_back(&hunk);
}

void PatchedFile::add_no_newline_marker_to_last_hunk()
{
    if (this->hunks->size() == 0)
    {
        std::cout << "Unexpected marker:" << LINE_VALUE_NO_NEWLINE << std::endl;
        return;
    }
    Hunk *last_hunk = (*this->hunks)[hunks->size()-1];
    last_hunk->lines->push_back(&Line(LINE_VALUE_NO_NEWLINE + "\n", LINE_TYPE_NO_NEWLINE));
}

void PatchedFile::append_trailing_empty_line()
{
    if (hunks->size() == 0)
    {
        std::cout << "Unexpected trailing newline character" << std::endl;
        return;
    }
    Hunk *last_hunk = (*this->hunks)[hunks->size()-1];
    last_hunk->lines->push_back(&Line("\n", LINE_TYPE_EMPTY));
}

// class PatchSet
PatchSet::PatchSet()
{
    this->patched_files = &(std::vector<PatchedFile*>());
}
PatchSet::~PatchSet()
{
    this->patched_files  = nullptr;
}

void PatchSet::parser_patch_file(std::string path)
{
    std::ifstream patch;
    std::string line;
    std::smatch result;
    PatchedFile *current_file = nullptr;
    PatchInfo *patch_info = nullptr;
    patch.open(path, std::ios::in);
    if (!patch.is_open())
    {
        std::cout << "Unexpected cannot open patch file: " << path << std::endl;
        return;
    }
    while (patch.good() && std::getline(patch, line))
    {
        // check for a git file rename
        if (std::regex_match(line, result, RE_DIFF_GIT_HEADER)
        || std::regex_match(line, result, RE_DIFF_GIT_HEADER_URI_LIKE)
        || std::regex_match(line, result, RE_DIFF_GIT_HEADER_NO_PREFIX))
        {
            patch_info = &PatchInfo();
            std::string source_file = std::string(result[1]); // <source>
            std::string target_file = std::string(result[2]); // <target>
            *current_file = PatchedFile(source_file, target_file, "", "", patch_info);
            this->patched_files->push_back(current_file);
            patch_info->info->push_back(line);
            continue;
        }
        // check for a git new file
        if (std::regex_match(line, result, RE_DIFF_GIT_NEW_FILE))
        {
            if (current_file == nullptr || patch_info == nullptr)
            {
                std::cout << "Unexpected new file found: " << line << std::endl;
                return;
            }
            current_file->source_file = DEV_NULL;
            patch_info->info->push_back(line);
            continue;
        }
        // check for a git deleted file
        if (std::regex_match(line, result, RE_DIFF_GIT_DELETED_FILE))
        {
            if (current_file == nullptr || patch_info == nullptr)
            {
                std::cout << "Unexpected deleted file found: " << line << std::endl;
                return;
            }
            current_file->target_file = DEV_NULL;
            patch_info->info->push_back(line);
            continue;
        }
        // check for source file header
        if (std::regex_match(line, result, RE_SOURCE_FILENAME))
        {
            std::string source_file = std::string(result[1]); // source <filename>
            std::string source_timestamp = std::string(result[2]); // source <timestamp>
            if (current_file != nullptr && (current_file->source_file != source_file))
                current_file = nullptr;
            else if (current_file != nullptr && (current_file->source_file == source_file))
                current_file->source_timestamp = source_timestamp;
            continue;
        }
        // check for target file header
        if (std::regex_match(line, result, RE_TARGET_FILENAME))
        {
            std::string target_file = std::string(result[1]); // target <filename>
            std::string target_timestamp = std::string(result[2]); // target <timestamp>
            if (current_file != nullptr && (current_file->target_file != target_file))
            {
                std::cout << "Target without source: " << line << std::endl;
                return;
            }
            if (current_file == nullptr)
            {
                *current_file = PatchedFile("", target_file, "", target_timestamp, patch_info);
                this->patched_files->push_back(current_file);
                patch_info = nullptr;
            } else
                current_file->target_timestamp = target_timestamp;
            continue;
        }
        // check for hunk header
        if (std::regex_match(line, result, RE_HUNK_HEADER))
        {
            patch_info = nullptr;
            if (current_file == nullptr)
            {
                std::cout << "Unexpected hunk found: " << line << std::endl;
                return;
            }
            current_file->parse_hunk(line, patch);
            continue;
        }
        // check for no newline marker
        if (std::regex_match(line, result, RE_NO_NEWLINE_MARKER))
        {
            if (current_file == nullptr)
            {
                std::cout << "Unexpected marker: " << line << std::endl;
                return;
            }
            current_file->add_no_newline_marker_to_last_hunk();
            continue;
        }
        // sometimes hunks can be followed by empty lines
        if (line == "\n" && current_file != nullptr)
        {
            current_file->append_trailing_empty_line();
            continue;
        }
        // if nothing has matched above then this line is a patch info
        if (patch_info == nullptr)
        {
            current_file = nullptr;
            patch_info = &PatchInfo();
        }
        patch_info->info->push_back(line);
    }
}