import asyncio
import aiohttp
import time

async def send_request(session, url):
    try:
        async with session.get(url, timeout=5) as response:
            if response.status == 200:
                return 1
    except Exception:
        pass
    return 0

async def main():
    url = 'http://localhost:8080/'
    total_requests = 0
    total_success = 0
    duration = 10  # Run the test for 10 seconds
    start_time = time.time()

    # Number of concurrent tasks
    max_concurrent_requests = 1000  # Adjust based on system and server capabilities

    semaphore = asyncio.Semaphore(max_concurrent_requests)
    tasks = []

    async with aiohttp.ClientSession() as session:
        while time.time() - start_time < duration:
            async with semaphore:
                task = asyncio.create_task(send_request(session, url))
                tasks.append(task)
                total_requests += 1

            # Optional: Sleep briefly to prevent overwhelming the server
            # await asyncio.sleep(0.001)

        # Wait for all tasks to complete
        results = await asyncio.gather(*tasks, return_exceptions=True)
        total_success = sum(result for result in results if isinstance(result, int))

    print(f"Total requests sent: {total_requests}")
    print(f"Total successful requests: {total_success}")

if __name__ == '__main__':
    asyncio.run(main())

