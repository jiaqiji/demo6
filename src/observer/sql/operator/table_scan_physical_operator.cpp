/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by WangYunlai on 2021/6/9.
//

#include "sql/operator/table_scan_physical_operator.h"
#include "event/sql_debug.h"
#include "storage/table/table.h"

using namespace std;

RC TableScanPhysicalOperator::open(Trx *trx) {
    RC rc = table_->get_record_scanner(record_scanner_, trx, readonly_);
    if (rc == RC::SUCCESS) {
        fileds = *(table_->table_meta().field_metas());
        tuple_.set_schema(table_, table_->table_meta().field_metas());
        record_Vas.set_schema(table_, table_->table_meta().field_metas());
    }
    trx_ = trx;
    return rc;
}

RC TableScanPhysicalOperator::next() {
    if (!record_scanner_.has_next()) {
        return RC::RECORD_EOF;
    }

    RC rc = RC::SUCCESS;
    bool filter_result = false;
    bool bfunc = false;
    while (record_scanner_.has_next()) {
        rc = record_scanner_.next(current_record_);
        if (rc != RC::SUCCESS) {
            return rc;
        }

        tuple_.set_record(&current_record_);
        rc = filter(tuple_, filter_result, bfunc);
        if (rc != RC::SUCCESS) {
            return rc;
        }
        if (filter_result) {
            sql_debug("get a tuple: %s", tuple_.to_string().c_str());
            break;
        } else {
            if (!bfunc) {
                sql_debug("a tuple is filtered: %s", tuple_.to_string().c_str());
                rc = RC::RECORD_EOF;
            } else {
            }
        }
    }
    return rc;
}

RC TableScanPhysicalOperator::close() {
    return record_scanner_.close_scan();
}

Tuple *TableScanPhysicalOperator::current_tuple() {

    ValuesType values;
    bool bFunction = false;
    MapVecRecords maprows;
    std::map<int, ValuesType> mapExepressValues;
    int index = 0;
    for (unique_ptr<Expression> &expr : predicates_) {
        if (expr->type() == ExprType::FUNCTION) {
            if (expr->functionType() == CompFunction::ORDER_FUN) {
                maprows = expr->getMapRecords();
            } else if (expr->functionType() == CompFunction::GROUP_FUN) {
                maprows = expr->getMapRecords();
            } else {
                values.push_back(expr->getValue());
            }
            mapExepressValues[index++] = expr->getValues();
            bFunction = true;
        }
    }
    if (bFunction) {
        


        if (values.size() > 0) {
            tuple_Vas.set_cells(values);
            return &tuple_Vas;
        } else {
            MapVecRecords::iterator it;
            MapVecRecords::iterator itBegin = maprows.begin();
            MapVecRecords::iterator itEnd = maprows.end();
            for (it = itBegin; it != itEnd; ++it) {
                record_Vas.AddRecord(it->second);
            }
            return &record_Vas;
        }
        return nullptr;
    }
    tuple_.set_record(&current_record_);
    return &tuple_;
}

string TableScanPhysicalOperator::param() const {
    return table_->name();
}

void TableScanPhysicalOperator::set_predicates(vector<unique_ptr<Expression>> &&exprs) {
    predicates_ = std::move(exprs);
}

RC TableScanPhysicalOperator::filter(RowTuple &tuple, bool &result, bool &bFun) {
    RC rc = RC::SUCCESS;
    Value value;
    bool bFunction = false;
    for (unique_ptr<Expression> &expr : predicates_) {
        rc = expr->get_value(tuple, value);
        if (expr->type() == ExprType::FUNCTION) {
            if (rc != RC::SUCCESS) {
                return rc;
            }
            expr->setValue(value);
            expr->AddRecord(value, current_record_);
            bool tmp_result = value.get_boolean();
            if (!tmp_result) {
                result = false;
                return rc;
            }
            bFunction = true;
        } else {
            if (rc != RC::SUCCESS) {
                return rc;
            }
            bool tmp_result = value.get_boolean();
            if (!tmp_result) {
                result = false;
                return rc;
            }
        }
    }
    bFun = bFunction;
    result = !bFunction;
    return rc;
}
