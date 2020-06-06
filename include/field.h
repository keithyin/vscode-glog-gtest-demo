/***************************************************************************
 *
 * Copyright (c) 2020 Baidu.com, Inc. All Rights Reserved
 *
 **************************************************************************/

/**
 * @file field.h
 * @author yinpeng02(yinpeng02@baidu.com)
 * @date 2020-05-31 15:32:48
 * @brief 字典解析 field 头文件
 *
 **/

#pragma once

#include <exception>
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "logging.h"

namespace dict_field {

// parse: 用来解析 string 的 函数模板, Field 的 deserilization 中调用
// 如果想要使用 Field<T> 务必特化 该类型的 parse 方法.
// input:
//      inp: 需要解析的 string
//      data: 解析的结果存储到 data 中
// output:
//      bool: 解析是否成功, 如果不成功则返回 false
template <typename T>
bool parse(const std::string &inp, T &data) {
    return data.parse(inp);
}

template <>
inline bool parse(const std::string &inp, int &data) {
    bool is_succ = true;
    try {
        data = std::stoi(inp);
    } catch (const std::exception &e) {
        data = 0;
        is_succ = false;
        LOG(ERROR) << e.what();
    }
    return is_succ;
}

template <>
inline bool parse(const std::string &inp, uint32_t &data) {
    bool is_succ = true;
    try {
        data = std::stoul(inp);
    } catch (const std::exception &e) {
        data = 0;
        is_succ = false;
        LOG(ERROR) << e.what();
    }
    return is_succ;
}

template <>
inline bool parse(const std::string &inp, uint64_t &data) {
    bool is_succ = true;
    try {
        data = std::stoull(inp);
    } catch (const std::exception &e) {
        data = 0;
        is_succ = false;
        LOG(WARNING) << e.what();
    }
    return is_succ;
}

template <>
inline bool parse(const std::string &inp, float &data) {
    bool is_succ = true;
    try {
        data = std::stof(inp);
    } catch (const std::exception &e) {
        data = 0;
        is_succ = false;
        LOG(WARNING) << e.what();
    }
    return is_succ;
}

template <>
inline bool parse(const std::string &inp, std::string &data) {
    bool is_succ = true;
    data = inp;
    if (data == std::string("")) {
        is_succ = false;
    }
    return is_succ;
}

inline void string_splitter(const std::string &str, const std::string &delim, std::vector<std::string> &oitems) {
    oitems.clear();
    auto first = std::begin(str);
    auto str_end = std::end(str);
    auto delim_begin = std::begin(delim);
    auto delim_end = std::end(delim);
    while (first != str_end) {
        const auto second = std::find_first_of(first, str_end, delim_begin, delim_end);

        if (first != second) {
            oitems.emplace_back(first, second);
        }
        if (second == str_end) break;

        first = second + delim.size();
    }
}

inline std::string Replace(const std::string &str, const std::string &nastychars) {
    std::set<char> chars_blacklist;
    for (int i = 0; i < nastychars.size(); i++) {
        chars_blacklist.insert(nastychars[i]);
    }
    std::ostringstream oss;
    for (int i = 0; i < str.size(); i++) {
        if (chars_blacklist.find(str[i]) == chars_blacklist.end()) {
            oss << str[i];
        }
    }
    return oss.str();
}

class FieldBase {
   public:
    FieldBase(std::string name) : name_(name) {}
    std::string name() { return name_; }
    virtual bool deserilization(const std::string &inp) = 0;
    virtual ~FieldBase(){};
    // 对于 非 ComposedFields 来说, num_fields 都为0
    virtual int num_fields() { return 0; }

   private:
    std::string name_;
};

// Field 当前支持 int, float, uint32, uint64, string. 如果需要支持其它类型, 需要自行特化 parse
template <typename T>
class Field : public FieldBase {
   public:
    Field(const std::string &name) : FieldBase(name) {}
    Field(const std::string &name, const T &value) : FieldBase(name), data_(value) {}

