# Transfer large file using UDP protocol
## About
An application using UDP protocol for transferring file between 2 machines Windows. This application use mechanism send-and-wait to ensure the integrity of the file.
## Syntax
### Sender
```
SendFile.exe <target> --path=<path> --buffer-size=<size>
```
### Receiver
```
RecvFile.exe --out=<out>
```
## For more details
I just developed mechanism `send-and-wait` but it's not compatible for a long road in network (the segment can be dropped). So, I recommend some features for application in the future.
- [ ] Representation data in base64 (we can use any delimiter - I'm using character `,` as a delimiter)
- [ ] Mechansim time out (integrity)
- [ ] Use cryptographic algorithms such as symmetric (RSA) or asymmetric (AES)
- [ ] Restrict the input and validate it before running into the program or in progress
## Result
![Capture the screen](https://github.com/grey-btw/transfer-file-udp/blob/master/image/capture.png)

