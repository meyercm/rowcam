

```bash
raspistill -w 128 -h 64 -bm -e jpg -t 1 -o - | tee pic_live.jpg | convert fd:0 -canny 0x0+10%+20% -format rgb fd:1 | tee edges_live.jpg | ./encoder | ./delayer > /dev/ttyAMA0
```
