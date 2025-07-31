# Order Book Reconstruction - Readme

This document details the implementation of the MBO to MBP-10 order book reconstruction program, with a focus on the performance optimizations and important operational notes.

## Overall Optimization Steps Taken

The primary goal was to maximize processing speed and minimize memory overhead to handle high-frequency data efficiently. The following key optimizations were implemented:

1.  **Full File Buffering:** The program reads the entire input file into an in-memory buffer at the start. This minimizes slow disk I/O system calls, which are a major performance bottleneck, by reducing them to a single read operation.

2.  **Buffered Output (`stringstream`):** Instead of writing to the output file after each event, the entire output is constructed in an in-memory `std::stringstream`. The buffer is then written to the `mbp_output.csv` file in a single, fast operation at the end of the program.

3.  **Fast, Heap-Free Parsing:** The parsing logic uses `std::string_view` to avoid memory allocations when splitting lines into tokens. For number conversion, it uses a small, stack-allocated buffer and C-style `atof`/`atoll`/`atoi` functions, which avoids the overhead and potential heap allocations of `std::stod`/`stoll`/`stoi` inside the tight processing loop.

4.  **Optimal Core Data Structures:**
    * **`std::unordered_map`:** Used to store individual orders by their ID. This provides an average time complexity of **O(1)** for the most frequent operations: finding an order to cancel or fill.
    * **`std::map`:** Used for the aggregated bid and ask books. This container automatically keeps the price levels sorted, which is a critical optimization as it **eliminates the need for any manual sorting** when writing snapshots.

5.  **Standard C++ I/O Acceleration:** `std::ios_base::sync_with_stdio(false);` is used at the start of `main()` to decouple C++ streams from the underlying C standard I/O library, providing a general speedup for all I/O.

## Unit Testing for Correctness

To ensure the correctness of the core logic, a suite of unit tests was developed using the **Google Test** framework.

* **Testable Design:** The core order book logic was refactored into a self-contained `OrderBook` class, separating it from the file I/O in `main()`. This allows the class to be tested in isolation.
* **Test Coverage:** The tests cover all fundamental operations:
    * Adding single bid and ask orders.
    * Correctly canceling orders and removing them from the book.
    * Handling both partial and full fills of an order.
    * Aggregating sizes correctly when multiple orders are placed at the same price level.
    * Resetting the book to an empty state.
* **Running Tests:** The tests can be compiled and run using the `Makefile`. This provides an automated way to verify that the `OrderBook` logic behaves exactly as expected under various scenarios.

## Thought Process, Limitations, and Future Improvements

### Thought Process

The core design philosophy was to minimize per-line processing overhead. The choice of `std::unordered_map` for individual order tracking and `std::map` for the sorted price book was central to this. This combination provides a "best of both worlds" approach: O(1) access for event-driven updates and O(log N) sorted maintenance for snapshotting.

### Alternative Approaches Considered

An initial attempt was made to use `std::from_chars` for string-to-number conversions, as it is theoretically the fastest method in modern C++. However, it was abandoned due to inconsistent compiler support, which could have caused issues on the test bench. The current stack-based C-style conversion method was chosen as it is nearly as fast but is universally supported, prioritizing **robustness and portability**.

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

5. **Running Unit Test Cases:** To compile and run the unit tests, run the below command.
    ```bash
    make test
    ```
