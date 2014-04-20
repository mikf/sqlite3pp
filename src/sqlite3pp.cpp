/*
* sqlite3pp.cpp
*
* Copyright 2014 Mike Fährmann <mike_faehrmann@web.de>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sqlite3pp.h"

#ifdef SQLITE3PP_DEBUG_ENABLED
#   include <iostream>
#   define SQLITE3PP_DEBUG_MSG(x) \
        std::cerr << __FILE__ << ':' << __LINE__ << '\n' << x << std::endl;
#else
#   define SQLITE3PP_DEBUG_MSG(x)
#endif


namespace sqlite3pp
{

database::database()
    : handle_(nullptr)
{
    SQLITE3PP_DEBUG_MSG("database constructor");
}

database::database(const char* path)
    : handle_(nullptr)
{
    SQLITE3PP_DEBUG_MSG("database constructor");
    open(path);
}

database::database(const std::string& path)
    : handle_(nullptr)
{
    SQLITE3PP_DEBUG_MSG("database constructor");
    open(path);
}

// database::database(database&& other)
    // : handle_(other.handle_)
// {
    // SQLITE3PP_DEBUG_MSG("database move constructor with handle_ == " << handle_);
    // other.handle_ = nullptr;
// }
//
// database& database::operator = (database&& other)
// {
    // SQLITE3PP_DEBUG_MSG("database move assignment operator with handle_ == " << handle_);
    // std::swap(handle_, other.handle_);
    // return*this;
// }

database::~database()
{
    SQLITE3PP_DEBUG_MSG("database destructor with handle_ == " << handle_);
    close();
}

void database::open(const char* path)
{
    if(handle_ != nullptr)
        close();
    if(sqlite3_open(path,&handle_) != SQLITE_OK)
        throw error(handle_);
    SQLITE3PP_DEBUG_MSG("database open with handle_ == " << handle_);
}

void database::open(const std::string& path)
{
    open(path.c_str());
}

void database::close()
{
    sqlite3_close(handle_);
    handle_ = nullptr;
}

statement database::prepare(const char* stmt_str) const
{
    sqlite3_stmt* stmt;

    sqlite3_prepare_v2(handle_, stmt_str, -1, &stmt, nullptr);
    if(stmt == nullptr)
        throw error(handle_);

    SQLITE3PP_DEBUG_MSG(stmt_str);
    return statement(stmt);
}

statement database::prepare(const std::string& stmt_str) const
{
    return prepare(stmt_str.c_str());
}

transaction database::begin_transaction(transaction_mode tm)
{
    switch(tm)
    {
    case deferred:
        prepare("BEGIN DEFERRED").exec();
        break;
    case immediate:
        prepare("BEGIN IMMEDIATE").exec();
        break;
    case exclusive:
        prepare("BEGIN EXCLUSIVE").exec();
        break;
    }

    return transaction(*this);
}

int database::changes() const
{
    return sqlite3_changes(handle_);
}

int database::total_changes() const
{
    return sqlite3_total_changes(handle_);
}

int database::last_insert_rowid() const
{
    return sqlite3_last_insert_rowid(handle_);
}



statement::statement()
    : stmt_(nullptr)
{}

statement::statement(sqlite3_stmt* stmt)
    : stmt_(stmt)
{
    SQLITE3PP_DEBUG_MSG("statement constructor with stmt_ == " << stmt_);
}

// statement::statement(statement&& other)
    // : stmt_(other.stmt_)
// {
    // SQLITE3PP_DEBUG_MSG("statement move constructor with stmt_ == " << stmt_);
    // other.stmt_ = nullptr;
// }
//
// statement& statement::operator = (statement&& other)
// {
    // SQLITE3PP_DEBUG_MSG("statement move assignment operator with stmt_ == " << stmt_);
    // std::swap(stmt_, other.stmt_);
    // return *this;
// }

statement::~statement()
{
    SQLITE3PP_DEBUG_MSG("statement destructor with stmt_ == " << stmt_);
    sqlite3_finalize(stmt_);
}

iterator statement::begin()
{
    reset();
    return iterator(stmt_);
}

iterator statement::end()
{
    return iterator();
}

void statement::reset()
{
    sqlite3_reset(stmt_);
}

void statement::exec()
{
    sqlite3_step(stmt_);
    reset();
}

void statement::bind(int pos, int value)
{ sqlite3_bind_int(stmt_, pos, value); }

void statement::bind(int pos, sqlite_int64 value)
{ sqlite3_bind_int64(stmt_, pos, value); }

void statement::bind(int pos, double value)
{ sqlite3_bind_double(stmt_, pos, value); }

void statement::bind(int pos, const char* value)
{ sqlite3_bind_text(stmt_, pos, value, -1, SQLITE_STATIC); }

void statement::bind(int pos, const std::string& value)
{ sqlite3_bind_text(stmt_, pos, value.c_str(), value.length(), SQLITE_STATIC); }

int statement::parameter_count() const
{
    return sqlite3_bind_parameter_count(stmt_);
}

int statement::parameter_index(const char* name) const
{
    return sqlite3_bind_parameter_index(stmt_, name);
}

int statement::parameter_index(const std::string& name) const
{
    return sqlite3_bind_parameter_index(stmt_, name.c_str());
}

const char* statement::parameter_name(int index) const
{
    return sqlite3_bind_parameter_name(stmt_, index);
}



iterator::iterator()
    : stmt_(nullptr)
{}

iterator::iterator(sqlite3_stmt* stmt)
    : stmt_(stmt)
{
    if(stmt_ != nullptr)
        step();
}

iterator& iterator::operator*()
{
    return *this;
}

iterator& iterator::operator ++()
{
    step();
    return *this;
}

bool iterator::operator == (const iterator& other)
{
    return stmt_ == other.stmt_;
}

bool iterator::operator != (const iterator& other)
{
    return stmt_ != other.stmt_;
}

void iterator::step()
{
    int ret = sqlite3_step(stmt_);

    if(ret == SQLITE_ROW)
        return;
    if(ret == SQLITE_DONE)
        stmt_ = nullptr;
    else
        throw error(sqlite3_db_handle(stmt_));
}

const char* iterator::operator [] (int column) const
{
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
}

const char* iterator::as_string(int column) const
{
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt_, column));
}

int iterator::as_int(int column) const
{
    return sqlite3_column_int(stmt_, column);
}

long iterator::as_long(int column) const
{
    return sqlite3_column_int64(stmt_, column);
}

double iterator::as_double(int column) const
{
    return sqlite3_column_double(stmt_, column);
}



transaction::transaction(database& db)
    : db_(db), rollback_(true)
{}

transaction::~transaction()
{
    if(rollback_)
        rollback();
}

void transaction::commit()
{
    db_.prepare("COMMIT").exec();
    rollback_ = false;
}

void transaction::rollback()
{
    db_.prepare("ROLLBACK").exec();
}



error::error(sqlite3* db)
{
    int size;
    int code = sqlite3_errcode(db);
    const char* fmt = "Error: %s\nErrorCode: %d - %s\n";
    const char* msg = sqlite3_errmsg(db);
    const char* str = sqlite3_errstr(code);

    // find appropriate size for buffer
    size = snprintf(nullptr, 0, fmt, msg, code, str);

    // allocate buffer
    buf_ = new char[size+1];

    // generate formatted error message
    snprintf(buf_, size, fmt, msg, code, str);
}

error::~error()
{
    delete[] buf_;
}

const char* error::what() const noexcept
{
    return buf_;
}

}
