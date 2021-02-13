# axftp / axsh

This is a set of standard tools to securely log in for shell and ftp-like
access.

## But... why?

This uses dedicated AX.25 stuff instead of running IP over AX.25 and then SSH
for a few reasons:
* Encryption is banned in amateur radio, so it has to be authentication only
* This protocol is more bandwidth efficient than SSH
* I want to use native AX.25 sockets to tweak parameters specific to it
* Because of the latency this more "batch" instead of "interactive" shell makes more sense
* Raw AX.25 allows more user-controlled multi-hop routing

## Running

See full demo [on youtube](https://www.youtube.com/watch?v=HRH6RpRlzZQ).

### Start server, currently only serving 'test.txt'

```
./axshd -r <radio1> -s <mycall>
```

### Run client

```
./axsh -r <radio1> -s <mycall> <remote call, -s arg to axftpd>
```

## Enabling authentication

Create keys with `./axkeys client` and `./axkeys server`. Then use `-k` and `-P` e.g.:

```
./axshd -r <radio1> -s <mycall> -k server.key -P client.pub
```

```
./axsh -r <radio1> -s <mycall> -k client.key -P server.pub <server call>
```
