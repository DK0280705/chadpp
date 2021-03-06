# Chadpp

A discord bot made with c++

This project is just made for fun.

## Building
Linux only.
Compile with g++ and cmake.
First, install dependecies below.

On ubuntu/debian like distributions:
```sh
    apt install libssl-dev zlib1g-dev libsodium-dev libopus-dev libpqxx-dev libmp3lame-dev
```

Then clone this repo.
```
    git clone https://github.com/DK0280705/chadpp.git
    cd chadpp
    git submodule update --init --recursive
```

Create `.env` file with your favorite editor.
```env
DISCORD_TOKEN="Your Token"    
BOTLOG_ID=123456789098765432
GUILD_ID=123456789098765432
OWNER_ID=123456789098765432
CONN_STRING="Your PostgreSQL connection string"
```

Then configure cmake. You can replace Ninja with GNU make.
```
    cmake -DCMAKE_BUILD_TYPE=Release -GNinja -B ./build
```
Compile with:
```
    cmake --build ./build -j$(nproc)
```

Then run your bot:
```
    ./launch.sh
```

## External Dependencies
- [LAME](https://lame.sourceforge.io)
- [Sodium](https://github.com/jedisct1/libsodium)
- [Opus](https://www.opus-codec.org)
- [OpenSSL](https://openssl.org)
- [Zlib](https://zlib.net)
- [PQXX](https://github.com/jtv/libpqxx)

## Included Dependencies
No need to install these as they're already included in this project.
- [DPP](https://github.com/brainboxdotcc/DPP) (Using my [fork](https://github.com/DK0280705/DPP))
- [Nlohmann JSON](https://github.com/nlohmann/json)
- [`{fmt}`](https://github.com/fmtlib/fmt)
