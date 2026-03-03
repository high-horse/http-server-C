import random
import requests

def main():
    pages = ['projects', 'index', 'about', 'contact']
    index = random.randint(0, len(pages) - 1)
    page = pages[index]
    url = f'http://localhost:9999/{page}'
    # print(f"Sending request to: {url}")
    try:
          response = requests.get(url)
          print(f"Status code: {response.status_code}")
          # print("Response body (first 200 chars):")
          # print(response.text[:200])  # print first 200 chars
    except requests.RequestException as e:
        print(f"Request failed: {e}")
    # send http request to localhost:9990/pages[index]
    

if __name__ == "__main__":
    main()