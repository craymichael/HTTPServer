""" An HTTP client that spams a server with GET requests. (Python 3 only)

    Author: Zach Carmichael
"""
import urllib.request  # Python 3
from multiprocessing import Pool

# Server to spam
SERVER      = 'http://localhost:8008'
# Webpage on server to spam (lead with '/')
PAGE        = '/'
# Initial number of requests
INIT_N_REQS = 2000
# Increment amount per iteration
INC_AMT     = 1000
# Number of threads
N_THREADS   = 100


def send_http(url):
    handler = urllib.request.urlopen(url)
    data = handler.read()
    handler.close()
    return data


def main():
    # Create URL to bother
    url = SERVER + PAGE
    # Initial number of processes to run
    n_reqs = INIT_N_REQS
    # Main loop
    while True:
        print('Spamming "{}" with {} requests.'.format(url, n_reqs))
        # Do the spamming
        with Pool(processes=N_THREADS) as pool:
            pool.map(send_http, [url]*n_reqs)
        # Check is spamming should continue
        opt = input("Continue?(Y/n)")
        if opt.lower() == "n":
            break
        # Increment number of processes by INC_AMT
        n_reqs += INC_AMT
    print('Finished with {} requests.'.format(n_reqs))


if __name__ == '__main__':
    main()

