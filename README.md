# 📈 Parallel Log File Analyzer

This project is a high-performance log file analyzer written in C++. It uses multithreading to efficiently process and extract insights from large server log files, demonstrating modern C++ features for concurrent programming and performance optimization.

## 🎯 Objective

The primary goal is to build a tool that can rapidly analyze gigabyte-scale log files without consuming excessive memory. It serves as a practical example of applying parallel processing techniques to common data-intensive tasks.

## Key objectives include:

Performance: Leverage multithreading to significantly reduce the time required to parse large files compared to a single-threaded approach.

Efficiency: Process files in chunks, ensuring a low memory footprint regardless of the file size.

Modularity: Maintain a clean and organized codebase by separating concerns like parsing, analysis, and thread management.

Data Insights: Extract meaningful analytics, including error rates, request counts by type, and average response times.

## 📂 Code Structure

The project is organized into several key components to ensure modularity and ease of management.

    main.cpp

        The entry point of the application.

        Handles file I/O, determines the number of threads, and divides the file into chunks.

        Orchestrates thread creation and joins, and merges the final results from all threads.

    Analytics.h

        Defines the Analytics struct, which holds aggregated data like error counts, status code distribution, etc.

        Contains a merge() function to combine results from different threads.

    LogParser.h / LogParser.cpp

        Defines the LogEntry struct, representing a single parsed line from the log file.

        Contains the parseLine() function, which takes a string and converts it into a LogEntry object.

    CMakeLists.txt / Makefile

        Build scripts to compile the C++ source code into an executable.

    LogGenerator.java / generate_log_file.py

        Utility scripts to generate large, realistic log files for testing the analyzer's performance.

## ⚙️ How It Works

The analyzer processes the log file in a five-step, parallel workflow:

1. Log Generation (Optional)

Before analysis, a large test log file (e.g., 1 GB) can be created using the provided Java or Python scripts. These scripts are also multithreaded for fast generation.

2. File Chunking

The application reads the total size of the log file and divides it into roughly equal-sized chunks, one for each available CPU core. To prevent splitting a log line between two threads, each thread intelligently adjusts its starting point to the beginning of the next new line.

3. Parallel Analysis

Multiple threads are spawned, with each thread responsible for analyzing one chunk.

Each thread reads its assigned portion of the file line by line.

It parses each line into a LogEntry struct.

It compiles the statistics for its chunk into a local Analytics object. This step occurs entirely in parallel, with no data sharing or locking required between threads.

4. Result Merging

Once all worker threads have finished processing their chunks, the main thread collects the individual Analytics objects from each one. It then calls the merge() function repeatedly to combine all the partial results into a single, comprehensive Analytics object that represents the entire file.

5. Displaying Insights

Finally, the application formats the aggregated data from the final Analytics object and prints a summary report to the terminal. This report includes key performance indicators like total requests, error rates, and average response time.