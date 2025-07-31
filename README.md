# Order Book Reconstruction - Readme

This document details the implementation of the MBO to MBP-10 order book reconstruction program, with a focus on the performance optimizations and important operational notes.

## Overall Optimization Steps Taken

The primary goal was to maximize processing speed and minimize memory overhead to handle high-frequency data efficiently. The following key optimizations were implemented:

1.  **Buffered I/O (`stringstream`):** Instead of writing to the output file after each event (which would cause thousands of slow disk I/O operations), the entire output is constructed in an in-memory `std::stringstream`. The buffer is then written to the `mbp_output.csv` file in a single, fast operation at the end of the program. This is the most significant optimization, as it dramatically reduces the number of expensive system calls.

2.  **Fast Manual Parsing (`string_view`):** The default C++ `stringstream` for parsing each CSV line was replaced with a lightweight manual parser. This parser uses `std::string_view`, which avoids creating new memory allocations and copies for each token from the line. This results in a substantial reduction in both processing time and memory usage per line.

3.  **Optimal Core Data Structures:**
    * **`std::unordered_map`:** Used to store individual orders by their ID. This provides an average time complexity of **O(1)** for the most frequent operations: finding an order to cancel or fill.
    * **`std::map`:** Used for the aggregated bid and ask books. This container automatically keeps the price levels sorted. This is a critical optimization as it **eliminates the need for any manual sorting** when writing snapshots of the top 10 levels. All updates are handled in efficient **O(log N)** time.

4.  **Standard C++ I/O Acceleration:** `std::ios_base::sync_with_stdio(false);` is used at the start of `main()` to decouple C++ streams from the underlying C standard I/O library, providing a general speedup for all input and output operations.

## Thought Process, Limitations, and Future Improvements

### Thought Process
The core design philosophy was to minimize per-line processing overhead. In high-frequency finance, the volume of data makes even small, repetitive costs significant. The choice of `std::unordered_map` for individual order tracking and `std::map` for the sorted price book was central to this. The combination provides a "best of both worlds" approach: O(1) access for event-driven updates and O(log N) sorted maintenance for snapshotting, avoiding costly full sorts of the book.

### Alternative Approaches Considered
An initial attempt was made to use `std::from_chars` for string-to-number conversions. Theoretically, it is the fastest parsing method in modern C++, avoiding the overhead of locales and exceptions present in `stod`/`stoll`. However, during testing, it became clear that compiler support for the floating-point version of `std::from_chars` can be inconsistent across different environments and GCC versions. To ensure the code would compile and run correctly on any test bench, the decision was made to revert to the universally supported `stod`/`stoll`. This was a trade-off where **robustness and portability were prioritized over a minor, potentially unavailable, performance gain.**

### Limitations and Potential Improvements
* **Memory Usage vs. Speed:** The current implementation buffers the entire output in memory before writing to disk. This is extremely fast for the given dataset size. However, for truly massive, multi-gigabyte input files, this could lead to excessive memory consumption. A potential improvement would be a hybrid approach: buffer output for a set number of lines (e.g., 100,000) and then flush the buffer to the file periodically. This would strike a better balance between I/O performance and memory footprint for larger-than-expected inputs.
* **Single-Threaded Execution:** The program is single-threaded. For a live market data feed or even larger historical files, performance could be further enhanced by parallelizing the workload. For instance, one thread could handle file reading and parsing, placing events onto a lock-free queue, while a second worker thread processes the events and reconstructs the book. This would add significant complexity regarding thread synchronization but could nearly double the throughput on a multi-core system.

## Special Things to Take Note When Running Your Code

1.  **Output Directory Must Exist:** The program is hardcoded to write its output to `output/mbp_output.csv`. You **must create the `output` directory** in the same folder where you run the executable. The program will fail if this directory does not exist.

2.  **Initial Reset Event is Ignored:** As per the project specification, the first "R" (Reset) event found in the input `mbo.csv` file is intentionally ignored. No snapshot is generated for this event, and the program begins its processing from a clean state.

3.  **Compilation:** The code should be compiled with a C++17 compliant compiler. The provided `Makefile` uses the `-std=c++17` and `-O2` flags for optimization.

4.  **Execution Command:** The program requires one command-line argument: the path to the input MBO file.
    ```bash
    ./reconstruction_aayush data/mbo.csv
    ```
