# BlinkDB
<b>BlinkDB</b> is an in-memory, key-value store optimized for balanced read and write workloads. The
design prioritizes efficient memory utilization, fast lookups.
I have optimized BlinkDB for balanced workloads, <b>balancing</b> between read and write performance. The system is designed to handle:
- High throughput of “SET”, “GET” and “DEL” operations.
- Optionally user can also print the contents of the DB using “print” command
- Low-latency data retrieval with efficient indexing.
- Memory constraints through intelligent eviction strategies.

The source code and documentation of client is present in part-a and that of the server in part-b.