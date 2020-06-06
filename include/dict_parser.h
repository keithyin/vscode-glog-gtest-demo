
#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "field.h"
#include "logging.h"
namespace dict_parser {
class DictParser {
   public:
    DictParser(
        const std::string& filename,
        std::function<std::shared_ptr<dict_field::Record>()> record_builder_func =
            []() { return std::make_shared<dict_field::Record>("record"); },
        const std::string& header_filename = "")
        : filename_(filename), header_filename_(header_filename), record_builder_func_(record_builder_func) {
        if (header_filename_ != "") {
            parse_header_file(field_names_);
            record_builder_func_ = std::bind(&dict_field::FieldManager::record_builder,
                                             dict_field::FieldManager::instance(), std::ref(field_names_));
        }
    }

    bool parse_file() {
        this->clear();

        std::ifstream ifile(filename_, std::ios::in);
        if (!ifile) {
            LOG(ERROR) << "open file:" << filename_ << " error";
            return false;
        }
        std::string line;

        while (getline(ifile, line)) {
            num_line_++;
            std::shared_ptr<dict_field::Record> record = record_builder_func_();
            bool is_succ = record->deserilization(line);
            if (is_succ) {
                parsed_result_.push_back(record);
                num_succ_parsed_line_++;
            } else {
                LOG(ERROR) << "parse " << line << " error";
            }
        }
        return true;
    }

    void clear() {
        parsed_result_.clear();
        num_line_ = 0;
        num_succ_parsed_line_ = 0;
    }

    uint64_t num_line() { return num_line_; }
    uint64_t num_succ_parsed_line() { return num_succ_parsed_line_; }

    const std::vector<std::shared_ptr<dict_field::Record>>& parsed_result() { return parsed_result_; }

   private:
    bool parse_header_file(std::vector<std::string>& field_names) {
        std::ifstream ifile(header_filename_, std::ios::in);
        if (!ifile) {
            LOG(ERROR) << "open file:" << header_filename_ << " error";
            return false;
        }
        std::string line;
        while (getline(ifile, line)) {
            dict_field::string_splitter(line, "\t", field_names);
            break;
        }
        return true;
    }

    std::string filename_;
    std::string header_filename_;
    std::vector<std::string> field_names_;
    std::function<std::shared_ptr<dict_field::Record>()> record_builder_func_;
    std::vector<std::shared_ptr<dict_field::Record>> parsed_result_;
    uint64_t num_line_;
    uint64_t num_succ_parsed_line_;
};
}  // namespace dict_parser