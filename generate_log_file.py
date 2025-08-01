# generate_log_file_fast.py
import datetime
import random
import uuid
import threading
import queue
import os
from faker import Faker
from tqdm import tqdm

# --- Configuration ---
FILE_NAME = "server_1gb.log"
TARGET_SIZE_MB = 1024  # 1 GB
TARGET_SIZE_BYTES = TARGET_SIZE_MB * 1024 * 1024
AVG_LINE_LENGTH_BYTES = 200 # Estimated average bytes per log line
NUM_LINES_TOTAL = TARGET_SIZE_BYTES // AVG_LINE_LENGTH_BYTES
BATCH_SIZE = 2000 # Number of lines each producer generates before queuing

# Use a sensible number of producer threads, e.g., number of CPU cores
NUM_PRODUCER_THREADS = os.cpu_count() or 4

# --- Log Generation Components (same as before) ---
LOG_LEVELS = ["INFO", "DEBUG", "WARN", "ERROR"]
HTTP_METHODS = ["GET", "POST", "PUT", "DELETE", "PATCH"]
ENDPOINTS = [
    "/api/users", "/api/products", "/api/orders", "/api/login",
    "/api/users/profile", "/api/products/search", "/api/orders/history"
]
STATUS_CODES = {
    "GET": [200, 200, 200, 404, 500],
    "POST": [201, 201, 400, 500],
    "PUT": [200, 404, 400, 500],
    "DELETE": [204, 404, 500],
    "PATCH": [200, 404, 400, 500]
}
ERROR_MESSAGES = [
    "Database connection timeout", "Authentication token expired",
    "Invalid input parameters", "Resource not found",
    "Internal Server Error: Null pointer exception"
]

def generate_log_line():
    """Generates a single, realistic log line."""
    fake = Faker() # Create a new Faker instance per line/thread if needed
    timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat().replace('+00:00', 'Z')
    log_level = random.choices(LOG_LEVELS, weights=[70, 15, 10, 5], k=1)[0]
    request_id = str(uuid.uuid4())
    source_ip = fake.ipv4()
    http_method = random.choice(HTTP_METHODS)
    endpoint = random.choice(ENDPOINTS)
    if 'users' in endpoint or 'orders' in endpoint:
        endpoint += f"/{random.randint(1, 1000)}"
    status_code = random.choice(STATUS_CODES[http_method])
    message = "Request processed successfully"
    if status_code >= 400:
        log_level = "ERROR" if status_code >= 500 else "WARN"
        message = random.choice(ERROR_MESSAGES)
    response_time_ms = random.randint(20, 500) if status_code < 500 else random.randint(500, 2000)
    return (
        f"{timestamp}|{log_level}|{request_id}|{source_ip}|"
        f"{http_method}|{endpoint}|{status_code}|{response_time_ms}|{message}\n"
    )

def log_producer(q, num_lines_to_produce):
    """
    Worker thread function. Generates log lines in batches and puts them in the queue.
    """
    for _ in range(num_lines_to_produce // BATCH_SIZE):
        batch = [generate_log_line() for _ in range(BATCH_SIZE)]
        q.put(batch)

def log_consumer(q, filename):
    """
    Writer thread function. Gets log batches from the queue and writes them to the file.
    """
    with open(filename, "w") as f, tqdm(total=NUM_LINES_TOTAL, unit='lines', desc="Writing logs") as pbar:
        producers_done = 0
        while producers_done < NUM_PRODUCER_THREADS:
            try:
                batch = q.get(timeout=1) # Wait for a batch
                if batch is None:
                    producers_done += 1
                    continue
                
                f.writelines(batch)
                pbar.update(len(batch))
            except queue.Empty:
                # If the queue is empty for a while, check if we're done
                if producers_done == NUM_PRODUCER_THREADS:
                    break


if __name__ == "__main__":
    print(f"Generating ~{TARGET_SIZE_MB} MB log file with {NUM_PRODUCER_THREADS} producer threads...")
    
    # A thread-safe queue for communication
    log_queue = queue.Queue(maxsize=100)
    
    # --- Start the consumer/writer thread ---
    consumer = threading.Thread(target=log_consumer, args=(log_queue, FILE_NAME))
    consumer.start()
    
    # --- Start the producer/worker threads ---
    producers = []
    lines_per_producer = NUM_LINES_TOTAL // NUM_PRODUCER_THREADS
    for i in range(NUM_PRODUCER_THREADS):
        # Give the last producer the remainder of lines to generate
        lines_to_gen = lines_per_producer + (NUM_LINES_TOTAL % NUM_PRODUCER_THREADS if i == NUM_PRODUCER_THREADS - 1 else 0)
        producer = threading.Thread(target=log_producer, args=(log_queue, lines_to_gen))
        producers.append(producer)
        producer.start()
        
    # --- Wait for producers to finish ---
    for p in producers:
        p.join()
        
    # --- Signal the consumer to stop ---
    # Put one 'None' for each producer to signal completion
    for _ in range(NUM_PRODUCER_THREADS):
        log_queue.put(None)
    
    # --- Wait for the consumer to finish writing ---
    consumer.join()
    
    print(f"\nSuccessfully generated {FILE_NAME}.")