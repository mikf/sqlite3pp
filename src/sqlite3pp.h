/*
* sqlite3++.h
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

#ifndef SQLITE3PP_H
#define SQLITE3PP_H

#include <sqlite3.h>

#include <exception>
#include <iterator>
#include <utility>
#include <string>



namespace sqlite3pp
{

////////////////////////////////////////////////////////////////////////////////
// forward declarations
class database;
class statement;
class iterator;
class transaction;
class error;



////////////////////////////////////////////////////////////////////////////////
//
class database
{
public:
    database();
    explicit database(const char* path);
    explicit database(const std::string& path);
    ~database();

    // no copy-constructor / copy-assignment-operator
    database(const database&) = delete;
    database(database&&) = default;
    database& operator = (const database&) = delete;
    database& operator = (database&&) = default;

    void open(const char* path);
    void open(const std::string& path);
    void close();

    statement prepare(const char* stmt) const;
    statement prepare(const std::string& stmt) const;

    int changes() const;
    int total_changes() const;
    int last_insert_rowid() const;

    inline void swap(database& other);

private:
    sqlite3* handle_;
};

inline void database::swap(database& other)
{ std::swap(handle_, other.handle_); }

inline void swap(database& lhs, database& rhs)
{ lhs.swap(rhs); }



////////////////////////////////////////////////////////////////////////////////
//
class statement
{
public:
    typedef sqlite3pp::iterator iterator;

    statement();
    ~statement();

    statement(const statement&) = delete;
    statement(statement&&) = default;
    statement& operator = (const statement&) = delete;
    statement& operator = (statement&&) = default;

    iterator begin();
    iterator end();

    void reset();
    void exec();

    void bind(int pos, int                value);
    void bind(int pos, sqlite_int64       value);
    void bind(int pos, double             value);
    void bind(int pos, const char*        value);
    void bind(int pos, const std::string& value);

    template <typename T>
    inline void bind(const char* name, T&& value);
    template <typename T>
    inline void bind(const std::string& name, T&& value);
    template <typename... Args>
    inline void bind_all(Args&&... values);

    int parameter_count() const;
    int parameter_index(const char* name) const;
    int parameter_index(const std::string& name) const;
    const char* parameter_name(int index) const;

    inline void swap(statement& other);

private:
    sqlite3_stmt* stmt_;

    template <typename T, typename... Args>
    inline void bind_all_impl_(int, T&&, Args&&...);
    inline void bind_all_impl_(int) const {};

    // a valid object (stmt_ != nullptr) can only be constructed
    // by class database
    explicit statement(sqlite3_stmt* stmt);
    friend class database;
};

inline void statement::swap(statement& other)
{ std::swap(stmt_, other.stmt_); }

inline void swap(statement& lhs, statement& rhs)
{ lhs.swap(rhs); }

template <typename T>
inline void statement::bind(const char* name, T&& value)
{ bind(parameter_index(name), std::forward<T>(value)); }

template <typename T>
inline void statement::bind(const std::string& name, T&& value)
{ bind(parameter_index(name), std::forward<T>(value)); }

template <typename... Args>
inline void statement::bind_all(Args&&... values)
{ bind_all_impl_(1, std::forward<Args>(values)...); }

template <typename T, typename... Args>
inline void statement::bind_all_impl_(int pos, T&& value, Args&&... values)
{
    bind(pos, std::forward<T>(value));
    bind_all_impl_(pos+1, std::forward<Args>(values)...);
}



////////////////////////////////////////////////////////////////////////////////
//
class iterator
    : public std::iterator<std::input_iterator_tag, iterator>
{
public:
    iterator();

    // Input Iterator Interface
    iterator& operator* ();
    iterator& operator ++();
    bool operator == (const iterator& other);
    bool operator != (const iterator& other);

    // Data Access
    const char* operator [](int column) const;
    const char* as_string  (int column) const;
    int         as_int     (int column) const;
    long        as_long    (int column) const;
    double      as_double  (int column) const;

private:
    sqlite3_stmt* stmt_;

    void step();

    // a valid object (stmt_ != nullptr) can only be constructed
    // by class statement
    explicit iterator(sqlite3_stmt* stmt);
    friend class statement;
};



////////////////////////////////////////////////////////////////////////////////
//
class transaction
{
// TODO
};



////////////////////////////////////////////////////////////////////////////////
//
class error
    : public std::exception
{
public:
    error(sqlite3* handle);
    virtual ~error();

    virtual const char* what() const noexcept;

private:
    char* buf_;
};



////////////////////////////////////////////////////////////////////////////////
//
std::string escape(const std::string&);
std::string escape(std::string&&);

}

#endif /* SQLITE3_BINDINGS_H*/
