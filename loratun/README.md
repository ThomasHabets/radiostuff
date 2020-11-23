Minimum RN2483 firmware: 1.0.5


# Latency

```
$ ping -i 20 192.0.2.2
PING 192.0.2.2 (192.0.2.2) 56(84) bytes of data.
64 bytes from 192.0.2.2: icmp_seq=1 ttl=64 time=7546 ms
64 bytes from 192.0.2.2: icmp_seq=2 ttl=64 time=7546 ms
64 bytes from 192.0.2.2: icmp_seq=3 ttl=64 time=7545 ms
64 bytes from 192.0.2.2: icmp_seq=4 ttl=64 time=7546 ms
64 bytes from 192.0.2.2: icmp_seq=5 ttl=64 time=7545 ms
64 bytes from 192.0.2.2: icmp_seq=6 ttl=64 time=7545 ms
64 bytes from 192.0.2.2: icmp_seq=7 ttl=64 time=7545 ms
64 bytes from 192.0.2.2: icmp_seq=8 ttl=64 time=7545 ms
64 bytes from 192.0.2.2: icmp_seq=9 ttl=64 time=7545 ms
^C
--- 192.0.2.2 ping statistics ---
9 packets transmitted, 9 received, 0% packet loss, time 264ms
rtt min/avg/max/mdev = 7544.662/7545.236/7545.866/2.629 ms
```
