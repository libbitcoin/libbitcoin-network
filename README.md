[![Build Status](https://travis-ci.org/libbitcoin/libbitcoin-network.svg?branch=master)](https://travis-ci.org/libbitcoin/libbitcoin-network)

[![Coverage Status](https://coveralls.io/repos/libbitcoin/libbitcoin-network/badge.svg)](https://coveralls.io/r/libbitcoin/libbitcoin-network)

# Libbitcoin Network

*Bitcoin P2P Network Library*

Note that you need g++ 4.8 or higher. For this reason Ubuntu 12.04 and older are not supported. Make sure you have installed [libbitcoin](https://github.com/libbitcoin/libbitcoin) beforehand according to its build instructions.

```sh
$ ./autogen.sh
$ ./configure
$ make
$ sudo make install
$ sudo ldconfig
```

libbitcoin-network is now installed in `/usr/local/`.

**About Libbitcoin Network**

Libbitcoin Network is a partial implementation of the Bitcoin P2P network protocol. Excluded are all protocols that require access to a blockchain. The [libbitcoin-node](https://github.com/libbitcoin/libbitcoin-node) library extends the P2P networking capability and incorporates [libbitcoin-blockchain](https://github.com/libbitcoin/libbitcoin-blockchain) in order to implement a full node. The [libbitcoin-explorer](https://github.com/libbitcoin/libbitcoin-explorer) library uses the P2P networking capability to post transactions to the P2P network.