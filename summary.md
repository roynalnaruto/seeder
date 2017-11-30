# Seeder Server

I have use 3 concurrent threads for the server.

**Thread 1**: Serves the purpose of accepting client connections
**Thread 2**: Serves queries of clients that have been added as nodes
**Thread 3**: Monitors _ping_ requests from nodes, to check their health (on 10 seconds timeout, node is removed from network)

# RDB

In the _classes/template-db.cpp_ I have tried implementing a generalized class _Table_ , with a variable number of members and variable types for those members. But since I have to declare the object definition at compile time, I restricted the types of tables to a few (with varying number of columns/class members).

There are 3 concurrent threads running in the server.

**Thread 1**: Serves the purpose of accepting new client connections
**Thread 2** and **Thread 3**: Serve queries of accepted clients.

I have use one single instance of the _GenericTrieDB_ class, and the naming convention for the _key_ is `table_name:id`. Clients can:

1. Add new tables (minimum 2 columns, maximum 5 columns)
2. Get list of tables
3. Insert data into any table
4. Lookup data from any table (by providing the _table_name_ and _id_)

There are multiple threads that can manipulate the _Table_ class. I have used a `shared_ptr` to read or write objects of that class. This makes it thread-safe. And in order to avoid any sort of race condition, I have made use of `std::mutex`. Before both operations (_getting_ or _setting_), the manipulating thread owns the _mutex_ and releases it only after end of that scope.

The Merkle Patricia tree provides _O(log(n))_ efficiency for **inserts** and **lookups**. I have maintained a map of _table_name_ and _n_columns_ in that table, so the **listtables** should be _O(k)_ where _k_ is the number of tables in the database.
