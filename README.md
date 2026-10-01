# pscrape

scrape + validate proxy lists. you supply the sources, it does the plumbing.

## build

    sudo apt install libcurl4-openssl-dev   # or brew install curl
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    cmake --build build

## test

    ctest --test-dir build --output-on-failure

## use

    ./build/pscrape -s sources.txt -o proxies.txt
    ./build/pscrape -s sources.txt -o proxies.txt --validate --thr 128
    ./build/pscrape -s sources.txt -o proxies.json -f json --http-probe

sources file: one url per line, `#` comments ok.

## what it does

1. fetches each source concurrently (respects robots.txt by default)
2. extracts `ip:port` and `ip:port:user:pass` via regex
3. dedupes by `ip:port`
4. optionally tcp-connects and/or http-probes through each
5. writes survivors in plain / auth / json format, atomic write

## what it does not do

- no hardcoded list of proxy sites
- no captcha / headless browser / auth bypass
- no per-source html parsers (generic regex only; add your own if needed)

## flags

| flag | default | meaning |
|---|---|---|
| `-s <path>` | `sources.txt` | source url list |
| `-o <path>` | `proxies.txt` | output |
| `-f <fmt>` | `plain` | `plain` / `auth` / `json` |
| `--thr <n>` | `64` | concurrency |
| `--timeout <s>` | `3` | per-op timeout |
| `--validate` | off | tcp connect check |
| `--http-probe` | off | http get through proxy (implies validate) |
| `--probe <url>` | httpbin.org/ip | probe url |
| `--no-robots` | off | skip robots.txt |

## license

MIT