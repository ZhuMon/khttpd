#! python3

import requests
import re
import argparse
import time


def pt5_fib(num):
    url = f"http://www.protocol5.com/Fibonacci/{num}.htm"

    prev = time.time()
    content = requests.get(url)
    tick = time.time() - prev

    pattern = "<li><h4>Decimal  Base 10:</h4><div>[0-9]*</div>"
    match_text = re.findall(pattern, content.text)

    res = match_text[0].split("<div>")[1].split("</div>")[0]
    return res, tick

def khttpd_fib(num):
    url = f"http://localhost:{args.port}/fib/{num}"
    prev = time.time()
    res = requests.get(url)
    tick = time.time() - prev
    return res.text, tick

def main(args):
    if args.num:

        print("match" if pt5_fib(args.num)[0] == khttpd_fib(args.num)[0] else "not equal")
    else:
        output = open("pt5_khttpd.txt", "w")
        for i in range(1, 100):
            pf, pt = pt5_fib(i)
            kf, kt = khttpd_fib(i)
            if pf != kf:
                print(f"not equal at fib({i})")
                break
            output.write(f"{i}\t{pt:.4f}\t{kt:.4f}\n")

            if i % 10 == 0:
                print(f"{i}/100")
            
        return


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--port", help="The port khttpd use", dest="port", default=1999)
    parser.add_argument("-n", "--num", help="The number for fibonacci", dest="num", default=0)
    args = parser.parse_args()
    main(args)
