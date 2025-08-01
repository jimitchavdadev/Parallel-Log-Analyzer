// LogGenerator.java

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.UUID;
import java.util.concurrent.*;

public class LogGenerator {

    // --- Configuration ---
    private static final String FILE_NAME = "server_1gb.log";
    private static final int TARGET_SIZE_MB = 1024; // 1 GB
    private static final int AVG_LINE_LENGTH_BYTES = 190;
    private static final int NUM_LINES_TOTAL = (TARGET_SIZE_MB * 1024 * 1024) / AVG_LINE_LENGTH_BYTES;
    private static final int BATCH_SIZE = 2000;
    private static final int NUM_PRODUCER_THREADS = Runtime.getRuntime().availableProcessors();

    // A thread-safe queue to hold batches of logs
    private static final BlockingQueue<List<String>> logQueue = new ArrayBlockingQueue<>(100);

    // Sentinel object to signal the end of production
    private static final List<String> POISON_PILL = new ArrayList<>();

    public static void main(String[] args) throws InterruptedException {
        System.out.printf("Generating ~%d MB log file with %d producer threads...%n", TARGET_SIZE_MB,
                NUM_PRODUCER_THREADS);
        long startTime = System.currentTimeMillis();

        // --- Start the Consumer (Writer) Thread ---
        Thread consumerThread = new Thread(new LogConsumer());
        consumerThread.start();

        // --- Start the Producer Threads ---
        ExecutorService producerPool = Executors.newFixedThreadPool(NUM_PRODUCER_THREADS);
        int linesPerProducer = NUM_LINES_TOTAL / NUM_PRODUCER_THREADS;
        for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
            producerPool.submit(new LogProducer(linesPerProducer));
        }

        // --- Shutdown ---
        producerPool.shutdown();
        producerPool.awaitTermination(10, TimeUnit.MINUTES); // Wait for producers to finish

        // Signal consumer to stop after producers are done
        logQueue.put(POISON_PILL);

        consumerThread.join(); // Wait for consumer to finish writing
        long endTime = System.currentTimeMillis();
        System.out.printf("%nSuccessfully generated %s in %.2f seconds.%n", FILE_NAME, (endTime - startTime) / 1000.0);
    }

    // --- Producer Task ---
    static class LogProducer implements Runnable {
        private final int linesToProduce;
        private final Random random = new Random();
        private final String[] LOG_LEVELS = { "INFO", "DEBUG", "WARN", "ERROR" };
        private final String[] HTTP_METHODS = { "GET", "POST", "PUT", "DELETE", "PATCH" };
        private final String[] ENDPOINTS = { "/api/users", "/api/products", "/api/orders", "/api/login" };

        public LogProducer(int linesToProduce) {
            this.linesToProduce = linesToProduce;
        }

        @Override
        public void run() {
            int linesGenerated = 0;
            while (linesGenerated < linesToProduce) {
                List<String> batch = new ArrayList<>(BATCH_SIZE);
                for (int i = 0; i < BATCH_SIZE && linesGenerated < linesToProduce; i++) {
                    batch.add(generateLogLine());
                    linesGenerated++;
                }
                try {
                    logQueue.put(batch);
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }

        private String generateRandomIp() {
            return random.nextInt(256) + "." + random.nextInt(256) + "." + random.nextInt(256) + "."
                    + random.nextInt(256);
        }

        private String generateLogLine() {
            String timestamp = ZonedDateTime.now().format(DateTimeFormatter.ISO_INSTANT);
            String logLevel = LOG_LEVELS[random.nextInt(LOG_LEVELS.length)];
            int statusCode = (logLevel.equals("ERROR")) ? 500 : (logLevel.equals("WARN")) ? 404 : 200;
            int responseTime = (statusCode >= 500) ? random.nextInt(1500) + 500 : random.nextInt(500) + 20;

            return String.join("|",
                    timestamp,
                    logLevel,
                    UUID.randomUUID().toString(),
                    generateRandomIp(),
                    HTTP_METHODS[random.nextInt(HTTP_METHODS.length)],
                    ENDPOINTS[random.nextInt(ENDPOINTS.length)],
                    String.valueOf(statusCode),
                    String.valueOf(responseTime),
                    "Request processed");
        }
    }

    // --- Consumer Task ---
    static class LogConsumer implements Runnable {
        @Override
        public void run() {
            System.out.print("Writing to disk: ");
            long batchCount = 0;
            try (FileWriter fw = new FileWriter(FILE_NAME);
                    BufferedWriter writer = new BufferedWriter(fw)) {

                while (true) {
                    List<String> batch = logQueue.take();
                    if (batch == POISON_PILL) {
                        break; // End of production
                    }
                    for (String line : batch) {
                        writer.write(line);
                        writer.newLine();
                    }
                    // Print a dot for every 50 batches to show progress
                    if (++batchCount % 50 == 0) {
                        System.out.print(".");
                    }
                }
            } catch (IOException | InterruptedException e) {
                e.printStackTrace();
                Thread.currentThread().interrupt();
            }
        }
    }
}