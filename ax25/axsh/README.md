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

## Speed

It's packet over modulated FM (usually 1200bps), so don't expect
much. Here are some examples with signatures turned on (so an extra
roundtrip for key negotiation).

| Size | Signed  | Packet size | Time  | bps
|------|---------|-------------|-------|------
| 10kB | yes     | 200         | 3m31s | 379
| 10kB | yes     | 500         | 1m55s | 695
| 10kB | no      | 500         | 1m23s | 963
| 10kB | no      | 1000        | 1m12s | 1111
| 10kB | yes     | 1000        | 1m20s | 1000

This was when downloading from a IC9700/direwolf into a Mobilink TNC3
using e.g.:

```
axftpd -l 500 -r radio -e -w63 -P client.pub -k server.key -s M6XXX-8
time echo get 10k.bin | \
  axftp -l 500 -r radio -e -w63 -P server.pub -k client.key -s M0XXX-8 M6XXX-8
```

## Source callsign

By default the callsign from `/etc/ax25/axports` is used, but it can
be overriden by `sudo axparms --assoc M0XXX-3 $USER` to be per-user,
or by supplying `-s` to the tools in the package.

## Digipeater path

Digipeater path can either be set using `-p`, or by using a
destination like `M0XXX-1 VIA M6YYY-3`.

I thought it should work to do `axparms --route add radio M0XXX-1
M6YYY-3`, but it doesn't seem to do anything.