    bool deserilization(const std::string &inp) override {
        bool is_succ;
        is_succ = parse(inp, data_);
        return is_succ;
    }

    T data() { return data_; }

    static std::shared_ptr<FieldBase> new_instance(const std::string &name) { return std::make_shared<Field<T>>(name); }

   private:
    T data_;
};

// ComposedFieldBase  组合 field 的基模板, ArrayField, NestedField, Record都继承该模板
template <typename T>
class ComposedFieldBase : public FieldBase {
   public:
    ComposedFieldBase(const std::string &name, const std::string &delim) : FieldBase(name), delim_(delim) {}

    bool add_field(std::shared_ptr<T> field) {
        if (named_fields_.find(field->name()) != named_fields_.end()) {
            LOG(ERROR) << "duplicated field name " << field->name();

            return false;
        }
        sub_fields_.push_back(field);
        named_fields_.insert(std::make_pair(field->name(), field));
        return true;
    }

    bool deserilization(const std::string &inp) override {
        // deserilization 设计为两个阶段: 一个是将数据 分成 list of string items, 然后再处理一下
        // 其中 deserilization_stage1 负责 讲 string spit 为 list of string items
        // set_data 负责 将 list of string items 解析成相应的 field
        // input:
        //      inp: 需要序列化的 string
        // output:
        //      bool : 解析过程中是否 出现错误, 如果发生错误, 返回 false
        std::vector<std::string> items;
        bool is_succ = deserilization_stage1(inp, items);

        if (!is_succ) {
            LOG(ERROR) << "parse error:" << inp;
            return is_succ;
        }

        is_succ = set_data(items);
        if (!is_succ) {
            LOG(ERROR) << "parse error:" << inp;
        }
        return is_succ;
    }

    std::shared_ptr<T> get_field(const std::string &name) {
        std::shared_ptr<T> retval;
        if (named_fields_.find(name) != named_fields_.end()) retval = named_fields_.find(name)->second;
        return retval;
    }

    std::shared_ptr<T> sub_fields_at(int i) {
        if (i >= sub_fields_.size()) {
            LOG(ERROR) << "i >= sub_fields_.size()";
            return nullptr;
        }
        return sub_fields_[i];
    }

    int num_fields() override { return sub_fields_.size(); }

   protected:
    bool set_data(const std::vector<std::string> &items) {
        if (items.size() != sub_fields_.size()) {
            std::ostringstream oss;
            oss << "items.size=" << items.size() << ", sub_fields_.size=" << sub_fields_.size() << " mismatch\n";
            LOG(ERROR) << oss.str();
            return false;
        }
        for (int i = 0; i < items.size() && i < sub_fields_.size(); i++) {
            std::shared_ptr<T> field = sub_fields_[i];
            if (!field->deserilization(items[i])) {
                return false;
            }
        }
        return true;
    }

    virtual bool deserilization_stage1(const std::string &inp, std::vector<std::string> &out) {
        string_splitter(inp, delim_, out);
        return true;
    };

    const std::vector<std::shared_ptr<T>> &sub_fields() { return sub_fields_; }
    const std::string &delim() { return delim_; }

   private:
    std::vector<std::shared_ptr<T>> sub_fields_;
    std::unordered_map<std::string, std::shared_ptr<T>> named_fields_;
    std::string delim_;
};

template <typename T>
class ArrayField : public ComposedFieldBase<T> {
   public:
    ArrayField(const std::string &name, const std::string &delim = ",") : ComposedFieldBase<T>(name, delim) {}
    static std::shared_ptr<FieldBase> new_instance(const std::string &name) {
        return std::make_shared<ArrayField<T>>(name);
    }

