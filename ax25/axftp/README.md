## Start server, currently only serving 'test.txt'

```
./axftpd -s <mycall> -p <main call listed in /etc/ax25/axports>
```

## Run client

```
./axftp -s <mycall> -p <main call> <remote call, first arg to axftpd>
```
