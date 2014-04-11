/*
* example_basic.cpp
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

#include <sqlite3pp.h>
#include <iostream>

int main()
{
    try
    {
        sqlite3pp::database db {":memory:"};

        db.prepare(
            "CREATE TABLE store ("
                "article  TEXT,"
                "category TEXT,"
                "amount   INT"
            ")"
        ).exec();

        auto stmt = db.prepare(
            "INSERT INTO store (article, category, amount) "
            "VALUES (?, ?, ?)"
        );
        stmt.bind(1, "apple");
        stmt.bind(2, "fruit");
        stmt.bind(3, 125);
        stmt.exec();

        stmt.bind_all("banana", "fruit", 70);
        stmt.exec();

        for(auto row : db.prepare("SELECT article, amount FROM store"))
            std::cout << row[0] << ": " << row[1] << std::endl;
    }
    catch(sqlite3pp::error& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
