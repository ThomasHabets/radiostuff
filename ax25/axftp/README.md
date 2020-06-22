## Turn off checksums on both ends

Otherwise it'll break after a few packets for some reason.

```
kissparms -p radio -c 1 
```

## Start server, currently only serving 'test.txt'

```
./axftpd -s <mycall> -p <main call listed in /etc/ax25/axports>
```

## Run client

```
./axftp -s <mycall> -p <main call> <remote call, first arg to axftpd>
```