   protected:
    bool deserilization_stage1(const std::string &inp, std::vector<std::string> &out) override {
        bool is_succ = true;
        std::vector<std::string> items;
        string_splitter(inp, ":", items);
        if (items.size() != 2) {
            LOG(ERROR) << "fmt error";
            return false;
        }

        int numele = 0;
        try {
            numele = std::stoi(items[0]);

            string_splitter(items[1], this->delim(), out);

            if (numele != out.size()) {
                std::ostringstream oss;
                oss << "arrayfield numele out.size() mismatch, numele=" << numele << ", out.size=" << out.size();
                LOG(ERROR) << oss.str();
                return false;
            }
            int sub_fields_size = ComposedFieldBase<T>::sub_fields().size();
            for (int i = sub_fields_size; i < out.size(); i++) {
                this->add_field(std::make_shared<T>("sub_field_" + std::to_string(i)));
            }
        } catch (const std::exception &err) {
            is_succ = false;
        }
        return is_succ;
    };
};

class NestedField : public ComposedFieldBase<FieldBase> {
   public:
    NestedField(const std::string &name, const std::string &delim = "#") : ComposedFieldBase<FieldBase>(name, delim) {}
    static std::shared_ptr<FieldBase> new_instance(const std::string &name) {
        return std::make_shared<NestedField>(name);
    }
};

class Record : public ComposedFieldBase<FieldBase> {
   public:
    Record(const std::string &name = "record", const std::string &delim = "\t")
        : ComposedFieldBase<FieldBase>(name, delim) {}
};

class FieldManager {
   public:
    static FieldManager *instance() {
        static FieldManager field_manager;
        return &field_manager;
    }
    void register_field(const std::string &name,
                        std::function<std::shared_ptr<FieldBase>(const std::string &)> field_builder) {
        if (registered_fields_.find(name) == registered_fields_.end()) {
            registered_fields_.insert(std::make_pair(name, field_builder));
        }
    }

    std::shared_ptr<FieldBase> get_field_by_name(const std::string &registered_name, const std::string &field_name) {
        std::shared_ptr<FieldBase> target_field;
        if (registered_fields_.find(registered_name) != registered_fields_.end()) {
            auto field_builder = registered_fields_.find(registered_name)->second;
            target_field = field_builder(field_name);
        }
        return target_field;
    }

    std::shared_ptr<Record> record_builder(const std::vector<std::string> &field_names) {
        std::shared_ptr<Record> record = std::make_shared<Record>("record");
        int unique_name = 0;
        for (const auto &name : field_names) {
            if (registered_fields_.find(name) != registered_fields_.end()) {
                std::shared_ptr<FieldBase> field = get_field_by_name(name, std::to_string(unique_name));
                record->add_field(field);
            } else {
                LOG(ERROR) << "key [" << name << "] not exist";
            }
            unique_name++;
        }
        return record;
    }

   private:
    FieldManager() {}
    std::unordered_map<std::string, std::function<std::shared_ptr<FieldBase>(const std::string &)>> registered_fields_;
};

class FieldRegister {
   public:
    FieldRegister(const std::string &name,
                  std::function<std::shared_ptr<FieldBase>(const std::string &)> field_builder) {
        FieldManager::instance()->register_field(name, field_builder);
    }
};

#define REGISTER_FIELD(name, field_builder, counter) static FieldRegister field_register_##counter(name, field_builder);

//  int, float, uint32, uint64, string
REGISTER_FIELD("Field<int>", Field<int>::new_instance, 0);
REGISTER_FIELD("Field<float>", Field<float>::new_instance, 1);
REGISTER_FIELD("Field<uint32>", Field<uint32_t>::new_instance, 2);
REGISTER_FIELD("Field<uint64>", Field<uint64_t>::new_instance, 3);
REGISTER_FIELD("Field<string>", Field<std::string>::new_instance, 4);

REGISTER_FIELD("ArrayField<int>", ArrayField<Field<int>>::new_instance, 5);
REGISTER_FIELD("ArrayField<float>", ArrayField<Field<float>>::new_instance, 6);
REGISTER_FIELD("ArrayField<uint32>", ArrayField<Field<uint32_t>>::new_instance, 7);
REGISTER_FIELD("ArrayField<uint64>", ArrayField<Field<uint64_t>>::new_instance, 8);
REGISTER_FIELD("ArrayField<string>", ArrayField<Field<std::string>>::new_instance, 9);

}  // namespace dict_field